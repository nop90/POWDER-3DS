/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        smokestack.h ( POWDER Library, C++ )
 *
 * COMMENTS:	This is to provide a simple way to manage all the smoke on
 *		a single level.  Currently implemented rather
 *		stupidly - space efficient, but *not* time efficient.
 */

#ifndef __smokestack__
#define __smokestack__

#include "mobref.h"

class SRAMSTREAM;

// The smoke item consists of the smoke type & position unioned together
// as a bitfield, and the owner stored as a MOBREF.

class SMOKEITEM
{
public:
    SMOKEITEM();
    ~SMOKEITEM();

    // Copy constructor & assignment operators.
    SMOKEITEM(const SMOKEITEM &smoke);
    SMOKEITEM &operator=(const SMOKEITEM &smoke);
	
    
    void	setPosition(int x, int y);
    void	setData(SMOKE_NAMES smoke, MOB *owner);

    bool	isPosition(int x, int y) const;
    void	getPosition(int &x, int &y) const;
    void	getData(SMOKE_NAMES &smoke, MOB **owner = 0) const;

    void	save(SRAMSTREAM &os) const;
    void	load(SRAMSTREAM &is);
    
private:
    u16		myPosSmoke;
    MOBREF	myOwner;
};

class SMOKESTACK
{
public:
    SMOKESTACK();
    ~SMOKESTACK();

    // This will add a new smoke element.  If there exists a smoke
    // element at x,y, it will replace it.  If smoke is NONE, it
    // will be compressed on the next compress call.
    void		set(int x, int y, SMOKE_NAMES smoke, MOB *owner);

    // Returns the value of the smoke at the given coordinate.
    SMOKE_NAMES		get(int x, int y, MOB **owner = 0) const;

    // Number of smokey squares - may be an overestimate as some might
    // be NONE.
    int			entries() const { return myEntries; }

    // This cleans all of the NONE entries out of the list.  It should
    // be done after extensive processing (ie, moveSmoke)
    void		collapse();

    // Retrieve a specific smoke entry.
    void		getEntry(int idx, SMOKE_NAMES &smoke, MOB **owner = 0);

    void		save(SRAMSTREAM &os);
    void		load(SRAMSTREAM &is);

private:
    u16			 myEntries;
    u16			 mySize;
    
    SMOKEITEM		*mySmoke;
};

#endif

