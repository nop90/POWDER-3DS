/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        control.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Gameboy Advance specific implementation of controls.
 */

#include "control.h"

#include <mygba.h>
#include "control.h"
#include "gfxengine.h"

// Global variables, these store our previous button states.
// If old state is 1, the oldframe contains which frame the button
// was last pressed on.
int	glb_oldstate[NUM_BUTTONS];
int	glb_oldframe[NUM_BUTTONS];

#ifdef _3DS // compensate low framerate
#define SHORT_REPEAT		3		// 6 per sec
#define LONG_REPEAT		6		// 3 per sec
#else
#define SHORT_REPEAT		10		// 6 per sec
#define LONG_REPEAT		20		// 3 per sec
#endif

int	glb_repeatrate[NUM_BUTTONS] =
{
    SHORT_REPEAT,		// UP
    SHORT_REPEAT,		// DOWN
    SHORT_REPEAT,		// LEFT
    SHORT_REPEAT,		// RIGHT
    LONG_REPEAT,		// A
    LONG_REPEAT, 		// B
    LONG_REPEAT,		// START
    LONG_REPEAT,		// SELECT
    LONG_REPEAT,		// R
    LONG_REPEAT,		// L
    LONG_REPEAT,		// X
    LONG_REPEAT,		// Y
    SHORT_REPEAT,		// TOUCH (We use as directions when repeated)
    LONG_REPEAT			// LID
};

// Conversion from button enums to valid key presses.
#define MAX_KEYEQUIV	3
int	glb_buttonkeys[NUM_BUTTONS][MAX_KEYEQUIV] =
{
    { GFX_KEYUP, 'k', '8' },
    { GFX_KEYDOWN, 'j', '2' },
    { GFX_KEYLEFT, 'h', '4' },
    { GFX_KEYRIGHT, 'l', '6' },
    { '\r', ' ', '5' },		// A
    { '\x1b', '0', 0 },		// B
    { 0, 0, 0 },		// START
    { 'i', 0, 0 },		// SELECT
    { 0, 0, 0 },		// R
    { 'p', 0, 0 },		// L

    { 0, 0, 0 },		// X
    { 0, 0, 0 },		// Y
    { GFX_KEYLMB, 0, 0 },	// TOUCH
    { 0, 0, 0 },		// LID
};

void
ctrl_init()
{
    memset(glb_oldstate, 0, sizeof(int) * NUM_BUTTONS);
    memset(glb_oldframe, 0, sizeof(int) * NUM_BUTTONS);
}

void
ctrl_setrepeat(BUTTONS button, int timedelay)
{
    glb_repeatrate[button] = timedelay;
}

int
ctrl_anyrawpressed()
{
#if defined(HAS_KEYBOARD)
    return 0;
#elif defined(_3DS)
	if(hamfake_isAnyPressed())
	return 0;
    else
	return 1;
#else
    if (~R_CTRLINPUT & 0x3FF)
	return 1;
    else
	return 0;
#endif
}

int
ctrl_rawpressed(int button)
{
#ifdef HAS_KEYBOARD
    return 0;
#else
    switch (button)
    {
	case BUTTON_UP:
	    return F_CTRLINPUT_UP_PRESSED;
	case BUTTON_DOWN:
	    return F_CTRLINPUT_DOWN_PRESSED;
	case BUTTON_LEFT:
	    return F_CTRLINPUT_LEFT_PRESSED;
	case BUTTON_RIGHT:
	    return F_CTRLINPUT_RIGHT_PRESSED;
	case BUTTON_A:
	    return F_CTRLINPUT_A_PRESSED;
	case BUTTON_B:
	    return F_CTRLINPUT_B_PRESSED;
	case BUTTON_R:
	    return F_CTRLINPUT_R_PRESSED;
	case BUTTON_L:
	    return F_CTRLINPUT_L_PRESSED;
	case BUTTON_SELECT:
	    return F_CTRLINPUT_SELECT_PRESSED;
	case BUTTON_START:
	    return F_CTRLINPUT_START_PRESSED;
#ifdef HAS_XYBUTTON
	case BUTTON_X:
	case BUTTON_Y:
	    return hamfake_isPressed((BUTTONS) button);
#else
	case BUTTON_X:
	case BUTTON_Y:
	    return false;
#endif
#ifdef USING_DS 
	case BUTTON_TOUCH:
	case BUTTON_LID:
	    return hamfake_isPressed((BUTTONS) button);
#else
	case BUTTON_TOUCH:
	    return hamfake_getstylusstate();
	case BUTTON_LID:
	    return false;
#endif
    }
#endif

    return 0;
}

bool
ctrl_hit(int button)
{
#ifdef HAS_KEYBOARD
    int		key, equiv;

    key = hamfake_peekKeyPress();

    // Trivial failure if no key hit.
    if (!key)
	return false;

    // See if this key matches this button's list.
    for (equiv = 0; equiv < MAX_KEYEQUIV; equiv++)
    {
	if (glb_buttonkeys[button][equiv] == key)
	{
	    // Consume the keypress.
	    hamfake_getKeyPress(false);
	    return true;
	}
    }

    // Not found, ignore.
    return false;

#else
    int		curstate;
    int		curframe = -1;
    
    // Now, see if we are pressed...
    curstate = ctrl_rawpressed(button);

    if (!curstate)
    {
	// We know the answer, just make sure our old state is clear.
	glb_oldstate[button] = 0;
	return false;
    }
    

    if (glb_oldstate[button])
    {
	curframe = gfx_getframecount();
	if (curframe < glb_oldframe[button] + glb_repeatrate[button])
	{
	    // Not enough time since the last press, fail.
	    return false;
	}
    }

    // Success!  We will return 1 from here, but first update the
    // old state with our current pos.
    curframe = gfx_getframecount();
    glb_oldframe[button] = curframe;
    glb_oldstate[button] = 1;
    
    return true;
#endif
}

void
ctrl_getdir(int &dx, int &dy, int raw, bool allowdiag)
{
    dx = dy = 0;

#ifdef HAS_KEYBOARD
    int		key;

    key = hamfake_peekKeyPress();
	
    ctrl_getdirfromkey(key, dx, dy, allowdiag);
    if (dx || dy)
    {
	// Consume the key
	hamfake_getKeyPress(false);
    }

#else
    
    if (raw)
    {
	if (ctrl_rawpressed(BUTTON_LEFT))
	    dx--;
	if (ctrl_rawpressed(BUTTON_RIGHT))
	    dx++;
	if (ctrl_rawpressed(BUTTON_UP))
	    dy--;
	if (ctrl_rawpressed(BUTTON_DOWN))
	    dy++;
    }
    else
    {
	if (ctrl_hit(BUTTON_LEFT))
	    dx--;
	if (ctrl_hit(BUTTON_RIGHT))
	    dx++;
	if (ctrl_hit(BUTTON_UP))
	    dy--;
	if (ctrl_hit(BUTTON_DOWN))
	    dy++;
    }

#endif
    // We prohibit diagonals as they are really hard to do with our
    // first down methods.
    if (!allowdiag && dx && dy)
	dy = 0;
}

void
ctrl_getdirfromkey(int keypress, int &dx, int &dy, bool allowdiag)
{
#ifndef HAS_KEYBOARD
    ctrl_getdir(dx, dy);
#else
    dx = dy = 0;

    switch (keypress)
    {
	case '8':
	case GFX_KEYUP:
	case 'k':
	    dy--;
	    break;
	case '2':
	case GFX_KEYDOWN:
	case 'j':
	    dy++;
	    break;
	case '4':
	case GFX_KEYLEFT:
	case 'h':
	    dx--;
	    break;
	case '6':
	case GFX_KEYRIGHT:
	case 'l':
	    dx++;
	    break;
    }

    if (allowdiag)
    {
	switch (keypress)
	{
	    case '7':
	    case 'y':
		dy--;	dx--;
		break;
	    case '9':
	    case 'u':
		dy--;	dx++;
		break;
	    case '1':
	    case 'b':
		dy++;	dx--;
		break;
	    case '3':
	    case 'n':
		dy++;	dx++;
		break;
	}
    }
#endif
}
