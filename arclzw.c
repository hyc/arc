/*
 * $Header: /cvsroot/arc/arc/arclzw.c,v 1.3 2005/10/08 20:24:37 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCLZW
 *
 * Version 2.03, created on 10/24/86 at 11:46:22
 *
 * (C) COPYRIGHT 1985,86 by System Enhancement Associates; ALL RIGHTS RESERVED
 *
 * By:  Thom Henderson
 *
 * Description: This file contains the routines used to implement Lempel-Zev
 * data compression, which calls for building a coding table on the fly.
 * This form of compression is especially good for encoding files which
 * contain repeated strings, and can often give dramatic improvements over
 * traditional Huffman SQueezing.
 *
 * Language: Computer Innovations Optimizing C86
 *
 * Programming notes: In this section I am drawing heavily on the COMPRESS
 * program from UNIX.  The basic method is taken from "A Technique for High
 * Performance Data Compression", Terry A. Welch, IEEE Computer Vol 17, No 6
 * (June 1984), pp 8-19.  Also see "Knuth's Fundamental Algorithms", Donald
 * Knuth, Vol 3, Section 6.4.
 *
 * As best as I can tell, this method works by tracing down a hash table of code
 * strings where each entry has the property:
 *
 * if <string> <char> is in the table then <string> is in the table.
 */
#include <stdio.h>
#include <stdlib.h>
#include "arc.h"

VOID            arcdie();
#if	MSDOS
char           *setmem();
#else
#if	NEEDMEMSET
char           *memset();
#else
#include <memory.h>
#endif
#endif

#include "proto.h"
static VOID     putcode();
/* definitions for older style crunching */

#define FALSE    0
#define TRUE     !FALSE
#define TABSIZE  4096
#define NO_PRED  0xFFFF
#define EMPTY    0xFFFF
#define NOT_FND  0xFFFF

extern u_char	*pinbuf;
u_char		*inbeg, *inend;
u_char          *outbuf;
u_char          *outbeg, *outend; 

static int      sp;		/* current stack pointer */
static int	inflag;

static struct entry {		/* string table entry format */
	char            used;	/* true when this entry is in use */
	u_char           follower;	/* char following string */
	u_short          next;	/* ptr to next in collision list */
	u_short          predecessor;	/* code for preceeding string */
} *string_tab;				/* the code string table */


/* definitions for the new dynamic Lempel-Zev crunching */

#define CRBITS	12		/* maximum bits per code */
#define CRHSIZE	5003		/* 80% occupancy */
#define	CRGAP	2048		/* ratio check interval */
#define	SQBITS	13		/* Squash values of above */
#define	SQHSIZE	10007
#define	SQGAP	10000
#define INIT_BITS 9		/* initial number of bits/code */

static int      Bits;
static int      Hsize;
static int      Check_Gap;

static int      n_bits;		/* number of bits/code */
static int      maxcode;	/* maximum code, given n_bits */
#define MAXCODE(n)      ((1<<(n)) - 1)	/* maximum code calculation */
static int      max_maxcode;	/* 1 << BITS; largest possible code (+1) */

static char     buf[SQBITS];	/* input/output buffer */

static u_char    lmask[9] =	/* left side masks */
{
	0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00
};
static u_char    rmask[9] =	/* right side masks */
{
	0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

static int      offset;		/* byte offset for code output */
static long     in_count;	/* length of input */
static int      in_off;		/* where to start reading input */
static long     bytes_out;	/* length of compressed output */
static long     bytes_last;	/* previous output size */
static u_short   ent;
static long     fcode;
static int      hshift;

/*
 * To save much memory (which we badly need at this point), we overlay the
 * table used by the previous version of Lempel-Zev with those used by the
 * new version.  Since no two of these routines will be used together, we can
 * safely do this.
 */

long            *htab;		/* hash code table   (crunch) */
u_short         *codetab;	/* string code table (crunch) */

static u_short  *prefix;	/* prefix code table (uncrunch) */
static u_char   *suffix;	/* suffix table (uncrunch) */

static int      free_ent;	/* first unused entry */
u_char          *stack;		/* local push/pop stack */

/*
 * block compression parameters -- after all codes are used up, and
 * compression rate changes, start over.
 */

static int      clear_flg;
static long     ratio;
static long     checkpoint;
VOID            upd_tab();

/*
 * the next two codes should not be changed lightly, as they must not lie
 * within the contiguous general code space.
 */
#define FIRST   257		/* first free entry */
#define CLEAR   256		/* table clear output code */

/*
 * The cl_block() routine is called at each checkpoint to determine if
 * compression would likely improve by resetting the code table.
 */

static VOID
cl_block(t)			/* table clear for block compress */
	FILE           *t;	/* our output file */
{
	long            rat;

	checkpoint = in_count + Check_Gap;

	if (in_count > 0x007fffffL) {	/* shift will overflow */
		rat = bytes_out >> 8;
		if (rat == 0)	/* Don't divide by zero */
			rat = 0x7fffffffL;
		else
			rat = in_count / rat;
	} else
		rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */

	if (rat > ratio)
		ratio = rat;
	else {
		ratio = 0;
		setmem(htab, Hsize * sizeof(long), 0xff);
		free_ent = FIRST;
		clear_flg = 1;
		putcode(CLEAR, t);
	}
}

#define FLUSH_BUF(bytes)	\
do {	bytes_out += bytes;\
	outbeg += bytes;\
	if (outend - outbeg < Bits) {\
		putb_pak(outbuf, (u_int) (bytes_out - bytes_last), t);\
		bytes_last = bytes_out;\
		outbeg = outbuf;\
	}\
	offset = 0;\
} while (0)

/*****************************************************************
 *
 * Output a given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (LONG)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  When the buffer fills up empty it and start over.
 */

static VOID
putcode(code, t)		/* output a code */
	int             code;	/* code to output */
	FILE           *t;	/* where to put it */
{
	int             r_off = offset;	/* right offset */
	int             bits = n_bits;	/* bits to go */
	u_char          *bp = outbeg;	/* buffer pointer */

	register int    ztmp;

	bp += (r_off >> 3);	/* Get to the first byte. */
	r_off &= 7;

	/*
	 * Since code is always >= 8 bits, only need to mask the
	 * first hunk on the left.
	 */
	ztmp = (code << r_off) & lmask[r_off];
	*bp = (*bp & rmask[r_off]) | ztmp;
	bp++;
	bits -= (8 - r_off);
	code >>= (8 - r_off);

	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if (bits >= 8) {
		*bp++ = code;
		code >>= 8;
		bits -= 8;
	}
	/* Last bits. */
	if (bits)
		*bp = code;
	offset += n_bits;

	if (offset == (n_bits << 3))
		FLUSH_BUF(n_bits);
	/*
	 * If the next entry is going to be too big for the code
	 * size, then increase it, if possible.
	 */
	if (free_ent > maxcode || clear_flg > 0) {
		/*
		 * Write the whole buffer, because the input side
		 * won't discover the size increase until after it
		 * has read it.
		 */
		if (offset > 0)
			FLUSH_BUF(n_bits);

		if (clear_flg) {	/* reset if clearing */
			maxcode = MAXCODE(n_bits = INIT_BITS);
			clear_flg = 0;
		} else {/* else use more bits */
			n_bits++;
			if (n_bits == Bits)
				maxcode = max_maxcode;
			else
				maxcode = MAXCODE(n_bits);
		}
	}
}

/*****************************************************************
 *
 * Read codes from the input file.  If EOF, return -1.
 * Inputs:
 *      cmpin
 * Outputs:
 *      code or -1 is returned.
 */

static int
getcode(f)			/* get a code */
	FILE           *f;	/* file to get from */
{
	int             code;
	static int      size = 0;
	int             r_off, bits;
	u_char          *bp = (u_char *) buf;

	if (clear_flg > 0 || offset >= size || free_ent > maxcode) {
		/*
		 * If the next entry will be too big for the current code
		 * size, then we must increase the size. This implies reading
		 * a new buffer full, too.
		 */
		if (free_ent > maxcode) {
			n_bits++;
			if (n_bits == Bits)
				maxcode = max_maxcode;	/* won't get any bigger
							 * now */
			else
				maxcode = MAXCODE(n_bits);
		}
		if (clear_flg > 0) {
			maxcode = MAXCODE(n_bits = INIT_BITS);
			clear_flg = 0;
		}
		for (size = 0; size < n_bits; size++) {
			if (inbeg >= inend) {
				u_int inlen = getb_unp(f);
				if (inlen == 0) {
					code = EOF;
					break;
				} else {
					inbeg = pinbuf;
					inend = &inbeg[inlen];
				}
			}
			code = *inbeg++;
			buf[size] = (char) code;
		}
		if (size <= 0)
			return -1;	/* end of file */

		offset = 0;
		/* Round size down to integral number of codes */
		size = (size << 3) - (n_bits - 1);
	}
	r_off = offset;
	bits = n_bits;

	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;

	/* Get first part (low order bits) */
	code = (*bp++ >> r_off);
	bits -= 8 - r_off;
	r_off = 8 - r_off;	/* now, offset into code word */

	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if (bits >= 8) {
		code |= *bp++ << r_off;
		r_off += 8;
		bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
	offset += n_bits;

	return code & MAXCODE(Bits);
}

static VOID
inittabs()
{
	if (!htab) {
		if (!(htab = (long *)malloc(SQHSIZE * sizeof(long))))
			arcdie("Not enough memory for crunch table.");
		if (!(codetab = (u_short *)malloc(SQHSIZE * sizeof(u_short))))
			arcdie("Not enough memory for crunch code table.");
		prefix = codetab;
		suffix = (u_char *)htab;
		stack = (u_char *) & htab[1 << SQBITS];
		string_tab = (struct entry *) htab;
	}
}

/*
 * compress a file
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the prefix
 * code / next character combination.  We do a variant of Knuth's algorithm D
 * (vol. 3, sec. 6.4) along with G. Knott's relatively-prime secondary probe.
 * Here, the modular division first probe is gives way to a faster
 * exclusive-or manipulation.  Also do block compression with an adaptive
 * reset, where the code table is cleared when the compression ratio
 * decreases, but after the table fills.  The variable-length output codes
 * are re-sized at this point, and a special CLEAR code is generated for the
 * decompressor.
 */

VOID
init_cm(buf)			/* initialize for compression */
	u_char          *buf;	/* input buffer */
{
	offset = 0;
	clear_flg = 0;
	ratio = 0;
	in_count = 1;
	in_off = 1;
	maxcode = MAXCODE(n_bits = INIT_BITS);
	free_ent = FIRST;
	n_bits = INIT_BITS;	/* set starting code size */
	outbeg = outbuf;
	bytes_last = 0;
	inittabs();

	if (dosquash) {
		Bits = SQBITS;
		Hsize = SQHSIZE;
		Check_Gap = SQGAP;
		bytes_out = 0;
	} else {
		Bits = CRBITS;
		Hsize = CRHSIZE;
		Check_Gap = CRGAP;
		bytes_out = 1;
		*outbeg++ = Bits;	/* note our max code length */
	}
	checkpoint = Check_Gap;
	max_maxcode = 1 << Bits;
	setmem(htab, Hsize * sizeof(long), 0xff);

	ent = *buf;
	hshift = 0;
	for (fcode = (long) Hsize; fcode < 65536L; fcode *= 2L)
		hshift++;
	hshift = 8 - hshift;
}

VOID
lzw_buf(buf, len, t)		/* compress a character */
	u_char          *buf;	/* buffer to compress */
	u_int            len;	/* length of buffer */
	FILE           *t;	/* where to put it */
{
	int             i, j;
	int             disp;

	j = in_off;
	buf += in_off;
	in_off = 0;
	for (; j < len; j++, buf++) {
		in_count++;

		fcode = (long) (((long) *buf << Bits) + ent);
		i = (*buf << hshift) ^ ent;	/* xor hashing */

		if (htab[i] == fcode) {
			ent = codetab[i];
			continue;
		} else if (htab[i] < 0)	/* empty slot */
			goto nomatch;
		disp = Hsize - i;	/* secondary hash (after G.Knott) */
		if (i == 0)
			disp = 1;

probe:
		if ((i -= disp) < 0)
			i += Hsize;

		if (htab[i] == fcode) {
			ent = codetab[i];
			continue;
		}
		if (htab[i] > 0)
			goto probe;

nomatch:
		putcode(ent, t);
		ent = *buf;
		if (free_ent < max_maxcode) {
			codetab[i] = free_ent++;	/* code -> hashtable */
			htab[i] = fcode;
		} else if (in_count >= checkpoint)
			cl_block(t);	/* check for adaptive reset */
	}
}

long
pred_cm(t)			/* report compressed size */
	FILE           *t;	/* where to put it */
{
	putcode(ent, t);	/* put out the final code */

	offset = (offset + 7) / 8;
	bytes_out += offset;

	return bytes_out;	/* say how big it got */
}

VOID
flsh_cm(t)			/* flush compressed file */
	FILE	      *t;
{
	putb_pak(outbuf, (u_int) (bytes_out - bytes_last), t);
}

/*
 * Decompress a file.  This routine adapts to the codes in the file building
 * the string table on-the-fly; requiring no table to be stored in the
 * compressed file.  The tables used herein are shared with those of the
 * compress() routine.  See the definitions above.
 */

VOID
decomp(squash, f, t)		/* decompress a file */
	int		squash;	/* squashed or crunched? */
	FILE           *f;	/* file to read codes from */
	FILE           *t;	/* file to write text to */
{
	u_char          *stackp;
	int             finchar;
	int             code, oldcode, incode;
	VOID            (*output) PROTO((u_char *buf, u_int len, FILE *f));
	u_int		inlen;

	inlen = getb_unp(f);
	inbeg = pinbuf;
	inend = &inbeg[inlen];
	outbeg = outbuf;
	inittabs();

	if (squash) {
		Bits = SQBITS;
		output = putb_unp;
	} else {
		Bits = CRBITS;
		output = putb_ncr;
		if ((code = *inbeg++) != CRBITS)
			arcdie("File packed with %d bits, I can only handle %d",
			      code, CRBITS);
	}

	if (inlen<=0)
		return;

	max_maxcode = 1 << Bits;
	clear_flg = 0;

	n_bits = INIT_BITS;	/* set starting code size */

	/*
	 * As above, initialize the first 256 entries in the table.
	 */
	maxcode = MAXCODE(n_bits);
	setmem(prefix, 256 * sizeof(short), 0);	/* reset decode string table */
	for (code = 255; code >= 0; code--)
		suffix[code] = (u_char) code;

	free_ent = FIRST;

	finchar = oldcode = getcode(f);
	if (oldcode == -1)	/* EOF already? */
		return;		/* Get out of here */
	*outbeg++ = finchar;	/* first code must be 8 bits=char */
	stackp = stack;

	while ((code = getcode(f)) > -1) {
		if (code == CLEAR) {	/* reset string table */
			setmem(prefix, 256 * sizeof(short), 0);
			clear_flg = 1;
			free_ent = FIRST - 1;
			if ((code = getcode(f)) == -1)	/* O, untimely death! */
				break;
		}
		incode = code;
		/*
		 * Special case for KwKwK string.
		 */
		if (code >= free_ent) {
			if (code > free_ent) {
				if (warn) {
					printf("Corrupted compressed file.\n");
					printf("Invalid code %d when max is %d.\n",
					       code, free_ent);
				}
				nerrs++;
				break;
			}
			*stackp++ = finchar;
			code = oldcode;
		}
		/*
		 * Generate output characters in reverse order
		 */
		while (code >= 256) {
			*stackp++ = suffix[code];
			code = prefix[code];
		}
		*stackp++ = finchar = suffix[code];

		/*
		 * And put them out in forward order
		 */
		do {
			*outbeg++ = *--stackp;
			if (outbeg >= outend) {
				(*output) (outbuf, outbeg-outbuf, t);
				outbeg = outbuf;
			}
		} while (stackp > stack);

		/*
		 * Generate the new entry.
		 */
		if ((code = free_ent) < max_maxcode) {
			prefix[code] = (u_short) oldcode;
			suffix[code] = finchar;
			free_ent = code + 1;
		}
		/*
		 * Remember previous code.
		 */
		oldcode = incode;
	}

	if (outbeg > outbuf)
		(*output) (outbuf, outbeg-outbuf, t);
}


/*************************************************************************
 * Please note how much trouble it can be to maintain upwards            *
 * compatibility.  All that follows is for the sole purpose of unpacking *
 * files which were packed using an older method.                        *
 *************************************************************************/


/*
 * The h() pointer points to the routine to use for calculating a hash value.
 * It is set in the init routines to point to either of oldh() or newh().
 *
 * oldh() calculates a hash value by taking the middle twelve bits of the square
 * of the key.
 *
 * newh() works somewhat differently, and was tried because it makes ARC about
 * 23% faster.  This approach was abandoned because dynamic Lempel-Zev
 * (above) works as well, and packs smaller also.  However, inadvertent
 * release of a developmental copy forces us to leave this in.
 */

static          u_short(*h) ();	/* pointer to hash function */

static          u_short
oldh(pred, foll)		/* old hash function */
	u_short          pred;	/* code for preceeding string */
	u_char           foll;	/* value of following char */
{
	long            local;	/* local hash value */

	local = ((pred + foll) | 0x0800) & 0xFFFF;	/* create the hash key */
	local *= local;		/* square it */
	return (local >> 6) & 0x0FFF;	/* return the middle 12 bits */
}

static          u_short
newh(pred, foll)		/* new hash function */
	u_short          pred;	/* code for preceeding string */
	u_char           foll;	/* value of following char */
{
	return (((pred + foll) & 0xFFFF) * 15073) & 0xFFF;	/* faster hash */
}

/*
 * The eolist() function is used to trace down a list of entries with
 * duplicate keys until the last duplicate is found.
 */

static          u_short
eolist(index)			/* find last duplicate */
	u_short          index;
{
	int             temp;

	while ((temp = string_tab[index].next))	/* while more duplicates */
		index = temp;

	return index;
}

/*
 * The hash() routine is used to find a spot in the hash table for a new
 * entry.  It performs a "hash and linear probe" lookup, using h() to
 * calculate the starting hash value and eolist() to perform the linear
 * probe.  This routine DOES NOT detect a table full condition.  That MUST be
 * checked for elsewhere.
 */

static          u_short
hash(pred, foll)		/* find spot in the string table */
	u_short          pred;	/* code for preceeding string */
	u_char           foll;	/* char following string */
{
	u_short          local, tempnext;	/* scratch storage */
	struct entry   *ep;	/* allows faster table handling */

	local = (*h) (pred, foll);	/* get initial hash value */

	if (!string_tab[local].used)	/* if that spot is free */
		return local;	/* then that's all we need */

	else {			/* else a collision has occured */
		local = eolist(local);	/* move to last duplicate */

		/*
		 * We must find an empty spot. We start looking 101 places
		 * down the table from the last duplicate.
		 */

		tempnext = (local + 101) & 0x0FFF;
		ep = &string_tab[tempnext];	/* initialize pointer */

		while (ep->used) {	/* while empty spot not found */
			if (++tempnext == TABSIZE) {	/* if we are at the end */
				tempnext = 0;	/* wrap to beginning of table */
				ep = string_tab;
			} else
				++ep;	/* point to next element in table */
		}

		/*
		 * local still has the pointer to the last duplicate, while
		 * tempnext has the pointer to the spot we found.  We use
		 * this to maintain the chain of pointers to duplicates.
		 */

		string_tab[local].next = tempnext;

		return tempnext;
	}
}

/*
 * The init_tab() routine is used to initialize our hash table. You realize,
 * of course, that "initialize" is a complete misnomer.
 */

static VOID
init_tab()
{				/* set ground state in hash table */
	unsigned int    i;	/* table index */

	setmem((char *) string_tab, TABSIZE * sizeof(struct entry), 0);

	for (i = 0; i < 256; i++)	/* list all single byte strings */
		upd_tab(NO_PRED, i);
}

/*
 * The upd_tab routine is used to add a new entry to the string table. As
 * previously stated, no checks are made to ensure that the table has any
 * room.  This must be done elsewhere.
 */

VOID
upd_tab(pred, foll)		/* add an entry to the table */
	u_short          pred;	/* code for preceeding string */
	u_short          foll;	/* character which follows string */
{
	struct entry   *ep;	/* pointer to current entry */

	/* calculate offset just once */

	ep = &string_tab[hash(pred, foll)];

	ep->used = TRUE;	/* this spot is now in use */
	ep->next = 0;		/* no duplicates after this yet */
	ep->predecessor = pred;	/* note code of preceeding string */
	ep->follower = foll;	/* note char after string */
}

/*
 * This algorithm encoded a file into twelve bit strings (three nybbles). The
 * gocode() routine is used to read these strings a byte (or two) at a time.
 */

#define	GOCODE(x)\
if ((inflag^=1)) { x = (*inbeg++ << 4); x |= (*inbeg >> 4); } \
else {x = (*inbeg++ & 0x0f) << 8; x |= (*inbeg++); }

/* push char onto stack */
#define	PUSH(c)	\
do {	stack[sp] = ((char) (c)); \
	if (++sp >= TABSIZE) \
		arcdie("Stack overflow\n"); \
} while (0)

/* pop character from stack */
#define POP()	((sp > 0) ? (int) stack[--sp] : EMPTY)

/***** LEMPEL-ZEV DECOMPRESSION *****/

static int      code_count;	/* needed to detect table full */
static int      oldcode, finchar;

VOID
init_ucr(new, f)		/* get set for uncrunching */
	int             new;	/* true to use new hash function */
	FILE	       *f;	/* input file */
{
	if (new)		/* set proper hash function */
		h = newh;
	else
		h = oldh;

	inittabs();		/* allocate table space */
	sp = 0;			/* clear out the stack */
	init_tab();		/* set up atomic code definitions */
	code_count = TABSIZE - 256;	/* note space left in table */
	inbeg = pinbuf;
	inend = &inbeg[getb_unp(f)];
	inflag = 0;
	GOCODE(oldcode);
	finchar = string_tab[oldcode].follower;
	outbeg = outbuf;
	*outbeg++ = finchar;
}

u_int
getb_ucr(f)			/* get next uncrunched byte */
	FILE           *f;	/* file containing crunched data */
{
	int             code, newcode;
	struct entry   *ep;	/* allows faster table handling */
	u_int		len;

	do {
	if (!sp) {		/* if stack is empty */
		if (inbeg >= inend-1) {
			inbeg = pinbuf;
			inend = &inbeg[getb_unp(f)];
			if (inbeg == inend) {
				break;
			}
		}
		GOCODE(newcode);
		code = newcode;

		ep = &string_tab[code];	/* initialize pointer */

		if (!ep->used) {/* if code isn't known */
			code = oldcode;
			ep = &string_tab[code];	/* re-initialize pointer */
			PUSH(finchar);
		}
		while (ep->predecessor != NO_PRED) {
			PUSH(ep->follower);	/* decode string backwards */
			code = ep->predecessor;
			ep = &string_tab[code];
		}

		PUSH(finchar = ep->follower);	/* save first character also */

		/*
		 * The above loop will terminate, one way or another, with
		 * string_tab[code].follower equal to the first character in
		 * the string.
		 */

		if (code_count) {	/* if room left in string table */
			upd_tab(oldcode, finchar);
			--code_count;
		}
		oldcode = newcode;
	}
	*outbeg++ = POP();
	} while (outbeg <= outend);
	len = outbeg - outbuf;
	outbeg = outbuf;
	return (len);
}
