/*
 * $Header: /cvsroot/arc/arc/arccode.c,v 1.2 2003/10/31 02:22:36 highlandsun Exp $
 */

/*
 * ARC - Archive utility - ARCCODE
 * 
 * Version 1.02, created on 01/20/86 at 13:33:35
 * 
 * (C) COPYRIGHT 1985 by System Enhancement Associates; ALL RIGHTS RESERVED
 * 
 * By:	Thom Henderson
 * 
 * Description: This file contains the routines used to encrypt and decrypt data
 * in an archive.  The encryption method is nothing fancy, being just a
 * routine XOR, but it is used on the packed data, and uses a variable length
 * key.	 The end result is something that is in theory crackable, but I'd
 * hate to try it.  It should be more than sufficient for casual use.
 * 
 * Language: Computer Innovations Optimizing C86
 */
#include <stdio.h>
#include "arc.h"

static char    *p;		/* password pointer */

VOID
setcode()
{				/* get set for encoding/decoding */
	p = password;		/* reset password pointer */
}

VOID
codebuf(buf, len)		/* encrypt a buffer */
	reg char       *buf;
	u_int		len;
{
	reg u_int	i;
	reg char       *pasptr = p;

	for (i = 0; i < len; i++) {
		if (!*pasptr)
			pasptr = password;
		*buf++ ^= *pasptr++;
	}
	p = pasptr;
}
