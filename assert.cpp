/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        assert.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This implements the UT_ASSERT methods for POWDER.  These are
 *	a bit more complex than your usual int3 as you don't have
 *	a debugger on the hardware.
 */

#include "assert.h"
//
// Assert implementation
//

#include "assert.h"
#include <stdio.h>
#include "control.h"
#include "gfxengine.h"
#include "buf.h"

#ifdef USING_SDL
#include <assert.h>
#endif

// THis is a blank line so we can undo the assert message.
#define blank "                             "

void
triggerassert(int cond, const char *msg, int line, const char *file)
{
#ifdef HAS_PRINTF
    if (!cond)
    {
	printf("Assertion failure: File %s, Line %d\n", file, line);
	assert(0);
    }
#else
    if (!cond)
    {
	BUF		buf;
	
	// Print the message...
	gfx_printtext(0, 10, msg);
	buf.sprintf("%s: %d", file, line);
	gfx_printtext(0, 11, buf);

	// Wait for R & L to go low.  This way if an assert occurs while
	// the user has double pressed, it doesn't go away quickly.
	while (ctrl_rawpressed(BUTTON_R) ||
	       ctrl_rawpressed(BUTTON_L))
	{
	}
	// Wait for R & L to go high.
	while (!ctrl_rawpressed(BUTTON_R) ||
	       !ctrl_rawpressed(BUTTON_L))
	{
	}
	// Wait for them to go low again...
	while (ctrl_rawpressed(BUTTON_R) ||
	       ctrl_rawpressed(BUTTON_L))
	{
	}

	// Clear out the assert...
	gfx_printtext(0, 10, blank);
	gfx_printtext(0, 11, blank);

	// Now we return to our regularly scheduled program, which will
	// likely crash.
    }
#endif
}
