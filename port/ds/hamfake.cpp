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

#include <nds.h>
#include <fat.h>

#include <sys/stat.h>

#define SRAMSIZE 65536

#define KEY_REPEAT_INITIAL 15
#define KEY_REPEAT_AFTER 7

//
// Global GBA state:
//

bg_info			ham_bg[4];

int			ham_vramtop = 0;
u8			ham_winin[2], ham_winout[2];
bool			ham_fatvalid = false;

bool			ham_extratileset = false;

short			glbStylusX, glbStylusY;
bool			glbStylusState;

// It is important to use this rather than the SpriteEntry
// defined in sprite.h because it uses a union of enums and
// we don't allow short enums, so it will have the entirely
// wrong size.
struct POWDER_SpriteEntry
{
    u16 attribute[4];
};

static const int POWDER_SPRITES = 128;
POWDER_SpriteEntry		glbSpriteList[POWDER_SPRITES];
bool			glbSpriteDirty = false;

// This is the path, including trailing slash, where we will
// save all of our data such as .SAV files and character dumps.
// If it doesn't exist, we create it on start up.  If creation
// fails, we dump a warning and use /
const char		*glbAbsoluteDataPath;

// Keyboard state:

void
hamfake_rebuildScreen()
{
}

void
updateOAM()
{
    if (!glbSpriteDirty)
	return;

    // Compiling without short-enums can cause problems with DS
    // data structures.
    DC_FlushRange(glbSpriteList,POWDER_SPRITES*sizeof(POWDER_SpriteEntry));
    dmaCopy(glbSpriteList, OAM, POWDER_SPRITES*sizeof(POWDER_SpriteEntry));
    
    glbSpriteDirty = false;
}

// Wait for an event to occur.
void
hamfake_awaitEvent()
{
    // Check for the lid going down, in which case we power down until
    // it goes up...
    if (ctrl_rawpressed(BUTTON_LID))
    {
	u16		powerstatus;

	powerstatus = REG_POWERCNT;
	REG_POWERCNT = 0;
	while (ctrl_rawpressed(BUTTON_LID))
	{
	    // Can't await frame here as we are already doing so :>
	    // Note that they await a VGL,
	    // perhaps that would halt the porcessor
	    // for more savings?
	    // Apparently it does, which is why we added it to
	    // the main loop but foolishly forgot it in the powerdown loop,
	    // ironically thus consuming less CPU power when the lid
	    // is open as opposed to closed.
	    swiWaitForVBlank();
	}
	REG_POWERCNT = powerstatus;
    }
    updateOAM();
    swiWaitForVBlank();
}

// Return our internal screen.
u16 *
hamfake_lockScreen()
{
    // Because we are the very epitome of laziness,
    // we give the proper offset to center this GBA style
    // rather than fixing everything else.
    return VRAM_A + 8 + 16 * 256;
}

void
hamfake_unlockScreen(u16 *)
{
}

char *glbSRAM = 0;

// Deal with our SRAM buffer.
char *
hamfake_writeLockSRAM()
{
    // TODO: determine if we are proper PASSME or not before
    // writing to SRAM.
    if (!glbSRAM)
    {
	glbSRAM = (char *)malloc(SRAMSIZE);

	// Read our .sav file from fat if possible.
	if (ham_fatvalid)
	{
	    FILE	*fp;

	    fp = hamfake_fopen("powder.sav", "rb");
	    if (!fp)
	    {
		iprintf("No save file found.\n");
	    }
	    else
	    {
		fread(glbSRAM, SRAMSIZE, 1, fp);
		fclose(fp);
	    }
	}
    }
    return glbSRAM;
}

void
hamfake_writeUnlockSRAM(char *)
{
}

char *
hamfake_readLockSRAM()
{
    return hamfake_writeLockSRAM();
}

void
hamfake_readUnlockSRAM(char *)
{
}

void
hamfake_endWritingSession()
{
    // Why the hell do I have a write unlock if there is an endWritingSession?
    FILE		*fp;

    fp = hamfake_fopen("powder.sav", "wb");

    if (!fp)
    {
	iprintf("Failure to open powder.sav for writing!\n");
	return;
    }

    fwrite(glbSRAM, SRAMSIZE, 1, fp);

    fclose(fp);
}

bool
hamfake_fatvalid()
{
    return ham_fatvalid;
}

// Called to run our event poll
void
hamfake_pollEvents()
{
    updateOAM();
    scanKeys();
    touchPosition	pos = touchReadXY();
    glbStylusX = pos.px;
    glbStylusY = pos.py;
    glbStylusState = (keysHeld() & KEY_TOUCH) ? true : false;
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

	case BUTTON_X:
	    return keysHeld() & KEY_X;
	case BUTTON_Y:
	    return keysHeld() & KEY_Y;

	case BUTTON_TOUCH:
	    return keysHeld() & KEY_TOUCH;

	case BUTTON_LID:
	    return keysHeld() & KEY_LID;

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
    if (fatInitDefault())
    {
	iprintf("FAT system initialized\n");
	ham_fatvalid = true;

	// Determine our save directory
	struct stat		buffer;
	int			status;

	status = stat("/data", &buffer);
	if (status < 0)
	{
	    // An error, try to make the directory.
	    mkdir("/data", S_IRWXU | S_IRWXG | S_IRWXO);
	}
	status = stat("/data/powder", &buffer);
	if (status < 0)
	{
	    // Again, an error, so try another make.  I don't
	    // think mkdir will make two...
	    mkdir("/data/powder", S_IRWXU | S_IRWXG | S_IRWXO);
	}

	// And stat again..
	status = stat("/data/powder", &buffer);

	if (!status && S_ISDIR(buffer.st_mode))
	{
	    // Everything is cool!
	    glbAbsoluteDataPath = "/data/powder/";
	}
	else
	{
	    // Failed to create directory...
	    iprintf("Failed to find/create /data/powder.  POWDER's data will be saved to the root of your flash card.\n");
	}
	
	// Attempt to load foreign images.
	if (bmp_loadExtraTileset())
	{
	    ham_extratileset = true;
	    iprintf("Extra tileset loaded.\n");
	}
    }
    else
    {
	iprintf("No FAT system found - save disabled\n");
	ham_fatvalid = false;
    }

    int		i;
    for (i = 0; i < POWDER_SPRITES; i++)
    {
	glbSpriteList[i].attribute[0] = ATTR0_DISABLED;
	glbSpriteList[i].attribute[1] = ATTR1_SIZE_16;
	glbSpriteList[i].attribute[2] = 0;
	glbSpriteList[i].attribute[3] = 0;
    }
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
#if 1
	    vramSetBankA(VRAM_A_LCD);
	    videoSetMode(MODE_FB0 |
		    DISPLAY_SPR_ACTIVE |
		    DISPLAY_SPR_1D |
		    DISPLAY_SPR_1D_BMP);
	    // Desire a flat memory model.
#else
	    videoSetMode(MODE_3_2D |
			DISPLAY_BG0_ACTIVE |
			DISPLAY_BG1_ACTIVE |
			DISPLAY_BG2_ACTIVE |
			DISPLAY_BG3_ACTIVE);
#endif
	    break;

	case 0:
	    // Desire tiled mode.
	    vramSetBankA(VRAM_A_MAIN_BG);
	    vramSetBankE(VRAM_E_MAIN_SPRITE);
	    videoSetMode(MODE_0_2D |
			DISPLAY_SPR_ACTIVE |
			DISPLAY_BG0_ACTIVE |
			DISPLAY_BG1_ACTIVE |
			DISPLAY_BG2_ACTIVE |
			DISPLAY_BG3_ACTIVE |
			DISPLAY_SPR_1D |	// I am too old school.
			DISPLAY_SPR_1D_BMP |
			DISPLAY_WIN0_ON);

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

void
hamfake_LoadSpritePal(void *vpal, int bytes)
{
    u16		*pal = (u16 *) vpal;
    bytes /= 2;
    int		i;

    for (i = 0; i < bytes; i++)
    {
	// The sprite palette could theoritically be entirely independent,
	// but since we'll likely just be grabbing tiles from the shared
	// graphics list...
	SPRITE_PALETTE[i] = pal[i];
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
	mapsize = BG_64x64;
    else
	mapsize = BG_32x32;
    
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
	    WIN0_X0 = x1;
	    WIN0_X1 = x2;
	    WIN0_Y0 = y1;
	    WIN0_Y1 = y2;
	    break;
	case 1:
	    WIN1_X0 = x1;
	    WIN1_X1 = x2;
	    WIN1_Y0 = y1;
	    WIN1_Y1 = y2;
	    break;
    }
    ham_winin[wid] = inside;
    ham_winout[wid] = outside;
    // Update control registers.
    WIN_IN = ham_winin[0] | (ham_winin[1] << 8);
    // This is wrong.
    WIN_OUT = ham_winout[0] | (ham_winout[1] << 8);
}

void
ham_DeleteWin(int wid)
{
    // Allow everything inside & outside.
    ham_winin[wid] = WIN_BG0 | WIN_BG1 | WIN_BG2 | WIN_BG3 | WIN_OBJ;
    ham_winout[wid] = WIN_BG0 | WIN_BG1 | WIN_BG2 | WIN_BG3 | WIN_OBJ;
    WIN_IN = ham_winin[0] | (ham_winin[1] << 8);
    WIN_OUT = ham_winout[0] | (ham_winout[1] << 8);
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
hamfake_ReloadSpriteGfx(const u16 *data, int destidx, int numtile)
{
    u16		*dest = &SPRITE_GFX[0];
    int		 i;
    
    dest += destidx * 4 * 8;
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
    return ham_extratileset;
}

void
hamfake_getstyluspos(int &x, int &y)
{
    x = glbStylusX;
    y = glbStylusY;

    // Stylus positions are returned in the original GBA window,
    // not the extended window of the DS!
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
    // The caller is specifying in GBA space.
    x += 8;
    y += 16;

    // Clear out our old coord.
    glbSpriteList[spriteno].attribute[0] &= ~255;
    glbSpriteList[spriteno].attribute[1] &= ~511;

    // Merge in new coord.
    y &= 255;
    x &= 511;
    glbSpriteList[spriteno].attribute[0] |= y;
    glbSpriteList[spriteno].attribute[1] |= x;

    glbSpriteDirty = true;
}

void
hamfake_enablesprite(int spriteno, bool enabled)
{
    // Change the flag to reflect the new value.
    // Track if dirty to save cycles.
    if (enabled)
    {
	int		y;
	y = glbSpriteList[spriteno].attribute[0] & 255;
	glbSpriteList[spriteno].attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE;
	glbSpriteList[spriteno].attribute[0] |= y;
	// We want sprites to always be under the text.
	// Exception is the cursor.
	if (spriteno)
	    glbSpriteList[spriteno].attribute[2] = 8 * spriteno | ATTR2_PRIORITY(1);
	else
	    glbSpriteList[spriteno].attribute[2] = 8 * spriteno;
	glbSpriteDirty = true;
    }
    else
    {
	glbSpriteList[spriteno].attribute[0] = ATTR0_DISABLED;
	glbSpriteDirty = true;
    }
}

FILE *
hamfake_fopen(const char *path, const char *mode)
{
    FILE		*result;

    // We want to use an absolute path.
    // Prefix /data/powder.
    char		*buf;
    buf = new char[strlen(path) + strlen(glbAbsoluteDataPath) + 5];
    sprintf(buf, "%s%s", glbAbsoluteDataPath, path);

    result = fopen(buf, mode);

    delete [] buf;

    return result;
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
