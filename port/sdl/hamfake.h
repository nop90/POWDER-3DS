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

// These are the native res of the SDL screen.
// measured in tiles.
#ifdef iPOWDER
#define HAM_SCRW_T		32
#define HAM_SCRH_T		26
#else
#define HAM_SCRW_T		32
#define HAM_SCRH_T		24
#endif

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

#ifdef iPOWDER

class SCREENDATA_REF;

class SCREENDATA
{
public:
		SCREENDATA(u8 *data, int w, int h, int i);
		SCREENDATA();
    virtual	~SCREENDATA();
		SCREENDATA(const SCREENDATA &data);
    SCREENDATA	&operator=(const SCREENDATA &data);

    int		width() const { return myWidth; }
    int		height() const { return myHeight; }

    const u8	*data();
    int		id() const { return myId; }

protected:
    SCREENDATA_REF	*myRef;
    int			 myWidth, myHeight;
    int			 myId;
};

class INPUTDATA
{
public:
    int			myX, myY;
    int			myMaxLen;
    char		myText[256];
};

extern INPUTDATA	glbInputData;

class EMAILDATA
{
public:
    const char		*myTo;
    const char		*mySubject;
    const char		*myBody;
};

extern EMAILDATA	glbEmailData;
extern volatile bool	glbCanSendEmail;

bool hamfake_isunlocked();
void hamfake_setunlocked(bool lockstate);
// The ability to unlock may be disabled.
bool hamfake_allowunlocks();
void hamfake_setallowunlocks(bool state);

bool hamfake_cansendemail();
void hamfake_sendemail(const char *to, const char *subject, const char *body);

void hamfake_getActualScreen(SCREENDATA &screen);
void hamfake_callThisFromVBL();
void hamfake_setstyluspos(bool state, int x, int y);
void hamfake_setdatapath(const char *path);

void hamfake_postaction(int action, int spell);

void hamfake_postdir(int dx, int dy);
void hamfake_flushdir();
bool hamfake_externaldir(int &dx, int &dy);
void hamfake_postorientation(bool isportrait);
void hamfake_postshake(bool isshook);
// This is a latch, so reading resets.
bool hamfake_hasbeenshaken();
#endif
// REturns false if none pending.
bool hamfake_getbuttonreq(int &mode, int &type);

void hamfake_externalaction(int &action, int &spell);
void hamfake_enableexternalactions(bool allow);
bool hamfake_externalactionsenabled();
void hamfake_setinventorymode(bool inventory);
bool hamfake_isinventorymode();

// Moves files
void hamfake_move(const char *dst, const char *src);

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
    u16			**tiles16;
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

#ifndef HAS_KEYBOARD

//
// Raw data accessors.
//

enum FAKE_BUTTONS
{
    FAKE_BUTTON_UP,
    FAKE_BUTTON_DOWN,
    FAKE_BUTTON_LEFT,
    FAKE_BUTTON_RIGHT,
    FAKE_BUTTON_A,
    FAKE_BUTTON_B,
    FAKE_BUTTON_START,
    FAKE_BUTTON_SELECT,
    FAKE_BUTTON_R,
    FAKE_BUTTON_L,
    FAKE_BUTTON_X,
    FAKE_BUTTON_Y,
    FAKE_BUTTON_TOUCH,
    FAKE_BUTTON_LID,
    NUM_FAKE_BUTTONS
};

bool hamfake_isFakeButtonPressed(FAKE_BUTTONS button);
void hamfake_setFakeButtonState(FAKE_BUTTONS button, bool state);
int hamfake_isAnyPressed();	// Note bizarre return semantics!

void hamfake_setForceQuit();

// These data accessors are left for historical reasons.  We now
// never use our built in key matching tech.

#define F_CTRLINPUT_UP_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_UP)
#define F_CTRLINPUT_DOWN_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_DOWN)
#define F_CTRLINPUT_LEFT_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_LEFT)
#define F_CTRLINPUT_RIGHT_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_RIGHT)

#define F_CTRLINPUT_A_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_A)
#define F_CTRLINPUT_B_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_B)
#define F_CTRLINPUT_R_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_R)
#define F_CTRLINPUT_L_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_L)

#define F_CTRLINPUT_SELECT_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_SELECT)
#define F_CTRLINPUT_START_PRESSED \
	hamfake_isFakeButtonPressed(FAKE_BUTTON_START)

#define R_CTRLINPUT \
	hamfake_isAnyPressed()

#endif
    
// Gets a new keypress.  Returns 0 if no keypress is ready.
// Clears key after getting it.  
int hamfake_peekKeyPress();
int hamfake_getKeyPress(bool onlyascii);
void hamfake_clearKeyboardBuffer();
void hamfake_insertKeyPress(int key);

// Returns the union of all active modifiers.
int hamfake_getModifiers();

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

