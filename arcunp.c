/*
 * $Header: /cvsroot/arc/arc/arcunp.c,v 1.2 2003/10/31 02:22:36 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCUNP
 * 
 * Version 3.17, created on 02/13/86 at 10:20:08
 * 
 * (C) COPYRIGHT 1985 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:  Thom Henderson
 * 
 * Description: This file contains the routines used to expand a file when
 * taking it out of an archive.
 * 
 * Language: Computer Innovations Optimizing C86
 */
#include <stdio.h>
#include "arc.h"
#if	_MTS
#include <mts.h>
#endif

VOID	setcode(), init_usq(), init_ucr(), decomp();
VOID	arcdie(), codebuf();

#include "proto.h"

/* stuff for repeat unpacking */

#define DLE 0x90		/* repeat byte flag */

extern u_char   state;		/* repeat unpacking state */
extern int	lastc;

/* repeat unpacking states */

#define NOHIST 0		/* no relevant history */
#define INREP 1			/* sending a repeated value */

short    crcval;		/* CRC check value */
long     stdlen;		/* bytes to read */
#if	!DOS
static int	gotcr;		/* got a carriage return? */
#endif

extern u_char	*pinbuf, *pakbuf, *outbuf;

int
unpack(f, t, hdr)		/* unpack an archive entry */
	FILE           *f, *t;	/* source, destination */
	struct heads   *hdr;	/* pointer to file header data */
{
	u_int		len;

	/* setups common to all methods */
#if	!DOS
	gotcr = 0;
#endif
	crcval = 0;		/* reset CRC check value */
	stdlen = hdr->size;	/* set input byte counter */
	state = NOHIST;		/* initial repeat unpacking state */
	setcode();		/* set up for decoding */

	/* use whatever method is appropriate */

	switch (hdrver) {	/* choose proper unpack method */
	case 1:		/* standard packing */
	case 2:
		do {
			len = getb_unp(f);
			putb_unp(pinbuf, len, t);
		} while (len == MYBUF);
		break;

	case 3:		/* non-repeat packing */
		do {
			len = getb_unp(f);
			putb_ncr(pinbuf, len, t);
		} while (len == MYBUF);
		break;

	case 4:		/* Huffman squeezing */
		init_usq(f);
		do {
			len = getb_usq(f);
			putb_ncr(outbuf, len, t);
		} while (len == MYBUF);
		break;

	case 5:		/* Lempel-Zev compression */
		init_ucr(0, f);
		do {
			len = getb_ucr(f);
			putb_unp(outbuf, len, t);
		} while (len == MYBUF);
		break;

	case 6:		/* Lempel-Zev plus non-repeat */
		init_ucr(0, f);
		do {
			len = getb_ucr(f);
			putb_ncr(outbuf, len, t);
		} while (len == MYBUF);
		break;

	case 7:		/* L-Z plus ncr with new hash */
		init_ucr(1, f);
		do {
			len = getb_ucr(f);
			putb_ncr(outbuf, len, t);
		} while (len == MYBUF);
		break;

	case 8:		/* dynamic Lempel-Zev */
		decomp(0, f, t);
		break;

	case 9:		/* Squashing */
		decomp(1, f, t);
		break;

	default:		/* unknown method */
		if (warn) {
			printf("I don't know how to unpack file %s\n", hdr->name);
			printf("I think you need a newer version of ARC\n");
			nerrs++;
		}
		fseek(f, hdr->size, 1);	/* skip over bad file */
		return 1;	/* note defective file */
	}

	/* cleanups common to all methods */

	if (crcval != hdr->crc) {
		if (warn || kludge) {
			printf("WARNING: File %s fails CRC check\n", hdr->name);
			nerrs++;
		}
		return 1;	/* note defective file */
	}
	return 0;		/* file is okay */
}

/*
 * This routine is used to put bytes in the output file.  It also performs
 * various housekeeping functions, such as maintaining the CRC check value.
 */

VOID
putb_unp(buf, len, t)
	u_char	*buf;
	u_int	len;
	FILE	*t;
{
	u_int i, j;

	crcval = crcbuf(crcval, len, buf);

	if (!t)
		return;
#if	!DOS
	if (!image) {
#if	_MTS
		atoe(buf, len);
#endif
		if (gotcr) {
			gotcr = 0;
			if (buf[0] != '\n')
				putc('\r', t);
		}

		for (i=0,j=0; i<len; i++)
			if (buf[i] != '\r' || buf[i+1] != '\n')
				buf[j++] = buf[i];
		len = j;

		if (buf[len-1] == '\r') {
			len--;
			gotcr = 1;
		}
	}
#endif	/* !DOS */
	i=fwrite(buf, 1, len, t);
	if (i != len)
		arcdie("Write fail");
}

/*
 * This routine is used to decode non-repeat compression.  Bytes are passed
 * one at a time in coded format, and are written out uncoded. The data is
 * stored normally, except that runs of more than two characters are
 * represented as:
 * 
 * <char> <DLE> <count>
 * 
 * With a special case that a count of zero indicates a DLE as data, not as a
 * repeat marker.
 */

VOID
putb_ncr(buf, len, t)		/* put NCR coded bytes */
	u_char	*buf;
	u_int	len;
	FILE	*t;		/* file to receive data */
{
	u_char	*pakptr=pakbuf;
	u_int	i;

	for (i=0; i<len; i++) {
		if (state == INREP) {
			if (buf[i])
				while (--buf[i])
					*pakptr++ = lastc;
			else
				*pakptr++ = DLE;
			state = NOHIST;
		} else {
			if (buf[i] != DLE)
				*pakptr++ = lastc = buf[i];
			else
				state = INREP;
		}

		if (pakptr - pakbuf > MYBUF) {
			putb_unp(pakbuf, (u_int) (pakptr-pakbuf), t);
			pakptr = pakbuf;
		}
	}
	putb_unp(pakbuf, (u_int) (pakptr-pakbuf), t);
}

/*
 * This routine reads a buffer of data from an archive.
 */

u_int
getb_unp(f)
	FILE		*f;
{
	register u_int len;

	if (stdlen) {
		len = (stdlen < MYBUF) ? stdlen : MYBUF;

		len = fread(pinbuf, 1, len, f);

		if (password)
			codebuf(pinbuf, len);
	
		stdlen -= len;
	} else
		len = 0;
	
	return (len);
}
