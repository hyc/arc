# $Header: /cvsroot/arc/arc/Makefile,v 1.4 2003/10/31 02:32:51 highlandsun Exp $
#       Makefile for portable ARC
#
# Originals from Dan Lanciani, James Turner, and others...
# This Makefile supports Atari ST and all Unix versions.
#
# I put SRCDIR on a real disk on the ST, but copy the makefile to a
# RAMdisk and compile from there. Makes things go a bit quicker...
# This has to be done in the shell, to get the trailing backslash
# specified correctly. e.g., setenv SRCDIR='d:\src\arc\'
SRCDIR = 

HEADER = $(SRCDIR)arc.h $(SRCDIR)arcs.h

# Add a ".TTP" suffix to the executable files on an ST.
#PROG = .ttp
PROG =

# SYSTEM defines your operating system:
# MSDOS for IBM PCs or other MSDOS machines
# GEMDOS for Atari ST (Predefined by MWC, so you don't need to define it.)
# BSD for Berkeley Unix
# SYSV for AT&T System V Unix or GNU/Linux
# (_MTS for Michigan Terminal System, which requires a different makefile)
# (_MTS also requires one of USEGFINFO or USECATSCAN for directory search)
# NEEDMEMSET if your C library does not have the memset() routine and/or
# your system include doesn't have <memory.h> (Most current systems don't
# need this.)
# NEED_TIMEVAL if you need struct timeval defined.
# NEED_ALPHASORT if your C library doesn't provide it.
#
# On Solaris, use -DSYSV=1 -DNEED_ALPHASORT and set SYSVOBJ=scandir.o
# (See the Sysvarcstuf shar file)
#SYSTEM = -DGEMDOS=1 -fstrength-reduce -fomit-frame-pointer -finline-functions -fdefer-pop -mpcrel
#SYSTEM = -DBSD=1
SYSTEM = -DSYSV=1

OPT = -O
# For MWC 3.0 on the Atari ST, use:
#CFLAGS = -VCOMPAC -VPEEP
CFLAGS = $(OPT) $(SYSTEM)

# GNU's gcc is very nice, if you've got it. Otherwise just cc.
#CC = cgcc -mshort -mbaserel
CC = cc

# tmclock is only needed on Unix systems...
TMCLOCK = tmclock.o

# Integer-only stdio routines for Atari ST.
#LIBS=-liio16

# Files needed for System V 
#SYSVOBJ =	getwd.o rename.o scandir.o utimes.o
SYSVOBJ =

OBJS = arc.o arcadd.o arccode.o arccvt.o arcdata.o arcdel.o arcdos.o \
arcext.o arcio.o arclst.o arclzw.o arcmatch.o arcpack.o arcrun.o \
arcsq.o arcsvc.o arctst.o arcunp.o arcusq.o arcmisc.o $(SYSVOBJ)

MOBJ = marc.o arcdata.o arcdos.o arcio.o arcmatch.o arcmisc.o $(SYSVOBJ)

all:	arc$(PROG) marc$(PROG)

arc$(PROG):	$(OBJS) $(TMCLOCK)
	$(CC) $(OPT) -o arc$(PROG) $(OBJS) $(TMCLOCK) $(LIBS)

marc$(PROG):	$(MOBJ) $(TMCLOCK)
	$(CC) $(OPT) -o marc$(PROG) $(MOBJ) $(TMCLOCK) $(LIBS)

clean:
	-rm *.o arc$(PROG) marc$(PROG)

arc.o:	$(SRCDIR)arc.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arc.c
marc.o:	$(SRCDIR)marc.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)marc.c
arcadd.o:	$(SRCDIR)arcadd.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcadd.c
arccode.o:	$(SRCDIR)arccode.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arccode.c
arccvt.o:	$(SRCDIR)arccvt.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arccvt.c
arcdata.o:	$(SRCDIR)arcdata.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcdata.c
arcdel.o:	$(SRCDIR)arcdel.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcdel.c
arcdir.o:	$(SRCDIR)arcdir.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcdir.c
arcdos.o:	$(SRCDIR)arcdos.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcdos.c
arcext.o:	$(SRCDIR)arcext.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcext.c
arcio.o:	$(SRCDIR)arcio.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcio.c
arclst.o:	$(SRCDIR)arclst.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arclst.c
arclzw.o:	$(SRCDIR)arclzw.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arclzw.c
arcmatch.o:	$(SRCDIR)arcmatch.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcmatch.c
arcmisc.o:	$(SRCDIR)arcmisc.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcmisc.c
arcpack.o:	$(SRCDIR)arcpack.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcpack.c
arcrun.o:	$(SRCDIR)arcrun.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcrun.c
arcsq.o:	$(SRCDIR)arcsq.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcsq.c
arcsvc.o:	$(SRCDIR)arcsvc.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcsvc.c
arctst.o:	$(SRCDIR)arctst.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arctst.c
arcunp.o:	$(SRCDIR)arcunp.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcunp.c
arcusq.o:	$(SRCDIR)arcusq.c	$(HEADER)
	$(CC) $(CFLAGS) -c $(SRCDIR)arcusq.c

tmclock.o:	$(SRCDIR)tmclock.c
	$(CC) $(CFLAGS) -c $(SRCDIR)tmclock.c
