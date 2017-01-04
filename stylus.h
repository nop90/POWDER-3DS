/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        stylus.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 */

#ifndef __stylus__
#define __stylus__

#include <mygba.h>

class MOB;

enum STYLUSREGIONS
{
    REGION_NONE = 0,
    // The increasingly inaccurately named bottom buttons.
    REGION_BOTTOMBUTTON = 1,
    REGION_MAPTILES = 2,
    REGION_INVENTORY = 4,
    REGION_SLOTS = 8,
    REGION_MENU = 16,
    REGION_DISPLAYTEXT = 32,
    REGION_TENDIR = 64,
    REGION_FOURDIR = 128,
    REGION_KEYBOARD = 256,
    REGION_SIDEBUTTON = 512,
};

// Create this object at the start of a method that will query the
// stylus to ensure stylus processing is activated & cleared as
// expected.
class STYLUSLOCK
{
public:
    explicit STYLUSLOCK(int regions = 0);
    virtual ~STYLUSLOCK();

    // You can only call this once and only if no region was specified
    // in the constructor.
    void	setRegion(int regions);

    // This is an *inclusive* range!
    // Specified relative to GBA.
    // Specified in *tile* coordinates - ie, everything is multiplied
    // by 8.
    // -1 means not to clip.
    void	setRange(int left, int top, int right, int bottom);

    bool	snap(int &x, int &y);

    // Returns true if inside user specified range.
    bool	inRange(int x, int y);

    // Processes the stylus, possibly drawing the sprite overlay if
    // needed.
    void 	update();

    // Clears any pending stylus buttons
    void 	clear();

    // Waits for a proper stylus press on an inventory item, returning it.
    bool 	selectinventoryitem(int &cx, int &cy);

    // Waits for a proper click on an itemslot.
    bool 	selectinventoryslot(int &slot);

    // Queries for a menu choice in the given range.
    bool 	selectmenu(int &menu, int x, int y, bool &inbounds);

    // Performs any drag requests, updating the action strip accordingly
    bool	performDrags(int menuy, int startoflist, int endoflist, 
			    const u8 *menu, 
			    u8 *actionstrip);

    // Perform inventory drags.  We are returned the start & end slots
    // if we return true.
    // This doesn't use "locking" but rather soft touch.
    // That sentence doesn't make any sense, does it?
    // This is like stylus_queryinventory in that it is independent
    // of normal menu options.
    bool	performInventoryDrag(int &sx, int &sy, int &ex, int &ey, MOB *owner, bool &dirty);

    // Returns true if a click occured in the bottom
    // row of buttons, each 16x16 and starting on y=160.
    // buttonsel is which button was pressed.
    // Nothing is consumed if the click is outside this range.
    bool 	getbottombutton(int &buttonsel);

    // Returns true if proper press.  dy is set to +/-1 or 0.
    bool 	getdisplaytext(int &dy);

    // Sets the ten dir options, sets cancel if a valid click outside.
    bool 	gettendir(int &dx, int &dy, int &dz, bool &cancel);

    // Sets the ten dir options, sets cancel if a valid click outside.
    bool 	getfourdir(int &dx, int &dy, bool &cancel);

    // Returns which character tile was hit.
    bool 	getchartile(int &cx, int &cy);

    // Returns true if a click has occured.
    // Cancel will be true if it was a cancel select
    // mx & my will store the map coordinates of where the click occured.
    bool 	getmaptile(int &mx, int &my, bool &cancel);

private:
    STYLUSLOCK	*myParent;
    int		 myRegion;
    int		 myLeft, myRight, myTop, myBottom;

    // Flags controlling our current state.
    bool	 myWaitForDown;
    bool	 myStarted, myValid, myButton, myBadSnap;
    bool	 myDragging;
    int		 myStartX, myStartY;
    int		 myLastDragX, myLastDragY;
};

// Sets cx/cy to inventory item if stylus is currently over them.
bool 	stylus_queryinventoryitem(int &cx, int &cy);

#endif

