/*
 * $Header: /cvsroot/arc/arc/arcusq.c,v 1.1 1988/06/02 21:28:00 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCUSQ
 * 
 * Version 3.14, created on 07/25/86 at 13:04:19
 * 
 * (C) COPYRIGHT 1985 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:  Thom Henderson
 * 
 * Description: This file contains the routines used to expand a file which was
 * packed using Huffman squeezing.
 * 
 * Most of this code is taken from an USQ program by Richard Greenlaw, which was
 * adapted to CI-C86 by Robert J. Beilstein.
 * 
 * Language: Computer Innovations Optimizing C86
 */
#include <stdio.h>
#include "arc.h"

void	abort();
int	getc_unp();

/* stuff for Huffman unsqueezing */

#define ERROR (-1)

#define SPEOF 256		/* special endfile token */
#define NUMVALS 257		/* 256 data values plus SPEOF */

extern	struct nd {		/* decoding tree */
	int	child[2];	/* left, right */
}               node[NUMVALS];	/* use large buffer */

static int      bpos;		/* last bit position read */
static int      curin;		/* last byte value read */
static int      numnodes;	/* number of nodes in decode tree */

static          short
get_int(f)			/* get a 16bit integer */
	FILE           *f;	/* file to get it from */
{
	int	i,j;
	i = getc_unp(f);
	j = getc_unp(f) << 8;
	return (i | j) & 0xFFFF;
}

void
init_usq(f)			/* initialize Huffman unsqueezing */
	FILE           *f;	/* file containing squeezed data */
{
	int             i;	/* node index */

	bpos = 99;		/* force initial read */

	numnodes = get_int(f);

	if (numnodes < 0 || numnodes >= NUMVALS)
		abort("File has an invalid decode tree");

	/* initialize for possible empty tree (SPEOF only) */

	node[0].child[0] = -(SPEOF + 1);
	node[0].child[1] = -(SPEOF + 1);

	for (i = 0; i < numnodes; ++i) {	/* get decoding tree from
						 * file */
		node[i].child[0] = get_int(f);
		node[i].child[1] = get_int(f);
	}
}

int
getc_usq(f)			/* get byte from squeezed file */
	FILE           *f;	/* file containing squeezed data */
{
	int             i;	/* tree index */

	/* follow bit stream in tree to a leaf */

	for (i = 0; i >= 0;) {	/* work down(up?) from root */
		if (++bpos > 7) {
			if ((curin = getc_unp(f)) == ERROR)
				return (ERROR);
			bpos = 0;

			/* move a level deeper in tree */
			i = node[i].child[1 & curin];
		} else
			i = node[i].child[1 & (curin >>= 1)];
	}

	/* decode fake node index to original data value */

	i = -(i + 1);

	/* decode special endfile token to normal EOF */

	i = (i == SPEOF) ? EOF : i;
	return i;
}
