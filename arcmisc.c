/*
 * Miscellaneous routines to get ARC running on non-MSDOS systems...
 * $Header: /cvsroot/arc/arc/arcmisc.c,v 1.2 2003/10/31 02:22:36 highlandsun Exp $ 
 */

#include <stdio.h>
#include <ctype.h>
#include "arc.h"

#include <string.h>
#if	BSD
#include <strings.h>
#endif

#if	MSDOS
#include <dir.h>
#include <stat.h>
#endif

#if	GEMDOS
#include <types.h>
#include <osbind.h>
#include <stat.h>

VOID 
exitpause()
{
	while (Cconis())
		Cnecin();
	fprintf(stderr, "Press any key to continue: ");
	fflush(stderr);
	Cnecin();
	fprintf(stderr, "\n");
}

int
chdir(dirname)
	char           *dirname;
{
	char           *i;
	int             drv;

	i = dirname;
	if ((i = index(dirname, ':')) != NULL) {
		drv = i[-1];
		i++;		/* Move past device spec */
		if (drv > '\'')
			drv -= 'a';
		else
			drv -= 'A';
		if (drv >= 0 && drv < 16)
			Dsetdrv(drv);
	}
	if (*i != '\0')
		return (Dsetpath(i));
}
#endif

#if	UNIX
#include <sys/types.h>
#if	SYSV
#include <dirent.h>
#define DIRECT dirent
#else
#include <sys/dir.h>
#define DIRECT direct
#endif
#include <sys/stat.h>
	int	rename(), unlink();
#endif

#if	NEEDMEMSET
char	*
memset(s, c, n)		/* This came from SVR2? */
	char	*s;
	int	c, n;
{
	register int i;
	for(i=0;i<n;i++)
		s[i]=c;
	return(s);
}
#else
#include <memory.h>
#endif

#ifndef	__STDC__
char		*malloc();
#ifndef _AIX
int		free();
#endif
#endif
int             match();

int
move(oldnam, newnam)
	char           *oldnam, *newnam;
{
	FILE           *fopen(), *old, *new;
#if	!_MTS
	struct stat     oldstat;
#endif
	VOID		filecopy();
#if	GEMDOS
	if (Frename(0, oldnam, newnam))
#else
	if (rename(oldnam, newnam))
#endif
#if	!_MTS
	{
		if (stat(oldnam, &oldstat))	/* different partition? */
			return (-1);
		old = fopen(oldnam, OPEN_R);
		if (old == NULL)
			return (-1);
		new = fopen(newnam, OPEN_W);
		if (new == NULL)
			return (-1);
		filecopy(old, new, oldstat.st_size);
		return(unlink(oldnam));
	}
	return 0;
#else
	return(-1);
#endif
}

static VOID
_makefn(source, dest)
	char           *source;
	char           *dest;
{
	int             j;
#if	MSDOS
	char	       *setmem();
#endif

	setmem(dest, 17, 0);	/* clear result field */
	for (j = 0; *source && *source != '.'; ++source)
		if (j < 8)
			dest[j++] = *source;
	for (j = 9; *source; ++source)
		if (j < 13)
			dest[j++] = *source;
}
/*
 * make a file name using a template 
 */

char           *
makefnam(rawfn, template, result)
	char           *rawfn;	/* the original file name */
	char           *template;	/* the template data */
	char           *result;	/* where to place the result */
{
	char            et[17], er[17], rawbuf[STRLEN], *i;

	*rawbuf = 0;
	strcpy(rawbuf, rawfn);
#if	_MTS
	i = rawbuf;
	if (rawbuf[0] == tmpchr[0]) {
		i++;
		strcpy(rawfn, i);
	} else
#endif
	if ((i = rindex(rawbuf, CUTOFF))) {
		i++;
		strcpy(rawfn, i);
	}
#if	DOS
	else if ((i = rindex(rawbuf, ':'))) {
		i++;
		strcpy(rawfn, i);
	}
#endif
	if (i)
		*i = 0;
	else
		*rawbuf = 0;

	_makefn(template, et);
	_makefn(rawfn, er);
	*result = 0;		/* assure no data */
	strcat(result, rawbuf);
	strcat(result, er[0] ? er : et);
	strcat(result, er[9] ? er + 9 : et + 9);
	return ((char *) &result[0]);
}

#if	MSDOS || SYSV

int
alphasort(dirptr1, dirptr2)
	struct DIRECT **dirptr1, **dirptr2;
{
	return (strcmp((*dirptr1)->d_name, (*dirptr2)->d_name));
}

#endif

VOID
upper(string)
	char           *string;
{
	char           *p;

	for (p = string; *p; p++)
		if (islower(*p))
			*p = toupper(*p);
}
/* VARARGS1 */
VOID
arcdie(s, arg1, arg2, arg3)
	char           *s;
{
	fprintf(stderr, "ARC: ");
	fprintf(stderr, s, arg1, arg2, arg3);
	fprintf(stderr, "\n");
#if	UNIX
	perror("UNIX");
#endif
#if	GEMDOS
	exitpause();
#endif
	exit(1);
}

#if	!_MTS

char           *
gcdir(dirname)
	char           *dirname;

{
	char           *getwd();
#if	GEMDOS
	int             drv;
	char           *buf;
#endif
	if (dirname == NULL || strlen(dirname) == 0)
		dirname = (char *) malloc(1024);

#if	!GEMDOS
	getwd(dirname);
#else
	buf = dirname;
	*buf++ = (drv = Dgetdrv()) + 'A';
	*buf++ = ':';
	Dgetpath(buf, 0);
#endif
	return (dirname);
}

#if	UNIX
char           *pattern;	/* global so that fmatch can use it */
#endif

char           *
dir(filename)		/* get files, one by one */
	char           *filename;	/* template, or NULL */
{
#if	GEMDOS
	static int      Nnum = 0;
#if	__GNUC__
#define	d_fname	dta_name	/* Wish these libraries would agree on names */
#define DMABUFFER	_DTA
#endif
	static DMABUFFER dbuf, *saved;
	char           *name;
	if (Nnum == 0) {	/* first call */
		saved = (DMABUFFER *) Fgetdta();
		Fsetdta(&dbuf);
		if (Fsfirst(filename, 0) == 0) {
			name = malloc(FNLEN);
			strcpy(name, dbuf.d_fname);
			Nnum++;
			return (name);
		} else {
			Fsetdta(saved);
			return (NULL);
		}
	} else {
		if (Fsnext() == 0) {
			name = malloc(FNLEN);
			strcpy(name, dbuf.d_fname);
			return (name);
		} else {
			Nnum = 0;
			Fsetdta(saved);
			return (NULL);
		}
	}
}
#else
	static struct DIRECT **namelist;
	static char   **NameList;
	static char	namecopy[STRLEN], *dirname;
#if	UNIX
	int             alphasort();
	int             scandir();
#endif				/* UNIX */
	int             fmatch();
	static int      Nnum = 0, ii;


	if (Nnum == 0) {	/* first call */
		strcpy(namecopy,filename);
		if(pattern=rindex(namecopy,CUTOFF)) {
			*pattern = 0;
			pattern++;
			dirname = namecopy;
		} else {
			pattern = filename;
			dirname = ".";
		}
		Nnum = scandir(dirname, &namelist, fmatch, alphasort);
		NameList = (char **) malloc(Nnum * sizeof(char *));
		for (ii = 0; ii < Nnum; ii++) {
			(NameList)[ii] = malloc(strlen(namelist[ii]->d_name) + 1);
			strcpy((NameList)[ii], namelist[ii]->d_name);
		}
		ii = 0;
	}
	if (ii >= Nnum) {	/* all out of files */
		if (Nnum) {	/* there were some files found */
			for (ii = 0; ii < Nnum; ii++)
				free(namelist[ii]);
			free(namelist);
		}
		Nnum = 0;
		return (NULL);
	} else {
		return ((NameList)[ii++]);
	}
}

/*
 * Filename match - here, * matches everything 
 */

int
fmatch(direntry)
	struct DIRECT  *direntry;
{
	char           *string;

	string = direntry->d_name;

	if (!strcmp(pattern, "") || !strcmp(pattern, "*.*") || !strcmp(pattern, "*"))
		return (1);
	return (match(string, pattern));
}
#endif				/* GEMDOS */
#else
/* dir code for MTS under Bell Labs C... */

char           *
dir(filepattern)
	char           *filepattern;	/* template or NULL */
{
#if	USECATSCAN
	fortran VOID    catscan(), fileinfo();

	struct catname {
		short           len;
		char            name[257];
	}               pattern;

	struct catval {
		int             maxlen;
		int             actlen;
		char            name[257];
	}               catreturn;

	char           *i;
	int             j, RETCODE;

	static int      catptr = 0;
	static int      catflag = 0x200;
	static int      cattype = 1;
	static int      patflag = 0;

	catreturn.maxlen = 256;

	if (patflag) {
		patflag = 0;
		catptr = 0;
		return (NULL);
	}
	if (filepattern) {
		strcpy(pattern.name, filepattern);
		pattern.len = strlen(filepattern);
		if (!index(filepattern, '?'))
			patflag = 1;
	}
	if (patflag) {
		fileinfo(&pattern, &cattype, "CINAME  ", &catreturn, _retcode RETCODE);
		catptr = RETCODE ? 0 : 1;
	} else
		catscan(&pattern, &catflag, &cattype, &catreturn, &catptr);

	if (!catptr)
		return (NULL);
	else {
		char           *k;

/*		k = index(catreturn.name, ' ');
		if (k)
			*k = 0;
		else {		This seems unnecessary now	*/
			j = catreturn.actlen;
			catreturn.name[j] = 0;
/*		}		*/
		k = catreturn.name;
		if (*k == tmpchr[0])
			k++;
		else if ((k = index(catreturn.name, sepchr[0])))
			k++;
		else
			k = catreturn.name;
		j = strlen(k);
		i = malloc(++j);
		strcpy(i, k);
		return (i);
	}
#else
	fortran VOID    gfinfo();
	static char     gfname[24];
	static char     pattern[20];
	static int      gfdummy[2] = {
				      0, 0
	},              gfflags;
	int             i, RETCODE;
	char           *j, *k;

	if (filepattern) {
		strcpy(pattern, filepattern);
		strcat(pattern, " ");
		for (i = 20; i < 24; i++)
			gfname[i] = '\0';
		if (index(pattern, '?'))
			gfflags = 0x0C;
		else
			gfflags = 0x09;
	} else if (gfflags == 0x09)
		return (NULL);

	gfinfo(pattern, gfname, &gfflags, gfdummy, gfdummy, gfdummy, _retcode RETCODE);
	if (RETCODE)
		return (NULL);
	else {
		k = index(gfname, ' ');
		*k = '\0';
		k = gfname;
		if (gfname[0] == tmpchr[0])
			k++;
		else if ((k = index(gfname, sepchr[0])))
			k++;
		else
			k = gfname;
		i = strlen(k);
		j = malloc(++i);
		strcpy(j, k);
		return (j);
	}
#endif
}

int
unlink(path)
	char           *path;	/* name of file to delete */
{
	fortran VOID    destroy();
	int             RETCODE;

	char            name[258];

	strcpy(name, path);
	strcat(name, " ");
	destroy(name, _retcode RETCODE);
	if (RETCODE)
		return (-1);
	else
		return (0);
}
#endif
