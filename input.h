/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        input.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	This provides a way of getting input from the user using the
 * 	standard 4 button approach.
 *
 *	By "standard", I mean unique to POWDER and completely uninituitive to
 *	the rest of the world.
 */

#ifndef __input_h__
#define __input_h__

// This inputs a line into the given buffer, maximum length of len.
// lx & ly is where to write the line's text.
// gx & gy is where the gnomon is to be placed (uses 3x3 grid centered there)
// initialtext is the initial text.
void
input_getline(char *text, int len, int lx, int ly, int gx, int gy,
		const char *initialtext = 0);

#endif
