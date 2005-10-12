/*
 * $Header: /cvsroot/arc/arc/arcext.c,v 1.5 2005/10/12 15:22:18 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCEXT
 * 
 * Version 2.19, created on 10/24/86 at 14:53:32
 * 
 * (C) COPYRIGHT 1985 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:  Thom Henderson
 * 
 * Description: This file contains the routines used to extract files from an
 * archive.
 * 
 * Language: Computer Innovations Optimizing C86
 */
#include <stdio.h>
#include "arc.h"
#if	!MSDOS
#include <ctype.h>
#endif
#include <string.h>
#if	BSD
#include <strings.h>
#endif

VOID	openarc(), closearc(), setstamp();
int	match(), readhdr(), unpack();
static	VOID	extfile();

#ifndef	__STDC__
char	*malloc();
#ifndef _AIX
VOID	free();
#endif
#endif

VOID
extarc(num, arg, prt)		/* extract files from archive */
	int             num;	/* number of arguments */
	char           *arg[];	/* pointers to arguments */
	int             prt;		/* true if printing */
{
	struct heads    hdr;	/* file header */
	int             save;	/* true to save current file */
	int             did[MAXARG];/* true when argument was used */
	char           *i;	/* string index */
	char          **name; 	/* name pointer list */
	int             n;	/* index */

	name = (char **) malloc(num * sizeof(char *));	/* get storage for name
							 * pointers */

	for (n = 0; n < num; n++) {	/* for each argument */
		did[n] = 0;	/* reset usage flag */
#if	!_MTS
		if (!(i = rindex(arg[n], '\\')))	/* find start of name */
			if (!(i = rindex(arg[n], '/')))
				if (!(i = rindex(arg[n], ':')))
					i = arg[n] - 1;
#else
		if (!(i = rindex(arg[n], sepchr[0])))
			if (arg[n][0] != tmpchr[0])
				i = arg[n] - 1;
			else
				i = arg[n];
#endif
		name[n] = i + 1;
	}


	openarc(0);		/* open archive for reading */

	if (num) {		/* if files were named */
		while (readhdr(&hdr, arc)) {	/* while more files to check */
			save = 0;	/* reset save flag */
			for (n = 0; n < num; n++) {	/* for each template
							 * given */
				if (match(hdr.name, name[n])) {
					save = 1;	/* turn on save flag */
					did[n] = 1;	/* turn on usage flag */
					break;	/* stop looking */
				}
			}

			if (save)	/* extract if desired, else skip */
				extfile(&hdr, arg[n], prt);
			else
				fseek(arc, hdr.size, 1);
		}
	} else
		while (readhdr(&hdr, arc))	/* else extract all files */
			extfile(&hdr, "", prt);

	closearc(0);		/* close archive after reading */

	if (note) {
		for (n = 0; n < num; n++) {	/* report unused args */
			if (!did[n]) {
				printf("File not found: %s\n", arg[n]);
				nerrs++;
			}
		}
	}
	free(name);
}

static VOID
extfile(hdr, path, prt)		/* extract a file */
	struct heads   *hdr;	/* pointer to header data */
	char           *path;	/* pointer to path name */
	int             prt;	/* true if printing */
{
	FILE           *f, *fopen();	/* extracted file, opener */
	char            buf[STRLEN];	/* input buffer */
	char            fix[STRLEN];	/* fixed name buffer */
	char           *i;	/* string index */

	if (prt) {		/* printing is much easier */
		unpack(arc, stdout, hdr);	/* unpack file from archive */
		printf("\f");	/* eject the form */
		return;		/* see? I told you! */
	}
	strcpy(fix, path);	/* note path name template */
#if	!_MTS
	if (*path) {
	if (!(i = rindex(fix, '\\')))	/* find start of name */
		if (!(i = rindex(fix, '/')))
			if (!(i = rindex(fix, ':')))
				i = fix - 1;
	} else i = fix -1;
#else
	if (!(i = rindex(fix, sepchr[0])))
		if (fix[0] != tmpchr[0])
			i = fix - 1;
		else
			i = fix;
#endif
	strcpy(i + 1, hdr->name);	/* replace template with name */

	if (note)
		printf("Extracting file: %s\n", fix);

	if (warn && !overlay) {
		if ((f = fopen(fix, "r"))) {	/* see if it exists */
				fclose(f);
				printf("WARNING: File %s already exists!", fix);
				fflush(stdout);
				while (1) {
					char *dummy;
					printf("  Overwrite it (y/n)? ");
					fflush(stdout);
					dummy = fgets(buf, STRLEN, stdin);
					*buf = toupper(*buf);
					if (*buf == 'Y' || *buf == 'N')
						break;
				}
				if (*buf == 'N') {
					printf("%s not extracted.\n", fix);
					fseek(arc, hdr->size, 1);
					return;
				}
		}
	}
#if	!_MTS
	if (!(f = fopen(fix, OPEN_W)))
#else
	{
		fortran         create();
		VOID		memset();
		char            c_name[256];
		struct crsize {
			short           maxsize, cursize;
		}               c_size;
		char            c_vol[6];
		int             c_type;
		strcpy(c_name, fix);
		strcat(c_name, " ");
		c_size.maxsize = 0;
		c_size.cursize = hdr->length / 4096 + 1;
		memset(c_vol, 0, sizeof(c_vol));
		c_type = 0x100;
		create(c_name, &c_size, c_vol, &c_type);
	}
	if (image) {
		f = fopen(fix, OPEN_W);
	} else
		f = fopen(fix, "w");
	if (!f)
#endif
	{
		if (warn) {
			printf("Cannot create %s\n", fix);
			nerrs++;
		}
		fseek(arc, hdr->size, 1);
		return;
	}

	/* now unpack the file */

	unpack(arc, f, hdr);	/* unpack file from archive */
	fclose(f);		/* all done writing to file */
#if	!_MTS
	setstamp(fix, hdr->date, hdr->time);	/* use filename for stamp */
#endif
}
