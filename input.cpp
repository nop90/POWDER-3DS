/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        input.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Routines to accept input...
 */

#include "input.h"

#include "assert.h"
#include "gfxengine.h"
#include "input.h"
#include "control.h"
#include "rand.h"
#include "stylus.h"

#define KEYWIDTH 14
#define KEYHEIGHT 5
#define KEYTOP 6
#define KEYLEFT 1

#ifdef USING_SDL
#define TILEWIDTH (gfx_gettilewidth())
#define TILEHEIGHT (gfx_gettileheight())
#else
#define TILEWIDTH 8
#define TILEHEIGHT 8
#endif


// Given the direction, this determines the character...
char
input_getstatechar(int dx, int dy, bool shifted)
{
    int			code = 0;
    int			bits;
    
    UT_ASSERT(dx || dy);
    
    if (dx)
	code = (dx < 0) ? 0 : 1;
    else
	code = (dy < 0) ? 2 : 3;

    // Now, we could just have a bit field for the additional buttons,
    // but that would make priority calculations difficult.  Instead,
    // we go:
    //      , A,   B,   L,    | R,   L+R,   A+L,   A+R,   
    // A+L+R, B+L, B+R, B+L+R,| A+B, A+B+L, A+B+R, A+B+L+R
    static const int	bitlut[16] =
    //     , A,   B,   A+B,   L,   L+A,   L+B,   L+A+B,
	{ 0, 1,   2,   12,    3,   6,     9,     13,
    //    R, R+A, R+B, R+A+B, R+L, R+L+A, R+L+B, R+L+B+A
	  4, 7,   10,  14,    5,   8,     11,    15 };
	  
    // 65 so we have the null terminator.
    static const char charlut[65] =
	"\b \t\n"
	"abcd" "efgh" "ijkl" "mnop" "qrst" "uvwx" "yz'\""
	"0123" "4567" "89!#"
	"+-/*" "|_^&"
	"()[]" "{}<>"
	".,:;";

    static const char charlutshift[65] =
	"\b \t\n"
	"ABCD" "EFGH" "IJKL" "MNOP" "QRST" "UVWX" "YZ'\""
	"0123" "4567" "89!#"
	"+-/*" "|_^&"
	"()[]" "{}<>"
	".,:;";

    bits = (ctrl_rawpressed(BUTTON_A)) |
	   (ctrl_rawpressed(BUTTON_B) << 1) |
	   (ctrl_rawpressed(BUTTON_L) << 2) |
	   (ctrl_rawpressed(BUTTON_R) << 3);

    // Mix in the X & Y so DS users have it even easier.
    bits |= (ctrl_rawpressed(BUTTON_X) << 3);
    bits |= (ctrl_rawpressed(BUTTON_Y) << 2);

    code = code | (bitlut[bits] << 2);

    if (shifted)
	return charlutshift[code];
    else
	return charlut[code];
}

void
input_updategnomon(int gx, int gy, bool shifted)
{
    int		dx, dy;

    for (dy = -1; dy <= 1; dy++)
    {
	for (dx = -1; dx <= 1; dx++)
	{
	    if ((!dx) ^ (!dy))
	    {
		gfx_printchar(gx+dx, gy+dy, input_getstatechar(dx, dy, shifted));
	    }
	}
    }
}

const char glbKeyboard[KEYWIDTH * KEYHEIGHT + 1] =
     "=1234567890-\b\b"
     " qwertyuiop\\ " SYMBOLSTRING_TRIDUDE
     " asdfghjkl;' \n"
     " zxcvbnm,./  \n"
     "\t`\\ \b   \n []\t ";

const char glbKeyboardShift[KEYWIDTH * KEYHEIGHT + 1] =
     "+!@#$%^&*()_ \b"
     " QWERTYUIOP| " SYMBOLSTRING_TRIDUDE
     " ASDFGHJKL:\" \n"
     " ZXCVBNM<>?  \n"
     "\t~| \b   \n {}\t ";

char
input_lookupkey(int x, int y, bool shifted)
{
    if (x < 0 || x >= KEYWIDTH)
	return 0;
    if (y < 0 || y >= KEYHEIGHT)
	return 0;

    if (shifted)
	return glbKeyboardShift[x + y * KEYWIDTH];
    else
	return glbKeyboard[x + y * KEYWIDTH];
}

void
input_drawkeyboard(bool shifted)
{
    // GBA has no stylus so gets no keyboard.  They are stuck
    // with the one true input method.
    // That is, until the fateful day we decided to abandon
    // the one true input method.
    int		x, y;
    const char	*key;

    if (shifted)
	key = glbKeyboardShift;
    else
	key = glbKeyboard;
    for (y = 0; y < KEYHEIGHT; y++)
    {
	for (x = 0; x < KEYWIDTH; x++)
	{
	    gfx_printchar(KEYLEFT+x*2, KEYTOP+y*2, *key);
	    key++;

	    gfx_setabsoverlay(x, 3+y, TILE_EMPTYSLOT);
	}
    }
}

void
input_erasecursor(int kx, int ky)
{
    gfx_printchar(kx*2 + KEYLEFT-1, ky*2 + KEYTOP, ' ');
    gfx_printchar(kx*2 + KEYLEFT+1, ky*2 + KEYTOP, ' ');
    gfx_printchar(kx*2 + KEYLEFT, ky*2 + KEYTOP-1, ' ');
    gfx_printchar(kx*2 + KEYLEFT, ky*2 + KEYTOP+1, ' ');
}

void
input_drawcursor(int kx, int ky)
{
    gfx_printcolourchar(kx*2 + KEYLEFT-1, ky*2 + KEYTOP, SYMBOL_RIGHT, COLOUR_RED);
    gfx_printcolourchar(kx*2 + KEYLEFT+1, ky*2 + KEYTOP, SYMBOL_LEFT, COLOUR_RED);
    gfx_printcolourchar(kx*2 + KEYLEFT, ky*2 + KEYTOP-1, SYMBOL_DOWN, COLOUR_RED);
    gfx_printcolourchar(kx*2 + KEYLEFT, ky*2 + KEYTOP+1, SYMBOL_UP, COLOUR_RED);
}

int
input_getchar(int gx, int gy, bool shifted)
{
    int			sx, sy;
#ifndef _NOSTYLUS
   STYLUSLOCK		styluslock(REGION_KEYBOARD);

    styluslock.setRange(KEYLEFT, KEYTOP, 
			KEYLEFT+2*KEYWIDTH-1, KEYTOP+2*KEYHEIGHT-1);
#endif

#ifdef HAS_KEYBOARD
    while (1)
    {
	int		key;
	
	// We do a random just to keep things ticking...
	rand_choice(3);
	hamfake_rebuildScreen();
	hamfake_awaitEvent();
	key = hamfake_getKeyPress(false);

	// Ignore mouse keys as we want the stylus lock to work.
	if (key == GFX_KEYLMB || key == GFX_KEYMMB || key == GFX_KEYRMB)
	    key = 0;

	if (key)
	    return key;

	// Check stylus.
#ifndef _NOSTYLUS
	if (styluslock.getchartile(sx, sy))
	{
	    key = input_lookupkey(sx, sy, shifted);
	    if (key)
		return key;
	}
#endif
    }
#else
    int		dx, dy, key = 0;
    // I have no idea what character this is!
    static int	key_x = 5, key_y = 2;

    input_drawcursor(key_x, key_y);
    while (1)
    {
	// We do a random just to keep things ticking...
	rand_choice(3);

	if (hamfake_forceQuit())
	    return '\n';

#ifdef iPOWDER
	if (hamfake_hasbeenshaken())
	{
	    return GFX_KEYF3;
	}
#endif

	hamfake_rebuildScreen();
	hamfake_awaitEvent();

	ctrl_getdir(dx, dy);
	if (dx || dy)
	{
	    // Move the current key.
	    input_erasecursor(key_x, key_y);
	    key_x += dx;
	    key_y += dy;
	    key_x %= KEYWIDTH;
	    key_y %= KEYHEIGHT;
	    if (key_x < 0)
		key_x += KEYWIDTH;
	    if (key_y < 0)
		key_y += KEYHEIGHT;

	    input_drawcursor(key_x, key_y);
	}

	if (ctrl_hit(BUTTON_A))
	{
	    key = input_lookupkey(key_x, key_y, shifted);
	    if (key)
		break;
	}
	
	// Other buttons are tied to certain built in keypresses.
	if (ctrl_hit(BUTTON_L))
	{
	    key = '\b';
	    break;
	}

	if (ctrl_hit(BUTTON_R))
	{
	    key = ' ';
	    break;
	}

	if (ctrl_hit(BUTTON_B))
	{
	    // This should be an Escape and thus quit without
	    // actually saving a line of text, except we don't support
	    // that option in general and it is much more useful to be
	    // able to hit enter without fiddling with our key position.
	    key = '\n';
	    break;
	}

	// Start makes sense to start the game so select your text
	if (ctrl_hit(BUTTON_START))
	{
	    key = '\n';
	    break;
	}

	// Select is for toggling shift - selecting the font?
	if (ctrl_hit(BUTTON_SELECT))
	{
	    key = '\t';
	    break;
	}
	
#ifndef _NOSTYLUS
	// Check stylus.
	if (styluslock.getchartile(sx, sy))
	{
	    key = input_lookupkey(sx, sy, shifted);
	    if (key)
	    {
		// Also adjust our button press location
		input_erasecursor(key_x, key_y);
		key_x = sx;
		key_y = sy;
		break;
	    }
	}
#endif
    }

    input_erasecursor(key_x, key_y);
    
    return key;
#endif
}

void
input_getline(char *text, int len, int lx, int ly, int gx, int gy,
		const char *initialtext)
{
    int		pos = 0;
    int 	c;
    bool	shifted = false;

#ifdef iPOWDER
    // Ensure the latest text is on screen.
    hamfake_rebuildScreen();
    if (initialtext)
	strcpy(glbInputData.myText, initialtext);
    else
	glbInputData.myText[0] = '\0';
    glbInputData.myMaxLen = len;
    glbInputData.myX = lx;
    glbInputData.myY = ly;
    hamfake_buttonreq(7, 0);

    // On return we have our string!
    int		i;
    for (i = 0; i < len; i++)
    {
	// Yes, I trust no one, not even myself.
	text[i] = glbInputData.myText[i];
	if (!text[i])
	    break;
    }
    text[i] = '\0';
    return;
#endif

    // Wait for the person to release the a button...
#ifndef HAS_KEYBOARD
    while (ctrl_anyrawpressed())
    {
	hamfake_awaitEvent();
    }
#endif

    // Clear any keyboard buffer.
    hamfake_clearKeyboardBuffer();

    // Input the initial text.
    if (initialtext)
    {
	while (*initialtext)
	{
	    c = *initialtext++;
	    if (pos < len)
	    {
		text[pos] = c;
		gfx_printchar(lx+pos, ly, c);
		pos++;
	    }
	}
    }

    gfx_nudgecenter(-TILEWIDTH/2, TILEHEIGHT/2);
    input_drawkeyboard(shifted);

#ifdef iPOWDER
    hamfake_hasbeenshaken();
#endif

    while (1)
    {
	gfx_printchar(lx+pos, ly, SYMBOL_CURSOR);
	c = input_getchar(gx, gy, shifted);

	if (hamfake_forceQuit())
	{
	    c = '\n';
	}
	
	switch (c)
	{
	    case '\r':
	    case '\n':
		text[pos] = '\0';
		// Our absolute tiles need to be reset.
		gfx_nudgecenter(0, 0);
		gfx_refreshtiles();
		return;

	    case '\b':
		if (pos)
		{
		    gfx_printchar(lx+pos, ly, ' ');
		    pos--;
		}
		break;

	    case '\t':
		shifted = !shifted;
		input_drawkeyboard(shifted);
		break;

	    case GFX_KEYF3:	// F3 because it is a tridude.
	    case SYMBOL_TRIDUDE:
		while (pos)
		{
		    gfx_printchar(lx+pos, ly, ' ');
		    pos--;
		}
		rand_name(text, len);
		while (text[pos])
		{
		    gfx_printchar(lx+pos, ly, text[pos]);
		    pos++;
		}
		break;

	    default:
		if (pos < len && c < 256)
		{
		    text[pos] = c;
		    gfx_printchar(lx+pos, ly, c);
		    pos++;
		}
		break;
	}
	hamfake_rebuildScreen();
    }

    // Our absolute tiles need to be reset.
    gfx_nudgecenter(0, 0);
    gfx_refreshtiles();
}

