/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        smokestack.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:	This manages the smokestack class..
 */

#include "assert.h"
#include "glbdef.h"
#include "smokestack.h"
#include "sramstream.h"
// Needed for MAP_WIDTH/HEIGHT, oddly enough.
#include "gfxengine.h"

//
// Definition of SMOKEITEM
//
SMOKEITEM::SMOKEITEM()
{
}

SMOKEITEM::~SMOKEITEM()
{
}

SMOKEITEM::SMOKEITEM(const SMOKEITEM &smoke)
{
    myPosSmoke = smoke.myPosSmoke;
    myOwner = smoke.myOwner;
}

SMOKEITEM &
SMOKEITEM::operator =(const SMOKEITEM &smoke)
{
    myPosSmoke = smoke.myPosSmoke;
    myOwner = smoke.myOwner;

    return *this;
}

void
SMOKEITEM::setPosition(int x, int y)
{
    UT_ASSERT(x >= 0 && x <= MAP_WIDTH);
    UT_ASSERT(y >= 0 && y <= MAP_HEIGHT);

    x &= MAP_WIDTH-1;
    y &= MAP_HEIGHT-1;
    x += y << 5;

    myPosSmoke &= ~1023;
    myPosSmoke |= x;
}

void
SMOKEITEM::setData(SMOKE_NAMES smoke, MOB *owner)
{
    myPosSmoke &= 1023;
    myPosSmoke |= (smoke << 10);

    myOwner.setMob(owner);
}

bool
SMOKEITEM::isPosition(int x, int y) const
{
    UT_ASSERT(x >= 0 && x <= MAP_WIDTH);
    UT_ASSERT(y >= 0 && y <= MAP_HEIGHT);

    x &= MAP_WIDTH-1;
    y &= MAP_HEIGHT-1;
    x += y << 5;
    
    return ( (myPosSmoke & 1023) == x );
}

void
SMOKEITEM::getPosition(int &x, int &y) const
{
    x = myPosSmoke & 31;
    y = (myPosSmoke >> 5) & 31;
}

void
SMOKEITEM::getData(SMOKE_NAMES &smoke, MOB **owner) const
{
    smoke = (SMOKE_NAMES) (myPosSmoke >> 10);

    if (owner)
	*owner = myOwner.getMob();
}

void
SMOKEITEM::save(SRAMSTREAM &os) const
{
    os.write(myPosSmoke, 16);
    myOwner.save(os);
}

void
SMOKEITEM::load(SRAMSTREAM &is)
{
    int		val;

    is.uread(val, 16);
    myPosSmoke = val;

    myOwner.load(is);
}

//
// SMOKESTACK definitions.
//

SMOKESTACK::SMOKESTACK()
{
    myEntries = 0;
    mySize = 0;
    mySmoke = 0;
}

SMOKESTACK::~SMOKESTACK()
{
    delete [] mySmoke;
}

void
SMOKESTACK::set(int x, int y, SMOKE_NAMES smoke, MOB *owner)
{
    int		i;

    for (i = 0; i < myEntries; i++)
    {
	if (mySmoke[i].isPosition(x, y))
	{
	    mySmoke[i].setData(smoke, owner);
	    return;
	}
    }
    // We need to append an entry.
    if (myEntries >= mySize)
    {
	int		 newsize;
	SMOKEITEM	*newsmoke;

	newsize = myEntries + 32;
	newsmoke = new SMOKEITEM[newsize];

	for (i = 0; i < myEntries; i++)
	{
	    newsmoke[i] = mySmoke[i];
	}

	delete [] mySmoke;
	mySmoke = newsmoke;
	mySize = newsize;
    }
    
    mySmoke[myEntries].setPosition(x, y);
    mySmoke[myEntries].setData(smoke, owner);
    myEntries++;
}

SMOKE_NAMES
SMOKESTACK::get(int x, int y, MOB **owner) const
{
    int			i;
    SMOKE_NAMES		smoke;

    for (i = 0; i < myEntries; i++)
    {
	if (mySmoke[i].isPosition(x, y))
	{
	    mySmoke[i].getData(smoke, owner);
	    return smoke;
	}
    }

    if (owner)
	*owner = 0;
    return SMOKE_NONE;
}


void
SMOKESTACK::collapse()
{
    int		i, di;
    SMOKE_NAMES	smoke;
    
    di = 0;		// Write index.
    for (i = 0; i < myEntries; i++)
    {
	mySmoke[i].getData(smoke);
	if (smoke == SMOKE_NONE)
	{
	    // We want to delete this dude.
	    // Do nothing.
	}
	else
	{
	    if (di != i)
		mySmoke[di] = mySmoke[i];
	    di++;
	}
    }

    myEntries = di;
    // TODO: Free memory?
}

void
SMOKESTACK::getEntry(int idx, SMOKE_NAMES &smoke, MOB **owner)
{
    UT_ASSERT(idx >= 0);
    UT_ASSERT(idx < myEntries);

    mySmoke[idx].getData(smoke, owner);
}

void
SMOKESTACK::save(SRAMSTREAM &os)
{
    int		 i;

    // Ensure we are fully collapsed.
    collapse();
    
    os.write(myEntries, 16);

    for (i = 0; i < myEntries; i++)
    {
	mySmoke[i].save(os);
    }
}

void
SMOKESTACK::load(SRAMSTREAM &is)
{
    int		val, i;

    is.uread(val, 16);

    delete [] mySmoke;
    mySmoke = 0;
    
    myEntries = val;

    if (myEntries)
    {
	mySmoke = new SMOKEITEM[myEntries+16];
	for (i = 0; i < myEntries; i++)
	{
	    mySmoke[i].load(is);
	}
    }
}
