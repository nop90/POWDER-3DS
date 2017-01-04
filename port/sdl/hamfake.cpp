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

#ifdef WIN32
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#endif

#ifdef _WIN32_WCE
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include <iostream>

using namespace std;

#include "mygba.h"
#include "hamfake.h"

#ifndef iPOWDER
#include "SDL.h"
#endif

#include "../../gfxengine.h"
#include "../../bmp.h"
#include "../../assert.h"
#include "../../buf.h"

#include "../../gfx/icon_sdl.bmp.c"

#ifdef iPOWDER
#include "../../queue.h"
#endif

#define SRAMSIZE 65536

#define KEY_REPEAT_INITIAL 15
#define KEY_REPEAT_AFTER 7

#define SAVENAME "powder.sav"
#define TRANSACTIONNAME "recover.sav"

//
// Global GBA state:
//
void (*glb_vblcallback)() = 0;

#ifndef iPOWDER
SDL_Surface	*glbVideoSurface;
#else
// This must be static for thread safety!
SCREENDATA	 glbOurScreen;
LOCK		 glbScreenLock;
int		 glbOurScreenId = 1;
QUEUE<int>	 glbOurEventQueue(1);	// Just used for pings, don't accum
QUEUE<int>	 glbOurActionQueue;
QUEUE<int>	 glbOurSpellQueue;
QUEUE<int>	 glbOurButtonQueue;
QUEUE<int>	 glbOurShutdownQueue;
QUEUE<int>	 glbOurDirQueue;
QUEUE<int>	 glbOurInputQueue;
char		*glbOurDataPath = 0;

#endif
bool		 glbFullScreen = false;

#ifdef USING_TILE10
int		 glbScreenWidth = 320;
int		 glbScreenHeight = 240;
int		 glbTileWidth = 10;
int		 glbTileHeight = 10;
#else
int		 glbScreenWidth = 256;
int		 glbScreenHeight = 192;
int		 glbTileWidth = 8;
int		 glbTileHeight = 8;
#endif

int		 glbScaleFactor = 1;
int		 glbScreenFudgeX = 0;
int		 glbScreenFudgeY = 0;

// These are set by another thread.
volatile int	 glbStylusX = 0, glbStylusY = 0;
volatile bool	 glbStylusState = false;
volatile bool	 glbExternalActionsEnabled = false;
volatile bool	 glbIsInventoryMode = false;
volatile bool	 glbIsForceQuit = false;
volatile bool	 glbIsPortrait = true;
volatile bool	 glbHasBeenShook = true;

#ifndef HAS_KEYBOARD
volatile bool	 glbFakeButtonState[NUM_FAKE_BUTTONS];
#endif

tile_info_ptr	 glbTheTileSet = 0;

#define TILEWIDTH (glbTileWidth)
#define TILEHEIGHT (glbTileHeight)

#define HAM_SCRW	(TILEWIDTH*HAM_SCRW_T)
#define HAM_SCRH	(TILEWIDTH*HAM_SCRH_T)

struct SPRITEDATA
{
    bool 	active;
    int 	x, y;
    u8		*data;
};

SPRITEDATA	 *glbSpriteList;

#define MAX_SPRITES 128

// Do we have an extra tileset loaded?
bool		 ham_extratileset = false;

// Keyboard state:
#ifndef iPOWDER
bool			glb_keystate[SDLK_LAST];
#endif

// Key to push
int			glb_keypusher = 0;
int			glb_keypusherraw = 0;
// Frame to push on.
int			glb_keypushtime = 0;

int			glb_keybuf[16];
s8			glb_keybufentry = -1;

int			glb_keymod = 0;

// Background state:
bg_info			ham_bg[4];

// Always set from vblanks.
volatile bool		glb_isnewframe = 0;
bool			glb_isdirty = false;

u8			glb_palette[2048];

// Mode 3 screen - a raw 15bit bitmap.
u16			*glb_rawscreen = 0;

// Paletted version of screen which is scaled into final SDL surface
// We use u16 as the high bit selects whether to use sprite or
// global palette
u16			*glb_nativescreen = 0;

char			glb_rawSRAM[SRAMSIZE];

int			glb_videomode = -1;

#ifdef iPOWDER

volatile bool		glb_isunlocked = false;

volatile bool		glbCanSendEmail = true;

bool hamfake_isunlocked()
{
    return glb_isunlocked;
}

void hamfake_setunlocked(bool state)
{
    glb_isunlocked = state;
}

volatile bool		 glb_allowunlocks = true;

bool hamfake_allowunlocks()
{
    return glb_allowunlocks;
}

void hamfake_setallowunlocks(bool state)
{
    glb_allowunlocks = state;
}

bool hamfake_cansendemail()
{
    return glbCanSendEmail;
}

void hamfake_sendemail(const char *to, const char *subject, const char *body)
{
    glbEmailData.myTo = to;
    glbEmailData.mySubject = subject;
    glbEmailData.myBody = body;
    hamfake_buttonreq(8, 0);
    glbEmailData.myTo = 0;
    glbEmailData.mySubject = 0;
    glbEmailData.myBody = 0;
}

//
// SCREENDATA_REF funcs
//

class SCREENDATA_REF
{
public:
    void		incRef();
    void		decRef();

    const u8		*data() { return myData; }
    // Steals ownership of data!
    static SCREENDATA_REF	*create(u8 *srcdata);
private:
	    SCREENDATA_REF();
    virtual ~SCREENDATA_REF();

    ATOMIC_INT32	 myRef;
    u8			*myData;
};

SCREENDATA_REF::SCREENDATA_REF()
{
    myData = 0;
    myRef.set(0);
}

SCREENDATA_REF::~SCREENDATA_REF()
{
    delete [] myData;
}

void
SCREENDATA_REF::incRef()
{
    myRef.add(1);
}

void
SCREENDATA_REF::decRef()
{
    int		ref = myRef.add(-1);

    if (ref <= 0)
    {
	// Must be only owned by one thread!
	delete this;
    }
}

SCREENDATA_REF *
SCREENDATA_REF::create(u8 *data)
{
    SCREENDATA_REF	*result;

    result = new SCREENDATA_REF;
    result->incRef();
    result->myData = data;

    return result;

}

//
// SCREENDATA funcs
//
SCREENDATA::SCREENDATA()
{
    myWidth = myHeight = myId = 0;
    myRef = 0;
}

SCREENDATA::SCREENDATA(u8 *data, int w, int h, int i)
{
    myWidth = w;
    myHeight = h;
    myId = i;
    myRef = SCREENDATA_REF::create(data);
}

SCREENDATA::SCREENDATA(const SCREENDATA &data)
{
    myRef = 0;
    *this = data;
}

SCREENDATA &
SCREENDATA::operator=(const SCREENDATA &data)
{
    if (this == &data)
	return *this;

    myWidth = data.myWidth;
    myHeight = data.myHeight;
    myId = data.myId;
    
    if (data.myRef)
	data.myRef->incRef();
    if (myRef)
	myRef->decRef();
    myRef = data.myRef;

    return *this;
}

SCREENDATA::~SCREENDATA()
{
    if (myRef)
	myRef->decRef();
}

const u8 *
SCREENDATA::data()
{
    if (!myRef)
	return 0;

    return myRef->data();
}

void
hamfake_postScreenUpdate(u8 *data, int w, int h, int id)
{
    SCREENDATA	screen(data, w, h, id);

    AUTOLOCK	a(glbScreenLock);

    glbOurScreen = screen;
}


void
hamfake_getActualScreen(SCREENDATA &screen)
{
    AUTOLOCK	a(glbScreenLock);
    screen = glbOurScreen;
}

void
hamfake_setdatapath(const char *path)
{
    if (glbOurDataPath)
	free(glbOurDataPath);

    glbOurDataPath = strdup(path);
}

void
hamfake_postdir(int dx, int dy)
{
    glbOurDirQueue.flush();
    glbOurDirQueue.append( ((dy + 2) << 2) + (dx + 1) );
}

void
hamfake_finishexternalinput()
{
    glbOurInputQueue.append(0);
}

void
hamfake_flushdir()
{
    glbOurDirQueue.flush();
}

bool
hamfake_externaldir(int &dx, int &dy)
{
    int		val;
    if (glbOurDirQueue.remove(val))
    {
	dx = (val & 3) - 1;
	dy = (val >> 2) - 2;
	return true;
    }
    return false;
}

void
hamfake_postaction(int action, int spell)
{
    if (action == 0 && spell == 0)
	return;

    // We do not want multiple actions!
    glbOurActionQueue.flush();
    glbOurSpellQueue.flush();

    if (action != 0)
	glbOurActionQueue.append(action);
    if (spell != 0)
	glbOurSpellQueue.append(spell);
}

void
hamfake_postorientation(bool isportrait)
{
    glbIsPortrait = isportrait;
}

void
hamfake_postshake(bool isshook)
{
    glbHasBeenShook = isshook;
}

bool
hamfake_hasbeenshaken()
{
    bool	result = glbHasBeenShook;
    glbHasBeenShook = false;
    return result;
}

INPUTDATA glbInputData;
EMAILDATA glbEmailData;

void
hamfake_buttonreq(int mode, int type)
{
    int		val;
    val = (mode << 16) + type;

    // Email and Keyboard inputs are locked.
    if (mode == 7 || mode == 8)
	glbOurInputQueue.flush();

    glbOurButtonQueue.append(val);

    if (mode == 7 || mode == 8)
    {
	glbOurInputQueue.waitAndRemove();
	glbOurInputQueue.flush();
    }
}

bool
hamfake_getbuttonreq(int &mode, int &type)
{
    int		val;

    if (glbOurButtonQueue.remove(val))
    {
	mode = val >> 16;
	type = val & 0xffff;
	return true;
    }
    return false;
}

void
hamfake_externalaction(int &action, int &spell)
{
    action = 0;
    spell = 0;

    glbOurActionQueue.remove(action);
    glbOurSpellQueue.remove(spell);

    glbOurActionQueue.flush();
    glbOurSpellQueue.flush();
}

void
hamfake_enableexternalactions(bool allow)
{
    glbExternalActionsEnabled = allow;
    glbOurActionQueue.flush();
    glbOurSpellQueue.flush();
}


bool
hamfake_externalactionsenabled()
{
    return glbExternalActionsEnabled;
}

void
hamfake_setinventorymode(bool inventory)
{
    glbIsInventoryMode = inventory;
}

#endif

bool
hamfake_isinventorymode()
{
    return glbIsInventoryMode;
}

void
rebuildVideoSystemFromGlobals()
{
#ifndef iPOWDER
    int		flags = SDL_RESIZABLE;

    if (glbFullScreen)
	flags |= SDL_FULLSCREEN;

    // Yes, we want a guaranteed 24bit video mode.
    glbVideoSurface = SDL_SetVideoMode(glbScreenWidth, glbScreenHeight, 
				    24, flags);

    if (!glbVideoSurface)
    {
	fprintf(stderr, "Failed to create screen: %s\n",
			SDL_GetError());
	SDL_Quit();
	exit(1);
    }

    // Setup our window environment.
    SDL_WM_SetCaption("POWDER", 0);

    // We want the cursor enabled now that you can click on stuff. 
    // SDL_ShowCursor(SDL_DISABLE);
#endif

    glb_isdirty = true;
}

void
hamfake_setFullScreen(bool fullscreen)
{
    // Pretty straight forward :>
    glbFullScreen = fullscreen;

    rebuildVideoSystemFromGlobals();
}

bool
hamfake_isFullScreen()
{
    return glbFullScreen;
}

// Updates our glbScreenWidth & glbScreenHeight appropriately.  Finds a scale
// factor that will fit.
void
setResolution(int width, int height)
{
    // Find maximum scale factor that fits.
    glbScaleFactor = 2;
    while (1)
    {
	glbScreenWidth = HAM_SCRW * glbScaleFactor;
	glbScreenHeight = HAM_SCRH * glbScaleFactor;
	if (glbScreenWidth > width || glbScreenHeight > height)
	{
	    glbScaleFactor--;
	    break;
	}
	glbScaleFactor++;
    }

    glbScreenWidth = width;
    glbScreenHeight = height;

    // Do not allow fractional scales.
    if (glbScreenWidth < HAM_SCRW)
	glbScreenWidth = HAM_SCRW;
    if (glbScreenHeight < HAM_SCRH)
	glbScreenHeight = HAM_SCRH;

    // Calculate our fudge factor...
    glbScreenFudgeX = ((glbScreenWidth - HAM_SCRW * glbScaleFactor) / 2);
    glbScreenFudgeY = ((glbScreenHeight - HAM_SCRH * glbScaleFactor) / 2);

    // Take effect!
    rebuildVideoSystemFromGlobals();
}

void
scaleScreenFromPaletted(u8 *dst, int pitch)
{
    int		x, y, s_y, s_x;
    u8		pixel[3];
    u16		idx;
    u16		*src;

    // Clear initial dst...
    memset(dst, 0, pitch * glbScreenHeight);

    // Add in fudge factor
    dst += glbScreenFudgeX * 3 + glbScreenFudgeY * pitch;

    src = glb_nativescreen;
    for (y = 0; y < HAM_SCRH; y++)
    {
	for (s_y = 0; s_y < glbScaleFactor; s_y++)
	{
	    for (x = 0; x < HAM_SCRW; x++)
	    {
		// Read in a pixel & decode
		idx = src[x];
#ifndef iPOWDER
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		pixel[0] = glb_palette[idx*4+3];
		pixel[1] = glb_palette[idx*4+2];
		pixel[2] = glb_palette[idx*4+1];
#else
		pixel[0] = glb_palette[idx*4+1];
		pixel[1] = glb_palette[idx*4+2];
		pixel[2] = glb_palette[idx*4+3];
#endif
#else
		pixel[0] = glb_palette[idx*4+3];
		pixel[1] = glb_palette[idx*4+2];
		pixel[2] = glb_palette[idx*4+1];
#endif
		// Now write out the needed number of times...
		for (s_x = 0; s_x < glbScaleFactor; s_x++)
		{
		    *dst++ = pixel[0];
		    *dst++ = pixel[1];
		    *dst++ = pixel[2];
		}
	    }
	    // Add remainder of pitch.
	    dst += pitch - 3 * glbScaleFactor * HAM_SCRW;
	}
	src += HAM_SCRW;
    }
}

void
scaleScreenFrom15bit(u8 *dst, int pitch)
{
    int		 x, y, s_y, s_x;
    u8		 pixel[3];
    u16		 raw;
    u16		*src;

    // Clear initial dst...
    memset(dst, 0, pitch * glbScreenHeight);

    // Add in fudge factor
    dst += glbScreenFudgeX * 3 + glbScreenFudgeY * pitch;

    src = glb_rawscreen;
    for (y = 0; y < HAM_SCRH; y++)
    {
	for (s_y = 0; s_y < glbScaleFactor; s_y++)
	{
	    for (x = 0; x < HAM_SCRW; x++)
	    {
		// Read in a pixel & decode
		raw = src[x];

#ifndef iPOWDER
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		pixel[0] = (raw & 31) << 3;
		pixel[1] = ((raw >> 5) & 31) << 3;
		pixel[2] = ((raw >> 10) & 31) << 3;
#else
		pixel[2] = (raw & 31) << 3;
		pixel[1] = ((raw >> 5) & 31) << 3;
		pixel[0] = ((raw >> 10) & 31) << 3;
#endif
#else
		pixel[0] = (raw & 31) << 3;
		pixel[1] = ((raw >> 5) & 31) << 3;
		pixel[2] = ((raw >> 10) & 31) << 3;
#endif

		// Now write out the needed number of times...
		for (s_x = 0; s_x < glbScaleFactor; s_x++)
		{
		    *dst++ = pixel[0];
		    *dst++ = pixel[1];
		    *dst++ = pixel[2];
		}
	    }
	    // Add remainder of pitch.
	    dst += pitch - 3 * glbScaleFactor * HAM_SCRW;
	}
	src += HAM_SCRW;
    }
}

// Draws the given sprite to the native buffer.
void
blitSprite(const SPRITEDATA &sprite)
{
    if (sprite.active)
    {
	// Finally, blit the sprite.
	int		sx, sy, x, y, tx, ty, srcoffset;
	u8		src;

	for (sy = 0; sy < TILEHEIGHT*2; sy++)
	{
	    for (sx = 0; sx < TILEWIDTH*2; sx++)
	    {
		x = sprite.x + sx;
		y = sprite.y + sy;
		// Convert to DS coords from GBA
		x += TILEWIDTH;
#ifdef iPOWDER
		y += 3*TILEHEIGHT;
#else
		y += 2*TILEHEIGHT;
#endif

		tx = (sx >= TILEWIDTH) ? 1 : 0;
		ty = (sy >= TILEHEIGHT) ? 1 : 0;
		srcoffset = sx + tx * TILEWIDTH * (TILEHEIGHT-1) +
			    ty * 1 * TILEWIDTH * (TILEHEIGHT) + sy * TILEWIDTH;
		src = sprite.data[srcoffset];
		if (src &&
		    x >= 0 && x < HAM_SCRW &&
		    y >= 0 && y < HAM_SCRH)
		{
		    glb_nativescreen[x + y * HAM_SCRW] = src + 256;
		}
	    }
	}
    }
}

void
hamfake_rebuildScreen()
{
    int			i, layer, x, y, tileidx, tx, ty, px, py;
    int			offset;
    static bool		oldorient = true;

    if (glbIsPortrait != oldorient)
    {
	oldorient = glbIsPortrait;
	glb_isdirty = true;
	gfx_tilesetmode(!glbIsPortrait);
    }

    if (!glb_isdirty)
	return;

    glb_isdirty = false;
    
    if (glb_videomode == 3)
    {
	u8		*dst;

#ifndef iPOWDER
	SDL_LockSurface(glbVideoSurface);

	dst = (u8 *) glbVideoSurface->pixels;
	if (!dst)
	{
	    printf("Lock failure: %s\n", SDL_GetError());
	}

	scaleScreenFrom15bit(dst, glbVideoSurface->pitch);

	SDL_UnlockSurface(glbVideoSurface);
	// Rebuild from 15bit screen.
	SDL_UpdateRect(glbVideoSurface, 0, 0, 0, 0);
#else
	dst = new u8[glbScreenWidth * glbScreenHeight * 3];
	scaleScreenFrom15bit(dst, glbScreenWidth*3);
	// Gains ownership...
	hamfake_postScreenUpdate(dst, glbScreenWidth, glbScreenHeight, glbOurScreenId);
	glbOurScreenId++;
#endif
	return;
    }

    // Clear our video surface.
    memset(glb_nativescreen, 0, HAM_SCRH * HAM_SCRW * sizeof(u16));

    // Blit each layer in turn.
    // Hard coded layer order:
    int		lay_ord[4] = { 1, 2, 3, 0 };
    for (i = 0; i < 4; i++)
    {
	layer = lay_ord[i];

	ty = ham_bg[layer].scrolly;
	if (ty < 0)
	    ty = - ((-ty + TILEHEIGHT-1) / TILEHEIGHT);
	else
	    ty = (ty + TILEHEIGHT-1) / TILEHEIGHT;
	py = ham_bg[layer].scrolly % TILEHEIGHT;
	if (py < 0)
	{
	    ty++;
	    py += TILEHEIGHT;
	}

	ty %= ham_bg[layer].mi->height;
	if (ty < 0)
	    ty += ham_bg[layer].mi->height;

	for (y = (py ? 1 : 0); y < HAM_SCRH_T; y++)
	{
	    tx = ham_bg[layer].scrollx;
	    if (tx < 0)
		tx = - ((-tx + TILEWIDTH-1) / TILEWIDTH);
	    else
		tx = (tx + TILEWIDTH-1) / TILEWIDTH;
	    px = ham_bg[layer].scrollx % TILEWIDTH;
	    if (px < 0)
	    {
		px += TILEWIDTH;
		tx++;
	    }

	    tx %= ham_bg[layer].mi->width;
	    if (tx < 0)
		tx += ham_bg[layer].mi->width;
	    
	    for (x = (px ? 1 : 0); x < HAM_SCRW_T; x++)
	    {
		u8		*tile;

		tileidx = ham_bg[layer].mi->tiles[ty * ham_bg[layer].mi->width + tx];
		    
		// Out of bound tiles are ignored.
		if ((tileidx & 1023) < ham_bg[layer].ti->numtiles)
		{
		    tile = ham_bg[layer].ti->tiles[tileidx & 1023];

		    // Compute offset into native screen
		    // This computation is very suspect and has only
		    // been vetted with px == 4, so may not work with
		    // other sub pixel scroll values.
		    offset = x * TILEWIDTH - px + (y * TILEHEIGHT - py) * HAM_SCRW;

		    // Figure out any flip flags.
		    if (tileidx & (2048 | 1024))
		    {
			int	    sx, sy, fx, fy;

			for (sy = 0; sy < TILEHEIGHT; sy++)
			{
			    if (tileidx & 2048)
				fy = TILEHEIGHT-1 - sy;
			    else
				fy = sy;

			    for (sx = 0; sx < TILEWIDTH; sx++)
			    {
				if (tileidx & 1024)
				    fx = TILEWIDTH-1 - sx;
				else
				    fx = sx;

				// Write if non-zero
				if (tile[fx + fy * TILEWIDTH])
				    glb_nativescreen[offset] = tile[fx + fy * TILEWIDTH];
				offset++;
			    }
			    offset += HAM_SCRW - TILEWIDTH;
			}
		    }
		    else
		    {
			int		sx, sy;

			// Straight forward write...
			for (sy = 0; sy < TILEHEIGHT; sy++)
			{
			    for (sx = 0; sx < TILEWIDTH; sx++)
			    {
				if (*tile)
				{
				    glb_nativescreen[offset] = *tile;
				}
				tile++;
				offset++;
			    }
			    offset += HAM_SCRW - TILEWIDTH;
			}
		    }
		}
		
		tx++;
		if (tx >= ham_bg[layer].mi->width)
		    tx -= ham_bg[layer].mi->width;
	    }

	    ty++;
	    if (ty >= ham_bg[layer].mi->height)
		ty -= ham_bg[layer].mi->height;
	}

	// Just before the text layer we draw our sprites
	// This ensures the buttons are under the text.
	if (i == 2)
	{
	    int		j;

	    // Since we composite on top, we want to do in reverse order of
	    // priority.  Hence the backwards loop.
	    // The last sprite is the cursor and is done at the very end.
	    for (j = MAX_SPRITES; j --> 1; )
	    {
		blitSprite(glbSpriteList[j]);
	    }
	}
    }

    // Draw the cursor sprite
    blitSprite(glbSpriteList[0]);

    // Lock and update the final surface using our lookup...
    {
	u8		*dst;

#ifndef iPOWDER
	SDL_LockSurface(glbVideoSurface);
	dst = (u8 *) glbVideoSurface->pixels;
	if (!dst)
	{
	    printf("Lock failure: %s\n", SDL_GetError());
	}

	scaleScreenFromPaletted(dst, glbVideoSurface->pitch);

	SDL_UnlockSurface(glbVideoSurface);
	// Rebuild from 8bit screen.
	SDL_UpdateRect(glbVideoSurface, 0, 0, 0, 0);
#else
	dst = new u8[glbScreenWidth * glbScreenHeight * 3];
	scaleScreenFromPaletted(dst, glbScreenWidth*3);
	// Gains ownership...
	hamfake_postScreenUpdate(dst, glbScreenWidth, glbScreenHeight, glbOurScreenId);
	glbOurScreenId++;
#endif
    }
}

// Converts a unicode key into an ASCII key.
int
processSDLKey(int unicode, int sdlkey)
{
#ifndef iPOWDER
#ifdef _WIN32_WCE
    // Windows CE defines some hard-coded buttons we
    // want to re-interpret
    // Ideally we'd handle these remaps using an actual
    // new defaults for the character mapping, but since POWDER
    // is still all 1970's with hardcoded key definitions, we'll
    // suffer with this for now.
    switch (unicode)
    {
	case 'r':
	    return '\x1b';
	case 'p':
	    return 'g';
	case 'q':
	    return 'x';
	case 's':
	    return 'm';

	// Shortcut buttons
	case 193:
	    return 'O';
	case 195:
	    return '\x1b';
	case 194:
	    return 'V';
	case 196:
	    return 'i';
    }
#endif

    // Check to see if we are intelligble unicode.
    if (unicode && unicode < 128)
    {
	// The control characters usually report as unicode
	// values, we want to instead treat these as the normal
	// lower case alpha just with the ctrl- modifier set.
	// This then allows control to be a modifier on any
	// normal key.
	if (unicode < ' ')
	{
	    if (sdlkey >= SDLK_a && sdlkey <= SDLK_z)
	    {
		return 'a' + sdlkey - SDLK_a;
	    }
	}
	// We want delete aliased to backspace because Macs lack
	// a backspace and I'm too lazy to support both elsewhere.
	if (unicode == SDLK_DELETE)
	    unicode = '\b';
	return unicode;
    }

    // Process stupid exceptions.
    switch (sdlkey)
    {
	// The mac often ships without a backspace key.  Out of laziness,
	// alias delete to backspace
	case SDLK_DELETE:
	// Remapping backspace should be a no-op, but I'm not
	// holding my breath and heard reports of backspace going
	// 'b' on me.
	case SDLK_BACKSPACE:
	    return '\b';
	// Insert several curses here...  This isn't necessary
	// anywhere except WIndows XP it seems...
	case SDLK_KP0:
	    return '0';
	case SDLK_KP1:
	    return '1';
	case SDLK_KP2:
	    return '2';
	case SDLK_KP3:
	    return '3';
	case SDLK_KP4:
	    return '4';
	case SDLK_KP5:
	    return '5';
	case SDLK_KP6:
	    return '6';
	case SDLK_KP7:
	    return '7';
	case SDLK_KP8:
	    return '8';
	case SDLK_KP9:
	    return '9';
	case SDLK_UP:
	    return GFX_KEYUP;
	case SDLK_DOWN:
	    return GFX_KEYDOWN;
	case SDLK_LEFT:
	    return GFX_KEYLEFT;
	case SDLK_RIGHT:
	    return GFX_KEYRIGHT;
	case SDLK_PAGEUP:
	    return GFX_KEYPGUP;
	case SDLK_PAGEDOWN:
	    return GFX_KEYPGDOWN;
	case SDLK_F1:
	    return GFX_KEYF1;
	case SDLK_F2:
	    return GFX_KEYF2;
	case SDLK_F3:
	    return GFX_KEYF3;
	case SDLK_F4:
	    return GFX_KEYF4;
	case SDLK_F5:
	    return GFX_KEYF5;
	case SDLK_F6:
	    return GFX_KEYF6;
	case SDLK_F7:
	    return GFX_KEYF7;
	case SDLK_F8:
	    return GFX_KEYF8;
	case SDLK_F9:
	    return GFX_KEYF9;
	case SDLK_F10:
	    return GFX_KEYF10;
	case SDLK_F11:
	    return GFX_KEYF11;
	case SDLK_F12:
	    return GFX_KEYF12;
	case SDLK_F13:
	    return GFX_KEYF13;
	case SDLK_F14:
	    return GFX_KEYF14;
	case SDLK_F15:
	    return GFX_KEYF15;
    }

#endif
    // Failed to parse key, 0.
    return 0;
}

// Wait for an event to occur.
void
hamfake_awaitEvent()
{
    if (hamfake_forceQuit())
    {
	if (glb_vblcallback)
	    (*glb_vblcallback)();
	return;
    }
#ifndef iPOWDER
    SDL_WaitEvent(0);
#else
    glbOurEventQueue.waitAndRemove();

    glbOurEventQueue.flush();
#endif
}

#ifdef iPOWDER
void
hamfake_awaitShutdown()
{
    glbOurShutdownQueue.waitAndRemove();
}

void
hamfake_postShutdown()
{
    glbOurShutdownQueue.append(0);
}
#endif

// Return our internal screen.
u16 *
hamfake_lockScreen()
{
    // Note we have to offset to the normal GBA start
    // location to account for our official offset.
#ifdef iPOWDER
    return glb_rawscreen + TILEWIDTH + 3*TILEHEIGHT * HAM_SCRW;
#else
    return glb_rawscreen + TILEWIDTH + 2*TILEHEIGHT * HAM_SCRW;
#endif
}

void
hamfake_unlockScreen(u16 *)
{
    glb_isdirty = true;
}

// Deal with our SRAM buffer.
char *
hamfake_writeLockSRAM()
{
    return glb_rawSRAM;
}

void
hamfake_writeUnlockSRAM(char *)
{
}

char *
hamfake_readLockSRAM()
{
    return glb_rawSRAM;
}

void
hamfake_readUnlockSRAM(char *)
{
}

void
hamfake_endWritingSession()
{
    FILE		*fp;
    int			bytes;

    // We run a simple transaction model here to try
    // and ensure even if we crash in the process the last
    // good save file will still be found.

    fp = hamfake_fopen(TRANSACTIONNAME, "wb");
    if (!fp)
    {
	fprintf(stderr, "Failure to open " TRANSACTIONNAME " for writing!\r\n");
	return;
    }

    bytes = fwrite(glb_rawSRAM, 1, SRAMSIZE, fp);
    fclose(fp);
    if (bytes != SRAMSIZE)
    {
	fprintf(stderr, "Only wrote %d to " TRANSACTIONNAME "\r\n", bytes);
	return;
    }

    // Now delete the old file and rename
    hamfake_move(SAVENAME, TRANSACTIONNAME);
}

void
processKeyMod(int mod)
{
#ifdef HAS_KEYBOARD
    if (mod & KMOD_CTRL || mod & KMOD_META)
	glb_keymod |= GFX_KEYMODCTRL;
    else
	glb_keymod &= ~GFX_KEYMODCTRL;

    if (mod & KMOD_SHIFT)
	glb_keymod |= GFX_KEYMODSHIFT;
    else
	glb_keymod &= ~GFX_KEYMODSHIFT;

    // Modal-T?  Is that a type of car?
    if (mod & KMOD_ALT)
	glb_keymod |= GFX_KEYMODALT;
    else
	glb_keymod &= ~GFX_KEYMODALT;
#endif
}

// Called to run our event poll
void
hamfake_pollEvents()
{
#ifndef iPOWDER
    SDL_Event	event;
    int		cookedkey;

    while (SDL_PollEvent(&event))
    {
	switch (event.type)
	{
	    case SDL_VIDEORESIZE:
		setResolution(event.resize.w, event.resize.h);
		break;
	    case SDL_KEYDOWN:
		if (event.key.keysym.sym < SDLK_LAST)
		{
		    glb_keystate[event.key.keysym.sym] = 1;

		    cookedkey = processSDLKey(event.key.keysym.unicode, event.key.keysym.sym);
		    processKeyMod(event.key.keysym.mod);

		    // What sort of drunk would insert the keypress
		    // on a key up?
 		    hamfake_insertKeyPress(cookedkey);

		    if (cookedkey)
		    {
			glb_keypushtime = gfx_getframecount() + KEY_REPEAT_INITIAL;
			glb_keypusher = cookedkey;
		        glb_keypusherraw = event.key.keysym.sym;
		    }
		}
		break;
	    case SDL_KEYUP:
		if (event.key.keysym.sym < SDLK_LAST)
		{
		    glb_keystate[event.key.keysym.sym] = 0;

		    // Sadly, unicode is *not* cooked on a keyup!
		    // Damn SDL!
		    // Thus, we always clear out on a key going high.
		    // cookedkey = processSDLKey(event.key.keysym.unicode);
		    // This will annoy copx, however, so we instead
		    // stop being lazy and store the original raw code.

		    // If our currently repeated key goes high,
		    // clear it out.
		    if (event.key.keysym.sym  == glb_keypusherraw)
		    {
			glb_keypusher = 0;
		    }
		}
		break;

	    case SDL_MOUSEMOTION:
	    {
		glbStylusX = event.motion.x;
		glbStylusY = event.motion.y;
		
		int x, y;
		hamfake_getstyluspos(x, y);
		break;
	    }
		
	    case SDL_MOUSEBUTTONDOWN:
		glbStylusX = event.button.x;
		glbStylusY = event.button.y;
		cookedkey = 0;
		switch (event.button.button)
		{
		    case SDL_BUTTON_LEFT:
			cookedkey = GFX_KEYLMB;
			glbStylusState = true;
			break;
		    case SDL_BUTTON_MIDDLE:
			cookedkey = GFX_KEYMMB;
			break;
		    case SDL_BUTTON_RIGHT:
			cookedkey = GFX_KEYRMB;
			break;
		}
		if (cookedkey)
		{
		    hamfake_insertKeyPress(cookedkey);
		    glb_keypushtime = gfx_getframecount() + KEY_REPEAT_INITIAL;
		    glb_keypusher = cookedkey;
		    // Special value for these...
		    glb_keypusherraw = -cookedkey;
		}
		break;

	    case SDL_MOUSEBUTTONUP:
		glbStylusX = event.button.x;
		glbStylusY = event.button.y;
		cookedkey = 0;
		switch (event.button.button)
		{
		    case SDL_BUTTON_LEFT:
			glbStylusState = false;
			cookedkey = GFX_KEYLMB;
			break;
		    case SDL_BUTTON_MIDDLE:
			cookedkey = GFX_KEYMMB;
			break;
		    case SDL_BUTTON_RIGHT:
			cookedkey = GFX_KEYRMB;
			break;
		}
		// Wipe out the pusher if we match.
		if (-cookedkey == glb_keypusherraw)
		{
		    glb_keypusher = 0;
		}
		break;

	    case SDL_QUIT:
		SDL_Quit();
		exit(0);
		break;
	}
    }

    // After chewing up all the events, we pump the keyboard repeat
    // This ensures it is the same thread as the keyboard repeat
    // set up.  It also ensures that if we are stalling on the main
    // thread, so weren't able to respond to the users key up command,
    // we will not send extra keys.
    // It is reasoned that extra repeats are more frustrating than
    // missed repeats.
    {
	int			frame;

	frame = gfx_getframecount();
	if (glb_keypusher)
	{
	    if (frame >= glb_keypushtime)
	    {
		hamfake_insertKeyPress(glb_keypusher);
		glb_keypushtime = frame + KEY_REPEAT_AFTER;
	    }
	}
    }
#endif
}

int
hamfake_isAnyPressed()
{
    hamfake_pollEvents();
    hamfake_rebuildScreen();

    int			i;
#ifndef iPOWDER
    for (i = 0; i < SDLK_LAST; i++)
	if (glb_keystate[i]) return 0x0;
#else
    for (i = 0; i < NUM_FAKE_BUTTONS; i++)
	if (hamfake_isFakeButtonPressed((FAKE_BUTTONS) i))
	    return 0;
    if (glbStylusState)
	return 0;

    // Eat any external action key press as an any key.
    int		action, spell;
    hamfake_externalaction(action, spell);
    if (action || spell)
	return 0;
#endif
    return 0x3FF;
}

#ifdef iPOWDER
bool
hamfake_forceQuit()
{
    return glbIsForceQuit;
}

void
hamfake_setForceQuit()
{
    glbIsForceQuit = true;

    // Ensure we aren't stalled on input.
    hamfake_finishexternalinput();

    // Trigger an event so we wake up immediately.
    hamfake_callThisFromVBL();
}
#endif

#ifndef HAS_KEYBOARD
bool hamfake_isFakeButtonPressed(FAKE_BUTTONS button)
{
    if (button < 0 || button >= NUM_FAKE_BUTTONS)
	return false;
    hamfake_pollEvents();
    hamfake_rebuildScreen();
    return glbFakeButtonState[button];
}

void hamfake_setFakeButtonState(FAKE_BUTTONS button, bool state)
{
    if (button < 0 || button >= NUM_FAKE_BUTTONS)
	return;
    glbFakeButtonState[button] = state;
}
#endif

int
hamfake_peekKeyPress()
{
    hamfake_pollEvents();
    hamfake_rebuildScreen();

    // Nothing available.
    if (glb_keybufentry < 0)
	return 0;

    return glb_keybuf[0];
}

int
hamfake_getKeyModifiers()
{
    hamfake_pollEvents();
    hamfake_rebuildScreen();

    return glb_keymod;
}

int
hamfake_getKeyPress(bool onlyascii)
{
    hamfake_pollEvents();
    hamfake_rebuildScreen();

    // Nothing available.
    if (glb_keybufentry < 0)
	return 0;

    int		newkey;

    newkey = glb_keybuf[0];

    // Eat up the keys.
    if (glb_keybufentry)
	memmove(glb_keybuf, &glb_keybuf[1], glb_keybufentry * sizeof(int));

    glb_keybufentry--;

    // If the key isn't ascii and we only want ascii, eat the key and try
    // again.
    if (onlyascii && newkey > 255)
    {
	return hamfake_getKeyPress(onlyascii);
    }

    return newkey;
}

void
hamfake_insertKeyPress(int key)
{
    // Ignore illegal keys.
    if (!key)
	return;
    
    if (glb_keybufentry < 15)
    {
	glb_keybuf[glb_keybufentry+1] = key;
	glb_keybufentry++;
    }
}

void
hamfake_clearKeyboardBuffer()
{
    // Stop pushing keys!
    glb_keypusher = 0;
    // Flush the buffer!
    glb_keybufentry = -1;

#ifndef iPOWDER
    // I think this is actually very questionable.  The DS, which as
    // the best stylus support, never releases it.
    // Release the stylus
    glbStylusState = false;
#else
    // Empty our action queues.
    glbOurActionQueue.flush();
    glbOurSpellQueue.flush();
#endif
}

void
hamfake_setScrollX(int layer, int scroll)
{
    ham_bg[layer].scrollx = scroll;	
}

void
hamfake_setScrollY(int layer, int scroll)
{
    ham_bg[layer].scrolly = scroll;	
}

u8 *
convertTo32Bit(const unsigned short *s, int numpixel)
{
    u8 		*result, *d;
    unsigned short	 p;

    result = (u8 *) malloc(numpixel * 4);
    d = result;
    
    while (numpixel--)
    {
	p = *s++;
	*d++ = (p & 31) * 8 + 4;
	p >>= 5;
	*d++ = (p & 31) * 8 + 4;
	p >>= 5;
	*d++ = (p & 31) * 8 + 4;
	*d++ = 0xff;
    }
    return result;
}

#ifdef CHANGE_WORK_DIRECTORY
// This is Linux friendly.
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

const char *
gethomedir()
{
    static char *home = 0;

    if (!home)
	home = getenv("HOME");
    if (!home)
    {
	struct passwd *pw = getpwuid(getuid());
	if (pw) home = pw->pw_dir;
    }

    return home;
}

bool
changeworkdir()
{
    struct stat buffer;
    int status;

    const char *home = gethomedir();
    if (!home)
    {
	fprintf(stderr, "Error finding home directory.\r\n");
	return false;
    }

    if (chdir(home))
    {
	perror("Changing home directory");
	return false;
    }

    const char *powderdir = ".powder";

    status = stat(powderdir, &buffer);
    if (status)
    {
	if (mkdir(powderdir, S_IRWXU | S_IRWXG | S_IRWXO))
	{
	    perror(powderdir);
	    return false;
	}
    }
    else if (!S_ISDIR(buffer.st_mode))
    {
	fprintf(stderr, "~/%s is not a directory!\r\n", powderdir);
	return false;
    }

    return !chdir(powderdir);
}
#endif

void
ham_Init()
{
    int		i;
    
    printf("\nPOWDER initializing...\n");

#ifdef CHANGE_WORK_DIRECTORY
    if (!changeworkdir())
    {
	fprintf(stderr, "Failed to initialize ~/.powder\r\n");
	exit(-1);
    }
#endif

#ifndef iPOWDER
    /* initialize SDL */
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 )
    {
	    fprintf( stderr, "Video initialization failed: %s\n",
		    SDL_GetError( ) );
	    SDL_Quit( );
    }
#endif

    u8 *rgbpixel;

    rgbpixel = convertTo32Bit(bmp_icon_sdl, 32 * 32);

    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#ifndef iPOWDER
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const Uint32 rmask = 0xff000000;
    const Uint32 gmask = 0x00ff0000;
    const Uint32 bmask = 0x0000ff00;
    const Uint32 amask = 0x000000ff;
#else
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif

    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((char *)rgbpixel, 32, 32, 32, 32*4,
			    rmask, gmask, bmask, amask);

    SDL_WM_SetIcon(surf, NULL);
    
    SDL_EnableUNICODE(1);
#endif

    glbSpriteList = new SPRITEDATA[MAX_SPRITES]; 
    for (i = 0; i < MAX_SPRITES; i++)
    {
	glbSpriteList[i].active = false;
	glbSpriteList[i].data = new u8[TILEWIDTH * TILEHEIGHT * 4];
    }

#ifndef iPOWDER
    memset(glb_keystate, 0, SDLK_LAST);
#endif

    glb_rawscreen = new u16[HAM_SCRW*HAM_SCRH];
    memset(glb_rawscreen, 0, HAM_SCRW*HAM_SCRH*sizeof(u16));
    glb_nativescreen = new u16[HAM_SCRW*HAM_SCRH];

    // Load any save games.
    // We give preference to the recovery name.  If it crashes
    // post unlink but before moving, this is cool.
    // We do, however, validate the fread.  If it fails to
    // get SRAMSIZE we had failure to save the complete file
    // so backup to powder.sav.
    FILE	*fp;
    bool	 foundtransaction = false;

    fp = hamfake_fopen(TRANSACTIONNAME, "rb");
    if (fp)
    {
	int			transactionsize;
	transactionsize = fread(glb_rawSRAM, 1, SRAMSIZE, fp);
	if (transactionsize == SRAMSIZE)
	{
	    // Successful recovery!
	    fprintf(stderr, "Loaded recover.sav\r\n");
	    foundtransaction = true;
	}
	else
	{
	    fprintf(stderr, TRANSACTIONNAME " is corrupt: %d, ignoring it.\r\n", transactionsize);
	}
	fclose(fp);
    }
    // Now try to load the normal powder.sav
    if (!foundtransaction)
    {
	fp = hamfake_fopen(SAVENAME, "rb");
	if (fp)
	{
	    int		savesize;
	    savesize = fread(glb_rawSRAM, 1, SRAMSIZE, fp);
	    if (savesize != SRAMSIZE)
	    {
		fprintf(stderr, SAVENAME " is corrupt: %d, but still reading it.\r\n", savesize);
	    }
	    else
	    {
		fprintf(stderr, "Loaded powder.sav\r\n");
	    }
	    fclose(fp);
	}
    }

    // Load any extra tilesets.
    if (bmp_loadExtraTileset())
    {
	ham_extratileset = true;
    }
    
    // Load any extra tilesets.
    return;
}

#ifndef iPOWDER
Uint32
glb_VBLTimerCallback(Uint32 interval, void *fp)
{
    glb_isnewframe = 1;
    (*glb_vblcallback)();

    // Push a fake event so we wake up.
    SDL_Event		event;

    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    
    return 16;
}
#else
void
hamfake_callThisFromVBL()
{
    glb_isnewframe = 1;
    if (glb_vblcallback)
	(*glb_vblcallback)();

    glbOurEventQueue.append(0);
}

#endif

void
ham_StartIntHandler(u8 intno, void (*fp)())
{
    assert(intno == INT_TYPE_VBL);
    
    glb_vblcallback = fp;
#ifndef iPOWDER
    SDL_AddTimer(16, glb_VBLTimerCallback, 0);
#endif
}

void
ham_SetBgMode(u8 mode)
{
    if (glb_videomode == -1)
    {
#if defined(_WIN32_WCE) || defined(iPOWDER)
	// For mobiles we want to default to the lowest res.
	setResolution(HAM_SCRW, HAM_SCRH);
#else
	setResolution(HAM_SCRW * 2, HAM_SCRH * 2);
#endif
	rebuildVideoSystemFromGlobals();
    }

    glb_videomode = mode;
}


void
ham_LoadBGPal(void *vpal, int bytes)
{
    // Need to promote our 15bit palette to a 24 bit one.
    int		entries = bytes / 2, i;
    u16		*pal = (u16 *) vpal;

    for (i = 0; i < entries; i++)
    {
	glb_palette[i*4+3] = (pal[i] & 31) << 3;
	glb_palette[i*4+2] = ((pal[i] >> 5) & 31) << 3;
	glb_palette[i*4+1] = ((pal[i] >> 10) & 31) << 3;
	if (i)
	    glb_palette[i*4] = 255;
	else
	    glb_palette[i*4] = 0;
    }
}

map_info_ptr
ham_InitMapEmptySet(u8 size, u8 )
{
    map_info_ptr		mi;

    mi = new map_info;
    
    if (size == 0)
    {
	mi->width = 32;
	mi->height = 32;
    }
    else
    {
	mi->width = 64;
	mi->height = 64;
    }

    mi->tiles = new int[mi->width * mi->height];
    memset(mi->tiles, 0, sizeof(int) * mi->width * mi->height);
    
    return mi;
}

tile_info_ptr
ham_InitTileEmptySet(int entries, int , int)
{
    tile_info_ptr		ti;
    int				i;

    ti = new tile_info;
    ti->numtiles = entries;
    ti->tiles = new u8 *[entries];

    for (i = 0; i < entries; i++)
    {
	ti->tiles[i] = new u8 [TILEWIDTH * TILEHEIGHT];
    }
    glbTheTileSet = ti;
    return ti;
}

void
ham_InitBg(int layer, int foo, int bar, int baz)
{
    ham_bg[layer].scrollx = 0;	
    ham_bg[layer].scrolly = 0;	
}

void
ham_CreateWin(int , int, int, int, int, int, int, int)
{
}

void
ham_DeleteWin(int wid)
{
}

void
ham_SetMapTile(int layer, int mx, int my, int tileidx)
{
    map_info_ptr		mi;

    mi = ham_bg[layer].mi;

    mx = (mx % mi->width);
    if (mx < 0)
	mx += mi->width;
    my = (my % mi->height);
    if (my < 0)
	my += mi->height;

    mi->tiles[mx + my * mi->width] = tileidx;

    glb_isdirty = true;
}

void
hamfake_setTileSize(int tilewidth, int tileheight)
{
    int			i;
    
    glbTileWidth = tilewidth;
    glbTileHeight = tileheight;

    delete [] glb_rawscreen;
    delete [] glb_nativescreen;
    glb_rawscreen = new u16[HAM_SCRW*HAM_SCRH];
    memset(glb_rawscreen, 0, HAM_SCRW*HAM_SCRH*sizeof(u16));
    glb_nativescreen = new u16[HAM_SCRW*HAM_SCRH];

    for (i = 0; glbTheTileSet && (i < glbTheTileSet->numtiles); i++)
    {
	delete [] glbTheTileSet->tiles[i];
	glbTheTileSet->tiles[i] = new u8 [TILEWIDTH * TILEHEIGHT];
    }

    for (i = 0; i < MAX_SPRITES; i++)
    {
	delete [] glbSpriteList[i].data;
	glbSpriteList[i].data = new u8[TILEWIDTH * TILEHEIGHT * 4];
    }
#ifdef iPOWDER
    // This platform works with a virtual screen so we never use
    // the black fudge space.
    glbScreenWidth = HAM_SCRW;
    glbScreenHeight = HAM_SCRH;
#endif
    setResolution(glbScreenWidth, glbScreenHeight);
}

void
ham_ReloadTileGfx(tile_info_ptr tiledata, const u16 *data, int destidx, 
		  int numtile)
{
    int		 t;
    u8		*raw;
    u8		*dst;

    raw = (u8*) data;
    for (t = destidx; t < destidx + numtile; t++)
    {
	dst = tiledata->tiles[t];
	
	memcpy(dst, raw, TILEWIDTH*TILEHEIGHT);
	raw += TILEWIDTH*TILEHEIGHT;
    }
    glb_isdirty = true;
}

void
hamfake_ReloadSpriteGfx(const u16 *data, int tileno, int numtile)
{
    tileno /= 4;
    UT_ASSERT(tileno >= 0 && tileno < MAX_SPRITES);
    if (tileno < 0 || tileno >= MAX_SPRITES)
	return;

    memcpy(glbSpriteList[tileno].data, data, numtile * TILEWIDTH*TILEHEIGHT);
    glb_isdirty = true;
}

void
hamfake_LoadSpritePal(void *vpal, int bytes)
{
    // Need to promote our 15bit palette to a 24 bit one.
    int		entries = bytes / 2, i;
    u16		*pal = (u16 *) vpal;

    for (i = 0; i < entries; i++)
    {
	glb_palette[1024+i*4+3] = (pal[i] & 31) << 3;
	glb_palette[1024+i*4+2] = ((pal[i] >> 5) & 31) << 3;
	glb_palette[1024+i*4+1] = ((pal[i] >> 10) & 31) << 3;
	if (i)
	    glb_palette[1024+i*4] = 255;
	else
	    glb_palette[1024+i*4] = 0;
    }
    glb_isdirty = true;
}

void
hamfake_softReset()
{
    hamfake_postShutdown();
#ifdef iPOWDER
#else
    SDL_Quit();
#endif
    exit(0);
}

bool
hamfake_extratileset()
{
    return ham_extratileset;
}

void
hamfake_setstyluspos(bool state, int x, int y)
{
    // Since we are threading, we want to make sure mouse up
    // occurs always before mouse down!
    if (!state)
    {
	glbStylusState = false;
    }
    else
    {
	glbStylusX = x;
	glbStylusY = y;
	glbStylusState = true;
    }
#ifdef iPOWDER
    glbOurEventQueue.append(1);
#endif
}

void
hamfake_getstyluspos(int &x, int &y)
{
    x = glbStylusX;
    y = glbStylusY;
    // Convert accordint to our scale.
    x -= glbScreenFudgeX;
    y -= glbScreenFudgeY;
    x /= glbScaleFactor;
    y /= glbScaleFactor;
    // After conversion we can be out of the valid bounds.
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;
    if (x >= HAM_SCRW)
	x = HAM_SCRW-1;
    if (y >= HAM_SCRH)
	y = HAM_SCRH-1;

    // Now return to the GBA coordinate system by adjusting for the DS coords.
    x -= TILEWIDTH;
#ifdef iPOWDER
    y -= 3*TILEHEIGHT;
#else
    y -= 2*TILEHEIGHT;
#endif
}

bool
hamfake_getstylusstate()
{
    return glbStylusState;
}

void
hamfake_movesprite(int spriteno, int x, int y)
{
    UT_ASSERT(spriteno >= 0 && spriteno < MAX_SPRITES);
    if (spriteno < 0 || spriteno >= MAX_SPRITES)
	return;
    glb_isdirty = true;
    glbSpriteList[spriteno].x = x;
    glbSpriteList[spriteno].y = y;
}

void
hamfake_enablesprite(int spriteno, bool enabled)
{
    UT_ASSERT(spriteno >= 0 && spriteno < MAX_SPRITES);
    if (spriteno < 0 || spriteno >= MAX_SPRITES)
	return;
    glb_isdirty = true;
    glbSpriteList[spriteno].active = enabled;
}

void
hamfake_move(const char *dst, const char *src)
{
    BUF		fulldst, fullsrc;

#ifdef _WIN32_WCE
    CreateDirectory(L"\\My Documents", NULL);
    CreateDirectory(L"\\My Documents\\POWDER", NULL);
    fulldst.sprintf("\\My Documents\\POWDER\\%s", dst);
    fullsrc.sprintf("\\My Documents\\POWDER\\%s", src);
#else
#ifdef iPOWDER
    if (glbOurDataPath)
    {
	fullsrc.sprintf("%s/%s", glbOurDataPath, src);
	fulldst.sprintf("%s/%s", glbOurDataPath, dst);
    }
    else
	return;
#else
    fullsrc.strcpy(src);
    fulldst.strcpy(dst);
#endif
#endif

    ::remove(fulldst.buffer());
    ::rename(fullsrc.buffer(), fulldst.buffer());
}

FILE *
hamfake_fopen(const char *path, const char *mode)
{
#ifdef _WIN32_WCE
    // No concept of current directory in Windows CE, so we
    // hard code into their documents.
    BUF		fullpath;
    CreateDirectory(L"\\My Documents", NULL);
    CreateDirectory(L"\\My Documents\\POWDER", NULL);
    fullpath.sprintf("\\My Documents\\POWDER\\%s", path);

    return fopen(fullpath.buffer(), mode);
#else
#ifdef iPOWDER
    // Prepend our given global path, if it exists.
    BUF		fullpath;

    // If no data path set yet, we shouldn't write as we should
    // not rely on the sandbox being secure.
    if (glbOurDataPath)
	fullpath.sprintf("%s/%s", glbOurDataPath, path);
    else
	return 0;
    return fopen(fullpath.buffer(), mode);
#else
    // Use the local directory on these platforms.
    return fopen(path, mode);
#endif
#endif
}

bool
hamfake_fatvalid()
{
    return true;
}
