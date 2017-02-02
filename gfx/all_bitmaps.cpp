/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        all_bitmaps.cpp ( gfx Library, C++ )
 *
 * COMMENTS:
 *		This file inlines all the auto-generated bitmap data,
 *		thereby reducing the compile time for gfxengine.cpp.
 */

#include "all_bitmaps.h"

// Include our raw graphics...
// This is done once for each tileset.

const char *glb_fontnames[NUM_FONTS] =
{
    "Classic",
    "Brass",
    "Shadow",
    "Heavy",
    "Light"
};

//
// Classic Tiles:
//

#ifndef _3DS
#define dungeon16_Tiles		dungeon16_classic_Tiles
#define alphabet_classic_Tiles	alphabet_classic_classic_Tiles
#define alphabet_brass_Tiles	alphabet_classic_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_classic_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_classic_heavy_Tiles
#define alphabet_light_Tiles	alphabet_classic_light_Tiles
#define mini16_Tiles		mini16_classic_Tiles
#define minif16_Tiles		minif16_classic_Tiles
#define sprite16_Tiles		sprite16_classic_Tiles
#define master_Palette		master_classic_Palette
#define sprite_Palette		sprite_classic_Palette
    #include "classic/dungeon16_16.c"
    #include "classic/alphabet_classic_8.c"
    #include "classic/alphabet_brass_8.c"
    #include "classic/alphabet_shadow_8.c"
    #include "classic/alphabet_heavy_8.c"
    #include "classic/alphabet_light_8.c"
    #include "classic/mini16_16.c"
    #include "classic/minif16_16.c"
    #include "classic/sprite16_16.c"
    #include "classic/sprite.pal.c"
    #include "classic/master.pal.c"

#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef master_Palette
#undef sprite_Palette


#ifndef iPOWDER
//
// Adam Bolt's Tiles:
//

#define dungeon16_Tiles		dungeon16_adambolt_Tiles
#define alphabet_classic_Tiles	alphabet_adambolt_classic_Tiles
#define alphabet_brass_Tiles	alphabet_adambolt_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_adambolt_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_adambolt_heavy_Tiles
#define alphabet_light_Tiles	alphabet_adambolt_light_Tiles
#define mini16_Tiles		mini16_adambolt_Tiles
#define minif16_Tiles		minif16_adambolt_Tiles
#define sprite16_Tiles		sprite16_adambolt_Tiles
#define sprite_Palette		sprite_adambolt_Palette
#define master_Palette		master_adambolt_Palette
    #include "adambolt/dungeon16_16.c"
    #include "adambolt/alphabet_classic_8.c"
    #include "adambolt/alphabet_brass_8.c"
    #include "adambolt/alphabet_shadow_8.c"
    #include "adambolt/alphabet_heavy_8.c"
    #include "adambolt/alphabet_light_8.c"
    #include "adambolt/mini16_16.c"
    #include "adambolt/minif16_16.c"
    #include "adambolt/sprite16_16.c"
    #include "adambolt/sprite.pal.c"
    #include "adambolt/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef master_Palette
#undef sprite_Palette

#endif

#ifndef iPOWDER
//
// Nethack Tiles: (Thanks to Andrea Menga)
//

#define dungeon16_Tiles		dungeon16_nethack_Tiles
#define alphabet_classic_Tiles	alphabet_nethack_classic_Tiles
#define alphabet_brass_Tiles	alphabet_nethack_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_nethack_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_nethack_heavy_Tiles
#define alphabet_light_Tiles	alphabet_nethack_light_Tiles
#define mini16_Tiles		mini16_nethack_Tiles
#define minif16_Tiles		minif16_nethack_Tiles
#define sprite16_Tiles		sprite16_nethack_Tiles
#define sprite_Palette		sprite_nethack_Palette
#define master_Palette		master_nethack_Palette
    #include "nethack/dungeon16_16.c"
    #include "nethack/alphabet_classic_8.c"
    #include "nethack/alphabet_brass_8.c"
    #include "nethack/alphabet_shadow_8.c"
    #include "nethack/alphabet_heavy_8.c"
    #include "nethack/alphabet_light_8.c"
    #include "nethack/mini16_16.c"
    #include "nethack/minif16_16.c"
    #include "nethack/sprite16_16.c"
    #include "nethack/sprite.pal.c"
    #include "nethack/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#endif

//
// ASCII's Tiles: (Thanks to Kelly Bailey)
//

#define dungeon16_Tiles		dungeon16_ascii_Tiles
#define alphabet_classic_Tiles	alphabet_ascii_classic_Tiles
#define alphabet_brass_Tiles	alphabet_ascii_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_ascii_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_ascii_heavy_Tiles
#define alphabet_light_Tiles	alphabet_ascii_light_Tiles
#define mini16_Tiles		mini16_ascii_Tiles
#define minif16_Tiles		minif16_ascii_Tiles
#define sprite16_Tiles		sprite16_ascii_Tiles
#define sprite_Palette		sprite_ascii_Palette
#define master_Palette		master_ascii_Palette
    #include "ascii/dungeon16_16.c"
    #include "ascii/alphabet_classic_8.c"
    #include "ascii/alphabet_brass_8.c"
    #include "ascii/alphabet_shadow_8.c"
    #include "ascii/alphabet_heavy_8.c"
    #include "ascii/alphabet_light_8.c"
    #include "ascii/mini16_16.c"
    #include "ascii/minif16_16.c"
    #include "ascii/sprite16_16.c"
    #include "ascii/sprite.pal.c"
    #include "ascii/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#if defined(USING_TILE10)

//
// Akoi Meexx's Tiles, distorted to 10x10 base tile: 
//

#define dungeon16_Tiles		dungeon16_distorted_Tiles
#define alphabet_classic_Tiles	alphabet_distorted_classic_Tiles
#define alphabet_brass_Tiles	alphabet_distorted_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_distorted_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_distorted_heavy_Tiles
#define alphabet_light_Tiles	alphabet_distorted_light_Tiles
#define mini16_Tiles		mini16_distorted_Tiles
#define minif16_Tiles		minif16_distorted_Tiles
#define sprite16_Tiles		sprite16_distorted_Tiles
#define sprite_Palette		sprite_distorted_Palette
#define master_Palette		master_distorted_Palette
    #include "distorted/dungeon16_16.c"
    #include "distorted/alphabet_classic_8.c"
    #include "distorted/alphabet_brass_8.c"
    #include "distorted/alphabet_shadow_8.c"
    #include "distorted/alphabet_heavy_8.c"
    #include "distorted/alphabet_light_8.c"
    #include "distorted/mini16_16.c"
    #include "distorted/minif16_16.c"
    #include "distorted/sprite16_16.c"
    #include "distorted/sprite.pal.c"
    #include "distorted/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#endif

#if defined(iPOWDER) || (defined(USING_SDL) && !defined(_WIN32_WCE))
//
// Akoi Meexx's Tiles, 12x12 base tile
//

#define dungeon16_Tiles		dungeon16_akoi12_Tiles
#define alphabet_classic_Tiles	alphabet_akoi12_classic_Tiles
#define alphabet_brass_Tiles	alphabet_akoi12_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_akoi12_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_akoi12_heavy_Tiles
#define alphabet_light_Tiles	alphabet_akoi12_light_Tiles
#define mini16_Tiles		mini16_akoi12_Tiles
#define minif16_Tiles		minif16_akoi12_Tiles
#define sprite16_Tiles		sprite16_akoi12_Tiles
#define sprite_Palette		sprite_akoi12_Palette
#define master_Palette		master_akoi12_Palette
    #include "akoi12/dungeon16_16.c"
    #include "akoi12/alphabet_classic_8.c"
    #include "akoi12/alphabet_brass_8.c"
    #include "akoi12/alphabet_shadow_8.c"
    #include "akoi12/alphabet_heavy_8.c"
    #include "akoi12/alphabet_light_8.c"
    #include "akoi12/mini16_16.c"
    #include "akoi12/minif16_16.c"
    #include "akoi12/sprite16_16.c"
    #include "akoi12/sprite.pal.c"
    #include "akoi12/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

//
// Akoi Meexx's Tiles, 10x10 base tile
//

#define dungeon16_Tiles		dungeon16_akoi10_Tiles
#define alphabet_classic_Tiles	alphabet_akoi10_classic_Tiles
#define alphabet_brass_Tiles	alphabet_akoi10_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_akoi10_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_akoi10_heavy_Tiles
#define alphabet_light_Tiles	alphabet_akoi10_light_Tiles
#define mini16_Tiles		mini16_akoi10_Tiles
#define minif16_Tiles		minif16_akoi10_Tiles
#define sprite16_Tiles		sprite16_akoi10_Tiles
#define sprite_Palette		sprite_akoi10_Palette
#define master_Palette		master_akoi10_Palette
    #include "akoi10/dungeon16_16.c"
    #include "akoi10/alphabet_classic_8.c"
    #include "akoi10/alphabet_brass_8.c"
    #include "akoi10/alphabet_shadow_8.c"
    #include "akoi10/alphabet_heavy_8.c"
    #include "akoi10/alphabet_light_8.c"
    #include "akoi10/mini16_16.c"
    #include "akoi10/minif16_16.c"
    #include "akoi10/sprite16_16.c"
    #include "akoi10/sprite.pal.c"
    #include "akoi10/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#endif

#ifndef USING_TILE10

//
// Akoi Meexx's Tiles: (Thanks to self-titled author)
//

#define dungeon16_Tiles		dungeon16_akoimeexx_Tiles
#define alphabet_classic_Tiles	alphabet_akoimeexx_classic_Tiles
#define alphabet_brass_Tiles	alphabet_akoimeexx_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_akoimeexx_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_akoimeexx_heavy_Tiles
#define alphabet_light_Tiles	alphabet_akoimeexx_light_Tiles
#define mini16_Tiles		mini16_akoimeexx_Tiles
#define minif16_Tiles		minif16_akoimeexx_Tiles
#define sprite16_Tiles		sprite16_akoimeexx_Tiles
#define sprite_Palette		sprite_akoimeexx_Palette
#define master_Palette		master_akoimeexx_Palette
    #include "akoimeexx/dungeon16_16.c"
    #include "akoimeexx/alphabet_classic_8.c"
    #include "akoimeexx/alphabet_brass_8.c"
    #include "akoimeexx/alphabet_shadow_8.c"
    #include "akoimeexx/alphabet_heavy_8.c"
    #include "akoimeexx/alphabet_light_8.c"
    #include "akoimeexx/mini16_16.c"
    #include "akoimeexx/minif16_16.c"
    #include "akoimeexx/sprite16_16.c"
    #include "akoimeexx/sprite.pal.c"
    #include "akoimeexx/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#endif

#if defined(USING_SDL) && !defined(_WIN32_WCE)

//
// Ibson the Grey's Tiles: (Thanks to self-titled author)
//

#define dungeon16_Tiles		dungeon16_ibsongrey_Tiles
#define alphabet_classic_Tiles	alphabet_ibsongrey_classic_Tiles
#define alphabet_brass_Tiles	alphabet_ibsongrey_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_ibsongrey_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_ibsongrey_heavy_Tiles
#define alphabet_light_Tiles	alphabet_ibsongrey_light_Tiles
#define mini16_Tiles		mini16_ibsongrey_Tiles
#define minif16_Tiles		minif16_ibsongrey_Tiles
#define sprite16_Tiles		sprite16_ibsongrey_Tiles
#define sprite_Palette		sprite_ibsongrey_Palette
#define master_Palette		master_ibsongrey_Palette
    #include "ibsongrey/dungeon16_16.c"
    #include "ibsongrey/alphabet_classic_8.c"
    #include "ibsongrey/alphabet_brass_8.c"
    #include "ibsongrey/alphabet_shadow_8.c"
    #include "ibsongrey/alphabet_heavy_8.c"
    #include "ibsongrey/alphabet_light_8.c"
    #include "ibsongrey/mini16_16.c"
    #include "ibsongrey/minif16_16.c"
    #include "ibsongrey/sprite16_16.c"
    #include "ibsongrey/sprite.pal.c"
    #include "ibsongrey/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#endif

//
// Chris Lomaka's Tiles: (Thanks to self-titled author)
//
#define dungeon16_Tiles		dungeon16_lomaka_Tiles
#define alphabet_classic_Tiles	alphabet_lomaka_classic_Tiles
#define alphabet_brass_Tiles	alphabet_lomaka_brass_Tiles
#define alphabet_shadow_Tiles	alphabet_lomaka_shadow_Tiles
#define alphabet_heavy_Tiles	alphabet_lomaka_heavy_Tiles
#define alphabet_light_Tiles	alphabet_lomaka_light_Tiles
#define mini16_Tiles		mini16_lomaka_Tiles
#define minif16_Tiles		minif16_lomaka_Tiles
#define sprite16_Tiles		sprite16_lomaka_Tiles
#define sprite_Palette		sprite_lomaka_Palette
#define master_Palette		master_lomaka_Palette
    #include "lomaka/dungeon16_16.c"
    #include "lomaka/alphabet_classic_8.c"
    #include "lomaka/alphabet_brass_8.c"
    #include "lomaka/alphabet_shadow_8.c"
    #include "lomaka/alphabet_heavy_8.c"
    #include "lomaka/alphabet_light_8.c"
    #include "lomaka/mini16_16.c"
    #include "lomaka/minif16_16.c"
    #include "lomaka/sprite16_16.c"
    #include "lomaka/sprite.pal.c"
    #include "lomaka/master.pal.c"
#undef dungeon16_Tiles
#undef alphabet_classic_Tiles
#undef alphabet_brass_Tiles
#undef alphabet_shadow_Tiles
#undef alphabet_heavy_Tiles
#undef alphabet_light_Tiles
#undef mini16_Tiles
#undef minif16_Tiles
#undef sprite16_Tiles
#undef sprite_Palette
#undef master_Palette

#endif // not def _3DS
//
// Full Screen Images:
//

#ifdef iPOWDER
#define bmp_sprite16_3x bmp_sprite16_3x_data
#include "akoi3x/sprite16_3x.bmp.c"
#undef bmp_sprite16_3x
const unsigned short *bmp_sprite16_3x = bmp_sprite16_3x_data;
#endif

#ifndef _3DS
#if defined(USING_DS) || defined(USING_SDL)

#define bmp_tridude_goodbye_hires bmp_slug_and_blood_data
    #include "tridude_goodbye_hires.bmp.c"
#undef tridude_goodbye_hires
const unsigned short *bmp_slug_and_blood = bmp_slug_and_blood_data;

#else

#define bmp_tridude_goodbye bmp_slug_and_blood_data
    #include "tridude_goodbye.bmp.c"
#undef bmp_tridude_goodbye
const unsigned short *bmp_slug_and_blood = bmp_slug_and_blood_data;

#endif

#else //_3DS
 const unsigned short *bmp_slug_and_blood;
#endif

//
// Tileset linkage:
//	This is intentionaly not const so we can edit it with any
//	tilesets we find on disk.
//

TILESET glb_tilesetdata[NUM_TILESETS] =
{
#ifndef _3DS
    {
	{ 
	    alphabet_classic_classic_Tiles,
	    alphabet_classic_brass_Tiles,
	    alphabet_classic_shadow_Tiles,
	    alphabet_classic_heavy_Tiles,
	    alphabet_classic_light_Tiles,
	},
	dungeon16_classic_Tiles,
	mini16_classic_Tiles,
	minif16_classic_Tiles,
	master_classic_Palette,
	sprite16_classic_Tiles,
	sprite_classic_Palette,
	"Classic",
	8
    }

	,
#ifndef iPOWDER
    {
	{ 
	    alphabet_adambolt_classic_Tiles,
	    alphabet_adambolt_brass_Tiles,
	    alphabet_adambolt_shadow_Tiles,
	    alphabet_adambolt_heavy_Tiles,
	    alphabet_adambolt_light_Tiles,
	},
	dungeon16_adambolt_Tiles,
	mini16_adambolt_Tiles,
	minif16_adambolt_Tiles,
	master_adambolt_Palette,
	sprite16_adambolt_Tiles,
	sprite_adambolt_Palette,
	"Adam Bolt",
	8
    },
#endif
#ifndef iPOWDER
    {
	{ 
	    alphabet_nethack_classic_Tiles,
	    alphabet_nethack_brass_Tiles,
	    alphabet_nethack_shadow_Tiles,
	    alphabet_nethack_heavy_Tiles,
	    alphabet_nethack_light_Tiles,
	},
	dungeon16_nethack_Tiles,
	mini16_nethack_Tiles,
	minif16_nethack_Tiles,
	master_nethack_Palette,
	sprite16_nethack_Tiles,
	sprite_nethack_Palette,
	"Nethackish",
	8
    },
#endif
    {
	{ 
	    alphabet_ascii_classic_Tiles,
	    alphabet_ascii_brass_Tiles,
	    alphabet_ascii_shadow_Tiles,
	    alphabet_ascii_heavy_Tiles,
	    alphabet_ascii_light_Tiles,
	},
	dungeon16_ascii_Tiles,
	mini16_ascii_Tiles,
	minif16_ascii_Tiles,
	master_ascii_Palette,
	sprite16_ascii_Tiles,
	sprite_ascii_Palette,
	"Graphical ASCII",
	8
    },
#ifndef USING_TILE10
    {
	{ 
	    alphabet_akoimeexx_classic_Tiles,
	    alphabet_akoimeexx_brass_Tiles,
	    alphabet_akoimeexx_shadow_Tiles,
	    alphabet_akoimeexx_heavy_Tiles,
	    alphabet_akoimeexx_light_Tiles,
	},
	dungeon16_akoimeexx_Tiles,
	mini16_akoimeexx_Tiles,
	minif16_akoimeexx_Tiles,
	master_akoimeexx_Palette,
	sprite16_akoimeexx_Tiles,
	sprite_akoimeexx_Palette,
	"Akoi Meexx",
	8
    },
#endif
#if defined(USING_TILE10)
    {
	{ 
	    alphabet_distorted_classic_Tiles,
	    alphabet_distorted_brass_Tiles,
	    alphabet_distorted_shadow_Tiles,
	    alphabet_distorted_heavy_Tiles,
	    alphabet_distorted_light_Tiles,
	},
	dungeon16_distorted_Tiles,
	mini16_distorted_Tiles,
	minif16_distorted_Tiles,
	master_distorted_Palette,
	sprite16_distorted_Tiles,
	sprite_distorted_Palette,
	"Akoi Meexx 10",
	10
    },
#endif
#if defined(iPOWDER) || (defined(USING_SDL) && !defined(_WIN32_WCE))
    {
	{ 
	    alphabet_akoi12_classic_Tiles,
	    alphabet_akoi12_brass_Tiles,
	    alphabet_akoi12_shadow_Tiles,
	    alphabet_akoi12_heavy_Tiles,
	    alphabet_akoi12_light_Tiles,
	},
	dungeon16_akoi12_Tiles,
	mini16_akoi12_Tiles,
	minif16_akoi12_Tiles,
	master_akoi12_Palette,
	sprite16_akoi12_Tiles,
	sprite_akoi12_Palette,
	"Akoi Meexx 12",
	12
    },
    {
	{ 
	    alphabet_akoi10_classic_Tiles,
	    alphabet_akoi10_brass_Tiles,
	    alphabet_akoi10_shadow_Tiles,
	    alphabet_akoi10_heavy_Tiles,
	    alphabet_akoi10_light_Tiles,
	},
	dungeon16_akoi10_Tiles,
	mini16_akoi10_Tiles,
	minif16_akoi10_Tiles,
	master_akoi10_Palette,
	sprite16_akoi10_Tiles,
	sprite_akoi10_Palette,
	"Akoi Meexx 10",
	10
    },
#endif
#if defined(USING_SDL)
    {
	{ 
	    alphabet_ibsongrey_classic_Tiles,
	    alphabet_ibsongrey_brass_Tiles,
	    alphabet_ibsongrey_shadow_Tiles,
	    alphabet_ibsongrey_heavy_Tiles,
	    alphabet_ibsongrey_light_Tiles,
	},
	dungeon16_ibsongrey_Tiles,
	mini16_ibsongrey_Tiles,
	minif16_ibsongrey_Tiles,
	master_ibsongrey_Palette,
	sprite16_ibsongrey_Tiles,
	sprite_ibsongrey_Palette,
	"Ibson the Grey",
	16
    },
#endif
    {
	{ 
	    alphabet_lomaka_classic_Tiles,
	    alphabet_lomaka_brass_Tiles,
	    alphabet_lomaka_shadow_Tiles,
	    alphabet_lomaka_heavy_Tiles,
	    alphabet_lomaka_light_Tiles,
	},
	dungeon16_lomaka_Tiles,
	mini16_lomaka_Tiles,
	minif16_lomaka_Tiles,
	master_lomaka_Palette,
	sprite16_lomaka_Tiles,
	sprite_lomaka_Palette,
	"Chris Lomaka",
	8
    },
#else
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"Akoi Meexx",
	8
    },
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"Classic",
	8
    },
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"Adam Bolt",
	8
    },
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"Nethackish",
	8
    },
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"Graphical ASCII",
	8
    },
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"Chris Lomaka",
	8
    },
#endif
    {
	{
	    0,
	    0,
	    0,
	    0,
	    0,
	},
	0,
	0,
	0,
	0,
	0,
	0,
	"From Disk",
	8
    }
};

TILESET	*glb_tilesets = glb_tilesetdata;
