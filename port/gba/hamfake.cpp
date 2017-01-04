/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        hamfake.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This file implements all the fake ham functions.
 *	It also stores the global state of the hardware.
 */


#include "mygba.h"
#include "hamfake.h"

#include "../../gfxengine.h"
#include "../../bmp.h"
#include "../../control.h"

#include <gba.h>

#define SRAMSIZE 65536

#define KEY_REPEAT_INITIAL 15
#define KEY_REPEAT_AFTER 7

//
// Global GBA state:
//

bg_info			ham_bg[4];

int			ham_vramtop = 0;
u8			ham_winin[2], ham_winout[2];

bool			ham_extratileset = false;

// Keyboard state:

void
hamfake_rebuildScreen()
{
}

void
updateOAM()
{
}

// Wait for an event to occur.
void
hamfake_awaitEvent()
{
    updateOAM();
}

// Return our internal screen.
u16 *
hamfake_lockScreen()
{
    return (u16 *) VRAM;
}

void
hamfake_unlockScreen(u16 *)
{
}

// Called to run our event poll
void
hamfake_pollEvents()
{
    updateOAM();
    scanKeys();
}

bool
hamfake_isPressed(BUTTONS button)
{
    hamfake_pollEvents();
    switch (button)
    {
	case BUTTON_UP:
	    return keysHeld() & KEY_UP;
	    
	case BUTTON_DOWN:
	    return keysHeld() & KEY_DOWN;

	case BUTTON_LEFT:
	    return keysHeld() & KEY_LEFT;

	case BUTTON_RIGHT:
	    return keysHeld() & KEY_RIGHT;

	case BUTTON_SELECT:
	    return keysHeld() & KEY_SELECT;
	case BUTTON_START:
	    return keysHeld() & KEY_START;

	case BUTTON_A:
	    return keysHeld() & KEY_A;
	case BUTTON_B:
	    return keysHeld() & KEY_B;
	case BUTTON_R:
	    return keysHeld() & KEY_R;
	case BUTTON_L:
	    return keysHeld() & KEY_L;

	// Not present on this hardware.
	case BUTTON_X:
	    return false;
	case BUTTON_Y:
	    return false;
	case BUTTON_TOUCH:
	    return false;
	case BUTTON_LID:
	    return false;

	case NUM_BUTTONS:
	    return false;
    }

    return false;
}

int
hamfake_isAnyPressed()
{
    hamfake_pollEvents();
    if (keysHeld() == 0)
    {
	// For backwards compatability with R_CTRLINPUT the sense of this
	// return is a bit odd...
	return 0x3FF;
    }
    return 0;
}

int
hamfake_peekKeyPress()
{
    return 0;
}

int
hamfake_getKeyPress(bool onlyascii)
{
    return 0;
}

void
hamfake_insertKeyPress(int key)
{
    return;
}

void
hamfake_clearKeyboardBuffer()
{
}

void
hamfake_setScrollX(int layer, int scroll)
{
    BG_OFFSET[layer].x = scroll;
}

void
hamfake_setScrollY(int layer, int scroll)
{
    BG_OFFSET[layer].y = scroll;
}

void
ham_Init()
{
}

void
ham_StartIntHandler(u8 intno, void (*fp)())
{
    // assert(intno == INT_TYPE_VBL);
    irqSet(IRQ_VBLANK, fp);
    irqEnable(IRQ_VBLANK);
}

void
ham_SetBgMode(u8 mode)
{
    switch (mode)
    {
	case 3:
	    REG_DISPCNT = MODE_3;
	    break;

	case 0:
	    // Desire tiled mode.
	    REG_DISPCNT = MODE_0 |
			BG0_ON |
			BG1_ON |
			BG2_ON |
			BG3_ON;

	    // Reset the top of our vram.
	    ham_vramtop = 0;
	    ham_DeleteWin(0);
	    ham_DeleteWin(1);
	    break;
    }
}


void
ham_LoadBGPal(void *vpal, int bytes)
{
    u16		*pal = (u16 *) vpal;
    bytes /= 2;
    int		i;

    for (i = 0; i < bytes; i++)
    {
	BG_PALETTE[i] = pal[i];
    }
}

u8
ham_AllocateVramBlock(int bytes, int blocksize)
{
    int		blockid;

    // Round current vram to nearest block.
    blockid = (ham_vramtop + blocksize-1) / blocksize;
    ham_vramtop = blockid * blocksize;
    
    // Round up bytes to allocate...
    bytes = (bytes + blocksize -1) / blocksize;
    bytes *= blocksize;

    ham_vramtop += bytes;

    return blockid;
}

map_info_ptr
ham_InitMapEmptySet(u8 size, u8 rot)
{
    map_info_ptr		mapinfo;
    int				vramsize;

    mapinfo = new map_info;

    // Find size in bytes of map.
    // if 0 size, is 32x32 so hence 2k.
    // If 3 size, is 64x64 so hence 8k.
    vramsize = 0;
    switch (size)
    {
	case 0:
	    vramsize = 2 * 32 * 32;
	    mapinfo->largemap = false;
	    break;
	case 3:
	    vramsize = 2 * 64 * 64;
	    mapinfo->largemap = true;
	    break;
    }

    mapinfo->blockid = ham_AllocateVramBlock(vramsize, 0x800);
    
    return mapinfo;
}

tile_info_ptr
ham_InitTileEmptySet(u16 entries, u8 colormode, u8 onlybaseblock)
{
    tile_info_ptr	tileinfo;
    int			vramsize;

    tileinfo = new tile_info;

    vramsize = entries;
    if (colormode == 0)
	vramsize *= 8 * 8 / 2;			// 16 colours
    else
	vramsize *= 8 * 8;

    tileinfo->blockid = ham_AllocateVramBlock(vramsize, 0x2000);
    
    return tileinfo;
}

void
ham_InitBg(int layer, int active, int priority, int mosaic)
{
    int		mapsize;
    if (ham_bg[layer].mi->largemap)
	mapsize = BG_SIZE(3);
    else
	mapsize = BG_SIZE(0);
    
    BGCTRL[layer] = BG_256_COLOR |
		    BG_PRIORITY(priority) |
		    BG_TILE_BASE(ham_bg[layer].ti->blockid) |
		    BG_MAP_BASE(ham_bg[layer].mi->blockid) |
		    mapsize;
}

void
ham_CreateWin(int wid, int x1, int y1, int x2, int y2, int inside, int outside, int fx)
{
    switch (wid)
    {
	case 0:
	    // Hmm.. this might be backwards :>
	    // Yep, it was.
	    REG_WIN0H = (x2 | (x1 << 8));
	    REG_WIN0V = (y2 | (y1 << 8));
	    break;
	case 1:
	    REG_WIN1H = (x2 | (x1 << 8));
	    REG_WIN1V = (y2 | (y1 << 8));
	    break;
    }
    ham_winin[wid] = inside;
    ham_winout[wid] = outside;
    // Update control registers.
    REG_WININ = ham_winin[0] | (ham_winin[1] << 8);
    // This is wrong.
    REG_WINOUT = ham_winout[0] | (ham_winout[1] << 8);
}

void
ham_DeleteWin(int wid)
{
    // Allow everything inside & outside.
    ham_winin[wid] = WIN_BG0 | WIN_BG1 | WIN_BG2 | WIN_BG3 | WIN_OBJ;
    ham_winout[wid] = WIN_BG0 | WIN_BG1 | WIN_BG2 | WIN_BG3 | WIN_OBJ;
    REG_WININ = ham_winin[0] | (ham_winin[1] << 8);
    REG_WINOUT = ham_winout[0] | (ham_winout[1] << 8);
}

void
ham_SetMapTile(int layer, int mx, int my, int tileidx)
{
    int		blockid = ham_bg[layer].mi->blockid;

    u16		*map = (u16 *) SCREEN_BASE_BLOCK(blockid);

    // We are interleaved in a funny way...
    if (mx >= 32)
    {
	mx -= 32;
	map += 32 * 32;
    }
    if (my >= 32)
    {
	my -= 32;
	map += 2 * 32 * 32;
    }
    map[my*32 + mx] = tileidx;
}

void
ham_ReloadTileGfx(tile_info_ptr tiledata, const u16 *data, int destidx, 
		  int numtile)
{
    int		i;
    int		 blockid = tiledata->blockid;
    u16		*dest = (u16 *) CHAR_BASE_BLOCK(blockid);
    dest += 4 * 8 * destidx;
    numtile *= 4 * 8;

    for (i = 0; i < numtile; i++)
    {
	dest[i] = data[i];
    }
}

void
hamfake_softReset()
{
}
