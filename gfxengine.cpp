/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        gfxengine.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This simple set of routines allows a somewhat platform independent
 *	way of drawing to the GBA.
 */

#include "gfxengine.h"
#include "gfxengine.h"

#include "mygba.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "gfx/all_bitmaps.h"

#include "map.h"
#include "creature.h"
#include "glbdef.h"
#include "assert.h"
#include "item.h"
#include "control.h"
#include "msg.h"
#include "stylus.h"
#include "victory.h"
#include "bmp.h"

// The number of 8x8 tiles to have.  The maximum for 256 colours
// on the GBA is 512.  SDL is theoritically infinite, except the 
// 1024 and 2048 bits are used for flip flags.
#if defined(USING_SDL)
#define TILEARRAY	1023
#else
#define TILEARRAY	512
#endif

// This is used for the maximum number the bitmap routines can grab.
// We can safely bump this up in the SDL case.
#if defined(USING_SDL)
// Since it is a 16x16 map, this is sufficient.    
#define TILESTASH	256
#else
#define TILESTASH	50
#endif

// Gets the number of bytes of a tile in our current tileset
#ifdef USING_SDL
#define BYTEPERTILE (glb_tilesets[glb_tileset].tilewidth*glb_tilesets[glb_tileset].tilewidth)
#define WORDPERTILE (glb_tilesets[glb_tileset].tilewidth*glb_tilesets[glb_tileset].tilewidth / 2)
#define TILEWIDTH (glb_tilesets[glb_tileset].tilewidth)
#define TILEHEIGHT (glb_tilesets[glb_tileset].tilewidth)
#else
#define BYTEPERTILE 8*8
#define WORDPERTILE 4*8
#define TILEWIDTH 8
#define TILEHEIGHT 8
#endif

#ifdef USING_DS
#define RAW_SCREEN_WIDTH 256
#define RAW_SCREEN_HEIGHT 192
#define PLATE_WIDTH 256
#define PLATE_HEIGHT 192
#include <fat.h>
#else
#ifdef USING_SDL
#define RAW_SCREEN_WIDTH (TILEWIDTH * 32)
#define RAW_SCREEN_HEIGHT (TILEWIDTH * 24)
#define PLATE_WIDTH 256
#define PLATE_HEIGHT 192
#else
#ifdef _3DS
#define RAW_SCREEN_WIDTH 256
#define RAW_SCREEN_HEIGHT 192
#define PLATE_WIDTH 256
#define PLATE_HEIGHT 192
#else
#define RAW_SCREEN_WIDTH 240
#define RAW_SCREEN_HEIGHT 160
#define PLATE_WIDTH 240
#define PLATE_HEIGHT 160
#endif
#endif
#endif

// Global variables
int	glb_newframe = 0;
volatile int	glb_framecount = 0;

int	glb_gfxmode = -1;

// Tileset defaults to Akoi Meexx.
#ifndef iPOWDER
#ifndef _3DS
int	glb_tileset = 4;
#else
int	glb_tileset = 0;
#endif
#else
int	glb_tileset = 2;
int	glb_modetilesets[2] = { 2, 2 };
int	glb_tilesetmode = 0;
#endif
#ifdef _3DS
u16* bmp_powder;
#endif
int	glb_currentfont = 1;

// This is our current tile reference array.
// glb_tileidx[TILE_NAME] gives the gba tile index.
// This has already been multiplied by 4!
u16	glb_tileidx[NUM_TILES];

// This is the gba tile that ties to the given character.
u16	glb_charidx[256];

#define MAX_COLOUR_CHAR 10
u8	*glb_colourchardata[MAX_COLOUR_CHAR];

// glb_idxcount[gbaidx] gives the number of times that tile
// is currently used.
u16	glb_idxcount[TILEARRAY];

// This is a map of our current 64x64 tile layout.
u16	*glb_tilemap, *glb_shadowmap, *glb_mobmap;
u16	*glb_abstilemap, *glb_absmobmap, *glb_absshadowmap;
#ifdef _3DS
u16	*glb_abstilemap2, *glb_absshadowmap2, *glb_absmobmap2;
#endif
// This is 32x24 and matches the currently displayed characters.
u8	*glb_charmap;

// Used in our compositing engine for building the paper doll
u8 *glb_comptiledata = 0;
int glb_lastdesttile;

// Used in our greyscale sprite engine, must be global so we
// can resize this
u8 *glb_greyscaletiledata = 0;

// Our one lone sprite.
u16	 glb_sprite = SPRITE_CURSOR;

// This is the current scroll offset of where our tile layout matches
// with the tilemap.
int	glb_scrollx, glb_scrolly;
// This is the tile offset of the center tile.  We use this for inventory.
int	glb_offx, glb_offy;
// This is the pixel level scroll to nudge all the scrolling planes.
s8	glb_nudgex = 0, glb_nudgey = 0;

// Current inventory position..  Defaults to where the first added
// item will go.
int	glb_invx = 1, glb_invy = 0;
MOB	*glb_invmob = 0;

// Are we in sprite or dungeon mode?
bool	glb_usesprites = true;

// bg2 tiles.  This maps to the screen with 240/8 = 30 to 160/8 = 20
// ratio.  Thus, there are are 600 tiles.
tile_info_ptr	 glb_bg2tiles;
tile_info_ptr	 glb_bg1tiles;
// This has the same format as tiles!  It is XxY for 8x8 blocks,
// of which there are 30x20.
char		*glb_bg2tiledata;

// This the palette entry for each of the standard colours.
char		 glb_stdcolor[NUM_COLOURS];

// This is the mapping for each colour in the sprite palette into
// the greyscale equivalent.
u8		*glb_spritegreyscale = 0;

// Text look up tables...
const char *glb_texttable[] = { 
    "abcdefghij",
    "klmnopqrst",
    "uvwxyz" SYMBOLSTRING_HEART SYMBOLSTRING_MAGIC SYMBOLSTRING_NEXT SYMBOLSTRING_DLEVEL,
    "ABCDEFGHIJ",
    "KLMNOPQRST",
    "UVWXYZ" SYMBOLSTRING_LEFT SYMBOLSTRING_RIGHT SYMBOLSTRING_UP SYMBOLSTRING_DOWN,
    "0123456789",
    "!@#$%^&*()",
    "=-\\',./+_|",
    "\"<>?[]{}`~",
    ",.<>;:" SYMBOLSTRING_AC SYMBOLSTRING_TRIDUDE SYMBOLSTRING_CURSOR " ",
    SYMBOLSTRING_UNIQUE "         ",
    0
};

u8
rgbtogrey(u8 cr, u8 cg, u8 cb)
{
    // Roughly a 5:6:1 weighting, but scaled up so we
    // get to divide by 16 for some meaningless efficiency.
    int 	grey = cg * 8 + cr * 6 + cb * 2;
    grey += 7;
    grey /= 16;
    if (grey > 255)
	grey = 255;

    return grey;
}

#ifdef USING_SDL
static int
rawtoplate_x(int x)
{
    int	sx =  (int) ((x * PLATE_WIDTH) / ((float)(RAW_SCREEN_WIDTH)) + 0.5);
    if (sx >= PLATE_WIDTH)
	sx = PLATE_WIDTH-1;
    return sx;
}

static int
rawtoplate_y(int y)
{
    int	sy =  (int) ((y * PLATE_HEIGHT) / ((float)(RAW_SCREEN_HEIGHT)) + 0.5);
    if (sy >= PLATE_HEIGHT)
	sy = PLATE_HEIGHT-1;
    return sy;
}
#else
static int
rawtoplate_x(int x)
{
    return x;
}

static int
rawtoplate_y(int y)
{
    return y;
}
#endif

int
gfx_getchartile(char c)
{
    int		textline;
    const char *text;
    int		pos;

    // Special cases...
    switch (c)
    {
	case '\b':
	    c = SYMBOL_LEFT;
	    break;
	case '\n':
	case '\r':
	    c = SYMBOL_NEXT;
	    break;
	case '\t':
	    c = SYMBOL_UP;
	    break;
    }

    pos = 0;
    textline = 0;
    while (glb_texttable[textline])
    {
	text = glb_texttable[textline];
	while (*text)
	{
	    if (*text == c)
		return pos;
	    pos++;
	    text++;
	}
	textline++;
    }
    // Failure, lower case a.
    return 1;
}

u16
gfx_lockColourCharTile(u8 c, COLOUR_NAMES colour, u8 *result)
{
    u8		virtualtile, charnum;
    int		idx, sourceidx;
    
    // Find a free colour tile in our desired range.
    for (virtualtile = COLOURED_SYMBOLS; virtualtile < COLOURED_SYMBOLS + MAX_COLOUR_CHAR; virtualtile++)
    {
	if (glb_charidx[virtualtile] == INVALID_TILEIDX)
	{
	    // This is a free index!
	    break;
	}
    }
    // Check to see if we failed, if so, return the no tile.
    // Flag that we shouldn't unlock this tile.
    if (virtualtile == COLOURED_SYMBOLS + MAX_COLOUR_CHAR)
    {
	*result = 0xff;
	return gfx_lookupTile(TILE_NOTILE);
    }

    // Now try and allocate a tile.
    idx = gfx_findFreeTile(1);
    if (idx == INVALID_TILEIDX)
    {
	// Bah, ran out of tiles!
	*result = 0xff;
	return gfx_lookupTile(TILE_NOTILE);
    }

    // Now find what origin tile we want to map from.
    sourceidx = gfx_getchartile(c);

    charnum = virtualtile - COLOURED_SYMBOLS;
    // Check to see if our tiledata is available.
    if (!glb_colourchardata[charnum])
	glb_colourchardata[charnum] = (u8 *) new u16[WORDPERTILE];

    u8		*dst;
    const u8	*src;
    u16 	*palette = (u16 *) glb_tilesets[glb_tileset].palette;
    int		 i;

    dst = glb_colourchardata[charnum];
    src = (const u8*)&glb_tilesets[glb_tileset].alphabet[glb_currentfont][sourceidx*BYTEPERTILE];

    for (i = 0; i < BYTEPERTILE; i++)
    {
	// Anything not black or transparent is turned into
	// our desired colour.
	if (*src == 0 || *src == glb_stdcolor[COLOUR_BLACK] || !palette[*src] || palette[*src] == 0x842)
	    *dst = *src;
	else
	    *dst = glb_stdcolor[colour];
	dst++;
	src++;
    }

    // And send the resulting tile to VRAM.
    ham_ReloadTileGfx(glb_bg1tiles,
		(u16*) glb_colourchardata[charnum],
		idx,
		1);
    glb_idxcount[idx]++;
    glb_charidx[virtualtile] = idx;
    
    // When we unlock, this is the tile we'll actually unlock!
    *result = virtualtile;

    return idx;
}

u16
gfx_lockCharTile(u8 c, u8 *result)
{
    int		idx;
    int		sourceidx;
    
    // Check our character tile array to see if it is already done.
    *result = c;

    idx = glb_charidx[c];
    if (idx != INVALID_TILEIDX)
    {
	// Great, already allocated, inc and return.
	glb_idxcount[idx]++;
	return idx;
    }

    // Must allocate it...
    idx = gfx_findFreeTile(1);

    if (idx != INVALID_TILEIDX)
    {
	sourceidx = gfx_getchartile(c);
	ham_ReloadTileGfx(glb_bg1tiles,
		(u16*)(&glb_tilesets[glb_tileset].alphabet[glb_currentfont][sourceidx * BYTEPERTILE]),
		idx,
		1);
	glb_idxcount[idx]++;
	glb_charidx[c] = idx;
	return idx;
    }

    // Damn, failure to allocate!
    UT_ASSERT(!"Failure to allocate char tile");
    // Flag that we shouldn't unlock this tile.
    *result = 0xff;
    return gfx_lookupTile(TILE_NOTILE);
}

void
gfx_unlockCharTile(u8 c)
{
    u16		idx;

    if (c == 0xff)
	return;		// Blank dude.

    idx = glb_charidx[c];
    
    UT_ASSERT(idx != INVALID_TILEIDX);
    if (idx == INVALID_TILEIDX)
	return;

    UT_ASSERT(glb_idxcount[idx]);
    if (glb_idxcount[idx])
	glb_idxcount[idx]--;
    if (!glb_idxcount[idx])
    {
	// TIle is now free.
	glb_charidx[c] = INVALID_TILEIDX;
    }
}

u16
gfx_findFreeTile(int size)
{
    int		tile, j;

    for (tile = 0; tile <= TILEARRAY - size; tile++)
    {
	if (glb_idxcount[tile])
	    continue;

	// We have a zero, check if next size are zero..
	for (j = 0; j < size; j++)
	{
	    if (glb_idxcount[tile+j])
	    {
		tile = tile+j;
		break;
	    }
	}

	// Making it here implies that we found a free tile.
	if (j == size)
	    return tile;
    }

    // Failure to find a free one.
    return INVALID_TILEIDX;
}

u16
gfx_lookupTile(TILE_NAMES tile)
{
    if (tile == INVALID_TILEIDX)
	return INVALID_TILEIDX;
    
    return glb_tileidx[tile];
}

u16 
gfx_lockTile(TILE_NAMES tile, TILE_NAMES *result)
{
    u16		idx;

    if (result)
	*result = tile;

    if (tile == INVALID_TILEIDX)
	return INVALID_TILEIDX;
    idx = glb_tileidx[tile];
    if (idx != INVALID_TILEIDX)
    {
	if (tile == TILE_VOID || tile == TILE_NOTILE)
	{
	    // We only lock these tiles once ever.
	    return idx;
	}
	glb_idxcount[idx]++;
	glb_idxcount[idx+1]++;
	glb_idxcount[idx+2]++;
	glb_idxcount[idx+3]++;
	return idx;
    }
    idx = gfx_findFreeTile(4);
    if (idx != INVALID_TILEIDX)
    {
	// A free index!  Return it.
	ham_ReloadTileGfx(glb_bg1tiles, 
		(u16*)(&glb_tilesets[glb_tileset].dungeon[tile * 4 * BYTEPERTILE]),
		idx,
		4);
	glb_idxcount[idx]++;
	glb_idxcount[idx+1]++;
	glb_idxcount[idx+2]++;
	glb_idxcount[idx+3]++;
	glb_tileidx[tile] = idx;
	return idx;
    }
    // Failed to find free...
    UT_ASSERT(!"FAILED TO ALLOC TILE!");
    if (result)
	*result = (TILE_NAMES) INVALID_TILEIDX;
    return gfx_lookupTile(TILE_NOTILE);
}

void
gfx_forceLockTile(TILE_NAMES tile, int mincount)
{
    int		idx, j;
    
    // See if our tile is already locked.
    // Problem: If we have a 
    idx = gfx_lookupTile(tile);
    if (idx == INVALID_TILEIDX)
    {
	// Tile not yet locked, lock it!
	idx = gfx_lockTile(tile, 0);
    }

    // Verify we have the minimum count.
    for (j = 0; j < 4; j++)
    {
	if (glb_idxcount[idx+j] < mincount)
	    glb_idxcount[idx+j] = mincount;
    }
}

void
gfx_unlockTile(TILE_NAMES tile)
{
    u16		idx;
    
    if (tile == INVALID_TILEIDX)
	return;

    if (tile == TILE_NOTILE || tile == TILE_VOID)
    {
	// Never unlock these special tiles.
	// We need to be able to always procur them
	return;
    }

    idx = glb_tileidx[tile];
    
    UT_ASSERT(idx != INVALID_TILEIDX);
    if (idx == INVALID_TILEIDX)
	return;
    
    // Must have already locked it...
    UT_ASSERT(glb_idxcount[idx]);
    UT_ASSERT(glb_idxcount[idx+1]);
    UT_ASSERT(glb_idxcount[idx+2]);
    UT_ASSERT(glb_idxcount[idx+3]);
    glb_idxcount[idx]--;
    glb_idxcount[idx+1]--;
    glb_idxcount[idx+2]--;
    glb_idxcount[idx+3]--;
    if (!glb_idxcount[idx])
    {
	// Tile is now free...
	glb_tileidx[tile] = INVALID_TILEIDX;
    }
}

int
gfx_getNumFreeTiles()
{
    int		idx, num;

    num = 0;
    for (idx = 0; idx < TILEARRAY; idx++)
    {
	if (glb_idxcount[idx] == 0)
	    num++;
    }
    return num;
}

//
// VBL func
//
void vblFunc()
{
    glb_newframe = 1;
    glb_framecount++;
}

void
gfx_buildstdcolors()
{
    COLOUR_NAMES		colour;

    FOREACH_COLOUR(colour)
    {
	glb_stdcolor[colour] = gfx_lookupcolor(glb_colourdefs[colour].red,
				    glb_colourdefs[colour].green,
				    glb_colourdefs[colour].blue);
    }
    // Build the standard colours...
    glb_stdcolor[COLOUR_INVISIBLE] = 0;		// Always true.

    // This is also a good time to update our grey table if we have it.
    gfx_updatespritegreytable();
}

void
gfx_init()
{
    // Initialize everything...
    ham_Init();

    // Install our interrupt handler
#if defined(USING_SDL) || defined(USING_DS) || defined(NO_HAM) || defined(_3DS)
    ham_StartIntHandler(INT_TYPE_VBL, &vblFunc);
#else
    ham_StartIntHandler(INT_TYPE_VBL, (void *)&vblFunc);
#endif

    // These maps store the TILE_NAMES currently at each map position.
    // The absolute version do the same for absolute overlays.
    // This allows us to avoid double writes, and load tiles into
    // VRAM as necessary.
    // 0xff is used as INVALID_TILEIDX is 65535.

    glb_tilemap = new u16[32*32];
    glb_shadowmap = new u16[32*32];
    glb_mobmap = new u16[32*32];
    glb_abstilemap = new u16[17*12];
    glb_absmobmap = new u16[17*12];
    glb_absshadowmap = new u16[17*12];
    glb_charmap = new u8[32*24];

    memset(glb_tilemap, 0xff, 32*32 * sizeof(u16));
    memset(glb_shadowmap, 0xff, 32*32 * sizeof(u16));
    memset(glb_mobmap, 0xff, 32*32 * sizeof(u16));
    memset(glb_abstilemap, 0xff, 17*12*sizeof(u16));
    memset(glb_absmobmap, 0xff, 17*12*sizeof(u16));
    memset(glb_absshadowmap, 0xff, 17*12*sizeof(u16));
    memset(glb_charmap, 0xff, 32*24);
#ifdef _3DS
    glb_abstilemap2 = new u16[17*12];
    glb_absmobmap2 = new u16[17*12];
    glb_absshadowmap2 = new u16[32*32];
    memset(glb_abstilemap2, 0xff, 17*12*sizeof(u16));
    memset(glb_absmobmap2, 0xff, 17*12*sizeof(u16));
    memset(glb_absshadowmap2, 0xff, 17*12*sizeof(u16));

    int dw, dh; // unused
	bmp_slug_and_blood = bmp_load("gfx/tridude_goodbye_hires.bmp", dw, dh, true);
	bmp_powder = bmp_load("gfx/powder.bmp", dw, dh, true);
#endif
}

void
gfx_reblitslugandblood()
{
    u16			*screen;
    int			x, y;

    screen = hamfake_lockScreen();
#if defined(USING_DS) || defined(USING_SDL)
    // Scroll back the screen since it was in GBA coords.
    screen -= TILEWIDTH + 2*TILEHEIGHT * RAW_SCREEN_WIDTH;

    if (RAW_SCREEN_WIDTH != PLATE_WIDTH ||
	RAW_SCREEN_HEIGHT != PLATE_HEIGHT)
    {
	for (y = 0; y < RAW_SCREEN_HEIGHT; y++)
	{
	    int	sy = rawtoplate_y(y);
	    for (x = 0; x < RAW_SCREEN_WIDTH; x++)
	    {
		int	sx = rawtoplate_x(x);
		
		screen[y*RAW_SCREEN_WIDTH+x] = bmp_slug_and_blood[sy*PLATE_WIDTH+sx];
	    }
	}
    }
    else
    {
	for (y = 0; y < PLATE_HEIGHT; y++)
	{
	    for (x = 0; x < PLATE_WIDTH; x++)
	    {
		screen[y*RAW_SCREEN_WIDTH+x] = bmp_slug_and_blood[y*PLATE_WIDTH+x];
	    }
	}
    }
#else
    for (y = 0; y < PLATE_WIDTH*PLATE_HEIGHT; y++)
    {
	screen[y] = bmp_slug_and_blood[y];
    }
#endif
    hamfake_unlockScreen(screen);

#ifdef _3DS
	screen = hamfake_lockRawScreen();
    for (y = 0; y < PLATE_WIDTH*PLATE_HEIGHT; y++)
    {
	screen[y] = bmp_powder[y];
    }
    hamfake_unlockScreen(screen);
#endif
}

void
gfx_setmode(int mode)
{
    int			x, y;
    map_info_ptr	bg2mapset;
    
    // A no-op.
    if (mode == glb_gfxmode)
	return;

    glb_gfxmode = mode;
    
    if (mode == 3)
    {
	// Intro screen...
	ham_SetBgMode(3);
	gfx_reblitslugandblood();

	// Ensure we have current sprite date.
	hamfake_LoadSpritePal((void *)glb_tilesets[glb_tileset].spritepalette, 512);

	hamfake_ReloadSpriteGfx((u16*)(&glb_tilesets[glb_tileset].sprite[glb_sprite*4*BYTEPERTILE]), 0, 4);
	
	// NOTE: This actually copies incorrect data!  Likely due to it
	// using 32bit copies or ignoring waitstates.    
	//    memcpy( (unsigned short *) 0x06000000,
	//	    bmp_slug_and_blood,
	//	    38400*2 );
	return;
    }
    
    // Must be tiled mode.
    UT_ASSERT(mode == 0);
    
    // This is all four bgs, no zooming.
    ham_SetBgMode(0);

    // Wipe any charmap left over from raw mode.
    memset(glb_charmap, 0xff, 32*24);

    ham_LoadBGPal((void *)glb_tilesets[glb_tileset].palette, 512);
    hamfake_LoadSpritePal((void *)glb_tilesets[glb_tileset].spritepalette, 512);

    // So, what are our layers?

    // 0: 32x32: Text and minimap.
    // 1: 64x64: Terrain tiles
    // 2: 64x64: Monters/Item tiles
    // 3: 64x64: Shadow tiles
#if 1
    // We want a 32x32 as we only will use one screen worth.
    ham_bg[0].mi = ham_InitMapEmptySet(0, 0);

    // We want a 64x64 screen.
    ham_bg[1].mi = ham_InitMapEmptySet(3, 0);

    // 64x64 map
    bg2mapset = ham_InitMapEmptySet(3, 0);

    // We want a 64x64 screen.
    ham_bg[3].mi = ham_InitMapEmptySet(3, 0);

#ifdef _3DS	
    ham_bg[4].mi = ham_InitMapEmptySet(0, 0);
    ham_bg[5].mi = ham_InitMapEmptySet(0, 0);
    ham_bg[6].mi = ham_InitMapEmptySet(0, 0);
#endif	

#else
    // We want a 32x32 as we only will use one screen worth.
    ham_bg[0].mi = ham_InitMapEmptySet(0, 0);

    // We want a 64x64 screen.
    ham_bg[1].mi = ham_bg[0].mi;

    // 32 x 32 map.
    bg2mapset = ham_bg[0].mi;

    // We want a 64x64 screen.
    ham_bg[3].mi = ham_bg[0].mi;
#endif
    
    ham_bg[1].ti = ham_InitTileEmptySet(TILEARRAY, 1, 1);

    // Init the dungeon tile set...
    //   ** EXCESSIVE PROFANITY DETECTED **
    //   ** DELETING OFFENDING LINES **
    //   ** RESUMING COMMENT **
    // to respond to so FOAD, HTH, HAND.
    
    // Thus, we now create a single tile set of TILEARRAY size.
    // NOTE: We now share this with the bitmap cache, so that
    // code currently assumes it is contiguous and uses 8x8 rather
    // than 16x16, so for now we'll keep them the same.
    //ham_bg[1].ti = ham_InitTileSet((void *)&DUNGEONTILES, 
    //				(NUM_TILES-3) * 4 * WORDPERTILE, 1, 1);
    glb_bg1tiles = ham_bg[1].ti;

    ham_bg[0].ti = ham_bg[1].ti;

    // Initialize our arrays to be all empty...
    memset(glb_tileidx, 0xff, sizeof(u16) * NUM_TILES);
    memset(glb_charidx, 0xff, sizeof(u16) * 256);
    memset(glb_idxcount, 0, sizeof(u16) * TILEARRAY);

    // Create the level 2 bg, where we do all our crazy pixel level
    // stuff.
    // You would think this size would have to be 600.  That crashes
    // due to out of vram, yet I seem to be able to write to it just
    // fine?  WTF?
    // Calculations show that we can't fit, even in an ideal case,
    // this much data in the 64k available in the tile/map data.
    // One option is to use the object data set (which is 32k), except
    // the actual screen required is 37.5k.  THus, we instead create
    // a tilestash which we can read from & deal with that way.
    // glb_bg2tiles = ham_InitTileEmptySet(TILESTASH, 1, 0);
    glb_bg2tiles = glb_bg1tiles;

    // Highest priority as it will sit on top.
    ham_InitBg(0, 1, 0, 0);

    ham_InitBg(1, 1, 3, 0);

    // Create our level 3 background, where we have our lighting overlay
    // and put blood sprites, etc.
    ham_bg[3].ti = ham_bg[1].ti;
    // Above the lower map, but below the text.
    ham_InitBg(3, 1, 1, 0);

    memset(glb_tilemap, 0xff, 32*32 * sizeof(u16));
    memset(glb_shadowmap, 0xff, 32*32 * sizeof(u16));
    memset(glb_mobmap, 0xff, 32*32 * sizeof(u16));
    memset(glb_abstilemap, 0xff, 17*12*sizeof(u16));
    memset(glb_absmobmap, 0xff, 17*12*sizeof(u16));
    memset(glb_absshadowmap, 0xff, 17*12*sizeof(u16));
    memset(glb_charmap, 0xff, 32*24);

    ham_bg[2].ti = glb_bg2tiles;
    ham_bg[2].mi = bg2mapset;

    // We allocate a half sized block as u16 and cast back to char
    // to ensure we are aligned to u16.
    glb_bg2tiledata = (char *) new u16[WORDPERTILE*TILESTASH];
    memset(glb_bg2tiledata, 0, BYTEPERTILE*TILESTASH);
    
    // No rot.  Above the ground but below the shadow.
    ham_InitBg(2, 1, 2, 0);

#ifdef _3DS	
    ham_bg[4].ti = ham_bg[1].ti;
    ham_bg[5].ti = ham_bg[2].ti;
    ham_bg[6].ti = ham_bg[0].ti;
    memset(glb_abstilemap2, 0xff, 17*12*sizeof(u16));
    memset(glb_absmobmap2, 0xff, 17*12*sizeof(u16));
    memset(glb_absshadowmap2, 0xff, 17*12*sizeof(u16));
    ham_InitBg(4, 1, 3, 0);
    ham_InitBg(5, 1, 2, 0);
    ham_InitBg(6, 1, 2, 0);
#endif
    
    gfx_buildstdcolors();

    // We always want the no tile locked so we can show it when
    // things go bad...
    // We want to lock VOID first as it will be tile 0, and thus
    // our default screen will be sweet, sweet, black.
    // Actaully, there is apparently no guarantee that the initial
    // tile values are 0.  Thus, we must manually set all of our
    // tile planes to void here.
    int			voidtileidx;

    // We know these locks must always succeed so we don't care to stash
    // the result.  In any case, we have engineered it that they are
    // never freed.
    voidtileidx = gfx_lockTile(TILE_VOID, 0);
    gfx_lockTile(TILE_NOTILE, 0);
    
    for (y = 0; y < 64; y++)
    {
	for (x = 0; x < 64; x++)
	{
	    if (x < 32 && y < 32)
	    {
		ham_SetMapTile(0, x, y, voidtileidx);
	    }
	    ham_SetMapTile(1, x, y, voidtileidx);
	    ham_SetMapTile(2, x, y, voidtileidx);
	    ham_SetMapTile(3, x, y, voidtileidx);
#ifdef _3DS
	    ham_SetMapTile(4, x, y, voidtileidx);
	    ham_SetMapTile(5, x, y, voidtileidx);
	    ham_SetMapTile(6, x, y, voidtileidx);
#endif
	}
    }

    // Load the sprite.
    hamfake_ReloadSpriteGfx((u16*)(&glb_tilesets[glb_tileset].sprite[glb_sprite*4*BYTEPERTILE]), 0, 4);
}


//
// This forces a rebuilding of all the tiles from our cache to the
// ham space.
//
void
gfx_refreshtiles()
{
    int		x, y, tile, tileidx;

    // Ignore this if we are not in a graphics mode.
    if (glb_gfxmode)
	return;

    // A refresh will also clear anything in the abs maps, so we
    // first unlock those tiles.
    for (y = 0; y < 12; y++)
    {
	for (x = 0; x < 17; x++)
	{
	    tile = glb_abstilemap[y * 17 + x];
	    if (tile != INVALID_TILEIDX)
	    {
		gfx_unlockTile((TILE_NAMES)tile);
		glb_abstilemap[y * 17 + x] = INVALID_TILEIDX;
	    }
#ifdef _3DS
	    tile = glb_abstilemap2[y * 17 + x];
	    if (tile != INVALID_TILEIDX)
	    {
		gfx_unlockTile((TILE_NAMES)tile);
		glb_abstilemap2[y * 17 + x] = INVALID_TILEIDX;
	    }
#endif
	    tile = glb_absshadowmap2[y * 17 + x];
	    if (tile != INVALID_TILEIDX)
	    {
		gfx_unlockTile((TILE_NAMES)tile);
		glb_absshadowmap2[y * 17 + x] = INVALID_TILEIDX;
	    }
#ifdef _3DS
	    tile = glb_abstilemap2[y * 17 + x];
	    if (tile != INVALID_TILEIDX)
	    {
		gfx_unlockTile((TILE_NAMES)tile);
		glb_abstilemap2[y * 17 + x] = INVALID_TILEIDX;
	    }
#endif
	    tile = glb_absmobmap[y * 17 + x];
	    if (tile != INVALID_TILEIDX)
	    {
		gfx_unlockTile((TILE_NAMES)tile);
		glb_absmobmap[y * 17 + x] = INVALID_TILEIDX;
	    }
#ifdef _3DS
	    tile = glb_absmobmap2[y * 17 + x];
	    if (tile != INVALID_TILEIDX)
	    {
		gfx_unlockTile((TILE_NAMES)tile);
		glb_absmobmap2[y * 17 + x] = INVALID_TILEIDX;
	    }
#endif		
	}
    }

    // TODO: Insert map might prove a tad bit more efficient, though
    // would need a full size scratch space.
    for (y = 0; y < 32; y++)
    {
	for (x = 0; x < 32; x++)
	{
	    tile = glb_tilemap[(y + glb_scrolly) * 32 + x + glb_scrollx];
	    tileidx = gfx_lockTile((TILE_NAMES)tile, (TILE_NAMES *) &tile);
	    ham_SetMapTile(1, x*2, y*2, tileidx);
	    ham_SetMapTile(1, x*2+1, y*2, tileidx+1);
	    ham_SetMapTile(1, x*2, y*2+1, tileidx+2);
	    ham_SetMapTile(1, x*2+1, y*2+1, tileidx+3);

	    // The above lock should be a double count, so we unlock
	    // afterwards.  Any write to glb_tilemap must have done
	    // a lock!
	    gfx_unlockTile((TILE_NAMES)tile);
	}
    }
    
    // Update overlay buffer...
    for (y = 0; y < 32; y++)
    {
	for (x = 0; x < 32; x++)
	{
	    tile = glb_shadowmap[(y + glb_scrolly) * 32 + x + glb_scrollx];
	    tileidx = gfx_lockTile((TILE_NAMES)tile, (TILE_NAMES *) &tile);
	    ham_SetMapTile(3, x*2, y*2, tileidx);
	    ham_SetMapTile(3, x*2+1, y*2, tileidx+1);
	    ham_SetMapTile(3, x*2, y*2+1, tileidx+2);
	    ham_SetMapTile(3, x*2+1, y*2+1, tileidx+3);
	    // See comment above why this is needed.
	    gfx_unlockTile((TILE_NAMES)tile);
	}
    }
    // Update mob buffer...
    for (y = 0; y < 32; y++)
    {
	for (x = 0; x < 32; x++)
	{
	    tile = glb_mobmap[(y + glb_scrolly) * 32 + x + glb_scrollx];
	    tileidx = gfx_lockTile((TILE_NAMES)tile, (TILE_NAMES *) &tile);
	    ham_SetMapTile(2, x*2, y*2, tileidx);
	    ham_SetMapTile(2, x*2+1, y*2, tileidx+1);
	    ham_SetMapTile(2, x*2, y*2+1, tileidx+2);
	    ham_SetMapTile(2, x*2+1, y*2+1, tileidx+3);
	    // See comment above why this is needed.
	    gfx_unlockTile((TILE_NAMES)tile);
	}
    }
#ifdef _3DS
    ham_bg[4].mi = ham_InitMapEmptySet(0, 0);
    ham_bg[5].mi = ham_InitMapEmptySet(0, 0);
    ham_bg[6].mi = ham_InitMapEmptySet(3, 0);
#endif

#if 0
    BUF			buf;
    buf.sprintf("%d, %d, %d, %d...%d, %d, %d, %d...",
	    (int)ham_bg[0].ti->first_block,
	    (int)ham_bg[1].ti->first_block,
	    (int)ham_bg[2].ti->first_block,
	    (int)ham_bg[3].ti->first_block,
	    (int)ham_bg[0].mi->first_block,
	    (int)ham_bg[1].mi->first_block,
	    (int)ham_bg[2].mi->first_block,
	    (int)ham_bg[3].mi->first_block);
    msg_report(buf);
#endif
}

void
gfx_settile(int tx, int ty, int tileno)
{
    int		hamx, hamy, tileidx;

    hamx = tx - glb_scrollx;
    hamy = ty - glb_scrolly;
    // Only send onto ham if on screen and is a real change.
    if (glb_tilemap[ty * 32 + tx] != tileno)
//	&& hamx < 32 && hamy < 32)
    {
	gfx_unlockTile((TILE_NAMES)glb_tilemap[ty * 32 + tx]);
	tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *) &tileno);
	ham_SetMapTile(1, tx*2, ty*2, tileidx);
	ham_SetMapTile(1, tx*2+1, ty*2, tileidx+1);
	ham_SetMapTile(1, tx*2, ty*2+1, tileidx+2);
	ham_SetMapTile(1, tx*2+1, ty*2+1, tileidx+3);

	// UPdate our local map.
	glb_tilemap[ty * 32 + tx] = tileno;
    }
}

void
gfx_setabstile(int tx, int ty, int tileno)
{
    // Handle ability to write to -1 column
    tx++;

    UT_ASSERT(tx >= 0);
    UT_ASSERT(tx < 17);
    UT_ASSERT(ty >= 0);
    UT_ASSERT(ty < 12);

    int		tx1, ty1, tileidx, abstile;

    // Update the abs map...
    abstile = glb_abstilemap[ty * 17 + tx];
    if (abstile == tileno)
	return;

    gfx_unlockTile((TILE_NAMES)abstile);
    tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
    glb_abstilemap[ty * 17 + tx] = tileno;
    
    // Extra -1 is for our offset.
    tx += glb_offx - 7 - 1;
    ty += glb_offy - 5;

    tx *= 2;
    ty *= 2;
    ty += 1;		// Correct for the off by one nature of glb_offx

    tx1 = tx + 1;
    ty1 = ty + 1;
    tx &= 63;
    ty &= 63;
    tx1 &= 63;
    ty1 &= 63;

    ham_SetMapTile(1, tx, ty, tileidx);
    ham_SetMapTile(1, tx1, ty, tileidx+1);
    ham_SetMapTile(1, tx, ty1, tileidx+2);
    ham_SetMapTile(1, tx1, ty1, tileidx+3);
}

#ifdef _3DS
void
gfx_setabstile2(int tx, int ty, int tileno)
{
    // Handle ability to write to -1 column
    tx++;

    UT_ASSERT(tx >= 0);
    UT_ASSERT(tx < 17);
    UT_ASSERT(ty >= 0);
    UT_ASSERT(ty < 12);

    int		tx1, ty1, tileidx, abstile;

    // Update the abs map...
    abstile = glb_abstilemap2[ty * 17 + tx];
    if (abstile == tileno)
	return;

    gfx_unlockTile((TILE_NAMES)abstile);
    tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
    glb_abstilemap2[ty * 17 + tx] = tileno;
/*    
    // Extra -1 is for our offset.
    tx += glb_offx - 7 - 1;
    ty += glb_offy - 5;
*/
    tx *= 2;
    ty *= 2;
    ty += 2;	
    tx -= 1;
	
    tx1 = tx + 1;
    ty1 = ty + 1;
    tx &= 63;
    ty &= 63;
    tx1 &= 63;
    ty1 &= 63;

    ham_SetMapTile(4, tx, ty, tileidx);
    ham_SetMapTile(4, tx1, ty, tileidx+1);
    ham_SetMapTile(4, tx, ty1, tileidx+2);
    ham_SetMapTile(4, tx1, ty1, tileidx+3);
}

#endif

void
gfx_setoverlay(int tx, int ty, int tileno)
{
    int		hamx, hamy, tileidx;

    hamx = tx - glb_scrollx;
    hamy = ty - glb_scrolly;
    // Only send onto ham if on screen and is a real change.
    if (glb_shadowmap[ty * 32 + tx] != tileno)
	// && hamx < 32 && hamy < 32)
    {
	gfx_unlockTile((TILE_NAMES)glb_shadowmap[ty * 32 + tx]);
	tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
	
	ham_SetMapTile(3, tx*2, ty*2, tileidx);
	ham_SetMapTile(3, tx*2+1, ty*2, tileidx+1);
	ham_SetMapTile(3, tx*2, ty*2+1, tileidx+2);
	ham_SetMapTile(3, tx*2+1, ty*2+1, tileidx+3);

	// UPdate our local map.
	glb_shadowmap[ty * 32 + tx] = tileno;
    }
}

int
gfx_getoverlay(int tx, int ty)
{
    return glb_shadowmap[ty * 32 + tx];
}

#ifdef _3DS
void
gfx_setabsoverlay2(int tx, int ty, int tileno)
{
    // Correct for ability to write to -1 column
    tx++;

    UT_ASSERT(tx >= 0);
    UT_ASSERT(tx < 17);
    UT_ASSERT(ty >= 0);
    UT_ASSERT(ty < 12);

    // Ignore this if we are not in a valid mode.
    if (glb_gfxmode)
	return;

    int		tx1, ty1, tileidx, abstile;
    
    // Update the abs map...
    abstile = glb_absshadowmap2[ty * 17 + tx];
    if (abstile == tileno)
	return;

    gfx_unlockTile((TILE_NAMES)abstile);
    tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
    glb_absshadowmap2[ty * 17 + tx] = tileno;

    tx *= 2;
    ty *= 2;
    ty += 2;
    tx -= 1;
	
    tx1 = tx + 1;
    ty1 = ty + 1;
    tx &= 63;
    ty &= 63;
    tx1 &= 63;
    ty1 &= 63;
    
    ham_SetMapTile(6, tx, ty, tileidx);
    ham_SetMapTile(6, tx1, ty, tileidx+1);
    ham_SetMapTile(6, tx, ty1, tileidx+2);
    ham_SetMapTile(6, tx1, ty1, tileidx+3);
}
#endif

void
gfx_setabsoverlay(int tx, int ty, int tileno)
{
    // Correct for ability to write to -1 column
    tx++;

    UT_ASSERT(tx >= 0);
    UT_ASSERT(tx < 17);
    UT_ASSERT(ty >= 0);
    UT_ASSERT(ty < 12);

    // Ignore this if we are not in a valid mode.
    if (glb_gfxmode)
	return;

    int		tx1, ty1, tileidx, abstile;
    
    // Update the abs map...
    abstile = glb_absshadowmap[ty * 17 + tx];
    if (abstile == tileno)
	return;

    gfx_unlockTile((TILE_NAMES)abstile);
    tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
    glb_absshadowmap[ty * 17 + tx] = tileno;

    tx += glb_offx - 7 - 1;	// Extra -1 is for writing to -1 column
    ty += glb_offy - 5;

    tx *= 2;
    ty *= 2;
    ty += 1;		// Correct for the off by one nature of glb_offx

    tx1 = tx + 1;
    ty1 = ty + 1;
    tx &= 63;
    ty &= 63;
    tx1 &= 63;
    ty1 &= 63;
    
    ham_SetMapTile(3, tx, ty, tileidx);
    ham_SetMapTile(3, tx1, ty, tileidx+1);
    ham_SetMapTile(3, tx, ty1, tileidx+2);
    ham_SetMapTile(3, tx1, ty1, tileidx+3);
}

void
gfx_setmoblayer(int tx, int ty, int tileno)
{
    int		hamx, hamy, tileidx;

    hamx = tx - glb_scrollx;
    hamy = ty - glb_scrolly;
    // Only send onto ham if on screen and is a real change.
    if (glb_mobmap[ty * 32 + tx] != tileno)
	// && hamx < 32 && hamy < 32)
    {
	gfx_unlockTile((TILE_NAMES)glb_mobmap[ty * 32 + tx]);
	tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
	
	ham_SetMapTile(2, tx*2, ty*2, tileidx);
	ham_SetMapTile(2, tx*2+1, ty*2, tileidx+1);
	ham_SetMapTile(2, tx*2, ty*2+1, tileidx+2);
	ham_SetMapTile(2, tx*2+1, ty*2+1, tileidx+3);

	// UPdate our local map.
	glb_mobmap[ty * 32 + tx] = tileno;
    }
}

void
gfx_setabsmob(int tx, int ty, int tileno)
{
    // Handle ability to write to -1 column
    tx++;

    UT_ASSERT(tx >= 0);
    UT_ASSERT(tx < 17);
    UT_ASSERT(ty >= 0);
    UT_ASSERT(ty < 12);

    int		tx1, ty1, tileidx, abstile;

    // Update the abs map...
    abstile = glb_absmobmap[ty * 17 + tx];
    if (abstile == tileno)
	return;

    gfx_unlockTile((TILE_NAMES)abstile);
    tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
    glb_absmobmap[ty * 17 + tx] = tileno;
    
    // Extra -1 is for our offset.
    tx += glb_offx - 7 - 1;
    ty += glb_offy - 5;

    tx *= 2;
    ty *= 2;
    ty += 1;		// Correct for the off by one nature of glb_offx

    tx1 = tx + 1;
    ty1 = ty + 1;
    tx &= 63;
    ty &= 63;
    tx1 &= 63;
    ty1 &= 63;

    ham_SetMapTile(2, tx, ty, tileidx);
    ham_SetMapTile(2, tx1, ty, tileidx+1);
    ham_SetMapTile(2, tx, ty1, tileidx+2);
    ham_SetMapTile(2, tx1, ty1, tileidx+3);
}

#ifdef _3DS
void
gfx_setabsmob2(int tx, int ty, int tileno)
{
    // Handle ability to write to -1 column
    tx++;

    UT_ASSERT(tx >= 0);
    UT_ASSERT(tx < 17);
    UT_ASSERT(ty >= 0);
    UT_ASSERT(ty < 12);

    int		tx1, ty1, tileidx, abstile;

    // Update the abs map...
    abstile = glb_absmobmap2[ty * 17 + tx];
    if (abstile == tileno)
	return;

    gfx_unlockTile((TILE_NAMES)abstile);
    tileidx = gfx_lockTile((TILE_NAMES)tileno, (TILE_NAMES *)&tileno);
    glb_absmobmap2[ty * 17 + tx] = tileno;
/*    
    // Extra -1 is for our offset.
    tx += glb_offx - 7 - 1;
    ty += glb_offy - 5;
*/
    tx *= 2;
    ty *= 2;
    ty += 2;		
    tx -= 1;		

    tx1 = tx + 1;
    ty1 = ty + 1;
    tx &= 63;
    ty &= 63;
    tx1 &= 63;
    ty1 &= 63;

    ham_SetMapTile(5, tx, ty, tileidx);
    ham_SetMapTile(5, tx1, ty, tileidx+1);
    ham_SetMapTile(5, tx, ty1, tileidx+2);
    ham_SetMapTile(5, tx1, ty1, tileidx+3);
}
#endif

int
gfx_getmoblayer(int tx, int ty)
{
    return glb_mobmap[ty * 32 + tx];
}

void
gfx_printtext(int x, int y, const char *text)
{
    // We silently clip at 30.
    while (x < 30 && *text)
    {
	gfx_printchar(x++, y, *text++);
    }
}

void
gfx_printcharraw(int x, int y, char c)
{
    int		 idx, offx, offy;
    u16 	*screen = hamfake_lockScreen();
    u8	 	*chardata;
    u16		*palette;
    u16		*backplate;

    idx = gfx_getchartile(c);

    glb_charmap[(y/TILEHEIGHT)*32 + (x/TILEWIDTH)] = c;

    chardata = (u8 *) (&glb_tilesets[glb_tileset].alphabet[glb_currentfont][idx * BYTEPERTILE]);
    palette = (u16 *) glb_tilesets[glb_tileset].palette;
    backplate = (u16 *) bmp_slug_and_blood;

    // Find offset for x & y.
    // We have to update the backplate location as we were given
    // GBA coords and backplate is in DS coords.
#if defined(USING_DS) || defined(USING_SDL)
    // This is not tile specific as we don't scale our plate (yet)
    backplate += 8 + 16 * PLATE_WIDTH;
#endif
#ifdef _3DS
	x+=8;
#endif


    int backx = x;
    int	backoff = 0;

    x += y * RAW_SCREEN_WIDTH;

    screen += x;

    for (offy = 0; offy < TILEHEIGHT; offy++)
    {
	backoff = rawtoplate_y(y+offy) * PLATE_WIDTH;

	for (offx = 0; offx < TILEWIDTH; offx++)
	{
	    if (*chardata)
	    {
		*screen = palette[*chardata];
	    }
	    else // Transparent.
		*screen = backplate[backoff + rawtoplate_x(backx+offx)];

	    screen++;
	    chardata++;
	}
	screen += RAW_SCREEN_WIDTH - TILEWIDTH;
    }

    hamfake_unlockScreen(screen);
}

void
gfx_printcolourcharraw(int x, int y, char c, COLOUR_NAMES colour)
{
    int		 idx, offx, offy;
    u16 	*screen = hamfake_lockScreen();
    u8	 	*chardata;
    u16		*palette;
    u16		*backplate;
    u16		 charcolour, desiredcolour;

    desiredcolour  = (glb_colourdefs[colour].blue >> 3) << 10;
    desiredcolour |= (glb_colourdefs[colour].green >> 3) << 5;
    desiredcolour |= (glb_colourdefs[colour].red >> 3);

    idx = gfx_getchartile(c);
    glb_charmap[(y/TILEHEIGHT)*32 + (x/TILEWIDTH)] = c;

    chardata = (u8 *) (&glb_tilesets[glb_tileset].alphabet[glb_currentfont][idx * BYTEPERTILE]);
    palette = (u16 *) glb_tilesets[glb_tileset].palette;
    backplate = (u16 *) bmp_slug_and_blood;

    // Find offset for x & y.
    // We have to update the backplate location as we were given
    // GBA coords and backplate is in DS coords.
#if defined(USING_DS) || defined(USING_SDL)
    backplate += 8 + 16 * PLATE_WIDTH;
#endif
#ifdef _3DS
	x+=8;
#endif

    int backx = x;
    int	backoff = 0;

    x += y * RAW_SCREEN_WIDTH;

    screen += x;

    for (offy = 0; offy < TILEHEIGHT; offy++)
    {
	backoff = rawtoplate_y(y+offy) * PLATE_WIDTH;

	for (offx = 0; offx < TILEWIDTH; offx++)
	{
	    if (*chardata)
	    {
		// If the palette is black, we pass it through.
		// Otherwise we use our desired colour.
		charcolour = palette[*chardata];
		if (!charcolour || charcolour == 0x842)
		    *screen = charcolour;
		else
		    *screen = desiredcolour;
	    }
	    else
		*screen = backplate[backoff + rawtoplate_x(backx+offx)];

	    screen++;
	    chardata++;
	}
	screen += RAW_SCREEN_WIDTH - TILEWIDTH;
    }

    hamfake_unlockScreen(screen);
}

void
gfx_printchar(int x, int y, char c)
{
    int		idx;

    if (glb_gfxmode == 3)
    {
	gfx_printcharraw(x * TILEWIDTH, y * TILEHEIGHT, c);
	return;
    }

    // If there is no change, don't do anything...
    if (glb_charmap[y * 32 + x] == c)
	return;

    // Locking error:
    // 1) Attempt lock on 'a'.  Fail and get notile
    // 2) Update charmap with 'a'
    // 3) Attempt lock on 'a', succeed.
    // 4) Unlock this charmap and free the tile despite extra count.
    // Thus, we request the actual number to record in our charmap
    // that is set to INVALID if lock fails.
    // Unlock the old tile...
    gfx_unlockCharTile(glb_charmap[y*32 + x]);

    idx = gfx_lockCharTile(c, (u8 *) &c);
    glb_charmap[y*32 + x] = c;

    ham_SetMapTile(0, x, y, idx);
}

void
gfx_printcolourchar(int x, int y, char c, COLOUR_NAMES colour)
{
    int		idx;

    if (glb_gfxmode == 3)
    {
	gfx_printcolourcharraw(x * TILEWIDTH, y * TILEHEIGHT, c, colour);
	return;
    }

    // There is no such thing as no change in the coloured char world.

    // Locking error:
    // 1) Attempt lock on 'a'.  Fail and get notile
    // 2) Update charmap with 'a'
    // 3) Attempt lock on 'a', succeed.
    // 4) Unlock this charmap and free the tile despite extra count.
    // Thus, we request the actual number to record in our charmap
    // that is set to INVALID if lock fails.
    // Unlock the old tile...
    gfx_unlockCharTile(glb_charmap[y*32 + x]);

    idx = gfx_lockColourCharTile(c, colour, (u8 *) &c);
    glb_charmap[y*32 + x] = c;

    ham_SetMapTile(0, x, y, idx);
}

void
gfx_cleartextline(int y, int startx)
{
    int		x;

    for (x = startx; x < 30; x++)
	gfx_printchar(x, y, ' ');
}

void
gfx_nudgecenter(int px, int py)
{
    int		tx, ty;
    
    glb_nudgex = px;
    glb_nudgey = py;
    
    gfx_getscrollcenter(tx, ty);
    gfx_scrollcenter(tx, ty);
}

void
gfx_getscrollcenter(int &tx, int &ty)
{ 
    tx = glb_offx;
    ty = glb_offy;
}


void gfx_scrollcenter(int tx, int ty)
{
//    int		dorebuild = 0;

    // Clamp to the size of the map...

#if 0
    if (tx - 8 < 0)
	tx = 8;
    if (tx + 8 >= MAP_WIDTH)
	tx = MAP_WIDTH - 8;
    if (ty - 5 < 0)
	ty = 5;
    if (ty + 5 >= MAP_HEIGHT)
	ty = MAP_HEIGHT - 5;
#endif
    
    // We want the specified tx/ty to become the center of our tile map.
    // The problem is if it doesn't fit on our current screen, we must
    // scroll & rebuild.
    // Our screen width is 15 tiles and height 10 tiles.
    
    // This code allows us to have a real screen larger than 32x32
    // by scrolling the area.  Sadly, my scroll code sucks and costs
    // way too much to do, so results in horrible flickering etc,
    // thus I decided to go back to 32x32.
#if 0
    if (tx + 8 - glb_scrollx > 16)
    {
	glb_scrollx += 16;
	dorebuild = 1;
    }
    if (tx - 8 < glb_scrollx)
    {
	glb_scrollx -= 16;
	dorebuild = 1;
    }
    if (ty + 5 - glb_scrolly > 16)
    {
	glb_scrolly += 16;
	dorebuild = 1;
    }
    if (ty - 5 < glb_scrolly)
    {
	glb_scrolly -= 16;
	dorebuild = 1;
    }
    if (dorebuild)
    {
	gfx_refreshtiles();
    }
#endif
    // If we are running on the DS we need slightly different magic constants.
    // Specifically, we have 256 rather than 240 wide, so need an extra
    // 8 offset horizontally.  Likewise, we have 192 rather than 160 vertically
    // so need an extra 16 vertically.  Ideally we will eventually use this
    // extra space.
#ifdef iPOWDER
    M_BG1SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH - TILEWIDTH + glb_nudgex);
    M_BG1SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT - 3*TILEHEIGHT + glb_nudgey);
    M_BG2SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH - TILEWIDTH + glb_nudgex);
    M_BG2SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT - 3*TILEHEIGHT + glb_nudgey);
    M_BG3SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH - TILEWIDTH + glb_nudgex);
    M_BG3SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT - 3*TILEHEIGHT + glb_nudgey);
    // We also need to set the text port
    M_BG0SCRLX_SET(-TILEWIDTH);
    M_BG0SCRLY_SET(-TILEHEIGHT*3);
#elif defined(USING_DS) || defined(USING_SDL) || defined(_3DS)
    M_BG1SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH - TILEWIDTH + glb_nudgex);
    M_BG1SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT - 2*TILEHEIGHT + glb_nudgey);
    M_BG2SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH - TILEWIDTH + glb_nudgex);
    M_BG2SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT - 2*TILEHEIGHT + glb_nudgey);
    M_BG3SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH - TILEWIDTH + glb_nudgex);
    M_BG3SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT - 2*TILEHEIGHT + glb_nudgey);
    // We also need to set the text port
    M_BG0SCRLX_SET(-TILEWIDTH);
    M_BG0SCRLY_SET(-TILEHEIGHT*2);
#else
    M_BG1SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH + glb_nudgex);
    M_BG1SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT + glb_nudgey);
    M_BG2SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH + glb_nudgex);
    M_BG2SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT + glb_nudgey);
    M_BG3SCRLX_SET((tx-glb_scrollx)*TILEWIDTH*2 - 14*TILEWIDTH + glb_nudgex);
    M_BG3SCRLY_SET((ty-glb_scrolly)*TILEHEIGHT*2 - 9*TILEHEIGHT + glb_nudgey);
    // No need to adjust the textport in raw GBA mode.
#endif

    glb_offx = tx;
    glb_offy = ty;
}

int
gfx_isnewframe()
{
    int result;

    hamfake_rebuildScreen();
//    hamfake_awaitEvent();
    // The event could be a new frame...
    result = glb_newframe; 
    glb_newframe = 0;
    return result;
}

int
gfx_getframecount()
{
    return glb_framecount;
}


u16 glb_tilestashtiles[TILESTASH];
char glb_tilestashdata[TILESTASH][4];
int glb_tilestashsize = 0;

void
gfx_updatebitmap()
{
    int		i;
    
    // Write out bg2tiledata to the bg2tiles.
    //ham_ReloadTileGfx(glb_bg2tiles, (u16 *)glb_bg2tiledata, 0, 600);
    //ham_ReloadTileGfx(glb_bg2tiles, (u16 *)glb_bg2tiledata, 0, glb_tilestashsize);

    for (i = 0; i < glb_tilestashsize; i++)
    {
	ham_ReloadTileGfx(glb_bg1tiles, 
	      &((u16 *) glb_bg2tiledata)[WORDPERTILE * i], 
	      glb_tilestashtiles[i], 1);
    }
}

void
gfx_putpixel(char c, int x, int y)
{
    int		tx, ty;

    UT_ASSERT(x >= 0 && y >= 0 && x < 240 && y < 160);

    tx = x >> 3;
    ty = y >> 3;
    x = x & 7;
    y = y & 7;
    
    glb_bg2tiledata[tx * BYTEPERTILE + ty * BYTEPERTILE * 30 + y * TILEWIDTH + x] = c;
}

void
gfx_drawverline(char c, int x, int ly, int hy)
{
    while (ly <= hy)
	gfx_putpixel(c, x, ly++);
}

void
gfx_drawhorline(char c, int lx, int y, int hx)
{
    while (lx <= hx)
	gfx_putpixel(c, lx++, y);
}

void
gfx_drawbox(char c, int lx, int ly, int hx, int hy, int solid)
{
    if (solid)
    {
	while (ly <= hy)
	    gfx_drawhorline(c, lx, ly++, hx);
    }
    else
    {
	gfx_drawhorline(c, lx, ly, hx);
	gfx_drawhorline(c, lx, hy, hx);
	gfx_drawverline(c, lx, ly+1, hy-1);
	gfx_drawverline(c, hx, ly+1, hy-1);
    }
}

// Given a SQUARE number give us the minimap colour.
int
gfx_getcolfromtile(int tile, int mapped)
{
    COLOUR_NAMES	c;

    if (mapped)
    {
	c = (COLOUR_NAMES) glb_squaredefs[tile].minicolour;
    }
    else
    {
	c = COLOUR_LIGHTBLACK;
    }
    return gfx_lookupcolor(c);
}

static void
gfx_resettilestash()
{
    int		i;

    for (i = 0; i < glb_tilestashsize; i++)
    {
	// Mark it as freed...
	glb_idxcount[glb_tilestashtiles[i]] = 0;
    }
    glb_tilestashsize = 0;
}

#ifdef USING_SDL
void
gfx_fillminimapblock(char *dst, char c[4])
{
    int			 sy, sx;
    for (sy = 0; sy < TILEHEIGHT/2; sy++)
    {
	for (sx = 0; sx < TILEWIDTH/2; sx++)
	    *dst++ = c[0];
	for (; sx < TILEWIDTH; sx++)
	    *dst++ = c[1];
    }
    
    for (; sy < TILEHEIGHT; sy++)
    {
	for (sx = 0; sx < TILEWIDTH/2; sx++)
	    *dst++ = c[2];
	for (; sx < TILEWIDTH; sx++)
	    *dst++ = c[3];
    }
}
#else
void
gfx_fillminimapblock(char *dst, char c[4])
{
    int			 sy;
    for (sy = 0; sy < 4; sy++)
    {
	*dst++ = c[0];
	*dst++ = c[0];
	*dst++ = c[0];
	*dst++ = c[0];
	*dst++ = c[1];
	*dst++ = c[1];
	*dst++ = c[1];
	*dst++ = c[1];
    }
    
    for (sy = 0; sy < 4; sy++)
    {
	*dst++ = c[2];
	*dst++ = c[2];
	*dst++ = c[2];
	*dst++ = c[2];
	*dst++ = c[3];
	*dst++ = c[3];
	*dst++ = c[3];
	*dst++ = c[3];
    }
}
#endif

int
gfx_findminimaptilenumber(char c[4])
{
    int		i;
    u16		tile;

    // Search the tile stash to see if we are already in it...
    for (i = 0; i < glb_tilestashsize; i++)
    {
	tile = glb_tilestashtiles[i];
	
	// We can flip horizontally, vertically, or both.
	if (c[0] == glb_tilestashdata[i][0])
	{
	    // No flip.
	    if (c[1] == glb_tilestashdata[i][1] &&
		c[2] == glb_tilestashdata[i][2] &&    
		c[3] == glb_tilestashdata[i][3])
	    {
		return tile;
	    }
	}
	if (c[0] == glb_tilestashdata[i][1])
	{
	    // Flip horizontally
	    if (c[1] == glb_tilestashdata[i][0] &&
		c[2] == glb_tilestashdata[i][3] &&    
		c[3] == glb_tilestashdata[i][2])
	    {
		return tile | 1024;
	    }
	}
	if (c[0] == glb_tilestashdata[i][2])
	{
	    // Flip vertically
	    if (c[1] == glb_tilestashdata[i][3] &&
		c[2] == glb_tilestashdata[i][0] &&    
		c[3] == glb_tilestashdata[i][1])
	    {
		return tile | 2048;
	    }
	}
	if (c[0] == glb_tilestashdata[i][3])
	{
	    // Flip both.	
	    if (c[1] == glb_tilestashdata[i][2] &&
		c[2] == glb_tilestashdata[i][1] &&    
		c[3] == glb_tilestashdata[i][0])
	    {
		return tile | 1024 | 2048;
	    }
	}
    }

    // We didn't find the tile.
    tile = gfx_findFreeTile(1);

    if (tile == INVALID_TILEIDX)
	return gfx_lookupTile(TILE_NOTILE);
    // Ignore for now as we know it occrus on Quizar's map.
    // Seems rather mean to keep this limit since we know we have
    // a hard limit with gfx_findFreeTile(1).
    // Why not allow stash to be bigger and just make sure 
    // we free the tile's afterwards?
    // UT_ASSERT(i < TILESTASH);
    if (i >= TILESTASH)
    {
	return gfx_lookupTile(TILE_NOTILE);
    }
    
    i = glb_tilestashsize++;

    glb_tilestashtiles[i] = tile;

    glb_idxcount[tile]++;

    // Build the tile.
    memcpy(glb_tilestashdata[i], c, 4);

    gfx_fillminimapblock(&glb_bg2tiledata[i * BYTEPERTILE], c);
    
    return tile;
}

// Fill in an 8x8 block given the colour array.  Colours are
// (0,0), (1,0), (0,1), (1,1).
void
gfx_drawminimapblock(int bx, int by, char c[4])
{
    char		*dst;

    dst = &glb_bg2tiledata[by * BYTEPERTILE * 30 + bx * BYTEPERTILE];
    gfx_fillminimapblock(dst, c);
}


void
gfx_displayminimap(MAP *level)
{
    gfx_resettilestash();
    
    // gfx_drawbox(glb_stdcolor[COLOUR_RED], 51, 11, 189, 149, 0);
    
    char	c[4];
    int		ax, ay;		// AVatar position.

    if (MOB::getAvatar())
    {
	ax = MOB::getAvatar()->getX();
	ay = MOB::getAvatar()->getY();
    }
    else
    {
	ax = -1;
	ay = -1;
    }

    // The map is 32x32.
    // With 4 pixel size elements, we have a 128x128 map.
    // We draw it from 56 to 184 and 16 to 144
    // We have a 10 pixel border on all sides.
    int		 x, y, bx, by, tile;

    // For efficiency, we write to one tile at a time.  This greatly
    // improves the efficiency...
    y = 0;
    for (by = 0; by < 16; by++)
    {
	x = 0;
	for (bx = 0; bx < 16; bx++)
	{
	    c[0] = gfx_getcolfromtile(level->getTile(x, y),
				  level->getFlag(x, y, SQUAREFLAG_MAPPED));
	    c[1] = gfx_getcolfromtile(level->getTile(x+1, y),
				  level->getFlag(x+1, y, SQUAREFLAG_MAPPED));
	    c[2] = gfx_getcolfromtile(level->getTile(x, y+1),
				  level->getFlag(x, y+1, SQUAREFLAG_MAPPED));
	    c[3] = gfx_getcolfromtile(level->getTile(x+1, y+1),
				  level->getFlag(x+1, y+1, SQUAREFLAG_MAPPED));

	    if ((ax >> 1) == bx && (ay >> 1) == by)
	    {
		c[(ax & 1) + ((ay &1) << 1)] = glb_stdcolor[COLOUR_WHITE];
	    }

	    tile = gfx_findminimaptilenumber(c);
	    ham_SetMapTile(0, bx+7, by+2, tile);
	    // gfx_drawminimapblock(bx + 7, by + 2, c);
	    
	    x += 2;
	}
	y += 2;
    }
    
#if 0
    for (item = level->getItemHead(); item; item = item->getNext())
    {
	gfx_drawbox(glb_stdcolor[COLOUR_GREEN],
		    56 + item->getX() * 4 + 1, 16 + item->getY() * 4 + 1,
		    56 + item->getX() * 4 + 2, 16 + item->getY() * 4 + 2,
		    1);
    }

    for (mob = level->getMobHead(); mob; mob = mob->getNext())
    {
	// Turn this #if off in order to get clarivoyance all the
	// time.
#if 1
	if (level->hasLOS(MOB::getAvatar()->getX(), MOB::getAvatar()->getY(),
			  mob->getX(), mob->getY()))
#endif
	{
	    gfx_drawbox(glb_stdcolor[COLOUR_WHITE],
			56 + mob->getX() * 4 + 1, 16 + mob->getY() * 4 + 1,
			56 + mob->getX() * 4 + 2, 16 + mob->getY() * 4 + 2,
			1);
	}
    }
#endif
    
    gfx_updatebitmap();

#if 0
{
    BUF			buf;
    buf.sprintf("%d tiles used", glb_tilestashsize);
    msg_report(buf);
}
#endif
}

void
gfx_hideminimap()
{
    // Free up tiles.
    gfx_resettilestash();

    // Erase the tile list.  
    int		 tile, bx, by;

    tile = gfx_lookupTile(TILE_VOID);

    for (by = 0; by < 16; by++)
    {
	// We corrupted the text line so just write spaces over it.
	gfx_cleartextline(by + 2);
	
	// The text clear may be a no-op if the tile was already a space
	// we thus explicitly set the tile to void
	for (bx = 0; bx < 16; bx++)
	    ham_SetMapTile(0, bx+7, by+2, tile);
    }
}

int
gfx_lookupcolor(int r, int g, int b)
{
    return gfx_lookupcolor(r, g, b, glb_tilesets[glb_tileset].palette);
}

int
gfx_lookupcolor(int r, int g, int b, const u16 *palette)
{
    int		i;
    int		minerror, closematch, cr, cg, cb;

    // Larger than any possible match.
    minerror = 255*255*3 + 1;
    closematch = 0;
    // We skip colour 0 as that is invisible.
    for (i = 1; i < (int)512 / 2; i++)
    {
	// Unpack the given colour from 5:5:5
	cb = (palette[i] >> 10) << 3;
	cg = ((palette[i] >> 5) & 31) << 3;
	cr = (palette[i] & 31) << 3;

	// Find the difference squared.
	cr -= r;
	cg -= g;
	cb -= b;
	cr *= cr;
	cg *= cg;
	cb *= cb;
	cr += cg;
	cr += cb;
	// cr is now the squared difference.  If this is a new low, keep.
	if (cr < minerror)
	{
	    minerror = cr;
	    closematch = i;
	}
    }
    UT_ASSERT(closematch);

    // Result is closematch.
    return closematch;
}

int
gfx_lookupcolor(COLOUR_NAMES color)
{
    UT_ASSERT(color < NUM_COLOURS);
    return glb_stdcolor[color];
}


void
gfx_sleep(int vcycles)
{
    if (glbStressTest)
	return;

    int		start = glb_framecount;

    while (glb_framecount < start + vcycles)
    {
	hamfake_rebuildScreen();
	hamfake_awaitEvent();
    }
}

void
gfx_updateinventoryline(MOB *mob)
{
    ITEM	*item;

    gfx_cleartextline(18);
    if (!mob)
    {
	gfx_printtext(0, 18, "No item slot");
	return;
    }
    item = mob->getItem(glb_invx, glb_invy);
    if (item)
	gfx_printtext(0, 18,
		      gram_capitalize(item->getName(false, false, true)));
    else if (glb_invx == 0)
    {
	ITEMSLOT_NAMES slot = (ITEMSLOT_NAMES) glb_invy;
	if (mob->hasSlot(slot))
	    gfx_printtext(0, 18,
		      gram_capitalize(mob->getSlotName(slot)));
	else
	    gfx_printtext(0, 18, "No item slot");
    }
    else
	gfx_printtext(0, 18, "Empty slot");
}

void
gfx_showinventory(MOB *mob)
{
    int		 tx, ty, ix, iy;
    ITEM	*item;
    BUF		 buf;

    glb_invmob = mob;

    // Clear out all the lines...
    for (ty = 2; ty < 19; ty++)
	gfx_cleartextline(ty);

    // Write out the descriptions...
    for (iy = 0; iy < NUM_ITEMSLOTS; iy++)
    {
	gfx_printtext(0, iy * 2 + 2, glb_itemslotdefs[iy].desc1);
	gfx_printtext(0, iy * 2 + 3, glb_itemslotdefs[iy].desc2);
    }

    // Width 12, height 8.
    // Our complete size 15 x 10
    for (iy = 0; iy < MOBINV_HEIGHT; iy++)
    {
	ty = iy + 1;
        for (ix = 0; ix < MOBINV_WIDTH; ix++)
	{
	    tx = ix + 2 + (ix!=0);

	    // Draw the inventory item with pass thru and clear out
	    // the overlay..
	    item = mob->getItem(ix, iy);
#ifndef _3DS
	    if (!item)
		gfx_setabstile(tx, ty, TILE_NORMALSLOT);
	    else if (item->isFavorite())
	    {
		gfx_setabstile(tx, ty, TILE_FAVORITESLOT);
	    }
	    else
	    {
		if (item->isKnownCursed())
		{
		    if (item->isBlessed())
			gfx_setabstile(tx, ty, TILE_HOLYSLOT);
		    else if (item->isCursed())
			gfx_setabstile(tx, ty, TILE_CURSEDSLOT);
		    else
			gfx_setabstile(tx, ty, TILE_NORMALSLOT);
		}
		else
		{
		    if (item->isKnownNotCursed())
			gfx_setabstile(tx, ty, TILE_NORMALSLOT);
		    else
			gfx_setabstile(tx, ty, TILE_EMPTYSLOT);
		}
	    }

	    if (!ix)
	    {
		if (!mob->hasSlot((ITEMSLOT_NAMES) iy))
		{
		    gfx_setabsmob(tx, ty, TILE_INVALIDSLOT);
		}
		else
		    gfx_setabsmob(tx, ty, 
				    (item ? item->getTile() : TILE_VOID));
	    }
	    else
		gfx_setabsmob(tx, ty, 
				    (item ? item->getTile() : TILE_VOID));

#else
	    if (!item)
		gfx_setabstile2(tx, ty, TILE_NORMALSLOT);
	    else if (item->isFavorite())
	    {
		gfx_setabstile2(tx, ty, TILE_FAVORITESLOT);
	    }
	    else
	    {
		if (item->isKnownCursed())
		{
		    if (item->isBlessed())
			gfx_setabstile2(tx, ty, TILE_HOLYSLOT);
		    else if (item->isCursed())
			gfx_setabstile2(tx, ty, TILE_CURSEDSLOT);
		    else
			gfx_setabstile2(tx, ty, TILE_NORMALSLOT);
		}
		else
		{
		    if (item->isKnownNotCursed())
			gfx_setabstile2(tx, ty, TILE_NORMALSLOT);
		    else
			gfx_setabstile2(tx, ty, TILE_EMPTYSLOT);
		}
	    }

	    if (!ix)
	    {
		if (!mob->hasSlot((ITEMSLOT_NAMES) iy))
		{
		    gfx_setabsmob2(tx, ty, TILE_INVALIDSLOT);
		}
		else
		    gfx_setabsmob2(tx, ty, 
				    (item ? item->getTile() : TILE_VOID));
	    }
	    else
		gfx_setabsmob2(tx, ty, 
				    (item ? item->getTile() : TILE_VOID));

#endif	    

	    if (ix == glb_invx && iy == glb_invy)
#ifdef _3DS
		gfx_setabsoverlay2(tx, ty, TILE_CURSOR);
#else
		gfx_setabsoverlay(tx, ty, TILE_CURSOR);
#endif
	    else
#ifdef _3DS
		gfx_setabsoverlay2(tx, ty, TILE_VOID);
#else
		gfx_setabsoverlay(tx, ty, TILE_VOID);
#endif

	    // If the item is quivered, add the q flag.
	    if (item && item->isQuivered())
	    {
		gfx_printtext(tx * 2 + 1, ty * 2, "q");
	    }
	    // Flag artifacts.
	    if (item && item->isArtifact())
	    {
		gfx_printtext(tx * 2, ty * 2, SYMBOLSTRING_UNIQUE);
	    }
	    // If the stack count is non-unit, we add the number.
	    if (item && item->getStackCount() != 1)
	    {
		buf.sprintf("%2d", item->getStackCount());
		
		gfx_printtext(tx * 2, ty * 2 + 1, buf);
	    }
	}
    }

    // Update the current inventory item...
    gfx_updateinventoryline(mob);
}

void
gfx_hideinventory()
{
    int		ty;

    // Clear out all the lines...
    for (ty = 2; ty < 19; ty++)
	gfx_cleartextline(ty);
    
    gfx_refreshtiles();
    glb_invmob = 0;
}

void
gfx_getinvcursor(int &ix, int &iy)
{
    ix = glb_invx;
    iy = glb_invy;
}

void
gfx_setinvcursor(int ix, int iy, bool onlyslots)
{
    int		 tx, ty;
    ITEM	*item;
    
    // Erase from old pos...
    tx = glb_invx + 2 + (glb_invx != 0);
    ty = glb_invy + 1;

    // but only if inventory currently up
    if (glb_invmob)
#ifdef _3DS
		gfx_setabsoverlay2(tx, ty, TILE_VOID);
#else
		gfx_setabsoverlay(tx, ty, TILE_VOID);
#endif

    // Scroll the cursor as you hit the top of the inventory
    // If we are on the equipment column, we always stay on the
    // equipment column.
    // If we are in the inventory area, we loop only in there.
    // Horizontal movement never adjusts y value.
    if (iy < 0)
    {
	if (ix)
	{
	    ix--;
	    if (!ix)
		ix = MOBINV_WIDTH-1;
	}
	iy = MOBINV_HEIGHT-1;
    }
    else if (iy >= MOBINV_HEIGHT)
    {
	if (ix)
	{
	    ix++;
	    if (ix == MOBINV_WIDTH)
		ix = 1;
	}
	iy = 0;
    }
    else if (ix < 0)
    {
	ix = MOBINV_WIDTH-1;
    }
    else if (ix >= MOBINV_WIDTH)
    {
	ix = 0;
    }

    // Ensure we never leave the itemslot list...
    if (onlyslots)
	ix = 0;
    
    // ix/iy might still be out of range, so it makes sense
    // to reclamp here.
    
    // Update new position...
    glb_invx = ix % MOBINV_WIDTH;
    if (glb_invx < 0) glb_invx += MOBINV_WIDTH;
    glb_invy = iy % MOBINV_HEIGHT;
    if (glb_invy < 0) glb_invy += MOBINV_HEIGHT;
    tx = glb_invx + 2 + (glb_invx != 0);
    ty = glb_invy + 1;

    // Rest of this is only needed if inventory is displayed.
    if (!glb_invmob)
	return;

#ifdef _3DS
	gfx_setabsoverlay2(tx, ty, TILE_CURSOR);
#else
	gfx_setabsoverlay(tx, ty, TILE_CURSOR);
#endif
    
    // Update the current inventory item...
    gfx_updateinventoryline(glb_invmob);
}

void
gfx_displaylist(int x, int y, int ylen, const char **list, int select, int &startoflist, int &endoflist)
{
    int		i, j, n, displaylen;
    int		scroll;
    const char	*entry = "You should stop reading the binary!";

	// Calculate list size.
    for (n = 0; list[n]; n++);

    // Determine the scroll offset required for select to be centered.
    displaylen = ylen-y;
    if (n < displaylen)
	scroll = 0;		// No scrolling necessary.
    else
    {
	scroll = select - displaylen/2 + 1;
	// Do not scroll down past end.
	if (scroll + displaylen > n)
	{
	    scroll = n - displaylen+1;
	}
	// Do not scroll up past start.
	// Note that a scroll of 1 merely hides the first entry, so
	// isn't at all useful.
	if (scroll <= 1)
	    scroll = 0;
    }
    
    // First, chew up the scrolled entries.
    for (i = 0; i < scroll; i++)
    {
	entry = list[i];
	if (!entry) break;
    }

    startoflist = scroll;
    
    // Now, display & clear the rest of the text.
    j = y;
    if (scroll)
    {
	// Print the scroll up message
	gfx_cleartextline(j, x-1);
	gfx_printtext(x, j, SYMBOLSTRING_UP SYMBOLSTRING_UP SYMBOLSTRING_UP);

	j++;
    }
    
    for (; j < ylen; j++)
    {
	gfx_cleartextline(j, x-1);
	if (entry)
	{
	    entry = list[i];
	    if (entry)
	    {
		// Last line may be scroll down.  This is true so long as
		// there is extra entries.
		if (list[i+1] && j == (ylen-1))
		{
		    gfx_printtext(x, j, 
			SYMBOLSTRING_DOWN SYMBOLSTRING_DOWN SYMBOLSTRING_DOWN);
		}
		else
		{
		    gfx_printtext(x, j, entry);
		    endoflist = i;
		}

		// Highlight current selection.
		if (i == select)
		{
		    gfx_printtext(x-1, j, SYMBOLSTRING_RIGHT);
		    gfx_printtext(x+strlen(entry), j, SYMBOLSTRING_LEFT);
		}
	    }
	    i++;
	}
    }
}

// Evil!
void writeGlobalActionBar(bool useemptyslot);

// Returns selected menu entry...
// x & y is where top left of menu will be.  Default is menu[0].
// -1 means the selection was cancelled.
int
gfx_selectmenu(int x, int y, const char **menu, int &aorb, int def,
		bool anykey, bool disablepaging,
		const u8 *menuactions,
		u8 *actionstrip)
{
    int		 i, num, select, dx, dy;
    const char	*entry;
    aorb = -1;
    num = 0;
    for (i = 0; (entry = menu[i]); i++)
    {
	num++;
    }
    // If there are no items to pick, cancel immediately.
    if (!num)
	return -1;
    
    // Now, we can treat menu as a 0..num-1 array.
    select = def;
    if (select >= num)
	select = 0;		// Illegal default given.
    if (select < 0)
	select = 0;		// Less than zero is also illegal.

    int		startoflist = 0, endoflist = 0;
    STYLUSLOCK	styluslock((!menuactions) ? REGION_MENU
			    : (REGION_MENU | REGION_BOTTOMBUTTON | REGION_SIDEBUTTON));

    // If we are in drag mode, we need to free up the borders so they
    // can be selected.
    if (menuactions)
	styluslock.setRange(x, 0, 29, 20);
    else
	styluslock.setRange(x, -1, -1, -1);

    // Display the menu.
    gfx_displaylist(x, y, 18, menu, select, startoflist, endoflist);

    // Wait for all buttons to go up to start...
    // We only check for buttons that we care about.  We don't want to
    // abort key repeat on arrow keys in case someone cancels a popup
    // window.
#ifndef HAS_KEYBOARD
    while ( (ctrl_rawpressed(BUTTON_A) || ctrl_rawpressed(BUTTON_B))
	    && !hamfake_forceQuit())
    {
		if (!gfx_isnewframe())
		{
			continue;
		}
    }
#endif
    // Wipe out all of our hit states.
    hamfake_clearKeyboardBuffer();
    if (menuactions) writeGlobalActionBar(true);

    while (1)
    {
		if (!gfx_isnewframe())
			continue;
		if (hamfake_forceQuit())
			return -1;

#ifdef HAS_KEYBOARD
		int		key;
		key = hamfake_getKeyPress(false);

		dx = 0;
		dy = 0;
		switch (key)
		{
			case GFX_KEYF1:
			case GFX_KEYF2:
			case GFX_KEYF3:
			case GFX_KEYF4:
			case GFX_KEYF5:
			case GFX_KEYF6:
			case GFX_KEYF7:
			case GFX_KEYF8:
			case GFX_KEYF9:
			case GFX_KEYF10:
			case GFX_KEYF11:
			case GFX_KEYF12:
			case GFX_KEYF13:
			case GFX_KEYF14:
			case GFX_KEYF15:
			if (menuactions)
			{
				// Bind the first 15 menu actions to our key
				int		index = key - GFX_KEYF1;

				actionstrip[index] = menuactions[select];

				// Rebuild the list.
				writeGlobalActionBar(true);
			}
			break;
			case 'k':
			case GFX_KEYUP:
			case '8':
			dy = -1;
			break;
			case 'j':
			case GFX_KEYDOWN:
			case '2':
			dy = 1;
			break;
			case GFX_KEYPGUP:
			dy = MAX(-10, -select);
			break;
			case GFX_KEYPGDOWN:
			dy = MIN(10, num - select - 1);
			break;
			case 'l':
			case 'h':
			case GFX_KEYLEFT:
			case GFX_KEYRIGHT:
			case '4':
			case '6':
			dy = 0;
			break;

			// Press a.
			case ' ':
			case '\n':
			case '\r':
			case '5':
			aorb = 0;
			if (anykey)
				aorb = BUTTON_A;
			if (menuactions) writeGlobalActionBar(false);
			return select;

			// Press b.
			case '0':
			case '\x1b':
			aorb = 1;
			if (anykey)
				aorb = BUTTON_B;
			if (menuactions) writeGlobalActionBar(false);
			return select;

			// Request for more information, a select key!
			case 'i':
			case '?':
			if (anykey)
			{
				aorb = BUTTON_SELECT;
				if (menuactions) writeGlobalActionBar(false);
				return select;
			}

			default:
			dx = 0;
			dy = 0;
			break;
		}
	#else
		dx = dy = 0;

		if (ctrl_hit(BUTTON_START))
		{
			// Cancel
			if (menuactions) writeGlobalActionBar(false);
			return -1;
		}

		if (ctrl_hit(BUTTON_A))
		{
			// Accept....
			aorb = 0;
			if (anykey)
			aorb = BUTTON_A;
			if (menuactions) writeGlobalActionBar(false);
			return select;
		}
		if (ctrl_hit(BUTTON_B))
		{
			aorb = 1;
			if (anykey)
			aorb = BUTTON_B;
			if (menuactions) writeGlobalActionBar(false);
			return select;
		}
		if (anykey)
		{
			if (ctrl_hit(BUTTON_SELECT))
			{
			aorb = BUTTON_SELECT;
			if (menuactions) writeGlobalActionBar(false);
			return select;
			}
			if (ctrl_hit(BUTTON_X))
			{
			aorb = BUTTON_X;
			if (menuactions) writeGlobalActionBar(false);
			return select;
			}
			if (ctrl_hit(BUTTON_Y))
			{
			aorb = BUTTON_Y;
			if (menuactions) writeGlobalActionBar(false);
			return select;
			}
		}
		if (anykey && disablepaging)
		{
			if (ctrl_hit(BUTTON_L))
			{
			aorb = BUTTON_L;
			if (menuactions) writeGlobalActionBar(false);
			return select;
			}
			if (ctrl_hit(BUTTON_R))
			{
			aorb = BUTTON_R;
			if (menuactions) writeGlobalActionBar(false);
			return select;
			}
		}
		else
		{
			// We clamp page up/down to the limits
			if (ctrl_hit(BUTTON_L))
			dy = MAX(-10, -select);
			if (ctrl_hit(BUTTON_R))
			dy = MIN(10, num - select - 1);
		}


		// Change our position if dy not zero.
		// Only test arrow keys if r or l not pressed
		if (!dx && !dy)
			ctrl_getdir(dx, dy);
	#endif

		// Process any drag requests.
		if (menuactions)
		{
			if (styluslock.performDrags(y*TILEHEIGHT, startoflist, endoflist, menuactions, actionstrip))
			{
			// Update...
			if (menuactions) writeGlobalActionBar(true);
			}
		}

		int		styluschoice;
		bool		inbounds = false;
		if (styluslock.selectmenu(styluschoice, x*TILEWIDTH, y*TILEHEIGHT, inbounds))
		{
			if (!inbounds)
			{
			// User clicked outside of the menu, consider
			// a cancel.
			aorb = 1;
			if (anykey)
				aorb = BUTTON_B;
			if (menuactions) writeGlobalActionBar(false);
			return select;
			}
			else
			{
			// A real selection if this was valid.
			// Check for the case of up arrows.
			if (startoflist)
				styluschoice--;
			if (styluschoice < 0)
			{
				// Scroll up.
				dy = MAX(-10, -select);
			}
			else if (startoflist + styluschoice > endoflist)
			{
				// Scroll down.
				dy = MIN(10, num - select - 1);
			}
			else
			{
				aorb = 0;
				if (anykey)
				aorb = BUTTON_A;
				if (menuactions) writeGlobalActionBar(false);
				return startoflist + styluschoice;
			}
			}
		}

		if (!dx && !dy)
		{
			// Nothing to do.
			continue;
		}
		else if (dx && !dy)
		{
			// Fast paging with left/right for cases where L/R
			// are already overload.
			if (ctrl_hit(BUTTON_L))
			dy = MAX(-10, -select);
			if (ctrl_hit(BUTTON_R))
			dy = MIN(10, num - select - 1);
		}

		if (dy)
		{
			// Adjust selection...
			select += dy;

			// Note: On the libdns system this triggers a compiler bug
			// of some sort with -O2.  Basically, if dy < 0 we take the
			// following branch as if select == dy.
			if (select < 0)
			select = num-1;
			if (select >= num)		// Not else so zero length lists yield 0
			select = 0;

			gfx_displaylist(x, y, 18, menu, select, startoflist, endoflist);
		}
    }

    if (menuactions) writeGlobalActionBar(false);

    return -1;
}

bool
gfx_yesnomenu(const char *prompt, bool defchoice)
{
    int		aorb;
    int		choice, width;
    // Prompt...
    const char *menu[4] =
    {
	"Yes",
	"No",
	0
    };

    width = strlen(prompt);

    // Build an overlay for our question...
    {
	// Erase all tiles.
	int		sx, sy;
	TILE_NAMES	tile;
	for (sy = 2; sy < 6; sy++)
	{
	    if (sy == 2)
		tile = TILE_SCROLL_TOP;
	    else if (sy == 5)
		tile = TILE_SCROLL_BOTTOM;
	    else
		tile = TILE_SCROLL_BACK;
	    for (sx = 2; sx < 2 + (width+1)/2; sx++)
	    {
		// We don't want to write past the end
		// even for a large prompt.
		if (sx >= 16)
		    break;
#ifdef _3DS
		gfx_setabsoverlay2(sx, sy, tile);
#else
		gfx_setabsoverlay(sx, sy, tile);
#endif
	    }
	}
    }

    // Write out the query.
    // This really should be made a horizontal
    // yes/no box!
    gfx_printtext(4, 6, prompt);

    choice = gfx_selectmenu(8, 8, menu, aorb, !defchoice);
    if (aorb)
	choice = -1;
    if (choice >= 1)
	choice = -1;

    // Clear
    {
	int		y;
	for (y = 3; y < 19; y++)
	    gfx_cleartextline(y);
    }
    // This is because of our overlay options...
    gfx_refreshtiles();

    if (choice == 0)
	return true;
    return false;
}

void
gfx_displayinfoblock(const char *text, bool prevline)
{
    int		x, y;

    gfx_cleartextline(3);
    gfx_cleartextline(18);
    if (prevline)
    {
	gfx_printchar(0, 3, SYMBOL_UP);
	gfx_printchar(14, 3, SYMBOL_UP);
	gfx_printchar(29, 3, SYMBOL_UP);
    }

    for (y = 4; y < 18; y++)
    {
	for (x = 0; x < 30; x++)
	{
	    if (!*text)
	    {
		gfx_printchar(x, y, ' ');
	    }
	    else
	    {
		gfx_printchar(x, y, *text);
		text++;
	    }
	}
    }

    // If there is more text (which there should be if we get here)
    // prompt the fact.
    if (*text)
    {
	gfx_printchar(0, 18, SYMBOL_DOWN);
	gfx_printchar(14, 18, SYMBOL_DOWN);
	gfx_printchar(29, 18, SYMBOL_DOWN);
    }
}

void
gfx_displayinfotext(const char *text)
{
    const char		*curtext;
    int			 dx, dy, y;

    STYLUSLOCK		 styluslock(REGION_DISPLAYTEXT);

    gfx_displayinfoblock(text, false);

    curtext = text;
    while (1)
    {
	if (!gfx_isnewframe())
	    continue;

	if (hamfake_forceQuit())
	    return;

#ifdef HAS_KEYBOARD
	int		key;
	bool		stop = false;

	key = hamfake_getKeyPress(false);

	dy = 0;
	switch (key)
	{
	    case 'k':
	    case GFX_KEYUP:
	    case '8':
		dy = -1;
		break;
	    case 'j':
	    case GFX_KEYDOWN:
	    case '2':
		dy = 1;
		break;
	    case GFX_KEYPGUP:
		dy = -10;
		break;
	    case GFX_KEYPGDOWN:
		dy = 10;
		break;
	    case 'l':
	    case 'h':
	    case '4':
	    case '6':
	    case GFX_KEYLEFT:
	    case GFX_KEYRIGHT:
		dy = 0;
		break;
	    case ' ':
	    case '5':
	    case '0':
	    case '\r':
	    case '\x1b':
		stop = true;
		break;
	    default:
		dy = 0;
		break;
	}

	if (stop)
	    break;
#else
	if (ctrl_hit(BUTTON_A))
	    break;
	if (ctrl_hit(BUTTON_B))
	    break;
	
	if (ctrl_hit(BUTTON_R))
	{
	    dy = 10;
	}
	else if (ctrl_hit(BUTTON_L))
	{
	    dy = -10;
	}
	else
	{
	    ctrl_getdir(dx, dy);
	}
	hamfake_clearKeyboardBuffer();
#endif

	if (styluslock.getdisplaytext(dy))
	{
	    dy = dy;
	    if (!dy)
		break;
	}

	if (!dy)
	    continue;

	while (dy)
	{
	    if (dy < 0)
	    {
		dy++;
		// User wants to scroll up.
		if (curtext != text)
		{
		    curtext -= 30;
		}
		else
		    dy = 0;
	    }
	    if (dy > 0)
	    {
		dy--;
		// User wants scroll down.
		if (strlen(curtext) >= 30 * (18 - 4))
		{
		    curtext += 30;
		}
		else
		    dy = 0;
	    }
	}
	gfx_displayinfoblock(curtext, curtext != text);
    }

    for (y = 3; y < 19; y++)
	gfx_cleartextline(y);
}

char **glb_pager = 0;
int    glb_pager_curline = -1, glb_pager_size = 0;
int	glb_pager_width = 30;

void
gfx_pager_setwidth(int width)
{
    glb_pager_width = width;
}

void
gfx_pager_addtext(BUF buf)
{
    gfx_pager_addtext(buf.buffer());
}

void
gfx_pager_addtext(const char *text)
{
    // Make sure we have a line...
    if (!glb_pager)
    {
	glb_pager_curline = -1;
	glb_pager_size = 0;
	gfx_pager_newline();
    }

    const char		*src;
    char		*dst, *origdst;
    int			 dstlen, dstmaxlen;

    // Point to the next write location...
    src = text;
    origdst = dst = glb_pager[glb_pager_curline];
    dstmaxlen = glb_pager_width;
    dstlen = strlen(dst);
    dst += dstlen;

    while (*src && dstlen < dstmaxlen)
    {
	*dst++ = *src++;
	dstlen++;
    }

    // If we ran out of source material, done.
    if (!*src)
    {
	*dst++ = '\0';
	return;
    }

    // We have to wrap :<  If src is a space, wrapping is trivial.
    // Otherwise, we backup src until it is a space...
    while (!isspace(*src) && src > text)
    {
	src--;
	dst--;
	dstlen--;
    }

    // Now, move forward until we ate all the spaces (this seems odd
    // except if *src was a space going into the last loop)
    while (*src && isspace(*src))
    {
	src++;
    }
    
    // It could be trailing spaces sent us over the bounds.  We will
    // thus eat these and ignore them.
    if (!*src)
    {
	*dst++ = '\0';
	return;
    }
    
    // Avoid throwing this on the stack as we recurse and GBA sucks.
    char		 *prefix = new char[30+1];
    prefix[0] = '\0';

    // Special case:  If src == str now, yet our dstlen == 0, we have a
    // single word over SCREEN_WIDTH.  We resolve this by dumping
    // dstmaxlen - 1 chars, a hyphen, and then continuing as if we
    // found a space.
    if (src == text && !dstlen)
    {
	while (*src && dstlen < dstmaxlen-1)
	{
	    *dst++ = *src++;
	    dstlen++;
	}
	// Append a hyphen...
	*dst++ = '-';
    }
    // Special case: If src == str, dstlen != 0, we are tacking onto
    // the end of an existing word.  If that word ends with a space,
    // great, we wrap as expected.  However, we often build up
    // a string with a series of writes, such as "foo" followed by ", bar".
    // We want to wrap the last word on dst.  If dst doesn't have a sub
    // word, obviously we avoid this.
    // Of course, this isn't the full story.  It could be that the
    // previous line got to exactly 30 columns.  Any spaces would be 
    // silently eaten so it would look like there are no spaces.
    // To solve this, we look for special non-breaking characters that
    // we feel should be enjoined.  Specifically, a punctuation on the
    // LHS means we should break, and a punctuation on the RHS means
    // we should not.
    else if (src == text)
    {
	char		rhs, lhs;
	rhs = src[0];
	lhs = origdst[dstlen-1];
	// Check if LHS is breaking, in which case we always break
	// so ignore this code path.
	if (!isspace(lhs) && lhs != '.' && lhs != ',' && lhs != '!' && lhs != '?')
	{
	    // Find the start of the word in dst...
	    int		wordstart = dstlen - 1;
	    while (!isspace(origdst[wordstart]) && wordstart > 0)
	    {
		wordstart--;
	    }

	    // If wordstart is now zero, our previuos entry was the
	    // full length.  We might hyphenate here, but I think it
	    // is simpler just to let the break lie where we broke
	    // it before.
	    if (wordstart)
	    {
		// Since wordstart points to a space, we can safely
		// mark wordstart as null & strcpy out wordstart+1
		strcpy(prefix, &origdst[wordstart+1]);
		origdst[wordstart] = '\0';
	    }
	}
    }

    // Wrap to the next line.  Null terminate dst and build
    // a new line.
    *dst++ = '\0';
    gfx_pager_newline();

    // And, because we are too lazy to make this a loop, recurse.
    gfx_pager_addtext(prefix);
    delete [] prefix;
    gfx_pager_addtext(src);
}

void
gfx_pager_addsingleline(const char *text)
{
    // Make sure we have a line...
    if (!glb_pager)
    {
	glb_pager_curline = -1;
	glb_pager_size = 0;
	gfx_pager_newline();
    }

    const char		*src;
    char		*dst;
    int			 dstlen, dstmaxlen;

    // Point to the next write location...
    src = text;
    dst = glb_pager[glb_pager_curline];
    dstmaxlen = glb_pager_width;
    dstlen = strlen(dst);

    if (dstlen > 0)
	gfx_pager_newline();

    dst = glb_pager[glb_pager_curline];
    dstmaxlen = glb_pager_width;
    dstlen = strlen(dst);
    
    dst += dstlen;

    while (*src && dstlen < dstmaxlen)
    {
	*dst++ = *src++;
	dstlen++;
    }

    // Even if we didn't run out of source, we truncate and return.
    *dst++ = '\0';

    // Ensure next line starts fresh.
    gfx_pager_newline();
}

void
gfx_pager_newline()
{
    // Move to the next line.
    glb_pager_curline++;

    // Check for over flow.
    if (glb_pager_curline >= glb_pager_size)
    {
	// Reallocate.
	char		**newpager;

	newpager = new char *[glb_pager_size + 32];

	if (glb_pager)
	    memcpy(newpager, glb_pager, sizeof(char *) * glb_pager_size);

	glb_pager_size += 32;
	delete [] glb_pager;
	glb_pager = newpager;
    }

    // And ensure the new buffer is allocated.
    glb_pager[glb_pager_curline] = new char[glb_pager_width+2];
    glb_pager[glb_pager_curline][0] = '\0';
}

void
gfx_pager_separator()
{
    // This is so long so 80 column print outs get a full separator.
    gfx_pager_addsingleline("-------------------------------------------------------------------------------");
}

void
gfx_pager_display_block(int y)
{
    int		 sx, sy;
    const char	*text;

    gfx_cleartextline(3);
    gfx_cleartextline(18);
    if (y)
    {
	gfx_printchar(0, 3, SYMBOL_UP);
	gfx_printchar(14, 3, SYMBOL_UP);
	gfx_printchar(29, 3, SYMBOL_UP);
    }

    for (sy = 4; sy < 18; sy++, y++)
    {
	if (y > glb_pager_curline)
	    gfx_cleartextline(sy);
	else
	{
	    text = glb_pager[y];
	    for (sx = 0; sx < 30; sx++)
	    {
		
		if (!*text)
		{
		    gfx_printchar(sx, sy, ' ');
		}
		else
		{
		    gfx_printchar(sx, sy, *text);
		    text++;
		}
	    }
	}
    }

    // If we still have rows left, display down arrows.
    if (y < glb_pager_curline)
    {
	gfx_printchar(0, 18, SYMBOL_DOWN);
	gfx_printchar(14, 18, SYMBOL_DOWN);
	gfx_printchar(29, 18, SYMBOL_DOWN);
    }
}

void
gfx_pager_savetofile(const char *name)
{
#ifdef HAS_DISKIO
    if (!hamfake_fatvalid())
    {
	gfx_pager_reset();
	return;
    }

#if defined (iPOWDER)
    if (hamfake_cansendemail())
    {
	for (int y = 3; y < 19; y++)
	    gfx_cleartextline(y);
	if (gfx_yesnomenu("Email Game Summary?", false))
	{
	    // Send an email.
	    BUF		body;
	    for (int i = 0; i <= glb_pager_curline; i++)
	    {
		body.strcat(glb_pager[i]);
		body.strcat("\n\r");
	    }

	    hamfake_sendemail("", name, body.buffer());
	}
    }
    gfx_pager_reset();
    return;
#endif
 
    FILE		*fp;

    fp = hamfake_fopen(name, "wt");
    if (!fp)
    {
	printf("OPEN %s FAILED!\n", name);
	return;
    }

    int			i;
    for (i = 0; i <= glb_pager_curline; i++)
    {
#if defined(LINUX) || defined(WIN32)
	fprintf(fp, "%s\n", glb_pager[i]);
#else
	// For embedded systems always use line feeds so
	// the windows users can parse it easier.
	// Linux users are more used to crappy line feeds.
	fprintf(fp, "%s\r\n", glb_pager[i]);
#endif
    }

    fclose(fp);
#endif
    gfx_pager_reset();
}

void hideSideActionBar();

void
gfx_pager_display(int y)
{
    if (!glb_pager) return;
    
    int			 dx, dy;
    int			 tile;

    // Make y the center.
    y -= (18 - 4) / 2;

    if (y >= glb_pager_curline - (18 - 4))
	y = glb_pager_curline - (18 - 4);

    // Hide the side buttons.
    hideSideActionBar();

    // Clip the requested y position.
    if (y < 0)
	y = 0;
    
    // Erase all tiles.
    int		sx, sy;
    for (sy = 1; sy < 10; sy++)
    {
	if (sy == 1)
	    tile = TILE_SCROLL_TOP;
	else if (sy == 9)
	    tile = TILE_SCROLL_BOTTOM;
	else
	    tile = TILE_SCROLL_BACK;
	for (sx = -1; sx < 16; sx++)
	{
#ifdef _3DS
		gfx_setabsoverlay2(sx, sy, tile);
#else
	    gfx_setabsoverlay(sx, sy, tile);
#endif
	}
    }

    gfx_pager_display_block(y);

#ifndef HAS_KEYBOARD
    // Wait for no button to be hit.
    while (ctrl_anyrawpressed() && !hamfake_forceQuit())
    {
	hamfake_awaitEvent();
    }
#endif
    
    // Clear the repeat queue of SDL
    hamfake_clearKeyboardBuffer();

    STYLUSLOCK		 styluslock(REGION_DISPLAYTEXT);

    while (1)
    {
	if (!gfx_isnewframe())
	    continue;

	if (hamfake_forceQuit())
	    return;

	// Quit immediately in stress test
	if (glbStressTest)
	    break;

#ifdef HAS_KEYBOARD
	int		key;
	bool		stop = false;

	key = hamfake_getKeyPress(false);

	dy = 0;
	switch (key)
	{
	    case 'k':
	    case GFX_KEYUP:
	    case '8':
		dy = -1;
		break;
	    case 'j':
	    case GFX_KEYDOWN:
	    case '2':
		dy = 1;
		break;
	    case GFX_KEYPGUP:
		dy = -10;
		break;
	    case GFX_KEYPGDOWN:
		dy = 10;
		break;
	    case 'l':
	    case 'h':
	    case GFX_KEYLEFT:
	    case GFX_KEYRIGHT:
	    case '4':
	    case '6':
		dy = 0;
		break;
	    case ' ':
	    case '\n':
	    case '\r':
	    case '5':
	    case '0':
	    case '\x1b':
		stop = true;
		break;
	    default:
		dy = 0;
		break;
	}

	if (stop)
	    break;
#else
	if (ctrl_hit(BUTTON_A))
	    break;
	if (ctrl_hit(BUTTON_B))
	    break;
	
	if (ctrl_hit(BUTTON_R))
	{
	    dy = 10;
	}
	else if (ctrl_hit(BUTTON_L))
	{
	    dy = -10;
	}
	else
	{
	    ctrl_getdir(dx, dy);
	}
	hamfake_clearKeyboardBuffer();
#endif

	if (styluslock.getdisplaytext(dy))
	{
	    dy = dy;
	    if (!dy)
		break;
	}

	if (dy < 0)
	{
	    // User wants to scroll up.
	    if (y)
	    {
		y += dy;
		if (y < 0)
		    y = 0;
		gfx_pager_display_block(y);
	    }
	}
	if (dy > 0)
	{
	    if (y < glb_pager_curline - (18 - 4))
	    {
		y += dy;
		if (y > glb_pager_curline - (18 - 4))
		{
		    y = glb_pager_curline - (18 - 4);
		}
		gfx_pager_display_block(y);
	    }
	}
    }

    for (y = 3; y < 19; y++)
	gfx_cleartextline(y);

    gfx_pager_reset();

    // Because we did absolute sets, this will restore.
    gfx_refreshtiles();

    // Clear the repeat queue of SDL
    hamfake_clearKeyboardBuffer();
}

void
gfx_pager_reset()
{
    int		y;

    // Clear out our pager.
    for (y = 0; y <= glb_pager_curline; y++)
    {
	delete [] glb_pager[y];
    }
    delete [] glb_pager;
    glb_pager = 0;
    glb_pager_size = 0;
    glb_pager_curline = -1;
}

bool
gfx_selecttile(int &tx, int &ty, bool quickselect, int *quickold, int *aorb,
		bool *stylus)
{
    int		dx, dy;
    int		oldoverlay;
    bool	cancel;

    STYLUSLOCK	styluslock(REGION_MAPTILES);

    if (stylus)
	*stylus = false;

    // If we are quickselecting, our old overlay may be the cursor
    // itself - we require the caller to maintain its value.
    oldoverlay = gfx_getoverlay(tx, ty);
    if (quickselect)
	oldoverlay = quickold ? *quickold : TILE_VOID;
    if (aorb)
	*aorb = 0;
    gfx_setoverlay(tx, ty, TILE_CURSOR);
    gfx_scrollcenter(tx, ty);
    while (1)
    {
	if (!gfx_isnewframe())
	    continue;

	if (hamfake_forceQuit())
	    return false;
	
	if (ctrl_hit(BUTTON_A))
	{
	    // Accept....
	    gfx_scrollcenter(MOB::getAvatar()->getX(), MOB::getAvatar()->getY());
	    gfx_setoverlay(tx, ty, oldoverlay);
	    if (aorb)
		*aorb = 0;
	    hamfake_clearKeyboardBuffer();
	    return true;
	}
	if (ctrl_hit(BUTTON_B))
	{
	    // Cancel
	    gfx_scrollcenter(MOB::getAvatar()->getX(), MOB::getAvatar()->getY());
	    gfx_setoverlay(tx, ty, oldoverlay);
	    if (aorb)
		*aorb = 1;
	    hamfake_clearKeyboardBuffer();
	    if (quickselect)
		return true;
	    else
		return false;
	}

	if (styluslock.getmaptile(dx, dy, cancel))
	{
	    // User pressed on a tile successfully.
	    gfx_scrollcenter(MOB::getAvatar()->getX(), MOB::getAvatar()->getY());
	    gfx_setoverlay(tx, ty, oldoverlay);

	    // Set the new location
	    tx = dx;
	    ty = dy;

	    // Determine the overlay value of the new tile that we
	    // are moving the curosr to.
	    oldoverlay = gfx_getoverlay(tx, ty);
	    if (quickselect && quickold)
		*quickold = oldoverlay;

	    if (aorb)
		*aorb = cancel;
	    if (stylus)
		*stylus = true;
	    hamfake_clearKeyboardBuffer();
	    return !cancel;
	}

	ctrl_getdir(dx, dy);
	if (!dx && !dy)
	{
	    // Consume any key in case an invalid key was hit
	    hamfake_getKeyPress(false);
	    continue;
	}

	// Erase old...
	gfx_setoverlay(tx, ty, oldoverlay);
	tx += dx;
	ty += dy;
	tx &= MAP_WIDTH - 1;
	ty &= MAP_HEIGHT - 1;
	oldoverlay = gfx_getoverlay(tx, ty);
	if (quickselect && quickold)
	    *quickold = oldoverlay;
	gfx_setoverlay(tx, ty, TILE_CURSOR);
	gfx_scrollcenter(tx, ty);

	if (quickselect)
	    return false;
    }
}

void writeYesNoBar();

bool
gfx_selectinventory(int &selectx, int &selecty)
{
    bool	madechoice = false;
    int		button;
    int		dx, dy;
    bool	apress, bpress;

    STYLUSLOCK	styluslock(REGION_BOTTOMBUTTON);

    writeYesNoBar();

    gfx_setinvcursor(selectx, selecty, false);
    while (1)
    {
	if (!gfx_isnewframe())
	    continue;

	if (hamfake_forceQuit())
	    return false;

	ctrl_getdir(dx, dy);
	apress = ctrl_hit(BUTTON_A);
	bpress = ctrl_hit(BUTTON_B);

	if (styluslock.getbottombutton(button))
	{
	    // Either a cancel or accept.
	    if (button == 18)
		apress = true;
	    if (button == 26)
		bpress = true;
	}

	if (bpress)
	{
	    // Cancel
	    madechoice = false;
	    break;
	}
	if (apress)
	{
	    madechoice = true;
	    break;
	}

	if (stylus_queryinventoryitem(selectx, selecty))
	{
	    gfx_setinvcursor(selectx, selecty, false);
	    continue;
	}

	// Consume invalid keys.
	hamfake_getKeyPress(false);

	// We refetch this as this will deal
	// silently with wrap.
	gfx_getinvcursor(selectx, selecty);
	selectx += dx;
	selecty += dy;
	gfx_setinvcursor(selectx, selecty, false);
    }

    return madechoice;
}

bool
gfx_selectdirection(int otx, int oty, int &rdx, int &rdy, int &rdz)
{
    int		dx, dy, ndx, ndy, tx, ty;
    int		oldoverlay;
    bool	chose = false;

    STYLUSLOCK	styluslock(REGION_TENDIR);

    hamfake_flushdir();
    hamfake_buttonreq(5, 1);

    rdx = rdy = rdz = 0;
    tx = otx;
    ty = oty;
    oldoverlay = gfx_getoverlay(tx, ty);
    gfx_setoverlay(tx, ty, TILE_CURSOR);

    dx = dy = 0;
    while (1)
    {
	if (!gfx_isnewframe())
	    continue;
	
	if (hamfake_forceQuit())
	    return false;

	if (ctrl_hit(BUTTON_A))
	{
	    // Accept....
	    gfx_setoverlay(tx, ty, oldoverlay);
	    chose = true;
	    break;
	}
	if (ctrl_hit(BUTTON_B))
	{
	    // Cancel
	    gfx_setoverlay(tx, ty, oldoverlay);
	    chose = false;
	    break;
	}

	ctrl_getdir(ndx, ndy);

	if (hamfake_externaldir(dx, dy))
	{
	    gfx_setoverlay(tx, ty, oldoverlay);
	    chose = true;
	    break;
	}

	if (styluslock.gettendir(rdx, rdy, rdz, chose))
	{
	    // Invert sense of choosing.
	    gfx_setoverlay(tx, ty, oldoverlay);
	    chose = !chose;
	    hamfake_buttonreq(5, 0);
	    return chose;
	}

	if (!ndx && !ndy)
	{
	    // Consume any key in case an invalid key was hit
	    hamfake_getKeyPress(false);
	    continue;
	}

	// Erase old...
	    gfx_setoverlay(tx, ty, oldoverlay);

	dx += ndx;
	dy += ndy;
	
	// Clamp to our funny shape...
	if (dy >= 2)
	{
	    dy = 2;
	    dx = 0;
	}
	if (dy <= -2)
	{
	    dy = -2;
	    dx = 0;
	}
	if (dx < -1)
	    dx = -1;
	if (dx > 1)
	    dx = 1;
	
	tx = otx + dx;
	ty = oty + dy;
	tx &= MAP_WIDTH - 1;
	ty &= MAP_HEIGHT - 1;
	
    oldoverlay = gfx_getoverlay(tx, ty);
    gfx_setoverlay(tx, ty, TILE_CURSOR);
    }

    // Parse out our dx,dy into rdx, rdy, rdz.
    
    if (dy == 2)
	rdz = -1;
    else if (dy == -2)
	rdz = 1;
    else
    {
	rdx = dx;
	rdy = dy;
    }

    hamfake_buttonreq(5, 0);
    return chose;
}

void
gfx_copytiledata(TILE_NAMES desttile, MINI_NAMES minitile, bool ismale)
{
    const u8		*tiledata;
    int			 i;
    u16			 tile;

    tile = gfx_lookupTile(desttile);

    if (tile == INVALID_TILEIDX)
	return;

    if (ismale)
	tiledata = &glb_tilesets[glb_tileset].mini[minitile * BYTEPERTILE*4];
    else
	tiledata = &glb_tilesets[glb_tileset].minif[minitile * BYTEPERTILE*4];

    if (!glb_comptiledata)
    {
	// Allocate a u16 to guarantee alignment.
	u16		*origdata = new u16 [WORDPERTILE*4];
	
	glb_comptiledata = (u8 *) origdata;
    }
    for (i = 0; i < BYTEPERTILE*4; i++)
	glb_comptiledata[i] = tiledata[i];
    
    // HMm...  I think ReloadTIleGfx needs aligned data, which is not
    // guaranteed with a static array.  No idea how the minitiles
    // work around this...
    glb_lastdesttile = tile;
    ham_ReloadTileGfx(glb_bg1tiles, (u16 *) glb_comptiledata, glb_lastdesttile, 4);
}

u8 *
gfx_extractsprite(int tileset, SPRITE_NAMES tile)
{
    int		w, h, x, y, srcidx;
    u8		*data, *dataout;
    u16		 c;

    w = glb_tilesets[tileset].tilewidth;
    h = glb_tilesets[tileset].tilewidth;

    data = (u8 *) malloc(4 * w * h * 3);
    dataout = data;

    for (y = 0; y < h * 2; y++)
    {
	for (x = 0; x < w * 2; x++)
	{
	    srcidx = 0;
	    srcidx += y * w;
	    srcidx += x;
	    if (x >= w)
		srcidx += w * h - w;
	    if (y >= h)
		srcidx += w * h * 2 - w * h;
	    c = glb_tilesets[tileset].spritepalette[glb_tilesets[tileset].sprite[tile * w * h * 4 + srcidx]];

	    int		cb, cg, cr;

	    cb = (c >> 10) << 3;
	    cg = ((c >> 5) & 31) << 3;
	    cr = (c & 31) << 3;
	    *dataout++ = cr;
	    *dataout++ = cg;
	    *dataout++ = cb;
	}
    }

    return data;
}

u8 *
gfx_extractlargesprite(SPRITE_NAMES tile, int grey, SPRITE_NAMES overlay)
{
#ifdef iPOWDER
    int		w, h, x, y, srcidx;
    u8		*data, *dataout;
    u16		 c;

    w = 48;
    h = 48;

    grey = 256 - grey;
    int		 cutoff = (grey * h + 128) / 256;

    data = (u8 *) malloc(w * h * 4);
    dataout = data;

    int		row, col;

    if (tile != SPRITE_INVALID)
    {
	row = tile / 24;
	col = tile - row*24;

	for (y = 0; y < h; y++)
	{
	    for (x = 0; x < w; x++)
	    {
		srcidx = col * w + row * w * h * 24;
		srcidx += y * w * 24;
		srcidx += x;

		c = bmp_sprite16_3x[srcidx];

		int		cb, cg, cr;

		cb = (c >> 10) << 3;
		cg = ((c >> 5) & 31) << 3;
		cr = (c & 31) << 3;

		if (y < cutoff)
		{
		    cr = rgbtogrey(cr, cg, cb);
		    cg = cb = cr;
		}

		*dataout++ = cr;
		*dataout++ = cg;
		*dataout++ = cb;
		if (c)
		    *dataout++ = 255;
		else
		    *dataout++ = 0;
	    }
	}
    }
    else
    {
	memset(data, 0, w * h * 4);
    }

    if (overlay != SPRITE_INVALID)
    {
	row = overlay / 24;
	col = overlay - row*24;

	dataout = data;

	for (y = 0; y < h; y++)
	{
	    for (x = 0; x < w; x++)
	    {
		srcidx = col * w + row * w * h * 24;
		srcidx += y * w * 24;
		srcidx += x;

		c = bmp_sprite16_3x[srcidx];
		if (!c)
		{
		    dataout += 4;
		}
		else
		{
		    int		cb, cg, cr;

		    cb = (c >> 10) << 3;
		    cg = ((c >> 5) & 31) << 3;
		    cr = (c & 31) << 3;

		    *dataout++ = cr;
		    *dataout++ = cg;
		    *dataout++ = cb;
		    *dataout++ = 255;
		}
	    }
	}
    }

    return data;
#else
    return 0;
#endif
}

void
gfx_compositetile(ITEMSLOT_NAMES dstslot, MINI_NAMES minitile, bool ismale)
{
    const u8		*minidata;
    int			 x, y, sx, sy;
    int			 dstidx, srcidx;

    // The use of the suffix "f" to denote the female tileset is
    // derogratory: it implies all humans are male until they are explicitly
    // labeled FEmale.  To atone for this, the parameter to determine gender
    // has the opposite sense: ismale implies it is the men that must be
    // explicitly specified.
    if (ismale)
	minidata = &glb_tilesets[glb_tileset].mini[minitile * 4 * BYTEPERTILE];
    else
	minidata = &glb_tilesets[glb_tileset].minif[minitile * 4 * BYTEPERTILE];

    // Determine our offset.
    int			 offx, offy;
    bool		 flipx, flipy;
    offx = glb_itemslotdefs[glb_minidefs[minitile].slot].posx;
    offx -= glb_itemslotdefs[dstslot].posx;
    offy = glb_itemslotdefs[glb_minidefs[minitile].slot].posy;
    offy -= glb_itemslotdefs[dstslot].posy;

    // Convert our offset into our new tilespace...
    if (TILEWIDTH != 8)
    {
	offx = (int) ((offx / 8.0) * TILEWIDTH + 0.5);
    }
    if (TILEHEIGHT != 8)
    {   
	offy = (int) ((offy / 8.0) * TILEHEIGHT + 0.5);
    }

    flipx = glb_itemslotdefs[glb_minidefs[minitile].slot].flipx ^
	    glb_itemslotdefs[dstslot].flipx;
    flipy = glb_itemslotdefs[glb_minidefs[minitile].slot].flipy ^
	    glb_itemslotdefs[dstslot].flipy;

    for (y = 0; y < TILEHEIGHT*2; y++)
    {
	for (x = 0; x < TILEWIDTH*2; x++)
	{
	    if (flipx)
		sx = TILEWIDTH*2-1 - x;
	    else
		sx = x;
	    if (flipy)
		sy = TILEHEIGHT*2-1 - y;
	    else
		sy = y;
	    sx += offx;
	    sy += offy;
	    if (sx < 0 || sx >= TILEWIDTH*2)
		continue;
	    if (sy < 0 || sy >= TILEHEIGHT*2)
		continue;

	    // Get position inside sub tile.
	    dstidx = 0;
	    dstidx += y * TILEWIDTH;
	    dstidx += x;
	    // Find actual sub tile.
	    if (x >= TILEWIDTH)
		dstidx += BYTEPERTILE - TILEWIDTH;
	    if (y >= TILEHEIGHT)
		dstidx += BYTEPERTILE*2 - BYTEPERTILE;

	    srcidx = 0;
	    srcidx += sy * TILEWIDTH;
	    srcidx += sx;
	    if (sx >= TILEWIDTH)
		srcidx += BYTEPERTILE - TILEWIDTH;
	    if (sy >= TILEHEIGHT)
		srcidx += BYTEPERTILE*2 - BYTEPERTILE;

	    // Actual comp:
	    if (minidata[srcidx])
		glb_comptiledata[dstidx] = minidata[srcidx];
	}
    }
    
    ham_ReloadTileGfx(glb_bg1tiles, (u16 *) glb_comptiledata, 
		      glb_lastdesttile, 4);
}

int
gfx_getfont()
{
    return glb_currentfont;
}

void
gfx_switchfonts(int newfont)
{
    if (newfont == glb_currentfont)
	return;

    glb_currentfont = newfont;
    
    // Do nothing in mode 3 or -1.
    if (glb_gfxmode)
	return;

    // Reload all locked font tiles..
    int		c;
    int		sourceidx;
    int		idx;

    // Active alphabet tiles:
    for (c = 0; c < 255; c++)
    {
	idx = glb_charidx[c];
	if (idx != INVALID_TILEIDX)
	{
	    sourceidx = gfx_getchartile(c);

	    ham_ReloadTileGfx(glb_bg1tiles,
		(u16*)(&glb_tilesets[glb_tileset].alphabet[glb_currentfont][sourceidx * BYTEPERTILE]),
		idx,
		1);
	}
    }
}

void
gfx_switchtilesets(int newtileset)
{
    int			i;

    if (newtileset == glb_tileset)
	return;

    glb_tileset = newtileset;
#ifdef iPOWDER
    glb_modetilesets[glb_tilesetmode] = newtileset;
#endif

    // TODO: Guard if sizes match,
    hamfake_setTileSize(glb_tilesets[glb_tileset].tilewidth,
			glb_tilesets[glb_tileset].tilewidth);

    // Rebuild our arrays:
    delete [] glb_bg2tiledata;
    glb_bg2tiledata = (char *) new u16[WORDPERTILE*TILESTASH];
    memset(glb_bg2tiledata, 0, BYTEPERTILE*TILESTASH);

    delete [] glb_comptiledata;
    glb_comptiledata = (u8 *) (new u16[WORDPERTILE*4]);

    for (i = 0; i < MAX_COLOUR_CHAR; i++)
    {
	if (glb_colourchardata[i])
	{
	    delete [] glb_colourchardata[i];
	    glb_colourchardata[i] = (u8 *) new u16[WORDPERTILE];
	}
    }
    
    delete [] glb_greyscaletiledata;
    glb_greyscaletiledata = (u8 *) (new u16[WORDPERTILE*4]);

    // If we are in mode 3, reblit our background!
    if (glb_gfxmode == 3)
    {
	gfx_reblitslugandblood();
	for (int y = 0; y < 20; y++)
	{
	    for (int x = 0; x < 32; x++)
	    {
		if (glb_charmap[y*32+x] != 0xff)
		    gfx_printcharraw(x*TILEWIDTH, y*TILEHEIGHT, glb_charmap[y*32+x]);
	    }
	}
    }

    // Do nothing in mode 3 or -1.
    if (glb_gfxmode)
	return;

    // Swap palette:
    ham_LoadBGPal((void *)glb_tilesets[glb_tileset].palette, 512);
    hamfake_LoadSpritePal((void *)glb_tilesets[glb_tileset].spritepalette, 512);

    // Rebuild our standard colours
    gfx_buildstdcolors();

    // Reload the sprite.
    hamfake_ReloadSpriteGfx((u16*)(&glb_tilesets[glb_tileset].dungeon[glb_sprite*4*BYTEPERTILE]),
		    0, 4);

    // Reload all locked tiles..
    int		c;
    int		sourceidx;
    int		idx;

    // Active alphabet tiles:
    for (c = 0; c < 255; c++)
    {
	idx = glb_charidx[c];
	if (idx != INVALID_TILEIDX)
	{
	    sourceidx = gfx_getchartile(c);

	    ham_ReloadTileGfx(glb_bg1tiles,
		(u16*)(&glb_tilesets[glb_tileset].alphabet[glb_currentfont][sourceidx * BYTEPERTILE]),
		idx,
		1);
	}
    }

    // Active dungeon tiles:
    int		tile;

    for (tile = 0; tile < NUM_TILES; tile++)
    {
	idx = glb_tileidx[tile];

	if (idx == INVALID_TILEIDX)
	    continue;

	ham_ReloadTileGfx(glb_bg1tiles, 
		(u16*)(&glb_tilesets[glb_tileset].dungeon[tile * 4 * BYTEPERTILE]),
		idx,
		4);
    }

    // Rebuild custom dude.
    if (MOB::getAvatar())
	MOB::getAvatar()->rebuildAppearance();

    // Reset our center as we may have switched to a different sized
    // tile and the lower layer stores the offset in pixels
    int		tx, ty;
    gfx_getscrollcenter(tx, ty);
    gfx_scrollcenter(tx, ty);
}

int
gfx_gettileset()
{
    return glb_tileset;
}

int
gfx_gettilesetmode(int mode)
{
#ifdef iPOWDER
    return glb_modetilesets[mode];
#else
    return glb_tileset;
#endif
}

void
gfx_tilesetmode(int mode)
{
#ifdef iPOWDER
    if (mode == glb_tilesetmode)
	return;

    glb_tilesetmode = mode;
    gfx_switchtilesets(glb_modetilesets[glb_tilesetmode]);
#endif
}

void
gfx_settilesetmode(int mode, int tileset)
{
#ifdef iPOWDER
    glb_modetilesets[mode] = tileset;
    if (mode == glb_tilesetmode)
	gfx_switchtilesets(tileset);
#else
    if (!mode)
	gfx_switchtilesets(tileset);
#endif
}

int
gfx_gettilewidth()
{
    return TILEWIDTH;
}

int
gfx_gettileheight()
{
    return TILEHEIGHT;
}

void
gfx_drawcursor(int x, int y)
{
    hamfake_movesprite(0, x-TILEWIDTH, y-TILEHEIGHT);
    hamfake_enablesprite(0, true);
}

void
gfx_removecursor()
{
    hamfake_enablesprite(0, false);
}

void
gfx_cursortile(SPRITE_NAMES tile)
{
    UT_ASSERT(glb_usesprites);
    glb_sprite = (u16) tile;
    hamfake_ReloadSpriteGfx((u16*)(&glb_tilesets[glb_tileset].sprite[glb_sprite*4*BYTEPERTILE]),
		    0, 4);
}

void
gfx_spritetile(int spriteno, SPRITE_NAMES tile)
{
    UT_ASSERT(glb_usesprites);

    hamfake_ReloadSpriteGfx((u16*)(&glb_tilesets[glb_tileset].sprite[tile*4*BYTEPERTILE]),
		    spriteno*4, 4);
}


u8
gfx_togreyscale(u8 src, const u16 *palette)
{
    int		cb, cg, cr;

    cb = (palette[src] >> 10) << 3;
    cg = ((palette[src] >> 5) & 31) << 3;
    cr = (palette[src] & 31) << 3;

    int grey = rgbtogrey(cr, cg, cb);
    return gfx_lookupcolor(grey, grey, grey, palette);
}

void
gfx_updateGreyTables(u8 *greytable, const u16 *palette)
{
    int		i;

    for (i = 0; i < 256; i++)
    {
	greytable[i] = gfx_togreyscale(i, palette);
    }
}

void
gfx_updatespritegreytable()
{
    if (!glb_spritegreyscale)
	return;

    gfx_updateGreyTables(glb_spritegreyscale, 
			glb_tilesets[glb_tileset].spritepalette);
}

const u8 *
gfx_getspritegreytable()
{
    if (!glb_spritegreyscale)
    {
	glb_spritegreyscale = new u8[256];
	gfx_updatespritegreytable();
    }

    return glb_spritegreyscale;
}



void
gfx_spritetile_grey(int spriteno, SPRITE_NAMES tile, int grey, SPRITE_NAMES overlay)
{
    const u8 *greytable;

    if (!glb_greyscaletiledata)
    {
	u16		*origdata = new u16 [WORDPERTILE*4];
	glb_greyscaletiledata = (u8 *) origdata;
    }

    greytable = gfx_getspritegreytable();

    u8 		*src;
    grey = 256 - grey;
    int		 cutoff = (grey * TILEHEIGHT * 2 + 128) / 256;
    int		 i;
    int		 x, y, t, yc;

    src = (u8 *) (&glb_tilesets[glb_tileset].sprite[tile*4*BYTEPERTILE]);

    i = 0;
    for (t = 0; t < 4; t++)
    {
	if (t < 2)
	    y = 0;
	else
	    y = TILEHEIGHT;

	for (yc = 0; yc < TILEHEIGHT; yc++)
	{
	    if (y + yc >= cutoff)
	    {
		for (x = 0; x < TILEWIDTH; x++)
		{
		    glb_greyscaletiledata[i] = src[i];
		    i++;
		}
	    }
	    else
	    {
		for (x = 0; x < TILEWIDTH; x++)
		{
		    glb_greyscaletiledata[i] = greytable[src[i]];
		    i++;
		}
	    }
	}
    }

    if (overlay != SPRITE_INVALID)
    {
	src = (u8 *) (&glb_tilesets[glb_tileset].sprite[overlay*4*BYTEPERTILE]);
	for (i = 0; i < BYTEPERTILE * 4; i++)
	{
	    if (src[i])
		glb_greyscaletiledata[i] = src[i];
	}
    }


    hamfake_ReloadSpriteGfx((u16*)glb_greyscaletiledata,
		    spriteno*4, 4);
}

void
gfx_spritefromdungeon(int spriteno, TILE_NAMES tile)
{
    UT_ASSERT(!glb_usesprites);
    hamfake_ReloadSpriteGfx((u16*)(&glb_tilesets[glb_tileset].dungeon[tile*4*BYTEPERTILE]),
		    spriteno*4, 4);
}

void
gfx_spritemode(bool usesprites)
{
    int		i;
    // Disable all sprites
    for (i = 0; i < 128; i++)
	hamfake_enablesprite(i, false);

    // Swap palette
    if (usesprites)
	hamfake_LoadSpritePal((void *)glb_tilesets[glb_tileset].spritepalette, 512);
    else
	hamfake_LoadSpritePal((void *)glb_tilesets[glb_tileset].palette, 512);

    glb_usesprites = usesprites;
}

//
// Fake Ham Functions
//
#if !defined(USING_SDL) && !defined(USING_DS) && !defined(_3DS)

#if !defined(NO_HAM)
void
hamfake_rebuildScreen()
{
}

void
hamfake_awaitEvent()
{
}

u16 *
hamfake_lockScreen()
{
    return ((u16 *) 0x06000000);
}

void
hamfake_unlockScreen(u16 *)
{
}

int
hamfake_peekKeyPress()
{
    return 0;
}

int
hamfake_getKeyPress(bool)
{
    return 0;
}

void
hamfake_clearKeyboardBuffer()
{
}

void
hamfake_insertKeyPress(u8 key)
{
}

void 
hamfake_softReset() 
{ 
    return;
} 

#endif

int
hamfake_getKeyModifiers()
{
    return 0;
}

void
hamfake_setFullScreen(bool fullscreen)
{
}

bool
hamfake_isFullScreen()
{
    return true;		// Always full :>
}

bool
hamfake_extratileset()
{
    return false;		// No fat system yet!
}

void
hamfake_getstyluspos(int &x, int &y)
{
    x = y = 0;
}

bool
hamfake_getstylusstate()
{
    return false;
}

void
hamfake_movesprite(int spriteno, int x, int y)
{
}

void
hamfake_enablesprite(int spriteno, bool enabled)
{
}

void
hamfake_ReloadSpriteGfx(const u16 *data, int tileno, int numtile)
{
}

void
hamfake_LoadSpritePal(void *data, int bytes)
{
}

void
hamfake_setTileSize(int width, int height)
{
}

#endif

#if !defined(iPOWDER)

bool hamfake_forceQuit() { return false; }
void hamfake_awaitShutdown() {}
void hamfake_postShutdown() {}

void hamfake_setinventorymode(bool invmode) {}
void hamfake_enableexternalactions(bool enable) {}
bool hamfake_isunlocked() { return true; }
void hamfake_externalaction(int &iact, int &ispell) 
{ iact = ACTION_NONE; ispell = SPELL_NONE; }

void hamfake_flushdir() {}
bool hamfake_externaldir(int &dx, int &dy) { return false; }

void hamfake_buttonreq(int mode, int type) { }
#endif

