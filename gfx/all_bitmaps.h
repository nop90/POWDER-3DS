/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        all_bitmaps.h ( gfx Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __all_bitmaps__
#define __all_bitmaps__

#include <mygba.h>

#ifdef _3DS
#define NUM_FONTS		5
#define NUM_TILESETS	1
#else

#define NUM_FONTS		5

#if defined(iPOWDER)
#define NUM_TILESETS		8
#else
#if defined(USING_SDL) && !defined(_WIN32_WCE)
#define NUM_TILESETS		10
#else
#define NUM_TILESETS		7
#endif
#endif
#endif

// Definition of a generic tileset.
struct TILESET
{
    const unsigned char		*alphabet[NUM_FONTS];
    const unsigned char		*dungeon;
    const unsigned char		*mini;
    const unsigned char		*minif;
    const unsigned short	*palette;
    const unsigned char		*sprite;
    const unsigned short	*spritepalette;

    const char			*name;

    int				 tilewidth;
};

extern TILESET *glb_tilesets;

extern const char *glb_fontnames[NUM_FONTS];

extern const unsigned short *bmp_slug_and_blood;
extern const unsigned short *bmp_sprite16_3x;

#endif

