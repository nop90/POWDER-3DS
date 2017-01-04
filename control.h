/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        control.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Platform independent control interface
 */

#ifndef __control__
#define __control__

// This allows the standard Game Boy Advance controls or their equivalents,
// namely:
// Direction, any or all of up/down/left/right
// Buttons A & B
// Buttons R & L
// Buttons Start & Select.
// Gameboy DS: X & Y, TOUCH and LID.

// Button definitions
enum BUTTONS
{
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_A,
    BUTTON_B,
    BUTTON_START,
    BUTTON_SELECT,
    BUTTON_R,
    BUTTON_L,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_TOUCH,
    BUTTON_LID,
    NUM_BUTTONS
};

// Initialize the control library.
void ctrl_init();

// Set the repeat rate for keypresses.
void ctrl_setrepeat(int timedelay);

// Determine if any button is pressed.
int ctrl_anyrawpressed();

// Check to see if a button is currently pressed.
int ctrl_rawpressed(int button);

// Check to see if a button has been pressed.  This triggers on button
// down. It will not retrigger until the button is released, or a timer
// is exceeded.  Each call will reset this status, so a second call will
// always report unpressed.
bool ctrl_hit(int button);

// This convience function will build your dx/dy.  Raw determines if
// it uses ctrl_hit or rawpressed.
void ctrl_getdir(int &dx, int &dy, int raw = 0, bool allowdiag=false);

// This convience function will build your dx/dy from a keypress.
// Handles those nifty hjkl crap.
void ctrl_getdirfromkey(int keypress, int &dx, int &dy, bool allowdiag=false);

#endif
