/*
 * $Header: /cvsroot/arc/arc/arcio.c,v 1.2 2003/10/31 02:22:36 highlandsun Exp $
 */

/*  ARC - Archive utility - ARCIO

    Version 2.50, created on 04/22/87 at 13:25:20

(C) COPYRIGHT 1985,86 by System Enhancement Associates; ALL RIGHTS RESERVED

    By:	 Thom Henderson

    Description:
	 This file contains the file I/O routines used to manipulate
	 an archive.

    Language:
	 Computer Innovations Optimizing C86
*/
#include <stdio.h>
#include "arc.h"
#if	_MTS
#include <mts.h>
#endif
#include <string.h>

VOID	arcdie();

int
readhdr(hdr, f)			/* read a header from an archive */
	struct heads   *hdr;	/* storage for header */
	FILE           *f;	/* archive to read header from */
{
#if	!MSDOS
	unsigned char   dummy[28];
	int             i;
#endif
	char            name[FNLEN];	/* filename buffer */
	int             try = 0;/* retry counter */
	static int      first = 1;	/* true only on first read */

	if (!f)			/* if archive didn't open */
		return 0;	/* then pretend it's the end */
	hdrver = fgetc(f);
	if (feof(f))		/* if no more data */
		return 0;	/* then signal end of archive */

	if (hdrver != ARCMARK) {	/* check archive validity */
		if (warn) {
			printf("An entry in %s has a bad header.\n", arcname);
			nerrs++;
		}
		printf("hdrver: %x\n", hdrver);
		while (!feof(f)) {
			try++;
			if (fgetc(f) == ARCMARK) {
				ungetc(hdrver = fgetc(f), f);
				if (!(hdrver & 0x80) && hdrver <= ARCVER)
					break;
			}
		}

		if (feof(f) && first)
			arcdie("%s is not an archive", arcname);

		if (changing && warn)
			arcdie("%s is corrupted -- changes disallowed", arcname);

		if (warn)
			printf("  %d bytes skipped.\n", try);

		if (feof(f))
			return 0;
	}
	hdrver = fgetc(f);	/* get header version */
	if (hdrver & 0x80)	/* sign bit? negative? */
		arcdie("Invalid header in archive %s", arcname);
	if (hdrver == 0)
		return 0;	/* note our end of archive marker */
	if (hdrver > ARCVER) {
		fread(name, sizeof(char), FNLEN, f);
#if	_MTS
		atoe(name, strlen(name));
#endif
		printf("I don't know how to handle file %s in archive %s\n",
		       name, arcname);
		printf("I think you need a newer version of ARC.\n");
		exit(1);
	}
	/* amount to read depends on header type */

	if (hdrver == 1) {	/* old style is shorter */
		fread(hdr, sizeof(struct heads) - sizeof(long int), 1, f);
		hdrver = 2;	/* convert header to new format */
		hdr->length = hdr->size;	/* size is same when not
						 * packed */
	} else
#if	MSDOS
		fread(hdr, sizeof(struct heads), 1, f);
#else
		fread(dummy, 27, 1, f);

	for (i = 0; i < FNLEN; hdr->name[i] = dummy[i], i++);
#if	_MTS
	(void) atoe(hdr->name, strlen(hdr->name));
#endif
	for (i = 0, hdr->size=0; i<4; hdr->size<<=8, hdr->size += dummy[16-i], i++);
	hdr->date = (short) ((dummy[18] << 8) + dummy[17]);
	hdr->time = (short) ((dummy[20] << 8) + dummy[19]);
	hdr->crc = (short) ((dummy[22] << 8) + dummy[21]);
	for (i = 0, hdr->length=0; i<4; hdr->length<<=8, hdr->length += dummy[26-i], i++);
#endif

	if (hdr->date > olddate
	    || (hdr->date == olddate && hdr->time > oldtime)) {
		olddate = hdr->date;
		oldtime = hdr->time;
	}
	first = 0;
	return 1;		/* we read something */
}

VOID
put_int(number, f)		/* write a 2 byte integer */
	short           number;
	FILE           *f;
{
	fputc((char) (number & 255), f);
	fputc((char) (number >> 8), f);
}

VOID
put_long(number, f)		/* write a 4 byte integer */
	long            number;
	FILE           *f;
{
	put_int((short) (number & 0xFFFF), f);
	put_int((short) (number >> 16), f);
}

VOID
writehdr(hdr, f)		/* write a header to an archive */
	struct heads   *hdr;	/* header to write */
	FILE           *f;	/* archive to write to */
{
	fputc(ARCMARK, f);		/* write out the mark of ARC */
	fputc(hdrver, f);	/* write out the header version */
	if (!hdrver)		/* if that's the end */
		return;		/* then write no more */
#if	MSDOS
	fwrite(hdr, sizeof(struct heads), 1, f);
#else
	/* byte/word ordering hassles... */
#if	_MTS
	etoa(hdr->name, strlen(hdr->name));
#endif
	fwrite(hdr->name, 1, FNLEN, f);
	put_long(hdr->size, f);
	put_int(hdr->date, f);
	put_int(hdr->time, f);
	put_int(hdr->crc, f);
	put_long(hdr->length, f);
#endif

	/* note the newest file for updating the archive timestamp */

	if (hdr->date > arcdate
	    || (hdr->date == arcdate && hdr->time > arctime)) {
		arcdate = hdr->date;
		arctime = hdr->time;
	}
}

extern char	*pinbuf;	/* general purpose input buffer */

/*
 * NOTE:  The filecopy() function is used to move large numbers of bytes from
 * one file to another.  This particular version has been modified to improve
 * performance in Computer Innovations C86 version 2.3 in the small memory
 * model.  It may not work as expected with other compilers or libraries, or
 * indeed with different versions of the CI-C86 compiler and library, or with
 * the same version in a different memory model.
 * 
 * The following is a functional equivalent to the filecopy() routine that
 * should work properly on any system using any compiler, albeit at the cost
 * of reduced performance:
 * 
 * filecopy(f,t,size) 
 *      FILE *f, *t; long size; 
 * { 
 *      while(size--)
 *              putc_tst(fgetc(f),t); 
 * }
 */
#if	MSDOS
#include <fileio2.h>

filecopy(f, t, size)		/* bulk file copier */
	FILE           *f, *t;	/* files from and to */
	long            size;	/* bytes to copy */
{
	unsigned int    bufl;	/* buffer length */
	unsigned int    coreleft();	/* space available reporter */
	unsigned int    cpy;	/* bytes being copied */
	long            floc, tloc, fseek();	/* file pointers, setter */
	struct regval   reg;	/* registers for DOS calls */

	bufl = (MYBUF > size) ? (u_int) size : MYBUF;

	floc = fseek(f, 0L, 1);	/* reset I/O system */
	tloc = fseek(t, 0L, 1);

	segread(&reg.si);	/* set segment registers for DOS */

	while (size > 0) {	/* while more to copy */
		reg.ax = 0x3F00;/* read from handle */
		reg.bx = filehand(f);
		reg.cx = bufl < size ? bufl : size;	/* amount to read */
		reg.dx = pinbuf;
		if (sysint21(&reg, &reg) & 1)
			arcdie("Read fail");

		cpy = reg.ax;	/* amount actually read */
		reg.ax = 0x4000;/* write to handle */
		reg.bx = filehand(t);
		reg.cx = cpy;
		reg.dx = pinbuf;
		sysint21(&reg, &reg);

		if (reg.ax != cpy)
			arcdie("Write fail (disk full?)");

		size -= (long) cpy;
	}
}
#else

VOID
filecopy(f, t, size)		/* bulk file copier */
	FILE           *f, *t;	/* files from and to */
	long            size;	/* bytes to copy */
{
	unsigned int    bufl;	/* buffer length */
	unsigned int    cpy;	/* bytes being copied */

	bufl = (MYBUF > size) ? (u_int) size : MYBUF;

	while (size > 0) {
		cpy = fread(pinbuf, sizeof(char), bufl, f);
		if (fwrite(pinbuf, sizeof(char), cpy, t) != cpy)
			arcdie("Write fail (no space?)");
		size -= cpy;
		if (bufl > size)
			bufl = size;
		if (ferror(f))
			arcdie("Unexpected EOF copying archive");
		if (!cpy) break;
	}
}
#endif
