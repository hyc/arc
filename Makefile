#
#       Makefile for Hack-attack 1.3
#       VAX 11/780 BSD4.2 "ARC" utility
#
# Originals from Dan Lanciani, James Turner, and others...
#
# Modified to support squashing, also added targets for the time routine
# library.  -- Howard Chu, hyc@umix.cc.umich.edu, 4-11-88
#
# I put SRCDIR on a real disk on the ST, but copy the makefile to a
# RAMdisk and compile from there. Makes things go a bit quicker...
# This has to be done in the shell, to get the trailing backslash
# specified correctly. e.g., setenv SRCDIR='d:\src\arc\'
SRCDIR = 

HEADER = $(SRCDIR)arc.h

# Add a ".TTP" suffix to the executable files on an ST.
#PROG = .ttp
PROG =

# TWSLIB is only needed on Unix systems. Likewise for TWHEAD.
#TWSLIB =
#TWHEAD =
TWSLIB = libtws.a
TWHEAD = tws.h

# For MWC 3.0 on the Atari ST, use:
#CFLAGS = -VCOMPAC -VPEEP
CFLAGS = -O

OBJS = arc.o arcadd.o arccode.o arccvt.o arcdata.o arcdel.o arcdos.o \
arcext.o arcio.o arclst.o arclzw.o arcmatch.o arcpack.o arcrun.o \
arcsq.o arcsqs.o arcsvc.o arctst.o arcunp.o arcusq.o arcmisc.o

MOBJ = marc.o arcdata.o arcdos.o arcio.o arcmatch.o arcmisc.o

arc$(PROG):	$(OBJS) $(TWSLIB)
	cc -o arc$(PROG) $(OBJS) $(TWSLIB)

marc$(PROG):	$(MOBJ) $(TWSLIB)
	cc -o marc$(PROG) $(MOBJ) $(TWSLIB)

clean:
	-rm *.o arc$(PROG) marc$(PROG) $(TWSLIB)

$(HEADER):	$(SRCDIR)arcs.h
	touch $(HEADER)

arc.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arc.c
marc.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)marc.c
arcadd.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcadd.c
arccode.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arccode.c
arccvt.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arccvt.c
arcdata.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcdata.c
arcdel.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcdel.c
arcdir.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcdir.c
arcdos.o:	$(HEADER) $(TWHEAD)
	cc $(CFLAGS) -c $(SRCDIR)arcdos.c
arcext.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcext.c
arcio.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcio.c
arclst.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arclst.c
arclzw.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arclzw.c
arcmatch.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcmatch.c
arcmisc.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcmisc.c
arcpack.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcpack.c
arcrun.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcrun.c
arcsq.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcsq.c
arcsqs.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcsqs.c
arcsvc.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcsvc.c
arctst.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arctst.c
arcunp.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcunp.c
arcusq.o:	$(HEADER)
	cc $(CFLAGS) -c $(SRCDIR)arcusq.c

libtws.a:
	make -f Make.tws libtws.a
