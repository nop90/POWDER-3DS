/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        bmp.h ( bmp Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __bmp__
#define __bmp__

/// Attempts to read external tileset from gfx subdirectory.
/// Returns true if loaded.
bool
bmp_loadExtraTileset();

// Loads the given named bitmap.
// Returns 0 on failure.
// If success, the buffer has been new[]ed and has 15bit RGB data.
// w & h have been assigned to the width and height.
// If quiet is set, supresses failure to open errors.
unsigned short *
bmp_load(const char *name, int &w, int &h, bool quiet);

#endif

