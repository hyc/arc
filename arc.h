/*
 * $Header: /cvsroot/arc/arc/arc.h,v 1.1 1988/06/02 00:59:00 highlandsun Exp $
 */

#undef	MSDOS
#undef	GEMDOS		/* This amusing garbage is to get all my */
#undef	DOS		/* define's past some compilers, which */
#undef	BSD		/* apparently define some of these themselves */
#undef	SYSV
#undef	UNIX
#undef	MTS

#define	MSDOS	0		/* MSDOS machine */
#define	GEMDOS	0		/* Atari, GEMDOS */
#define	BSD	1		/* BSD4.2 or 4.3 */
#define	SYSV	0		/* Also uses BSD */
#define	MTS	0		/* MTS or 370(?) */

/*
 * Assumptions:
 * char = 8 bits
 * short = 16 bits
 * long = 32 bits
 * int >= 16 bits
 */

#if	MSDOS || GEMDOS
#define	DOS	1
#define	CUTOFF	'\\'
#endif

#if	!MSDOS
#define	envfind	getenv
#define	setmem(a, b, c)	memset(a, c, b)
#endif

#if	BSD || SYSV
#define	UNIX	1
#define	CUTOFF	'/'
#include <ctype.h>
#endif

#if	MTS
#define rindex strrchr
#define index strchr
#undef  USEGFINFO		/* define this to use GFINFO for directory */
#define USECATSCAN		/* scanning, else use CATSCAN/FILEINFO... */
#define	CUTOFF	sepchr[0]
#endif

/*  ARC - Archive utility - ARC Header
  
    Version 2.17, created on 04/22/87 at 13:09:43
  
(C) COPYRIGHT 1985,86 by System Enhancement Associates; ALL RIGHTS RESERVED
  
    By:	 Thom Henderson
  
    Description: 
	 This is the header file for the ARC archive utility.  It defines
	 global parameters and the references to the external data.
  
  
    Language:
	 Computer Innovations Optimizing C86
*/

#define ARCMARK 26		/* special archive marker        */
#define ARCVER 9		/* archive header version code   */
#define STRLEN 100		/* system standard string length */
#define FNLEN 13		/* file name length              */
#define MAXARG 25		/* maximum number of arguments   */

#ifndef DONT_DEFINE		/* Defined by arcdata.c */
#include "arcs.h"

extern int      keepbak;	/* true if saving the old archive */
#if	!DOS
extern int      image;		/* true to suppress CRLF/LF x-late */
#endif
#if	MTS
extern char     sepchr[2];	/* Shared file separator, default = ':' */
extern char     tmpchr[2];	/* Temporary file prefix, default = '-' */
#endif
#if	GEMDOS
extern int      hold;		/* hold screen before exiting */
#endif
extern int      warn;		/* true to print warnings */
extern int      note;		/* true to print comments */
extern int      bose;		/* true to be verbose */
extern int      nocomp;		/* true to suppress compression */
extern int      overlay;	/* true to overlay on extract */
extern int      kludge;		/* kludge flag */
extern char    *arctemp;	/* arc temp file prefix */
extern char    *password;	/* encryption password pointer */
extern int      nerrs;		/* number of errors encountered */
extern int      changing;	/* true if archive being modified */

extern char     hdrver;		/* header version */

extern FILE    *arc;		/* the old archive */
extern FILE    *new;		/* the new archive */
extern char     arcname[STRLEN];/* storage for archive name */
extern char     bakname[STRLEN];/* storage for backup copy name */
extern char     newname[STRLEN];/* storage for new archive name */
extern unsigned short arcdate;	/* archive date stamp */
extern unsigned short arctime;	/* archive time stamp */
extern unsigned short olddate;	/* old archive date stamp */
extern unsigned short oldtime;	/* old archive time stamp */
extern int      dosquash;	/* squash instead of crunch */
#endif				/* DONT_DEFINE */
