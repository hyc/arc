/*
 * $Header: /cvsroot/arc/arc/arcpack.c,v 1.2 2003/10/31 02:22:36 highlandsun Exp $
 */

/*  ARC - Archive utility - ARCPACK

    Version 3.49, created on 04/21/87 at 11:26:51

(C) COPYRIGHT 1985-87 by System Enhancement Associates; ALL RIGHTS RESERVED

    By:	 Thom Henderson

    Description:
	 This file contains the routines used to compress a file
	 when placing it in an archive.

    Language:
	 Computer Innovations Optimizing C86
*/
#include <stdio.h>
#include "arc.h"
#if	_MTS
#include <ctype.h>
#endif

#include "proto.h"

VOID		setcode(), init_cm(), codebuf();
VOID		arcdie(), init_sq(), flsh_cm();
int		crcbuf();
u_int		ncr_buf();

int		lastc;

/* stuff for non-repeat packing */

#define DLE 0x90		/* repeat sequence marker */

u_char	state;		/* current packing state */

/* non-repeat packing states */

#define NOHIST	0		/* don't consider previous input */
#define SENTCHAR 1		/* lastchar set, no lookahead yet */

extern	u_char	*pinbuf;
extern	u_char	*pakbuf;	/* worst case, 2*inbuf */

/* packing results */

long		stdlen;		/* length for standard packing */
short		crcval;		/* CRC check value */

VOID
pack(f, t, hdr)			/* pack file into an archive */
	FILE	       *f, *t;	/* source, destination */
	struct heads   *hdr;	/* pointer to header data */
{
	long		ncrlen; /* length after packing */
	long		huflen; /* length after squeezing */
	long		lzwlen; /* length after crunching */
	long		pred_sq(), head_sq(), huf_buf();	/* stuff for squeezing */
	long		pred_cm();	/* dynamic crunching cleanup */
	long		tloc, ftell();	/* start of output */
	u_int		inbytes, pakbytes;

	/* first pass - see which method is best */

	tloc = ftell(t);	/* note start of output */

	if (!nocomp) {		/* if storage kludge not active */
		if (note) {
			printf(" analyzing, ");
			fflush(stdout);
		}
		state = NOHIST; /* initialize ncr packing */
		stdlen = ncrlen = 0;	/* reset size counters */
		huflen = lzwlen = 0;
		crcval = 0;	/* initialize CRC check value */
		setcode();	/* initialize encryption */
		init_sq();	/* initialize for squeeze scan */

		inbytes = getbuf(f);
		if (inbytes) {

			init_cm(pinbuf);

			for (;; inbytes = getbuf(f)) {
				pakbytes = ncr_buf(inbytes);
				ncrlen += pakbytes;
				hufb_tab(pakbuf, pakbytes);
				if (dosquash)
					lzw_buf(pinbuf, inbytes, t);
				else
					lzw_buf(pakbuf, pakbytes, t);
				if (inbytes < MYBUF)
					break;
			}
			lzwlen = pred_cm(t);
			huflen = pred_sq();
		}
	} else {		/* else kludge the method */
		stdlen = 0;	/* make standard look best */
		ncrlen = huflen = lzwlen = 1;
	}

	/* standard set-ups common to all methods */

	hdr->crc = crcval;	/* note CRC check value */
	hdr->length = stdlen;	/* set actual file length */
	if (stdlen > MYBUF) {
		fseek(f, 0L, 0);/* rewind input */
		state = NOHIST; /* reinitialize ncr packing */
		setcode();	/* reinitialize encryption */
	} else
		inbytes = stdlen;

	/* choose and use the shortest method */

	if (kludge && note)
		printf("\n\tS:%ld  P:%ld  S:%ld	 C:%ld,\t ",
		       stdlen, ncrlen, huflen, lzwlen);

	if (stdlen <= ncrlen && stdlen <= huflen && stdlen <= lzwlen) {
		if (note) {
			printf("storing, ");	/* store without compression */
			fflush(stdout);
		}
		hdrver = 2;	/* note packing method */
		fseek(t, tloc, 0);	/* reset output for new method */
		if (nocomp || (stdlen > MYBUF)) {
			stdlen = crcval = 0;
			while ((inbytes = getbuf(f)) != 0)
				putb_pak(pinbuf, inbytes, t); /* store it straight */
		} else
			putb_pak(pinbuf, inbytes, t);
		hdr->crc = crcval;
		hdr->length = hdr->size = stdlen;
	} else if (ncrlen < lzwlen && ncrlen < huflen) {
		if (note) {
			printf("packing, ");	/* pack with repeat */
			fflush(stdout); /* suppression */
		}
		hdrver = 3;	/* note packing method */
		hdr->size = ncrlen;	/* set data length */
		fseek(t, tloc, 0);	/* reset output for new method */
		if (stdlen > MYBUF) {
			do {
				inbytes = getbuf(f);
				pakbytes = ncr_buf(inbytes);
				putb_pak(pakbuf, pakbytes, t);
			} while (inbytes != 0);
		} else
			putb_pak(pakbuf, pakbytes, t);
	} else if (huflen < lzwlen) {
		if (note) {
			printf("squeezing, ");
			fflush(stdout);
		}
		hdrver = 4;	/* note packing method */
		fseek(t, tloc, 0);	/* reset output for new method */
		huflen = head_sq();
		if (stdlen > MYBUF) {
			do {
				inbytes = getbuf(f);
				pakbytes = ncr_buf(inbytes);
				huflen += huf_buf(pakbuf, pakbytes, inbytes, t);
			} while (inbytes != 0);
		} else
			huflen += huf_buf(pakbuf, pakbytes, 0, t);

		hdr->size = huflen;	/* note final size */
	} else {
		if (note)
			printf(dosquash ? "squashed, " : "crunched, ");
		flsh_cm(t);
		hdrver = dosquash ? 9 : 8;
		hdr->size = lzwlen;	/* size should not change */
	}

	/* standard cleanups common to all methods */

	if (note)
		printf("done. (%ld%%)\n", hdr->length == 0 ?
		       0L : 100L - (100L * hdr->size) / hdr->length);
}

/*
 * Non-repeat compression - text is passed through normally, except that a
 * run of more than two is encoded as:
 *
 * <char> <DLE> <count>
 *
 * Special case: a count of zero indicates that the DLE is really a DLE, not a
 * repeat marker.
 */

u_int
ncr_buf(inbytes)
	u_int		inbytes;	/* number of bytes in inbuf */
{
	u_int		i;
	int		c;
	reg u_char     *inptr, *pakptr;
	static int	cnt;

	inptr = pinbuf;
	pakptr = pakbuf;

	if (state == NOHIST) {
		lastc = (-1);
		cnt = 1;
		state = SENTCHAR;
	}
	for (i = 0; i < inbytes; i++) {
		c = *inptr++;
		if (c == lastc && cnt < 255)
			cnt++;
		else {
			if (cnt == 2) {
				*pakptr++ = lastc;
			} else if (cnt > 2) {
				*pakptr++ = DLE;
				*pakptr++ = cnt;
			}
			*pakptr++ = c;
			lastc = c;
			cnt = 1;
		}
		if (c == DLE) {
			*pakptr++ = '\0';
			lastc = (-1);
		}
	}
	if (inbytes < MYBUF && cnt > 1) {	/* trailing stuff */
		if (cnt == 2)
			*pakptr++ = lastc;
		else {
			*pakptr++ = DLE;
			*pakptr++ = cnt;
		}
	}
	return (pakptr - pakbuf);
}

u_int
getbuf(f)
	FILE	       *f;
{
	u_int		i;
#if	!DOS
	int		c;
	static int	cr = 0;
	register u_char *ptr;
	if (image) {
#endif
		i = fread(pinbuf, 1, MYBUF, f);
#if	!DOS
	} else {
		ptr = pinbuf;
		for (i = 0, c = 0; (c != EOF) && (i < MYBUF); i++) {
			if (cr) {
				c = '\n';
				cr = 0;
			} else {
				c = fgetc(f);
				if (c == EOF)
					break;
				else if (c == '\n') {
					c = '\r';
					cr = 1;
				}
			}
			*ptr++ = c;
		}
#if	_MTS
		etoa(pinbuf, i);
#endif				/* _MTS */
	}
#endif				/* !DOS */
	crcval = crcbuf(crcval, i, pinbuf);
	stdlen += i;
	return (i);
}

VOID
putb_pak(buf, len, f)
	u_char	       *buf;
	u_int		len;
	FILE	       *f;
{
	u_int		i;

	if (f && len) {
		if (password)
			codebuf(buf, len);
		i = fwrite(buf, 1, len, f);
		if (i != len)
			arcdie("Write failed");
	}
}
