/*
 * $Header: /cvsroot/arc/arc/arcdos.c,v 1.1 1988/06/13 19:08:00 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCDOS
 * 
 * Version 1.44, created on 07/25/86 at 14:17:38
 * 
 * (C) COPYRIGHT 1985 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:  Thom Henderson
 * 
 * Description: This file contains certain DOS level routines that assist in
 * doing fancy things with an archive, primarily reading and setting the date
 * and time last modified.
 * 
 * These are, by nature, system dependant functions.  But they are also, by
 * nature, very expendable.
 * 
 * Language: Computer Innovations Optimizing C86
 */
#include <stdio.h>
#include "arc.h"

#if	MSDOS
#include "fileio2.h"		/* needed for filehand */
#endif

#if	UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "tws.h"
#endif

#if	SYSV
struct timeval {
	long tv_sec;
	long tv_usec;
};
#endif

#if	GEMDOS
#include <osbind.h>
#endif

char	*strcpy(), *strcat(), *malloc();

void
getstamp(f, date, time)		/* get a file's date/time stamp */
#if	!MTS
	FILE           *f;	/* file to get stamp from */
#else
	char           *f;	/* filename "" "" */
#endif
	unsigned short   *date, *time;	/* storage for the stamp */
{
#if	MSDOS
	struct {
		int             ax, bx, cx, dx, si, di, ds, es;
	}               reg;

	reg.ax = 0x5700;	/* get date/time */
	reg.bx = filehand(f);	/* file handle */
	if (sysint21(&reg, &reg) & 1)	/* DOS call */
		printf("Get timestamp fail (%d)\n", reg.ax);

	*date = reg.dx;		/* save date/time */
	*time = reg.cx;
#endif
#if	GEMDOS
	int	fd, ret[2];

	fd = fileno(f);
	Fdatime(ret, fd, 0);
	*date = ret[1];
	*time = ret[0];
#endif
#if	UNIX
	char	       *ctime();
	struct stat    *buf;
	int             day, hr, min, sec, yr, imon;
	static char     mon[4], *mons[12] = {
				   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	buf = (struct stat *) malloc(sizeof(struct stat));
	fstat(fileno(f), buf);

	sscanf(ctime(&(buf->st_mtime)), "%*4s%3s%d%d:%d:%d%d", mon, &day, &hr, &min,
	       &sec, &yr);
	for (imon = 0; imon < 12 && strcmp(mon, mons[imon]); imon++);

	*date = (unsigned short) (((yr - 1980) << 9) + ((imon + 1) << 5) + day);
	*time = (unsigned short) ((hr << 11) + (min << 5) + sec / 2);
#endif
#if	MTS
	fortran         timein(),
#if	USEGFINFO
	                gfinfo();
#else
	                fileinfo();
#endif
	int             stclk[2];
	char            name[24];
	struct bigtime {
		int             greg;
		int             year;
		int             mon;
		int             day;
		int             hour;
		int             min;
		int             sec;
		int             usec;
		int             week;
		int             toff;
		int             tzn1;
		int             tzn2;
	}               tvec;
#if	USEGFINFO
	static int      gfflag = 0x0009, gfdummy[2] = {
						       0, 0
	};
	int             gfcinfo[18];
#else
	static int      cattype = 2;
#endif

	strcpy(name, f);
	strcat(name, " ");
#if	USEGFINFO
	gfcinfo[0] = 18;
	gfinfo(name, name, &gfflag, gfcinfo, gfdummy, gfdummy);
	timein("*IBM MICROSECONDS*", &gfcinfo[16], &tvec);
#else
	fileinfo(name, &cattype, "CILCCT  ", stclk);
	timein("*IBM MICROSECONDS*", stclk, &tvec);
#endif

	*date = (unsigned short) (((tvec.year - 1980) << 9) + ((tvec.mon) << 5) + tvec.day);
	*time = (unsigned short) ((tvec.hour << 11) + (tvec.min << 5) + tvec.sec / 2);
#endif
}

void
setstamp(f, date, time)		/* set a file's date/time stamp */
	char           *f;	/* filename to stamp */
	unsigned short    date, time;	/* desired date, time */
{
#if	MSDOS
	FILE	*ff;
	struct {
		int             ax, bx, cx, dx, si, di, ds, es;
	}               reg;

	ff = fopen(f, "w+");	/* How else can I get a handle? */

	reg.ax = 0x5701;	/* set date/time */
	reg.bx = filehand(f);	/* file handle */
	reg.cx = time;		/* desired time */
	reg.dx = date;		/* desired date */
	if (sysint21(&reg, &reg) & 1)	/* DOS call */
		printf("Set timestamp fail (%d)\n", reg.ax);
	fclose(ff);
#endif
#if	GEMDOS
	int	fd, set[2];

	fd = Fopen(f, 0);
	set[0] = time;
	set[1] = date;
	Fdatime(set, fd, 1);
	Fclose(fd);
#endif
#if	UNIX
	struct tws      tms;
	struct timeval  tvp[2];
	int	utimes();
	twscopy(&tms, dtwstime());
	tms.tw_sec = (time & 31) * 2;
	tms.tw_min = (time >> 5) & 63;
	tms.tw_hour = (time >> 11);
	tms.tw_mday = date & 31;
	tms.tw_mon = ((date >> 5) & 15) - 1;
	tms.tw_year = (date >> 9) + 80;
	tms.tw_clock = 0L;
	tms.tw_flags = TW_NULL;
	tvp[0].tv_sec = twclock(&tms);
	tvp[1].tv_sec = tvp[0].tv_sec;
	tvp[0].tv_usec = tvp[1].tv_usec = 0;
	utimes(f, tvp);
#endif
}

#if	MSDOS
int
filehand(stream)		/* find handle on a file */
	struct bufstr  *stream;	/* file to grab onto */
{
	return stream->bufhand;	/* return DOS 2.0 file handle */
}
#endif

#if	UNIX
int
izadir(filename)		/* Is filename a directory? */
	char           *filename;
{
	struct stat     buf;

	if (stat(filename, &buf) != 0)
		return (0);	/* Ignore if stat fails since */
	else
		return (buf.st_mode & S_IFDIR);	/* bad files trapped later */
}
#endif
