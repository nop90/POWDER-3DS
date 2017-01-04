/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        signpost.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Tracks signposts and their values.  Creates a heap of signposts
 *	which can be queried to get a sign at a position.  O(N) to
 *	access so be wary!
 */

#include "signpost.h"
#include "sramstream.h"

void
SIGNPOSTDATA::init(SIGNPOST_NAMES sign, int x, int y)
{
    myPos = y & 31;
    myPos <<= 5;
    myPos |= x & 31;
    mySign = sign;
}

bool
SIGNPOSTDATA::isPos(int x, int y) const
{
    int		x1, y1;
    getPos(x1, y1);
    return (x1 == x) && (y1 == y);
}

void
SIGNPOSTDATA::getPos(int &x, int &y) const
{
    x = myPos & 31;
    y = myPos >> 5;
}

SIGNPOST_NAMES
SIGNPOSTDATA::getSign() const
{
    return (SIGNPOST_NAMES) mySign;
}

void
SIGNPOSTDATA::save(SRAMSTREAM &os) const
{
    int		collapse;

    collapse = mySign << 10 | myPos;
    os.write(collapse, 16);
}

void
SIGNPOSTDATA::load(SRAMSTREAM &is)
{
    int		val;

    is.uread(val, 16);
    myPos= val & 1023;
    mySign= val >> 10;
}

SIGNLIST::SIGNLIST()
{
}

SIGNLIST::~SIGNLIST()
{
}

SIGNPOST_NAMES
SIGNLIST::find(int x, int y) const
{
    int		i;

    for (i = 0; i < myPosts.entries(); i++)
    {
	if (myPosts(i).isPos(x, y))
	    return myPosts(i).getSign();
    }
    return SIGNPOST_NONE;
}

void
SIGNLIST::append(SIGNPOST_NAMES sign, int x, int y)
{
    SIGNPOSTDATA post;

    post.init(sign, x, y);
    myPosts.append(post);
}

void
SIGNLIST::save(SRAMSTREAM &os) const
{
    int		i;

    os.write(myPosts.entries(), 8);
    for (i = 0; i < myPosts.entries(); i++)
    {
	myPosts(i).save(os);
    }
}

void
SIGNLIST::load(SRAMSTREAM &is)
{
    int		i;
    int		val;

    is.uread(val, 8);
    for (i = 0; i < val; i++)
    {
	SIGNPOSTDATA	post;
	post.load(is);

	myPosts.append(post);
    }
}
