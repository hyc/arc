/*
 * $Header: /cvsroot/arc/arc/arcusq.c,v 1.2 2003/10/31 02:22:36 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCUSQ
 * 
 * Version 3.14, created on 07/25/86 at 13:04:19
 * 
 * (C) COPYRIGHT 1985 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:	Thom Henderson
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

#include "proto.h"

VOID		arcdie();

/* stuff for Huffman unsqueezing */

#define ERROR (-1)

#define SPEOF 256		/* special endfile token */
#define NUMVALS 257		/* 256 data values plus SPEOF */

extern struct nd {		/* decoding tree */
	int		child[2];	/* left, right */
}		node[NUMVALS];	/* use large buffer */

extern char	*pinbuf;
extern u_char	*outbuf, *outbeg, *outend;

static int	bpos;		/* last bit position read */
extern int	curin;		/* last byte value read */
static int	numnodes;	/* number of nodes in decode tree */

extern char  *inbeg, *inend;

/* get a 16bit integer */
#define GET_INT(x)	\
{	x = (u_char) (*inbeg++); \
	x |= *inbeg++ << 8;}

VOID
init_usq(f)			/* initialize Huffman unsqueezing */
	FILE	       *f;	/* file containing squeezed data */
{
	int		i;	/* node index */
	u_int		inlen;

	bpos = 99;		/* force initial read */

	inlen = getb_unp(f);
	inbeg = pinbuf;
	inend = &pinbuf[inlen];

	GET_INT(numnodes);

	if (numnodes < 0 || numnodes >= NUMVALS)
		arcdie("File has an invalid decode tree");

	/* initialize for possible empty tree (SPEOF only) */

	node[0].child[0] = -(SPEOF + 1);
	node[0].child[1] = -(SPEOF + 1);

	for (i = 0; i < numnodes; ++i) {	/* get decoding tree from
						 * file */
		GET_INT(node[i].child[0]);
		GET_INT(node[i].child[1]);
	}
}

u_int
getb_usq(f)			/* get byte from squeezed file */
	FILE	       *f;	/* file containing squeezed data */
{
	int		i;	/* tree index */
	u_int		j;

	outbeg = outbuf;
	for (j = 0; j < MYBUF; j++) {
		/* follow bit stream in tree to a leaf */

		for (i = 0; i >= 0;) {	/* work down(up?) from root */
			if (++bpos > 7) {
				if (inbeg >= inend) {
					inend = &pinbuf[getb_unp(f)];
					inbeg = pinbuf;
					if (inend == inbeg) {
						if (!curin)
							j--;
						return(j);
					}
				}
				curin = *inbeg++;
				bpos = 0;

				/* move a level deeper in tree */
				i = node[i].child[1 & curin];
			} else
				i = node[i].child[1 & (curin >>= 1)];
		}

		/* decode fake node index to original data value */

		i = -(i + 1);

		if (i != SPEOF)
			*outbeg++ = i;
		else
			break;
	}
	/*NOTREACHED*/
	return (j);
}
