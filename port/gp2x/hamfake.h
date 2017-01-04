/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        hamfake.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __hamfake__
#define __hamfake__

#include "SDL.h"
#include "../../control.h"

// These are the native res of the SDL screen.
// measured in tiles.
#define HAM_SCRW_T		32
#define HAM_SCRH_T		24

//
// External Hooks
// These are HAMish functions that don't exist in the HAM library, as
// they are unneeded or non-existant on the GBA.  They have blank
// functions if USING_SDL is not defined.
//
void hamfake_rebuildScreen();
void hamfake_awaitEvent();

u16 *hamfake_lockScreen();
void hamfake_unlockScreen(u16 *screen);

char *hamfake_writeLockSRAM();
void hamfake_writeUnlockSRAM(char *);
char *hamfake_readLockSRAM();
void hamfake_readUnlockSRAM(char *);

// Flags that we are done writing to the SRAM.  Allows us to only
// write to disk once.
void hamfake_endWritingSession();

void hamfake_softReset();

void hamfake_setFullScreen(bool fullscreen);
bool hamfake_isFullScreen();


//
// Type clones.
//
struct tile_info
{
    // Private to hamfake:
    int	    		numtiles;

    u8			**tiles;
};

typedef tile_info *tile_info_ptr;

struct map_info
{
    // Private to hamfake
    int		width, height;
    int		*tiles;
    
};

typedef map_info *map_info_ptr;

struct bg_info
{
    map_info_ptr	mi;
    tile_info_ptr	ti;

    int			scrollx, scrolly;
};

extern bg_info	ham_bg[];

//
// Raw data accessors.
//
int hamfake_isAnyPressed();
bool hamfake_isPressed(BUTTONS button);

#define F_CTRLINPUT_UP_PRESSED hamfake_isPressed(BUTTON_UP)
#define F_CTRLINPUT_DOWN_PRESSED hamfake_isPressed(BUTTON_DOWN)
#define F_CTRLINPUT_LEFT_PRESSED hamfake_isPressed(BUTTON_LEFT)
#define F_CTRLINPUT_RIGHT_PRESSED hamfake_isPressed(BUTTON_RIGHT)

#define F_CTRLINPUT_A_PRESSED hamfake_isPressed(BUTTON_A)
#define F_CTRLINPUT_B_PRESSED hamfake_isPressed(BUTTON_B)
#define F_CTRLINPUT_R_PRESSED hamfake_isPressed(BUTTON_R)
#define F_CTRLINPUT_L_PRESSED hamfake_isPressed(BUTTON_L)

#define F_CTRLINPUT_SELECT_PRESSED hamfake_isPressed(BUTTON_SELECT)
#define F_CTRLINPUT_START_PRESSED hamfake_isPressed(BUTTON_START)

#define R_CTRLINPUT \
	hamfake_isAnyPressed()
    
// Gets a new keypress.  Returns 0 if no keypress is ready.
// Clears key after getting it.  
int hamfake_peekKeyPress();
int hamfake_getKeyPress(bool onlyascii);
void hamfake_clearKeyboardBuffer();
void hamfake_insertKeyPress(int key);

void hamfake_setScrollX(int layer, int scroll);
void hamfake_setScrollY(int layer, int scroll);
    
#define M_BG0SCRLX_SET(xoff) \
	hamfake_setScrollX(0, xoff);
#define M_BG0SCRLY_SET(yoff) \
	hamfake_setScrollY(0, yoff);
#define M_BG1SCRLX_SET(xoff) \
	hamfake_setScrollX(1, xoff);
#define M_BG1SCRLY_SET(yoff) \
	hamfake_setScrollY(1, yoff);
#define M_BG2SCRLX_SET(xoff) \
	hamfake_setScrollX(2, xoff);
#define M_BG2SCRLY_SET(yoff) \
	hamfake_setScrollY(2, yoff);
#define M_BG3SCRLX_SET(xoff) \
	hamfake_setScrollX(3, xoff);
#define M_BG3SCRLY_SET(yoff) \
	hamfake_setScrollY(3, yoff);
    
//
// Cloned Ham functions
//

void ham_Init();
void ham_StartIntHandler(u8 intno, void (*fp)());
void ham_SetBgMode(u8 mode);
void ham_LoadBGPal(void *vpal, int bytes);
map_info_ptr ham_InitMapEmptySet(u8 size, u8 );
tile_info_ptr ham_InitTileEmptySet(int entries, int , int);
void ham_InitBg(int layer, int foo, int bar, int baz);
void ham_CreateWin(int wid, int, int, int, int, int, int, int);
void ham_DeleteWin(int wid);
void ham_SetMapTile(int layer, int mx, int my, int tileidx);
void ham_ReloadTileGfx(tile_info_ptr tiledata, const u16 *data, int destidx, 
		  int numtile);

#endif

