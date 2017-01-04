/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        signpost.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Tracks signposts and their values.  Creates a heap of signposts
 *	which can be queried to get a sign at a position.  O(N) to
 *	access so be wary!
 */

#ifndef __signpost_h__
#define __signpost_h__

#include "glbdef.h"
#include "itemstack.h"

class SRAMSTREAM;

class SIGNPOSTDATA
{
public:
    SIGNPOSTDATA() {}
    SIGNPOSTDATA(int zero) {}

    void	init(SIGNPOST_NAMES sign, int x, int y);
    
    bool	isPos(int x, int y) const;
    void	getPos(int &x, int &y) const;
    SIGNPOST_NAMES	getSign() const;
    
    void	save(SRAMSTREAM &os) const;
    void	load(SRAMSTREAM &is);

protected:
    unsigned int		myPos:10;
    unsigned int		mySign:6;
};


class SIGNLIST
{
public:
    SIGNLIST();
    ~SIGNLIST();

    SIGNPOST_NAMES	find(int x, int y) const;
    void		append(SIGNPOST_NAMES sign, int x, int y);
    int			entries() const { return myPosts.entries(); }

    void	save(SRAMSTREAM &os) const;
    void	load(SRAMSTREAM &is);

protected:
    PTRSTACK<SIGNPOSTDATA>	myPosts;
};

#endif

