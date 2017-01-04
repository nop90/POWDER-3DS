/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        stylus.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 */

#include "stylus.h"
#include <mygba.h>
#include "gfxengine.h"
#include "assert.h"

#ifdef USING_SDL
#define TILEWIDTH (gfx_gettilewidth())
#define TILEHEIGHT (gfx_gettileheight())
#define MODBYTILE(x, tw) (x - ((x)%(tw)))
#define SNAPTX(x) (MODBYTILE(x,TILEWIDTH*2) + TILEWIDTH)
#define SNAPTY(y) (MODBYTILE(y,TILEHEIGHT*2) + TILEHEIGHT)
#define SNAPCX(x) (MODBYTILE(x,TILEWIDTH) + TILEWIDTH/2)
#define SNAPCY(y) (MODBYTILE(y,TILEHEIGHT) + TILEHEIGHT/2)
#define SNAPTX_o(x) (MODBYTILE(x+TILEWIDTH,TILEWIDTH*2))
#define SNAPTY_o(y) (MODBYTILE(y+TILEHEIGHT,TILEHEIGHT*2))

#define SNAPTX_k(x) (SNAPTX(x-myLeft+TILEWIDTH/2)+myLeft-TILEWIDTH/2)
#define SNAPTY_k(y) (SNAPTY(y-myTop+TILEHEIGHT/2)+myTop-TILEHEIGHT/2)
#else
#define TILEWIDTH 8
#define TILEHEIGHT 8
#define SNAPTX(x) (((x) & ~15)+8)
#define SNAPTY(x) SNAPTX(x)
#define SNAPCX(x) (((x) & ~7)+4)
#define SNAPCY(x) SNAPCX(x)
#define SNAPTX_o(x) ((x+8) & ~15)
#define SNAPTY_o(x) SNAPTX_o(x)

#define SNAPTX_k(x) (((x-myLeft+4) & ~15)+4+myLeft)
#define SNAPTY_k(x) (((x-myTop+4) & ~15)+4+myTop)
#endif

// Ugliness for our dragging of inventory items.
#include "creature.h"
#include "item.h"

STYLUSLOCK *glbStylusActive = 0;

STYLUSLOCK::STYLUSLOCK(int regions)
{
    myParent = glbStylusActive;
    myDragging = false;
    if (regions)
	setRegion(regions);
}

void
STYLUSLOCK::setRegion(int regions)
{
    myParent = glbStylusActive;
    glbStylusActive = this;
    myDragging = false;

    // Ensure we start with a clean slate.
    clear();

    // If the stylus is currently down we don't want to process it
    // since it is a press from before our stylus lock.
    myWaitForDown = hamfake_getstylusstate();

    myRegion = regions;

    myLeft = myRight = myTop = myBottom = -1;
}

STYLUSLOCK::~STYLUSLOCK()
{
    glbStylusActive = myParent;

    // Even if we left by another code path we don't want
    // to leave half a click.
    clear();
}

bool
STYLUSLOCK::inRange(int x, int y)
{
    if (myLeft >= 0 && x < myLeft)
	return false;

    if (myRight >= 0 && x > myRight)
	return false;

    if (myTop >= 0 && y < myTop)
	return false;

    if (myBottom >= 0 && y > myBottom)
	return false;

    return true;
}

bool
STYLUSLOCK::snap(int &x, int &y)
{
    bool		valid = false;
    if (myRegion & REGION_BOTTOMBUTTON)
    {
	if ((y >= 20*TILEHEIGHT || y < 0) &&
	    x >= 0 && x < 30*TILEWIDTH)
	{
	    valid = true;
	    if (y >= 20*TILEHEIGHT)
		y = 20*TILEHEIGHT + TILEHEIGHT;
	    else
		y = -TILEHEIGHT;
	    x = SNAPTX(x);
	    gfx_cursortile(SPRITE_TILE);
	}
    }
    if (myRegion & REGION_SIDEBUTTON)
    {
	if (y < 20*TILEHEIGHT && y >= 0)
	{
	    if (x < TILEWIDTH || x >= 29*TILEWIDTH)
	    {
		valid = true;
		if (x > 20*TILEWIDTH)
		    x = 30*TILEWIDTH;
		else
		    x = 0;
		// Set the y pos
		y = SNAPTY(y);
		gfx_cursortile(SPRITE_TILE);
	    }
	}
    }
    if (myRegion & REGION_MAPTILES)
    {
	x = SNAPTX(x);
	y = SNAPTY_o(y);
	valid = true;
	gfx_cursortile(SPRITE_CURSOR);
    }
    if (myRegion & REGION_KEYBOARD)
    {
	if (inRange(x, y))
	{
	    x = SNAPTX_k(x);
	    y = SNAPTY_k(y);
	    valid = true;
	    gfx_cursortile(SPRITE_TEXT);
	}
	else
	{
	    // Rest of map is used for cancelling.
	    valid = true;
	    gfx_cursortile(SPRITE_CANCEL);
	}
    }
    if (myRegion & REGION_INVENTORY)
    {
	int	sx = x, sy = y;

	valid = true;
	if (sx >= TILEWIDTH*4 && sy >= TILEHEIGHT*2 && sx < TILEWIDTH*30 && sy < TILEHEIGHT*18)
	{
	    // Ignore central column
	    if (sx < TILEWIDTH*6 || sx >= TILEWIDTH*8)
	    {
		gfx_cursortile(SPRITE_CURSOR);
		x = SNAPTX(x);
		y = SNAPTY(y);
	    }
	    else
		gfx_cursortile(SPRITE_CANCEL);
	}
	else
	    gfx_cursortile(SPRITE_CANCEL);
    }
    if (myRegion & REGION_SLOTS)
    {
	int	sx = x, sy = y;

	valid = true;
	if (sx >= TILEWIDTH*4 && sy >= TILEHEIGHT*2 && sx < TILEWIDTH*6 && sy < TILEHEIGHT*18)
	{
	    gfx_cursortile(SPRITE_CURSOR);
	    x = SNAPTX(x);
	    y = SNAPTX(y);
	}
	else
	{
	    gfx_cursortile(SPRITE_CANCEL);
	}
    }
    if (myRegion & REGION_MENU)
    {
	if (inRange(x, y))
	{
	    valid = true;
	    gfx_cursortile(SPRITE_TEXT);
	    x = SNAPCX(x);
	    y = SNAPCY(y);
	}
	else
	{
	    // Rest of map is used for cancelling.
	    valid = true;
	    gfx_cursortile(SPRITE_CANCEL);
	}
    }

    if (myRegion & REGION_DISPLAYTEXT)
    {
	valid = true;
	if (y < TILEHEIGHT*8)
	    gfx_cursortile(SPRITE_NORTH);
	else if (y < TILEHEIGHT*15)
	    gfx_cursortile(SPRITE_CANCEL);
	else
	    gfx_cursortile(SPRITE_SOUTH);
    }

    if (myRegion & REGION_TENDIR)
    {
	int	sx = x, sy = y;

	// Select around center
	sy += TILEHEIGHT;
	sx /= TILEWIDTH*2;
	sy /= TILEHEIGHT*2;
	sx -= 7;
	sy -= 5;
	// We now have +/-1 with option of +/-2 in y.
	if ((ABS(sx) <= 1 && ABS(sy) <= 1) ||
	    (sx == 0 && ABS(sy) == 2))		
	{
	    valid = true;
	    x = SNAPTX(x);
	    y = SNAPTY_o(y);
	    switch (sx)
	    {
		case -1:
		    if (sy == -1)
			gfx_cursortile(SPRITE_NORTHWEST);
		    else if (sy == 0)
			gfx_cursortile(SPRITE_WEST);
		    else
			gfx_cursortile(SPRITE_SOUTHWEST);
		    break;
		case 0:
		    if (sy == -2)
			gfx_cursortile(SPRITE_UP);
		    else if (sy == 2)
			gfx_cursortile(SPRITE_DOWN);
		    else if (sy == -1)
			gfx_cursortile(SPRITE_NORTH);
		    else if (sy == 0)
			gfx_cursortile(SPRITE_CURSOR);
		    else
			gfx_cursortile(SPRITE_SOUTH);
		    break;
		case 1:
		    if (sy == -1)
			gfx_cursortile(SPRITE_NORTHEAST);
		    else if (sy == 0)
			gfx_cursortile(SPRITE_EAST);
		    else
			gfx_cursortile(SPRITE_SOUTHEAST);
		    break;
	    }
	}
	else
	{
	    // Cancel
	    valid = true;
	    gfx_cursortile(SPRITE_CANCEL);
	}
    }

    if (myRegion & REGION_FOURDIR)
    {
	int	sx = x, sy = y;

	// Select around center
	sy += TILEHEIGHT;
	sx /= TILEWIDTH*2;
	sy /= TILEHEIGHT*2;
	sx -= 7;
	sy -= 5;
	if (ABS(sx) <= 1 && ABS(sy) <= 1 && (sx * sy == 0))
	{
	    valid = true;
	    x = SNAPTX(x);
	    y = SNAPTY_o(y);
	    switch (sx)
	    {
		case -1:
		    gfx_cursortile(SPRITE_WEST);
		    break;
		case 0:
		    if (sy == -1)
			gfx_cursortile(SPRITE_NORTH);
		    else if (sy == 0)
			gfx_cursortile(SPRITE_CURSOR);
		    else
			gfx_cursortile(SPRITE_SOUTH);
		    break;
		case 1:
		    gfx_cursortile(SPRITE_EAST);
		    break;
	    }
	}
	else
	{
	    // Cancel
	    valid = true;
	    gfx_cursortile(SPRITE_CANCEL);
	}
    }
    return valid;
}

void
STYLUSLOCK::setRange(int left, int top, int right, int bottom)
{
    myLeft = left*TILEWIDTH;
    myTop = top*TILEHEIGHT;
    myRight = right*TILEWIDTH + TILEWIDTH-1;
    myBottom = bottom*TILEHEIGHT + TILEHEIGHT-1;
}

void
STYLUSLOCK::update()
{
    // Check if we are waiting for a down event to end.
    if (myWaitForDown)
    {
	if (hamfake_getstylusstate())
	{
	    // Still down, keep wiating.
	    return;
	}
	else
	{
	    // Has gone up, valid stylus events now
	    myWaitForDown = false;
	}
    }

    if (!myStarted)
    {
	// Check to see if stylus is now down.
	if (hamfake_getstylusstate())
	{
	    myStarted = true;
	    myValid = true;
	    myButton = false;
	    myDragging = false;
	    hamfake_getstyluspos(myStartX, myStartY);
	    
	    myBadSnap = !snap(myStartX, myStartY);

	    if (!myBadSnap)
		gfx_drawcursor(myStartX, myStartY);
	}
	else
	{
	    // Make sure we clear any bad state.
	    myBadSnap = false;
	}
    }
    else
    {
	// Check if stylus is released or continued.
	if (hamfake_getstylusstate())
	{
	    // User is still waffling...
	    // Determine if we are inbounds and update sprite accordingly!
	    int		nx, ny;
	    hamfake_getstyluspos(nx, ny);
	    if (ABS(nx - myStartX) < TILEWIDTH &&
		ABS(ny - myStartY) < TILEHEIGHT)
	    {
		// Still valid...
		myValid = true;
		if (!myBadSnap)
		    gfx_drawcursor(myStartX, myStartY);
	    }
	    else
	    {
		// User dragged away from the click pos so we want to cancel.
		myValid = false;
		if (!myBadSnap)
		    gfx_removecursor();
	    }
	}
	else
	{
	    // Stylus released, we have a button click!
	    myStarted = false;
	    myDragging = false;
	    myButton = true;
	    if (!myBadSnap)
		gfx_removecursor();
	}
    }
}

void
STYLUSLOCK::clear()
{
    myButton = false;
    myStarted = false;
    myBadSnap = false;
    myDragging = false;
    gfx_removecursor();
}

bool
stylus_postoinventory(int x, int y, int &cx, int &cy)
{
    // Convert the mouse location to an inventory item.
    // Return false if doesn't line up.
    if (x < 0)
	return false;
    if (x >= TILEWIDTH*30)
	return false;
    if (y < 0)
	return false;
    if (y >= TILEHEIGHT*20)
	return false;
    
    x /= TILEWIDTH*2;
    // First two columns are text
    if (x < 2)
	return false;
    x -= 2;
    if (x)
    {
	// There is a blank column for the text by the slots.
	if (x == 1)
	    return false;
	x--;
    }

    y /= TILEHEIGHT * 2;
    if (!y)
    {
	// Top line is status messages
	return false;
    }
    y--;
    // Only 8 lines of inventory
    if (y >= 8)
	return false;

    cx = x;
    cy = y;
    
    return true;
}

bool
stylus_queryinventoryitem(int &cx, int &cy)
{
    int		x, y;
    if (!hamfake_getstylusstate())
	return false;
    hamfake_getstyluspos(x, y);
    return stylus_postoinventory(x, y, cx, cy);
}

bool
STYLUSLOCK::selectinventoryitem(int &cx, int &cy)
{
    // Should not be called on an inactive stylus.
    update();
    // Check for a valid click.
    if (myButton && myValid)
    {
	// Determine if a valid inventory click.
	bool		valid;

	// A succesful click.
	myButton = false;

	valid = stylus_postoinventory(myStartX, myStartY, cx, cy);
	if (!valid)
	{
	    // It is a click, but somewhere else!  Return invalid
	    // so user can cancel the inventory screen
	    cx = cy = -1;
	}

	return true;
    }
    return false;
}

bool
STYLUSLOCK::selectinventoryslot(int &slot)
{
    update();
    // Check for a valid click.
    if (myButton && myValid)
    {
	// Determine if a valid inventory click.
	bool		valid;
	int		cx, cy;

	myButton = false;
	valid = stylus_postoinventory(myStartX, myStartY, cx, cy);
	if (!valid)
	{
	    // It is a click, but somewhere else!  Make it a cancel.
	    slot = -1;
	}
	else if (cx)
	{
	    // A succesful click.  But was it on an inventory slot?
	    slot = -1;
	}
	else
	{
	    // All good
	    slot = cy;
	}
	return true;
    }
    return false;
}

bool
STYLUSLOCK::getbottombutton(int &buttonsel)
{
    int		bottom = TILEHEIGHT*20;

    update();

    if (myButton && myValid)
    {
	// Ignore anyone outside our range
	if (myStartY < bottom && myStartY >= 0)
	{
	    if (myRegion & REGION_SIDEBUTTON)
	    {
		// Check side buttons...
		if (myStartX < TILEWIDTH || myStartX >= TILEWIDTH*29)
		{
		    // Consume the button
		    myButton = false;
		    buttonsel = myStartY / (TILEHEIGHT*2);
		    if (myStartX > TILEWIDTH*20)
			buttonsel += 10;

		    buttonsel += 30;

		    return true;
		}
	    }
	    
	    return false;
	}

	// Ignore the corner buttons
	if (myStartX < 0 || myStartX >= TILEWIDTH*30)
	    return false;

	// Consume the button
	myButton = false;
	buttonsel = myStartX / (TILEWIDTH*2);
	if (myStartY >= bottom)
	    buttonsel += 15;
	return true;
    }
    return false;
}

bool
STYLUSLOCK::selectmenu(int &menu, int x, int y, bool &inbounds)
{
    update();

    if (myButton)
    {
	myButton = false;
	if (!myValid)
	    return false;

	if (!inRange(myStartX, myStartY))
	{
	    // Outside of the valid menu area.
	    inbounds = false;
	    menu = -1;
	    return true;
	}
	inbounds = true;
	// Truncate to our menu choice
	menu = (myStartY / TILEHEIGHT) - (y / TILEHEIGHT);
	return true;
    }

    return false;
}

bool
STYLUSLOCK::getdisplaytext(int &dy)
{
    update();

    if (myButton)
    {
	myButton = false;
	if (!myValid)
	    return false;

	if (myStartY < TILEHEIGHT*8)
	    dy = -10;
	else if (myStartY < TILEHEIGHT*15)
	    dy = 0;
	else
	    dy = 10;
	    
	return true;
    }

    // See if we can start a drag, ie, we are started but have moved
    // outside of a click range.
    if (!myDragging && myStarted && !myValid)
    {
	myLastDragX = myStartX;
	myLastDragY = myStartY;
	myDragging = true;
    }

    // Check if we are dragging.
    if (myDragging)
    {
	int			curx, cury;

	hamfake_getstyluspos(curx, cury);

	int			newy, oldy;

	// We make sure our deltas are positive to avoid annoying
	// round to zero problems
	oldy = myLastDragY - myStartY + TILEHEIGHT*32;
	oldy /= TILEHEIGHT;
	newy = cury - myStartY + TILEHEIGHT*32;
	newy /= TILEHEIGHT;

	oldy -= newy;
	if (oldy)
	{
	    myLastDragY = cury;
	    myLastDragX = curx;
	    dy = oldy;
	    return true;
	}
    }

    return false;
}

bool
STYLUSLOCK::gettendir(int &dx, int &dy, int &dz, bool &cancel)
{
    update();

    if (myButton)
    {
	myButton = false;
	if (!myValid)
	    return false;

	int	sx = myStartX, sy = myStartY;

	// Select around center
	sy += TILEHEIGHT;
	sx /= TILEWIDTH*2;
	sy /= TILEHEIGHT*2;
	sx -= 7;
	sy -= 5;
	// We now have +/-1 with option of +/-2 in y.
	if ((ABS(sx) <= 1 && ABS(sy) <= 1))
	{
	    dz = 0;
	    dx = sx;
	    dy = sy;
	    cancel = false;
	    return true;
	}
	if (sx == 0 && ABS(sy) == 2)
	{
	    dz = -SIGN(sy);
	    dx = 0;
	    dy = 0;
	    cancel = false;
	    return true;
	}
	// Cancel request.
	cancel = true;
	    
	return true;
    }

    return false;
}

bool
STYLUSLOCK::getfourdir(int &dx, int &dy, bool &cancel)
{
    update();

    if (myButton)
    {
	myButton = false;
	if (!myValid)
	    return false;

	int	sx = myStartX, sy = myStartY;

	// Select around center
	sy += TILEHEIGHT;
	sx /= TILEWIDTH*2;
	sy /= TILEHEIGHT*2;
	sx -= 7;
	sy -= 5;
	// We now have +/-1 with option of +/-2 in y.
	if ((ABS(sx) <= 1 && ABS(sy) <= 1) && (sx * sy == 0))
	{
	    dx = sx;
	    dy = sy;
	    cancel = false;
	    return true;
	}
	// Cancel request.
	cancel = true;
	    
	return true;
    }

    return false;
}

bool
STYLUSLOCK::getchartile(int &cx, int &cy)
{
    update();

    if (myButton)
    {
	myButton = false;
	
	if (!myValid)
	    return false;

	int	sx = myStartX, sy = myStartY;

	if (inRange(sx, sy))
	{
	    // Compute the actual tile location.
	    if (myLeft >= 0)
		sx -= myLeft;
	    if (myTop >= 0)
		sy -= myTop;
	    sx /= TILEWIDTH*2;
	    sy /= TILEHEIGHT*2;

	    cx = sx;
	    cy = sy;
	    return true;
	}
    }

    return false;
}

bool
STYLUSLOCK::getmaptile(int &mx, int &my, bool &cancel)
{
    update();

    // We are a simple soul and care only for a button.
    if (myButton)
    {
	int		cx, cy;
	
	myButton = false;
	// Now trigger the requested click by downsampling.
	cancel = !myValid;
	mx = myStartX;
	my = myStartY + TILEHEIGHT;
	mx /= TILEWIDTH*2;
	my /= TILEHEIGHT*2;

	gfx_getscrollcenter(cx, cy);
	mx += cx;
	my += cy;

	// Joyous constants propagated without mercy
	mx -= 7;
	my -= 5;

	// Now clamp to the mapwidth/height
	mx &= MAP_WIDTH-1;
	my &= MAP_HEIGHT-1;

	return true;
    }

    // No valid stylus yet
    return false;
}

bool
STYLUSLOCK::performDrags(int menuy, int startoflist, int endoflist, const u8 *menu, u8 *actionstrip)
{
    update();

    // First question: Do we have a drag at all?
    // We should be inside a current button press: myStarted == true.
    // We should have left our button location - myValid == false.
    if (myStarted && !myValid)
    {
	// We tight loop until either:
	// 1) Drag completed
	// 2) User returns to the original box.
	// In the case of 2 it is important to leave things as they were
	// so people can press the button properly.
	int			x, y, i, stripstart, stripend;
	u8			startaction, endaction;

	x = myStartX;
	y = myStartY;

	stripstart = -1;
	startaction = 0;
	// Determine what cursor to use.
	if (y < 0 || y >= TILEHEIGHT*20)
	{
	    i = x / (TILEWIDTH*2);
	    if (y >= TILEHEIGHT*20)
		i += 15;
	    if (i < 0) i = 0;
	    if (i > 30) i = 29;
	    stripstart = i;
	    startaction = actionstrip[i];
	}
	else if ((myRegion & REGION_SIDEBUTTON) &&
		(x < TILEWIDTH || x >= TILEWIDTH*29))
	{
	    i = y / (TILEHEIGHT*2);
	    if (x >= TILEWIDTH*20)
		i += 10;
	    if (i < 0) i = 0;
	    if (i > 20) i = 19;
	    i += 30;
	    stripstart = i;
	    startaction = actionstrip[i];
	}
	else if (inRange(x, y))
	{
	    // Trancate to our menu choice.
	    i = (y/TILEHEIGHT) - (menuy / TILEHEIGHT);
	    // Account for the up arrows.
	    if (startoflist)
		i--;
	    if (i >= 0)
	    {
		// Adjust for our start of list.
		i += startoflist;
		if (i <= endoflist)
		{
		    startaction = menu[i];
		}
	    }
	}

	if (startaction == 0)
	{
	    // Nothing to do!
	    return false;
	}
	    
	gfx_cursortile(action_spriteFromStripButton(startaction));

	while (1)
	{
	    if (!gfx_isnewframe())
		continue;
	    if (hamfake_forceQuit())
		break;

	    bool		state;

	    // Poll events...
	    hamfake_peekKeyPress();

	    state = hamfake_getstylusstate();

	    if (!state)
	    {
		// User released the stylus - completed the drag.
		break;
	    }
	    
	    hamfake_getstyluspos(x, y);

	    // Still good, move the curosr.
	    gfx_drawcursor(x, y);
	}

	// Getting here implies drag completed.
	// But is our destination a valid tile?
	// In any case, if source was a strip, we swap it
	// Which means ACTION_NONE if destination invalid.
	
	stripend = -1;
	endaction = 0;
	// Determine where we ended up
	if (y < 0 || y >= TILEHEIGHT*20)
	{
	    i = x / (TILEWIDTH*2);
	    if (y >= TILEHEIGHT*20)
		i += 15;
	    if (i < 0) i = 0;
	    if (i > 30) i = 29;
	    stripend = i;
	    endaction = actionstrip[i];
	}
	else if ((myRegion & REGION_SIDEBUTTON) &&
		(x < TILEWIDTH || x >= TILEWIDTH*29))
	{
	    i = y / (TILEHEIGHT*2);
	    if (x >= TILEWIDTH*20)
		i += 10;
	    if (i < 0) i = 0;
	    if (i > 20) i = 19;
	    i += 30;
	    stripend = i;
	    endaction = actionstrip[i];
	}
	else if (inRange(x, y))
	{
	    // Trancate to our menu choice.
	    i = (y/TILEHEIGHT) - (menuy / TILEHEIGHT);
	    // Adjust for our start of list.
	    if (i >= 0)
	    {
		i += startoflist;
		if (i <= endoflist)
		{
		    endaction = menu[i];
		}
	    }
	}

	// Swap the two
	if (stripstart >= 0)
	    actionstrip[stripstart] = endaction;
	if (stripend >= 0)
	    actionstrip[stripend] = startaction;

	clear();
	// We were ignoring keypresses during drag.
	hamfake_clearKeyboardBuffer();

	return true;
    }
    return false;
}

bool
STYLUSLOCK::performInventoryDrag(int &sx, int &sy, int &ex, int &ey,
	MOB *owner, bool &dirtiedsprites)
{
    dirtiedsprites = false;

    update();

    // First question: Do we have a drag at all?
    // We want the mouse to be currently down this is set in myStarted
    // in the right way.
    if (myStarted)
    {
	// We tight loop until drag is complete.
	int			x, y, rx, ry;
	ITEM			*item;
	bool			result = true;

	// We use the start position, *not* the current position,
	// as that ensures someone dragging an action button won't
	// suddenly pick an inventory item.
	x = myStartX;
	y = myStartY;

	if (!stylus_postoinventory(x, y, sx, sy))
	{
	    // We didn't start on an inventory item!
	    return false;
	}

	// Convert start position into a precise location so
	// we measure our 8 pixels from the center of the tile.
	ry = sy * TILEHEIGHT*2 + TILEHEIGHT + TILEHEIGHT*2;
	rx = sx * TILEWIDTH*2 + (sx ? TILEWIDTH*2 : 0) + TILEWIDTH*4 + TILEWIDTH;

	// Make sure we have dragged far enough.  8 pixels lets us escape
	// our standard sized box so is our threshold
	hamfake_getstyluspos(x, y);
	if (ABS(x - rx) < TILEWIDTH &&
	    ABS(y - ry) < TILEHEIGHT)
	{
	    // Not yet escaped
	    return false;
	}

	item = owner->getItem(sx, sy);
	if (!item)
	{
	    // Dragging an empty slot, no-op
	    return false;
	}

	// Switch to dungeon tiles and set the cursor.
	gfx_spritemode(false);
	dirtiedsprites = true;
	gfx_spritefromdungeon(0, item->getTile());
	hamfake_movesprite(0, x-TILEWIDTH, y-TILEHEIGHT);
	hamfake_enablesprite(0, true);

	// Hide the inventory item.
	gfx_setabsmob(sx ? (sx + 3) : (sx+2), sy+1, TILE_VOID);

	while (1)
	{
	    if (!gfx_isnewframe())
		continue;
	    if (hamfake_forceQuit())
		break;

	    bool		state;

	    // Poll events...
	    hamfake_peekKeyPress();

	    state = hamfake_getstylusstate();

	    if (!state)
	    {
		// User released the stylus - completed the drag.
		break;
	    }
	    
	    hamfake_getstyluspos(x, y);

	    // Still good, move the curosr.
	    hamfake_movesprite(0, x-TILEWIDTH, y-TILEHEIGHT);
	}

	// Getting here implies drag completed.
	// But is our destination a valid inventory slot?
	if (!stylus_postoinventory(x, y, ex, ey))
	    result = false;

	// Switch back to sprites.
	gfx_spritemode(true);

	clear();
	// We were ignoring keypresses during drag.
	hamfake_clearKeyboardBuffer();

	return result;
    }
    return false;
}
