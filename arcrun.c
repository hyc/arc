/*
 * $Header: /cvsroot/arc/arc/arcrun.c,v 1.7 2005/10/10 17:07:11 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCRUN
 * 
 * Version 1.20, created on 03/24/86 at 19:34:31
 * 
 * (C) COPYRIGHT 1985,85 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:  Thom Henderson
 * 
 * Description: This file contains the routines used to "run" a file which is
 * stored in an archive.  At present, all we really do is (a) extract a
 * temporary file, (b) give its name as a system command, and then (c) delete
 * the file.
 * 
 * Language: Computer Innovations Optimizing C86
 */
#include <stdio.h>
#include <string.h>
#include "arc.h"
#if UNIX
#include <unistd.h>
#include <sys/stat.h>
#endif

VOID	rempath(), openarc(), closearc(), arcdie();
int	readhdr(), match(), unpack();
static	VOID	runfile();
FILE	*tmpopen();

VOID
runarc(num, arg)		/* run file from archive */
	int             num;	/* number of arguments */
	char           *arg[];	/* pointers to arguments */
{
	struct heads    hdr;	/* file header */
	char           *makefnam();	/* filename fixer */
	char            buf[STRLEN];	/* filename buffer */
	FILE           *fopen();/* file opener */
	char	       *dummy[2];

	dummy[0]="dummy";
	dummy[1]=NULL;
	rempath(num, arg);	/* strip off paths */

	openarc(0);		/* open archive for reading */

	if (num) {		/* if files were named */
		while (readhdr(&hdr, arc)) {	/* while more files to check */
			if (match(hdr.name, makefnam(arg[0], ".*", buf)))
				runfile(&hdr, num, arg);
			else
				fseek(arc, hdr.size, 1);
		}
	} else
		while (readhdr(&hdr, arc))	/* else run all files */
			runfile(&hdr, 1, dummy);

	closearc(0);		/* close archive after changes */
}

static  VOID
runfile(hdr, num, arg)		/* run a file */
	struct heads   *hdr;	/* pointer to header data */
	int             num;	/* number of arguments */
	char           *arg[];	/* pointers to arguments */
{
	FILE           *tmp, *fopen();	/* temporary file */
	char            buf[STRLEN], *makefnam();	/* temp file name, fixer */
#if	DOS
	char		nbuf[64], *i, *rindex();
#endif
#if	!GEMDOS
	int             n;	/* index */
	char            sys[STRLEN];	/* invocation command buffer */
#endif

	/* makefnam("$ARCTEMP",hdr->name,buf); */
#if	UNIX
	sprintf(buf, "%s.RUN", arctemp);
	strcpy(sys, buf);
#else
	strcpy(nbuf, arctemp);
	makefnam(nbuf,hdr->name,buf);
	i = rindex(buf,'.');
#endif
#if	MSDOS
	if (!strcmp(i, ".BAS")) {
		strcpy(sys, "BASICA ");
		strcat(sys, buf);
	}
	else if (!strcmp(i, ".BAT")
		 || !strcmp(i, ".COM")
		 || !strcmp(i, ".EXE"))
		strcpy(sys, buf);

	else {
		if (warn) {
			printf("File %s is not a .BAS, .BAT, .COM, or .EXE\n",
			       hdr->name);
			nerrs++;
		}
		fseek(arc, hdr->size, 1);	/* skip this file */
		return;
	}
#endif
#if	GEMDOS
      if (strcmp(i, ".PRG")
              && strcmp(i, ".TTP")
              && strcmp(i, ".TOS"))
      {
              if (warn) {
                      printf("File %s is not a .PRG, .TOS, or .TTP\n",
                              hdr->name);
                      nerrs++;
              }
              fseek(arc, hdr->size, 1);       /* skip this file */
              return;
      }
#endif

	if (warn)
		if ((tmp = fopen(buf, "r")))
			arcdie("Temporary file %s already exists", buf);
	if (!(tmp = tmpopen(buf)))
		arcdie("Unable to create temporary file %s", buf);

	if (note)
		printf("Invoking file: %s\n", hdr->name);

	unpack(arc, tmp, hdr);	/* unpack the entry */
	fclose(tmp);		/* release the file */
#if UNIX
	chmod(buf, 0700);	/* make it executable */
#endif
#if	GEMDOS
	execve(buf, arg, NULL);
#else
	for (n = 1; n < num; n++) {	/* add command line arguments */
		strcat(sys, " ");
		strcat(sys, arg[n]);
	}
	if (system(buf))		/* try to invoke it */
		arcdie("Execution failed for %s", buf);
#endif
	if (unlink(buf) && warn) {
		printf("Cannot unsave temporary file %s\n", buf);
		nerrs++;
	}
}
