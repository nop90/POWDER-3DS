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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

using namespace std;

#include "mygba.h"
#include "hamfake.h"
#include "SDL.h"

#include "../../gfxengine.h"
#include "../../bmp.h"
#include "../../assert.h"
#include "../../control.h"

//#include "../../gfx/icon_sdl.bmp.c"

#define SRAMSIZE 65536

#define KEY_REPEAT_INITIAL 15
#define KEY_REPEAT_AFTER 7

SDL_Joystick *joystick = NULL;

#define MAX_JOY_BUTTON 12

//
// Global GBA state:
//
SDL_Surface	*glbVideoSurface;
bool		 glbFullScreen = false;
int		 glbScreenWidth = HAM_SCRW;
int		 glbScreenHeight = HAM_SCRH;
int		 glbScaleFactor = 1;
int		 glbScreenFudgeX = 0;
int		 glbScreenFudgeY = 0;

int		 glbStylusX = 0, glbStylusY = 0;
bool		 glbStylusState = false;

bool		 glbJoyState[MAX_JOY_BUTTON];

BUTTONS	 glbJoyToButton[MAX_JOY_BUTTON] =
{
    BUTTON_Y,	// Tri
    BUTTON_B,	// Circle
    BUTTON_A,	// Cross
    BUTTON_X,	// Square
    BUTTON_L,
    BUTTON_R,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_UP,
    BUTTON_RIGHT,
    BUTTON_SELECT,
    BUTTON_START
};

struct SPRITEDATA
{
  bool 	active;
  int 	x, y;
  u8		data[4*8*8];
};

SPRITEDATA	 *glbSpriteList;

#define MAX_SPRITES 128

// Do we have an extra tileset loaded?
bool		 ham_extratileset = false;


// Background state:
bg_info			ham_bg[4];

bool			glb_isnewframe = 0, glb_isdirty = false;

u8			glb_palette[2048];

// Mode 3 screen - a raw 15bit bitmap.
u16			glb_rawscreen[HAM_SCRW*HAM_SCRH];

// Paletted version of screen which is scaled into final SDL surface
// We use u16 as the high bit selects whether to use sprite or
// global palette
u16			glb_nativescreen[HAM_SCRW*HAM_SCRH];

char			glb_rawSRAM[SRAMSIZE];

int			glb_videomode = -1;

void
rebuildVideoSystemFromGlobals()
{
  int		flags = SDL_RESIZABLE;

  if (glbFullScreen)
    flags |= SDL_FULLSCREEN;

  // do resize, no fullscreen
  flags = SDL_SWSURFACE;// | SDL_DOUBLEBUF;

  // Yes, we want a guaranteed 24bit video mode.
  glbVideoSurface = SDL_SetVideoMode(glbScreenWidth, glbScreenHeight,
                                     32, flags);

  if (!glbVideoSurface)
  {
    //printf("Failed to create screen: %s\n", SDL_GetError());
    //SDL_Quit();
    return;
  }

  // Setup our window environment.
  //SDL_WM_SetCaption("POWDER", 0);

  // We want the cursor enabled now that you can click on stuff.
  // SDL_ShowCursor(SDL_DISABLE);

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
  dst += glbScreenFudgeX * 4 + glbScreenFudgeY * pitch;

  src = glb_nativescreen;
  for (y = 0; y < HAM_SCRH; y++)
  {
    for (s_y = 0; s_y < glbScaleFactor; s_y++)
    {
      for (x = 0; x < HAM_SCRW; x++)
      {
        // Read in a pixel & decode
        idx = src[x];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

        pixel[0] = glb_palette[idx*4+3];
        pixel[1] = glb_palette[idx*4+2];
        pixel[2] = glb_palette[idx*4+1];
#else

        pixel[0] = glb_palette[idx*4+1];
        pixel[1] = glb_palette[idx*4+2];
        pixel[2] = glb_palette[idx*4+3];
#endif

        // Now write out the needed number of times...
        for (s_x = 0; s_x < glbScaleFactor; s_x++)
        {
          *dst++ = pixel[2];
          *dst++ = pixel[1];
          *dst++ = pixel[0];
          *dst++ = 0xff;
        }
      }
      // Add remainder of pitch.
      dst += pitch - 4 * glbScaleFactor * HAM_SCRW;
    }
    src += HAM_SCRW;
  }

  //SDL_Flip(glbVideoSurface);
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
  dst += glbScreenFudgeX * 4 + glbScreenFudgeY * pitch;

  src = glb_rawscreen;
  for (y = 0; y < HAM_SCRH; y++)
  {
    for (s_y = 0; s_y < glbScaleFactor; s_y++)
    {
      for (x = 0; x < HAM_SCRW; x++)
      {
        // Read in a pixel & decode
        raw = src[x];

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

        pixel[0] = (raw & 31) << 3;
        pixel[1] = ((raw >> 5) & 31) << 3;
        pixel[2] = ((raw >> 10) & 31) << 3;
#else

        pixel[2] = (raw & 31) << 3;
        pixel[1] = ((raw >> 5) & 31) << 3;
        pixel[0] = ((raw >> 10) & 31) << 3;
#endif


        // Now write out the needed number of times...
        for (s_x = 0; s_x < glbScaleFactor; s_x++)
        {
          *dst++ = pixel[2];
          *dst++ = pixel[1];
          *dst++ = pixel[0];
          *dst++ = 0xff;
        }
      }
      // Add remainder of pitch.
      dst += pitch - 4 * glbScaleFactor * HAM_SCRW;
    }
    src += HAM_SCRW;
  }

  //SDL_Flip(glbVideoSurface);
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

    for (sy = 0; sy < 16; sy++)
    {
      for (sx = 0; sx < 16; sx++)
      {
        x = sprite.x + sx;
        y = sprite.y + sy;
        // Convert to DS coords from GBA
        x += 8;
        y += 16;
        tx = sx >> 3;
        ty = sy >> 3;
        srcoffset = (sx & 7) + tx * 8 * 8 +
                    ty * 2 * 8 * 8 + (sy & 7) * 8;
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
  int			i, layer, x, y, tileidx, tx, ty;
  int			offset;

  if (!glb_isdirty)
    return;

  glb_isdirty = false;

  if (glb_videomode == 3)
  {
    u8		*dst = 0;

    //SDL_LockSurface(glbVideoSurface);

    if (glbVideoSurface)
    {
      dst = (u8 *) glbVideoSurface->pixels;
      if (!dst)
      {
        //printf("Lock failure: %s\n", SDL_GetError());
      }

      scaleScreenFrom15bit(dst, glbVideoSurface->pitch);

      //SDL_UnlockSurface(glbVideoSurface);
      // Rebuild from 15bit screen.
      SDL_UpdateRect(glbVideoSurface, 0, 0, 0, 0);
    }

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

    ty = ham_bg[layer].scrolly / 8;
    ty %= ham_bg[layer].mi->height;
    if (ty < 0)
      ty += ham_bg[layer].mi->height;
    for (y = 0; y < HAM_SCRH / 8; y++)
    {
      tx = ham_bg[layer].scrollx / 8;
      tx %= ham_bg[layer].mi->width;
      if (tx < 0)
        tx += ham_bg[layer].mi->width;

      for (x = 0; x < HAM_SCRW / 8; x++)
      {
        u8		*tile;

        tileidx = ham_bg[layer].mi->tiles[ty * ham_bg[layer].mi->width + tx];

        // Out of bound tiles are ignored.
        if ((tileidx & 1023) < ham_bg[layer].ti->numtiles)
        {
          tile = ham_bg[layer].ti->tiles[tileidx & 1023];

          // Compute offset into native screen
          offset = x * 8 + y * 8 * HAM_SCRW;

          // Figure out any flip flags.
          if (tileidx & (2048 | 1024))
          {
            int	    sx, sy, fx, fy;

            for (sy = 0; sy < 8; sy++)
            {
              if (tileidx & 2048)
                fy = 7 - sy;
              else
                fy = sy;

              for (sx = 0; sx < 8; sx++)
              {
                if (tileidx & 1024)
                  fx = 7 - sx;
                else
                  fx = sx;

                // Write if non-zero
                if (tile[fx + fy * 8])
                  glb_nativescreen[offset] = tile[fx + fy * 8];
                offset++;
              }
              offset += HAM_SCRW - 8;
            }
          }
          else
          {
            int		sx, sy;

            // Straight forward write...
            for (sy = 0; sy < 8; sy++)
            {
              for (sx = 0; sx < 8; sx++)
              {
                if (*tile)
                {
                  glb_nativescreen[offset] = *tile;
                }
                tile++;
                offset++;
              }
              offset += HAM_SCRW - 8;
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
    u8		*dst = 0;

    //SDL_LockSurface(glbVideoSurface);
    if (glbVideoSurface)
    {
      dst = (u8 *) glbVideoSurface->pixels;
      if (!dst)
      {
        //printf("Lock failure: %s\n", SDL_GetError());
      }

      scaleScreenFromPaletted(dst, glbVideoSurface->pitch);

      //SDL_UnlockSurface(glbVideoSurface);
      // Rebuild from 8bit screen.
      SDL_UpdateRect(glbVideoSurface, 0, 0, 0, 0);
    }

  }
}

// Wait for an event to occur.
void
hamfake_awaitEvent()
{
  SDL_WaitEvent(0);
}

// Return our internal screen.
u16 *
hamfake_lockScreen()
{
  // Note we have to offset to the normal GBA start
  // location to account for our official offset.
  return glb_rawscreen + 8 + 16 * 256;
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
{}

char *
hamfake_readLockSRAM()
{
  return glb_rawSRAM;
}

void
hamfake_readUnlockSRAM(char *)
{}

void
hamfake_endWritingSession()
{
  FILE		*fp;

  fp = fopen("powder.sav", "wb");

  if (!fp)
  {
    cerr << "Failure to open powder.sav for writing!" << endl;
    return;
  }

  fwrite(glb_rawSRAM, SRAMSIZE, 1, fp);

  fclose(fp);
}


int getJoyButton(BUTTONS button)
{
    int		i;

    for (i = 0; i < MAX_JOY_BUTTON; i++)
	if (glbJoyToButton[i] == button)
	    return i;
    return -1;
}

// Called to run our event poll
void
hamfake_pollEvents()
{
  SDL_Event	event;

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_VIDEORESIZE:
      setResolution(event.resize.w, event.resize.h);
      break;

    case SDL_JOYBUTTONDOWN:
      // If button press is in range, set our state
      if (event.jbutton.button < MAX_JOY_BUTTON)
	  glbJoyState[event.jbutton.button] = true;
      break;
    case SDL_JOYBUTTONUP:
      if (event.jbutton.button < MAX_JOY_BUTTON)
	  glbJoyState[event.jbutton.button] = false;
      break;

    case SDL_QUIT:
      SDL_Quit();
      exit(0);
      break;
    }
  }
}


bool
hamfake_isPressed(BUTTONS button)
{
    int		bnum;

    hamfake_pollEvents();
    hamfake_rebuildScreen();

    bnum = getJoyButton(button);
    if (bnum < 0)
	return false;

    return glbJoyState[bnum];
}

int
hamfake_isAnyPressed()
{
  hamfake_pollEvents();
  hamfake_rebuildScreen();

  int			i;
  for (i = 0; i < MAX_JOY_BUTTON; i++)
    if (glbJoyState[i])
      return 0x0;

  return 0x3FF;
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
}

void
hamfake_clearKeyboardBuffer()
{
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

void
ham_Init()
{
  int		i;

  printf("\nPOWDER initializing...\n");

  /* initialize SDL */
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0 )
  {
    printf("Video initialization failed: %s\n", SDL_GetError( ) );
    return;
  }

  SDL_ShowCursor(0);
  
  if(SDL_NumJoysticks())
  {
    joystick = SDL_JoystickOpen(0);
  }
  else
  {
    printf("no joystick detected\n");
  }

  SDL_WM_SetCaption("Powder", 0);
  SDL_Delay(1000);

  glbSpriteList = new SPRITEDATA[MAX_SPRITES];
  for (i = 0; i < MAX_SPRITES; i++)
  {
    glbSpriteList[i].active = false;
  }

  memset(glbJoyState, 0, MAX_JOY_BUTTON * sizeof(bool));

  // Load any save games.
  FILE	*fp;

  fp = fopen("powder.sav", "rb");
  if (fp)
  {
    fread(glb_rawSRAM, SRAMSIZE, 1, fp);
    fclose(fp);
  }

  // Load any extra tilesets.
  if (bmp_loadExtraTileset())
  {
    ham_extratileset = true;
    printf("External tileset loaded.\n");
  }

  return;
}

void (*glb_vblcallback)() = 0;

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

void
ham_StartIntHandler(u8 intno, void (*fp)())
{
  assert(intno == INT_TYPE_VBL);

  glb_vblcallback = fp;
  SDL_AddTimer(16, glb_VBLTimerCallback, 0);
}

void
ham_SetBgMode(u8 mode)
{
  if (glb_videomode == -1)
  {
    setResolution(HAM_SCRW * 2, HAM_SCRH * 2);
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
    ti->tiles[i] = new u8 [8 * 8];
  }
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
{}

void
ham_DeleteWin(int wid)
{}

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

    memcpy(dst, raw, 8*8);
    raw += 8 * 8;
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

  memcpy(&glbSpriteList[tileno].data, data, numtile * 8 * 8);
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
  SDL_Quit();
  exit(0);
}

bool
hamfake_extratileset()
{
  return ham_extratileset;
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
  x -= 8;
  y -= 16;
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

FILE *
hamfake_fopen(const char *path, const char *mode)
{
  // Use the local directory on these platforms.
  return fopen(path, mode);
}

bool
hamfake_fatvalid()
{
    return true;
}

void
hamfake_setTileSize(int width, int height)
{
}

int
hamfake_getKeyModifiers()
{
    return 0;
}
