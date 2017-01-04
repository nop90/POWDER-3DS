/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        gfxengine.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	Platform independent graphics engine.
 * 	The mentioned tiles are the tile number in the resource files.
 * 	The world consists of 32x32, 16x16 tiles.
 */

#ifndef __gfxengine_h__
#define __gfxengine_h__

#include "glbdef.h"
#include "buf.h"

// These are special symbols in our character set.  The SYMBOLSTRING
// version is "".
#define SYMBOL_HEART		'\200'
#define SYMBOLSTRING_HEART	"\200"
#define SYMBOL_MAGIC		'\201'
#define SYMBOLSTRING_MAGIC	"\201"
#define SYMBOL_LEFT		'\202'
#define SYMBOLSTRING_LEFT	"\202"
#define SYMBOL_RIGHT		'\203'
#define SYMBOLSTRING_RIGHT	"\203"
#define SYMBOL_UP		'\204'
#define SYMBOLSTRING_UP		"\204"
#define SYMBOL_DOWN		'\205'
#define SYMBOLSTRING_DOWN	"\205"
#define SYMBOL_NEXT		'\206'
#define SYMBOLSTRING_NEXT	"\206"
#define SYMBOL_DLEVEL		'\207'
#define SYMBOLSTRING_DLEVEL	"\207"
#define SYMBOL_AC		'\210'
#define SYMBOLSTRING_AC		"\210"
#define SYMBOL_TRIDUDE		'\211'
#define SYMBOLSTRING_TRIDUDE	"\211"
#define SYMBOL_CURSOR		'\212'
#define SYMBOLSTRING_CURSOR	"\212"
#define SYMBOL_UNIQUE		'\213'
#define SYMBOLSTRING_UNIQUE	"\213"

// 230 in octal...
#define COLOURED_SYMBOLS	152

#define SYMBOL_STRENGTH		'S'
#define SYMBOLSTRING_STRENGTH	"S"
#define SYMBOL_SMARTS		'I'
#define SYMBOLSTRING_SMARTS	"I"
#define SYMBOL_EXP		'X'
#define SYMBOLSTRING_EXP	"X"

// Key defines.
#define GFX_KEYMODSHIFT		1
#define GFX_KEYMODCTRL		2
#define GFX_KEYMODALT		4

enum GFX_Keydefine
{
    GFX_KEYSTART = 256,
    GFX_KEYUP,
    GFX_KEYLEFT,
    GFX_KEYRIGHT,
    GFX_KEYDOWN,
    GFX_KEYCTRLUP,
    GFX_KEYCTRLLEFT,
    GFX_KEYCTRLRIGHT,
    GFX_KEYCTRLDOWN,
    GFX_KEYPGUP,
    GFX_KEYPGDOWN,
    GFX_KEYLMB,
    GFX_KEYMMB,
    GFX_KEYRMB,
    // We expect these to be contiguous.
    GFX_KEYF1,
    GFX_KEYF2,
    GFX_KEYF3,
    GFX_KEYF4,
    GFX_KEYF5,
    GFX_KEYF6,
    GFX_KEYF7,
    GFX_KEYF8,
    GFX_KEYF9,
    GFX_KEYF10,
    GFX_KEYF11,
    GFX_KEYF12,
    GFX_KEYF13,
    GFX_KEYF14,
    GFX_KEYF15,
    GFX_KEYLAST
};

class MAP;
class MOB;

// Global defines...
#define MAP_WIDTH	32
#define MAP_HEIGHT	32

#define INVALID_TILEIDX		65535

//
// Fake ham functions used by SDL code.
//
void hamfake_rebuildScreen();
void hamfake_awaitEvent();

// Locks the screen for writing as a 240x160x16bit plane.
u16 *hamfake_lockScreen();
void hamfake_unlockScreen(u16 *screen);

// For systems that support it, get a keypress.
int hamfake_peekKeyPress();
int hamfake_getKeyPress(bool onlyascii);
void hamfake_clearKeyboardBuffer();
void hamfake_insertKeyPress(int key);
int hamfake_getKeyModifiers();

// Returns the current stylus/mouse location.
// Returned in terms of screen pixels
void hamfake_getstyluspos(int &x, int &y);

// Returns if the stylus is currently pressed or not.
// If not pressed, you should not trust the pos!
bool hamfake_getstylusstate();

// Returns if an extra tileset is present in the system
bool hamfake_extratileset();

// Reboots the machine.
void hamfake_softReset();

// Returns true if the external UI has demanded POWDER to shutdown.
bool hamfake_forceQuit();

// External UI directions.
void hamfake_flushdir();
bool hamfake_externaldir(int &dx, int &dy);

void hamfake_buttonreq(int mode,	// 0 move, 1 action, 2 spell, 3 del
					// 4 defaults
					// 5 Direction request.
					// 6 Unlock request
			int type = 0);	// action/spell value.

// awaitShutdown is used by external UI to wait until POWDER has finsihed
// an emergency save & quit sparked by forceQuit.
// POWDER lets the ui know it is done with the postShutdown
void hamfake_awaitShutdown();
void hamfake_postShutdown();

void hamfake_setinventorymode(bool invmode);
void hamfake_enableexternalactions(bool enable);
void hamfake_externalaction(int &iact, int &ispell);
bool hamfake_isunlocked();

// Toggles full screen mode
void hamfake_setFullScreen(bool fullscreen);
bool hamfake_isFullScreen();

// Controls sprites by their OAM numbers.
void hamfake_movesprite(int spriteno, int x, int y);
void hamfake_enablesprite(int spriteno, bool enabled);

void hamfake_ReloadSpriteGfx(const u16 *data, int tileno, int numtile);
void hamfake_setTileSize(int tilewidth, int tileheight);

void hamfake_LoadSpritePal(void *data, int bytes);

#ifdef HAS_DISKIO
FILE *hamfake_fopen(const char *path, const char *mode);
bool hamfake_fatvalid();
#endif

void gfx_init();

void gfx_setmode(int mode);

// This finds a free tile block of the given size...
u16 gfx_findFreeTile(int size);

// These lock the tile, returning an index to be used in the map.
// Failure returns -1.  unlockTile frees it for further use.
// The actual tile allocated is passed back in result - that is the
// value that should be used in the free!
u16 gfx_lockTile(TILE_NAMES tile, TILE_NAMES *result);
void gfx_unlockTile(TILE_NAMES tile);

// Ensures the tile is locked at least given number.  The idea is that one
// intends to leave a stale lock pointer, but don't want the stale
// pointer to accumulate over time.
void gfx_forceLockTile(TILE_NAMES tile, int mincount = 1);

// These lock character tiles which are on the same tile array
// as the other tiles.
// The actual tile allocated is passed back in result - that is the
// value that should be used in the free!
u16 gfx_lockCharTile(u8 c, u8 *result);
u16 gfx_lockColourCharTile(u8 c, COLOUR_NAMES colour, u8 *result);
void gfx_unlockCharTile(u8 c);

// Looking up a tile will find the tile idx for the given name.
// It will not lock it.  This can be used for refresh, etc.
// If it returns the invalid index, the tile doesn't exist and the
// generic should be used.
u16 gfx_lookupTile(TILE_NAMES tile);

// This returns the number of unused tiles.
int gfx_getNumFreeTiles();

// Note these all work with TILE_NAMES, NOT the tile index.
void gfx_refreshtiles();
void gfx_settile(int tx, int ty, int tileno);
// This sets the overlay tile.
void gfx_setoverlay(int tx, int ty, int tileno);
int gfx_getoverlay(int tx, int ty);
// This sets the mob layer tiles
void gfx_setmoblayer(int tx, int ty, int tileno);
int gfx_getmoblayer(int tx, int ty);

// These set tiles using absolute coordinates starting from top left
// of the screen.  The screen is thus 17x12.
// These do NOT use, or set, the tile caches, so refreshtiles will
// undo them.
void gfx_setabstile(int tx, int ty, int tileno);
void gfx_setabsoverlay(int tx, int ty, int tileno);
void gfx_setabsmob(int tx, int ty, int tileno);

// This clears the given line of any text.
void gfx_cleartextline(int y, int startx=0);

// This prints the text on the screen relative coords x/y, which
// are in text coords, screen is 30x20.
void gfx_printtext(int x, int y, const char *text);
inline void gfx_printtext(int x, int y, BUF buf)
{ gfx_printtext(x, y, buf.buffer()); }

// This writes a character into a mode 3 buffer.
// x & y are pixel level offsets.
void gfx_printcharraw(int x, int y, char c);
void gfx_printcolourcharraw(int x, int y, char c);

// This also works in screen coords, writing a single character.
void gfx_printchar(int x, int y, char c);

// Remaps the character to a coloured character.
// Each coloured character has its own unique tile!
void gfx_printcolourchar(int x, int y, char c, COLOUR_NAMES colour);

// Returns the current center of the viewport in tiles
void gfx_getscrollcenter(int &cx, int &cy);

// This scrolls so tx/ty is the center tile in the viewport.
void gfx_scrollcenter(int tx, int ty);

// Adjust the pixel level scroll by the given number of pixels
// on the given layer.
void gfx_nudgecenter(int px, int py);

// This returns true if we are on a new frame, and then resets
// its new frame counter.
int gfx_isnewframe();

// This returns the current frame number.
int gfx_getframecount();

void gfx_displayminimap(MAP *level);
void gfx_hideminimap();

void gfx_showinventory(MOB *mob);
void gfx_hideinventory();
void gfx_getinvcursor(int &ix, int &iy);
void gfx_setinvcursor(int ix, int iy, bool onlyslots);

// Find the palette entry which most closely matches the given RGB.
int gfx_lookupcolor(int r, int g, int b);
int gfx_lookupcolor(int r, int g, int b, const u16 *palette);
// Find the palette entry for the given predefine.
int gfx_lookupcolor(COLOUR_NAMES color);

void gfx_updatespritegreytable();
const u8 *gfx_getspritegreytable();

// Pauses for the given number of vertical retraces.
void gfx_sleep(int vcycles);

// Displays a null terminated list.
// ylen is the maximum y to write to.
// Select will be vaguely centered and will have >< around it.
void gfx_displaylist(int x, int y, int ylen, const char **list, int select);

// Null terminated list which is prompted at x & y.  Returns -1
// if cancelled, otherwise, index into list.
// If the any button is set, it will select with R & L.  In this case,
// aorb is set to the appropraite BUTTON_ value.
// Actually, it only selects with R&L if disable paging is set, otherwise
// those are pageup/pagedown.  SELECT, on the other hand, is enabled
// with anykey.
// If an action bar is specified, it will become *live* and one can
// drag stuff too and from it.  
int gfx_selectmenu(int x, int y, const char **menu, int &aorb, int def=0,
		    bool anykey = false, bool disablepaging = false,
		    const u8 *menuactions = 0,
		    u8 *actionstrip = 0);

// Queries with on screen menu to confirm choice with yes/no prompt
// yes returns true, no false.
bool gfx_yesnomenu(const char *buf, bool defchoice = true);
inline bool gfx_yesnomenu(BUF buf, bool defchoice = true)
{ return gfx_yesnomenu(buf.buffer(), defchoice); }

// This takes a long list which it displays 30 chars at a time,
// allowing the user to scroll as necessary.
// Any key ends it. (and clears it)
void gfx_displayinfotext(const char *text);
inline void gfx_displayinfotext(BUF buf)
{ gfx_displayinfotext(buf.buffer()); }

// These routines allow you to dynamically create a long section of text.
// Each line can be greater than 30 chars and will word wrap properly.
// Nothing will be shown until the gfx_pager_display, at which
// point it will all be displayed in a scrollable view until
// a key is hit.  At that point, the temporary structures will
// all be wiped.
void
gfx_pager_addtext(const char *text);
void
gfx_pager_addtext(BUF buf);

// Adds a single line to the pager.  Forces this to start
// a new line, adds a new line afterwards.  If text is over 30 chars,
// it is truncated.
void
gfx_pager_addsingleline(const char *text);

void
gfx_pager_newline();

void
gfx_pager_reset();

// Adds a --- separator and a new line afterwards.
void
gfx_pager_separator();

// Sets the width of the pager, does not get auto reset so be careful
void
gfx_pager_setwidth(int width);

// Saves the pager data to a text file.  Also resets the pager.
void
gfx_pager_savetofile(const char *name);
inline void gfx_pager_savetofile(BUF buf) { gfx_pager_savetofile(buf.buffer()); }

// The given y value is the initial center of the text.
void
gfx_pager_display(int centery = 0);

// Returns true if a tile was selected, false if cancelled.
// tx & ty should start off at initial selection position.
// if quickselect is true, it will return immediately on the first
// movement key.  The bool in this case will be false until a or
// b is pressed.  aorb will be set to 0 if a pressed, 1 if b.
// Stylus is set to true if stylus used to do selection.
bool gfx_selecttile(int &tx, int &ty, bool quickselect=false, int *quickold=0,
		    int *aorb = 0, bool *stylus = 0);

// Assumes inventory currently displayed.  selectx & selecty are
// starting location to select from.  Returns true if user chose
// a new slot, false if they aborted.
bool gfx_selectinventory(int &selectx, int &selecty);
    
// Returns true if not cancelled.
// If dz is set, the user decided up or down.
// If nothing is set, they decided themselves.
// Otherwise, we have a standard direction.
bool gfx_selectdirection(int tx, int ty, int &dx, int &dy, int &dz);

// This copies the given mini tile on top of the given destination tile
// in graphics memory.
void gfx_copytiledata(TILE_NAMES desttile, MINI_NAMES minitile, bool ismale);

// This only works ontop of the last dest tile to be written to.
void gfx_compositetile(ITEMSLOT_NAMES dstslot, MINI_NAMES minitile, bool ismale);

// Allocates a tile and returns the contents, extracting from our
// graphics tile list.  Requires an explicit tileset as we don't
// want to end up with non-power of 2 suddenly.
// The result must be free()d
u8 *gfx_extractsprite(int tileset, SPRITE_NAMES tile);
u8 *gfx_extractlargesprite(SPRITE_NAMES tile, int grey, SPRITE_NAMES overlay);

void gfx_switchfonts(int newfont);
int gfx_getfont();

// We support two tile sets that external UIs can swap between at their
// leisure
void gfx_settilesetmode(int mode, int tileset);
int gfx_gettilesetmode(int mode);
void gfx_tilesetmode(int mode);

// These affect/refer to the current set for the current mode.
void gfx_switchtilesets(int newtileset);
int gfx_gettileset();
int gfx_gettilewidth();
int gfx_gettileheight();

// Cooridinates are in GBA space.
void gfx_drawcursor(int x, int y);
void gfx_cursortile(SPRITE_NAMES tile);
void gfx_removecursor();

// Note that you cannot mix sprite and dungeon tiles.  You need to
// use one system or the other.
void gfx_spritetile(int spriteno, SPRITE_NAMES tile);

// The sprite will be greyd out according to the grey variable.
// grey of 0 means entirely grey scale, 256 means full colour.
// greying starts from the top of the tile.
// The overlay tile will then be comped on top.
void gfx_spritetile_grey(int spriteno, SPRITE_NAMES tile, int grey, SPRITE_NAMES overlay);

// Loads a sprite from the dungeon tileset, note you cannot mix
// with normal sprites!
void gfx_spritefromdungeon(int spriteno, TILE_NAMES tile);
// Switches whether we are in dungeon or sprite mode
void gfx_spritemode(bool usesprites);

#endif
