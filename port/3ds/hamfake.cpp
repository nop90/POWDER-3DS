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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

using namespace std;

#include "mygba.h"
#include "hamfake.h"
#include "3ds.h"
#include "sf2d.h"

#include "../../gfxengine.h"
#include "../../bmp.h"
#include "../../assert.h"
#include "../../control.h"

#define SRAMSIZE 65536

#define KEY_REPEAT_INITIAL 15
#define KEY_REPEAT_AFTER 7

#define MAX_JOY_BUTTON 12

//
// Global GBA state:
//
sf2d_texture *glbVideoSurface;
bool		 glbFullScreen = false;
int		 glbScreenWidth = HAM_SCRW;
int		 glbScreenHeight = HAM_SCRH;
int		 glbScaleFactor = 1;
int		 glbScreenFudgeX = 0;
int		 glbScreenFudgeY = 0;

int		 glbStylusX = 0, glbStylusY = 0;
bool		 glbStylusState = false;

bool		 glbJoyState[MAX_JOY_BUTTON];

extern int	glb_newframe; // from gfxengine.c

BUTTONS	 glbJoyToButton[MAX_JOY_BUTTON] =
{
    BUTTON_Y,	
    BUTTON_B,	
    BUTTON_A,	
    BUTTON_X,	
    BUTTON_L,
    BUTTON_R,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_UP,
    BUTTON_RIGHT,
    BUTTON_SELECT,
    BUTTON_START
};

u32	 hidButton[MAX_JOY_BUTTON] =
{
    KEY_Y,	
    KEY_B,	
    KEY_A,	
    KEY_X,	
    KEY_L,
    KEY_R,
    KEY_DDOWN,
    KEY_DLEFT,
    KEY_DUP,
    KEY_DRIGHT,
    KEY_SELECT,
    KEY_START
};


struct SPRITEDATA
{
  bool 	active;
  int 	x, y;
  u8		data[4*8*8];
};

#define MAX_SPRITES 128


SPRITEDATA	 *glbSpriteList;


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
  if(glbVideoSurface) sf2d_free_texture(glbVideoSurface);
  glbVideoSurface = sf2d_create_texture (HAM_SCRW, HAM_SCRH, TEXFMT_RGBA8, SF2D_PLACE_RAM);

  if (!glbVideoSurface) 
  {
    printf("Failed to create screen\n");
    //N3ds_Quit();
    return;
  }
  sf2d_texture_set_params(glbVideoSurface, GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR));
  sf2d_texture_tile32(glbVideoSurface);

  glb_isdirty = true;
}

void
hamfake_setFullScreen(bool fullscreen)
{
  // Pretty straight forward :>
  glbFullScreen = true;
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
  if(glbFullScreen) {
	glbScreenWidth = 400;
	glbScreenHeight = 240;
    glbScaleFactor= 1;
  } else {
	glbScreenWidth = 320;
	glbScreenHeight = 240;
    glbScaleFactor= 1;
  }

//  glbScreenWidth = width;
//  glbScreenHeight = height;

  // Calculate our fudge factor...
  glbScreenFudgeX = ((400 - glbScreenWidth) / 2);
  glbScreenFudgeY = 0;

  // Take effect!
//  rebuildVideoSystemFromGlobals();
}

void
scaleScreenFromPaletted(u8 *dst, int pitch)
{
  int		x, y;
  u16		*src;

  src = glb_nativescreen;
  for (y = 0; y < HAM_SCRH; y++){
      for (x = 0; x < HAM_SCRW; x++) 
        sf2d_set_pixel( glbVideoSurface,x,y,RGBA8(0xff, glb_palette[src[x]*4+3], glb_palette[src[x]*4+2], glb_palette[src[x]*4+1]));
	  src+=HAM_SCRW;
	}
	sf2d_start_frame(GFX_TOP, GFX_LEFT);
	sf2d_draw_texture_scale(glbVideoSurface, glbScreenFudgeX, 0, glbFullScreen?1.5625:1.25 ,1.25);
	sf2d_end_frame();
	sf2d_swapbuffers();
}

void
scaleScreenFrom15bit(u8 *dst, int pitch)
{
  int		x, y;
  u16		*src;

  src = glb_nativescreen;

  for (y = 0; y < HAM_SCRH; y++) {
      for (x = 0; x < HAM_SCRW; x++)
        sf2d_set_pixel( glbVideoSurface,x,y,RGBA8(0xff, ((src[x]>> 10) & 31) << 3, ((src[x]>> 5) & 31) << 3, (src[x]& 31) << 3)); //wrong order
	  src+=HAM_SCRW;
	}
	sf2d_start_frame(GFX_TOP, GFX_LEFT);
	sf2d_draw_texture_scale(glbVideoSurface, glbScreenFudgeX, 0, glbFullScreen?1.5625:1.25 ,1.25);
	sf2d_end_frame();
	sf2d_swapbuffers();
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

/*
  if (!glb_isdirty)
    return;
  glb_isdirty = false;
*/
  if (glb_videomode == 3)
  {
    u8		*dst = 0;

    if (glbVideoSurface)
    {
      dst = (u8 *) glbVideoSurface;
      scaleScreenFrom15bit(dst, 0);
	  // nop90: cosider to flush texture memory here
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

    if (glbVideoSurface)
    {
      dst = (u8 *) glbVideoSurface;
      scaleScreenFromPaletted(dst, 0);
	  // nop90: cosider to flush texture memory here
    }

  }
}

// Wait for an event to occur.
void
hamfake_awaitEvent()
{
//nop90
	do {
// we need here to put  a sleepthread call...
		aptMainLoop(); // if(!aptMainLoop()) exit=1;
// ... an here handling home menu event
		hidScanInput();
	} while (!hidKeysDown() && !hidKeysUp());
//  SDL_WaitEvent(0);
	glb_newframe = 1; // this is dirty, should be set only every 1/60 of second
}

// Return our internal screen.
u16 *
hamfake_lockScreen()
{
  return glb_nativescreen;//glb_rawscreen;
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

  fp = fopen("/3ds/Powder/powder.sav", "wb");

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
//nop90
	unsigned int keydown = hidKeysDown();
	unsigned int keyup = hidKeysUp();
	int i;
	for (i=0;i<MAX_JOY_BUTTON;i++) {
		if (keydown & hidButton[i]) glbJoyState[i] = true;
		if (keyup & hidButton[i]) glbJoyState[i] = false;
	}
/*
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
*/
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

  glbFullScreen = true;

/*
  for (i = 0; i < MAX_SPRITES; i++)
  {
    glbSpriteList[i].active = false;
  }
*/
  memset(glbJoyState, 0, MAX_JOY_BUTTON * sizeof(bool));

  // Load any save games.
  FILE	*fp;

  fp = fopen("/3ds/powder/powder.sav", "rb");
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

/*
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
*/

void
ham_StartIntHandler(u8 intno, void (*fp)())
{
//  assert(intno == INT_TYPE_VBL);

//  glb_vblcallback = fp;
//  SDL_AddTimer(16, glb_VBLTimerCallback, 0);
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
//  UT_ASSERT(tileno >= 0 && tileno < MAX_SPRITES);
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
//  SDL_Quit();
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
char buf[256]; 
  // Use the local directory on these platforms.
  if (path[0]=='/') //3DS Old Hack. Not usng romfs anymore
	return fopen(path, mode);
  else {
	sprintf(buf,"romfs:/%s",path);
	return fopen(buf, mode);
  }
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
