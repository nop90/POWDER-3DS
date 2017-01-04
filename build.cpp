/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        build.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	build.cpp contains all the routines used to build dungeon levels.
 *	These are as many and varied as are the types of levels that can
 *	be built.
 *
 *	The major hook is MAP::build, which is where the level layout
 *	is determined and used for determining which generator should
 *	be applied.
 */

#include <stdio.h>
#include <math.h>
#include "mygba.h"
#include "map.h"
#include "msg.h"
#include "rand.h"
#include "assert.h"
#include "creature.h"
#include "item.h"
#include "victory.h"

#include "rooms/allrooms.h"

void
MAP::clear()
{
    memset(mySquares, 0, MAP_WIDTH * MAP_HEIGHT);
    memset(myFlags, 0, MAP_WIDTH * MAP_HEIGHT);
    //memset(myFlags, SQUAREFLAG_LIT, MAP_WIDTH * MAP_HEIGHT);
    //memset(myFlags, SQUAREFLAG_MAPPED, MAP_WIDTH * MAP_HEIGHT);

    MOB		*mob, *mnext;
    ITEM	*item, *inext;

    for (mob = myMobHead; mob; mob = mnext)
    {
	mnext = mob->getNext();
	mob->setNext(0);
	delete mob;
    }
    myMobHead = 0;
    myMobCount = 0;

    for (item = myItemHead; item; item = inext)
    {
	inext = item->getNext();
	item->setNext(0);
	delete item;
    }
    myItemHead = 0;
    myItemCount = 0;
}

void
MAP::build(bool alreadysavedseed)
{
    // Save our random seed used...
    if (!alreadysavedseed)
	mySeed = rand_getseed();
    
    // Empty the map...
    clear();

    // Clear out our wall map.
    memset(ourWallMap, 0, MAP_WIDTH * MAP_HEIGHT);
    
    // Decide what sort of map to build.  If the current level is 10
    // we do the big room.
    // Rough map:
    // 0: Surface World.
    // 1-5: Roguelikes.
    // 6-7: Lit QIX.
    // 8-9: Unlit QIX.
    // 10: Big Room
    // 11-14: Mazes
    // 15: Cretan Minotaur
    // 16-17: Lit cavern
    // 18: Dark cavern
    // 19-20: Dark water cavern
    // 21: Dark cavern, Belweir
    // 22: Quizar
    // 23: Hruth
    // 24: Klaskov
    // 25: Circles of Hell.
    if (0)
    {
	// Stress test all room generators.
	static int ct = 0;
	ct++;
	switch (ct % 9)
	{
	    case 0:
		buildRoguelike();
		break;
	    case 1:
		buildDrunkenCavern(true, false, true, true);
		break;
	    case 2:
	    {
		static int	ct2 = 0;

		ct2++;
		switch (ct2 % 5)
		{
		    case 0:
			// We really don't want a crash here!
			buildSurfaceWorld();
			break;
		    case 1:
			buildQuest(GOD_ROGUE);
			break;
		    case 2:
			buildQuest(GOD_BARB);
			break;
		    case 3:
			buildQuest(GOD_FIGHTER);
			break;
		    case 4:
			buildTutorial();
			break;
		}
		break;
	    }
	    case 3:
		buildQix(true);
		break;
	    case 4:
		// This breaks, I eat my shoe.
		buildBigRoom();
		break;
	    case 5:
		buildMaze(rand_choice(4)+1);
		break;
	    case 6:
		buildRiverRoom();
		break;
	    case 7:
		buildHello();
		break;
	    case 8:
		buildSpaceShip();
		break;
	}

	if (!(ct & 127))
	    printf("Current room count %d\n", ct);
    }
    else if (glbStressTest && !glbMapStats)
    {
	// Pick a random depth.
	myDepth = rand_range(0, 25);
	// Pick a random branch... Currently only tridude supported.
	if (!rand_choice(15))
	    myBranch = BRANCH_TRIDUDE;
	else
	    myBranch = BRANCH_MAIN;
    }

    if (glbTutorial)
    {
	buildTutorial();
	return;
    }

    if (branchName() == BRANCH_TRIDUDE)
    {
	// Simple enough.
	buildSpaceShip();
	return;
    }

    if (myDepth == 0)
    {
	buildSurfaceWorld();
    }
    else if (myDepth <= 5)
    {
	//buildQix(true);
	//buildQuest(GOD_ROGUE);
	buildRoguelike();
    }
    else if (myDepth >= 6 && myDepth <= 7)
    {
	buildQix(true);
    }
    else if (myDepth >= 8 && myDepth <= 9)
    {
	buildQix(false);
    }
    else if (myDepth == 10)
    {
	buildBigRoom();
    }
    else if (myDepth >= 11 && myDepth <= 15)
    {
	buildMaze(rand_choice(4)+1);
    }
    else if (myDepth >= 16 && myDepth <= 17)
    {
	buildDrunkenCavern(true, false, true, true);
    }
    else if (myDepth >= 18 && myDepth <= 18)
    {
	buildDrunkenCavern(false, false, true, true);
    }
    else if (myDepth >= 19 && myDepth <= 20)
    {
	buildDrunkenCavern(false, true, true, true);
	// If depth is 20, we want hard floors to prevent cheating!
	myGlobalFlags &= ~MAPFLAG_DIG;
    }
    else if (myDepth == 21)
    {
	// Belweir's library is here!
	buildDrunkenCavern(false, false, true, true);
    }
    else if (myDepth == 22)
    {
	buildQuest(GOD_ROGUE);
    }
    else if (myDepth == 23)
    {
	buildQuest(GOD_BARB);
    }
    else if (myDepth == 24)
    {
	buildQuest(GOD_FIGHTER);
    }
    else if (myDepth == 25)
    {
	buildHello();
    }
    else
    {
	// This should not happen.
	buildRoguelike();
    }
    
    // Build some random traps.
    int 	numtraps, i;

    numtraps = rand_choice(myDepth);
    for (i = 0; i < numtraps; i++)
    {
	int		tries = 10;
	int		x, y;

	while (tries--)
	{
	    if (!findRandomLoc(x, y, MOVE_WALK,
			    true, false, false, false, false, false))
	    {
		tries = 0;
		break;
	    }

	    if (createTrap(x, y, 0))
		break;
	}
	// Failed to find after 10 tries? Give up on all traps!
	if (!tries)
	    break;
    }

    // Verify we have appropriate up and down stairs
#ifdef USEASSERT
    if (myDepth)
    {
	int x, y;
	UT_ASSERT(findTile(SQUARE_LADDERUP, x, y));
    }
    if (myDepth < 25)
    {
	int x, y;
	UT_ASSERT(findTile(SQUARE_LADDERDOWN, x, y));
    }
#endif
}

void
MAP::roomToMapCoords(int &x, int &y, const ROOM &room, int rx, int ry)
{
    int		tmp;

    if (room.rotation & ROOM_FLIP_X)
	x = room.definition->size.x - rx - 1;
    else
	x = rx;
    if (room.rotation & ROOM_FLIP_Y)
	y = room.definition->size.y - ry - 1;
    else
	y = ry;
    if (room.rotation & ROOM_FLOP)
    {
	tmp = x;
	x = y;
	y = tmp;
    }
    x += room.l.x;
    y += room.l.y;
}

void
MAP::mapToRoomCoords(int &rx, int &ry, const ROOM &room, int x, int y)
{
    int		tmp;
    
    x -= room.l.x;
    y -= room.l.y;
    if (room.rotation & ROOM_FLOP)
    {
	tmp = x;
	x = y;
	y = tmp;
    }
    if (room.rotation & ROOM_FLIP_Y)
	ry = room.definition->size.y - y - 1;
    else
	ry = y;
    if (room.rotation & ROOM_FLIP_X)
	rx = room.definition->size.x - x - 1;
    else
	rx = x;
}

// Draws a walled room using the inclusive rectangle specified.
void
MAP::drawRoom(const ROOM &room, bool lit, bool mazetype, bool caverntype)
{
    int		x, y;

    if (room.definition)
    {
	int		squarenum;
	int		rx, ry, i;

	squarenum = 0;
	for (ry = 0; ry < room.definition->size.y; ry++)
	{
	    for (rx = 0; rx < room.definition->size.x; rx++)
	    {
		roomToMapCoords(x, y, room, rx, ry);
		ourWallMap[y * MAP_WIDTH + x] = 1;
		SQUARE_NAMES		square;

		square = room.definition->squarelist[squarenum];
		if (caverntype && square == SQUARE_WALL)
		    square = SQUARE_EMPTY;
		if (caverntype && square == SQUARE_FLOOR)
		    square = SQUARE_CORRIDOR;
		if (mazetype && square == SQUARE_CORRIDOR)
		    square = SQUARE_FLOOR;
		if (mazetype && square == SQUARE_EMPTY)
		    square = SQUARE_WALL;
		setTile(x, y, square);

		squarenum++;
	    }
	}

	for (i = 0; room.definition->squareflaglist[i].data != SQUAREFLAG_NONE;
		i++)
	{
	    rx = room.definition->squareflaglist[i].x;
	    ry = room.definition->squareflaglist[i].y;
	    roomToMapCoords(x, y, room, rx, ry);
	    setFlag(x, y, room.definition->squareflaglist[i].data);
	}
	
	return;
    }

    for (y = room.l.y; y <= room.h.y; y++)
    {
        for (x = room.l.x; x <= room.h.x; x++)
	{
	    // Mark the entire room in the wall map.
	    ourWallMap[y * MAP_WIDTH + x] = 1;

	    if (x == room.l.x || x == room.h.x ||
		y == room.l.y || y == room.h.y)
	    {
		// Draw a wall.
		setTile(x, y, caverntype ? SQUARE_EMPTY : SQUARE_WALL);
		setFlag(x, y, SQUAREFLAG_LIT, lit);
	    }
	    else
	    {
		// Draw the floor.
		setTile(x, y, caverntype ? SQUARE_CORRIDOR : SQUARE_FLOOR);
		setFlag(x, y, SQUAREFLAG_LIT, lit);
	    }
	}
    }
}

bool
MAP::isMapExit(int x, int y) const
{
    UT_ASSERT(x >= 0 && x <= MAP_WIDTH);
    UT_ASSERT(y >= 0 && y <= MAP_HEIGHT);
    switch (getTile(x, y))
    {
	case SQUARE_DOOR:
	case SQUARE_BLOCKEDDOOR:
	case SQUARE_SECRETDOOR:
	case SQUARE_CORRIDOR:
	case SQUARE_FLOOR:
	case SQUARE_OPENDOOR:
	    return true;
	default:
	    break;
    }
    return false;
}

bool
MAP::isRoomExit(const ROOM &room, int x, int y) const
{
    UT_ASSERT(x >= 0 && x < room.definition->size.x);
    UT_ASSERT(y >= 0 && y < room.definition->size.y);

    UT_ASSERT(room.definition != 0);

    switch (room.definition->squarelist[x + y * room.definition->size.x])
    {
	case SQUARE_DOOR:
	case SQUARE_BLOCKEDDOOR:
	case SQUARE_SECRETDOOR:
	case SQUARE_CORRIDOR:
	case SQUARE_FLOOR:
	case SQUARE_OPENDOOR:
	    return true;

	case SQUARE_WATER:
	case SQUARE_LAVA:
	case SQUARE_ACID:
	default:
	    return false;
    }
}

bool
MAP::isMapPathway(int x, int y) const
{
    if (x < 0 || x >= MAP_WIDTH)
	return false;
    if (y < 0 || y >= MAP_HEIGHT)
	return false;

    switch (getTile(x, y))
    {
	case SQUARE_CORRIDOR:
	case SQUARE_FLOOR:
	case SQUARE_WATER:
	case SQUARE_LAVA:
	case SQUARE_ACID:
	    return true;
	default:
	    break;
    }
    return false;
}

bool
MAP::isTileWall(int tile) const
{
    switch (tile)
    {
	case SQUARE_WALL:
	case SQUARE_DOOR:
	case SQUARE_BLOCKEDDOOR:
	case SQUARE_SECRETDOOR:
	case SQUARE_OPENDOOR:
	    return true;
	default:
	    break;
    }
    return false;
}

bool
MAP::isMapWall(int x, int y) const
{
    // The -1 for width & hieght here is to intentionally leave a 1 space
    // border on the edges so we can scroll past the edges.
    if (x < 0 || x >= MAP_WIDTH-1)
	return true;
    if (y < 0 || y >= MAP_HEIGHT-1)
	return true;

    if (ourWallMap[y * MAP_WIDTH + x])
	return true;

    return false;
}

//
// Logic for setting a tile on a path...
//
void
MAP::setPathTile(int x, int y, bool lit, bool usefloor)
{
    SQUARE_NAMES	oldtile, newtile;

    oldtile = getTile(x, y);
    newtile = oldtile;		// Do nothing by default.
    switch (oldtile)
    {
	case SQUARE_EMPTY:
	    if (usefloor)
		newtile = SQUARE_FLOOR;
	    else
		newtile = SQUARE_CORRIDOR;
	    break;
	case SQUARE_FLOOR:
	    newtile = SQUARE_FLOOR;
	    break;
	case SQUARE_CORRIDOR:
	    if (usefloor)
		newtile = SQUARE_FLOOR;
	    else
		newtile = SQUARE_CORRIDOR;
	    break;
	case SQUARE_BLOCKEDDOOR:    
	    // We only have two choices here.  We don't want to
	    // turn into a corridor or normal door or we allow creatures
	    // to escape.  Secret door is acceptable as creatures don't
	    // yet search by default
	    switch (rand_choice(6))
	    {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		    newtile = SQUARE_BLOCKEDDOOR;
		    lit = true;
		    break;
		case 5:
		    newtile = SQUARE_SECRETDOOR;
		    // Inherit existing lit flag.
		    lit = getFlag(x, y, SQUAREFLAG_LIT);
		    break;
	    }
	    
	    break;
	case SQUARE_DOOR:    
	case SQUARE_WALL:
	case SQUARE_SECRETDOOR:
	case SQUARE_OPENDOOR:
	    // We have several choices:
	    // 1) Secret door
	    // 2) Door
	    // 3) No door.
	    switch (rand_choice(6))
	    {
		case 0:
		case 1:
		case 2:
		    newtile = SQUARE_DOOR;
		    lit = true;
		    break;
		case 3:
		    newtile = SQUARE_OPENDOOR;
		    lit = true;
		    break;
		case 4:
		    if (usefloor)
			newtile = SQUARE_FLOOR;
		    else
			newtile = SQUARE_CORRIDOR;
		    break;
		case 5:
		    newtile = SQUARE_SECRETDOOR;
		    // Inherit existing lit flag.
		    lit = getFlag(x, y, SQUAREFLAG_LIT);
		    break;
	    }
	    break;
	// Error checking:    
	default:
	    newtile = SQUARE_WATER;
	    lit = true;
	    break;
    }

    // Set the tile.
    setTile(x, y, newtile);
    setFlag(x, y, SQUAREFLAG_LIT, lit);
}

//
// This initializes ourDistanceMap with the distance to go, including
// tunnelling as allowed.  Thus, if isMapWall, we can't go through it.
// If we must tunnel, distance is 2.
// We return false if there is no path (should not happen!)
//
#define STACK_SIZE 2048

bool
MAP::buildMoveDistance(int px, int py, MOVE_NAMES move)
{
    s8		stackx[STACK_SIZE], stacky[STACK_SIZE];
    int		stackdepth = 0, stackbot = 0, stacktop = 0;
    int		dist, nextdist;
    int		dx, dy, x, y;
    
    // Initialize to infinity.
    memset(ourDistanceMap, 0xff, MAP_WIDTH * MAP_HEIGHT * sizeof(u16));

    // Zero distance to self.
    ourDistanceMap[py * MAP_WIDTH + px] = 0;
    
    // Add to our distance stack...
    stackx[stacktop] = px;
    stacky[stacktop] = py;
    stacktop++;
    stackdepth++;
    stacktop &= STACK_SIZE-1;
    
    // And recurse...
    while (stackdepth)
    {
	// Pull top guy off the stack...
	stackdepth--;
	px = stackx[stackbot];
	py = stacky[stackbot];
	stackbot++;

	// Now, see if anyone will have a shorter distance if they came
	// from here...
	dist = ourDistanceMap[py * MAP_WIDTH + px];

	FORALL_4DIR(dx, dy)
	{
	    y = py + dy;
	    if (y < 0)
		continue;
	    if (y > MAP_HEIGHT-1)
		continue;

	    x = px + dx;
	    if (x < 0)
		continue;
	    if (x > MAP_WIDTH-1)
		continue;

	    // See if we can go there...
	    if (// canMove(x, y, move, true, true, true) ||
		(glb_squaredefs[getTile(x, y)].movetype & move) ||
		getTile(x, y) == SQUARE_SECRETDOOR ||
		getTile(x, y) == SQUARE_DOOR ||
		getTile(x, y) == SQUARE_BLOCKEDDOOR)
	    {
		nextdist = 1;
		nextdist += dist;
		// Check if it is strictly faster...
		if (nextdist < ourDistanceMap[y * MAP_WIDTH + x])
		{
		    // Store it.
		    ourDistanceMap[y * MAP_WIDTH + x] = nextdist;
		    // Add to our stack.
		    stackx[stacktop] = x;
		    stacky[stacktop] = y;
		    stackdepth++;
		    stacktop++;
		    stacktop &= STACK_SIZE-1;
		    UT_ASSERT(stackdepth < STACK_SIZE-2);
		}
	    }
	}
    }

    return true;
}

bool
MAP::buildDistance(int px, int py)
{
    s8		stackx[STACK_SIZE], stacky[STACK_SIZE];
    int		stackdepth = 0;
    u16		dist, nextdist;
    int		dx, dy, x, y;
    
    // Initialize to infinity.
    memset(ourDistanceMap, 0xff, MAP_WIDTH * MAP_HEIGHT * sizeof(u16));

    // Zero distance to self.
    ourDistanceMap[py * MAP_WIDTH + py] = 0;
    
    // Add to our distance stack...
    stackx[stackdepth] = px;
    stacky[stackdepth] = py;
    stackdepth++;
    
    // And recurse...
    while (stackdepth)
    {
	// Pull top guy off the stack...
	stackdepth--;
	px = stackx[stackdepth];
	py = stacky[stackdepth];

	// Now, see if anyone will have a shorter distance if they came
	// from here...
	dist = ourDistanceMap[py * MAP_WIDTH + px];

	for (dy = -1; dy <= 1; dy++)
	{
	    y = py + dy;
	    if (y < 0)
		continue;
	    if (y >= MAP_HEIGHT-1)
		break;
	    for (dx = -1; dx <= 1; dx++)
	    {
		if (!dx && !dy)
		    continue;

		x = px + dx;
		if (x < 0)
		    continue;
		if (x >= MAP_WIDTH-1)
		    break;

		// See if we can go there...
		if (!isMapWall(x, y))
		{
		    nextdist = (getTile(x, y) == SQUARE_EMPTY) ? 2 : 1;
		    //nextdist = 1;
		    nextdist += dist;
		    // Check if it is strictly faster...
		    if (nextdist < ourDistanceMap[y * MAP_WIDTH + x])
		    {
			// Store it.
			ourDistanceMap[y * MAP_WIDTH + x] = nextdist;
			// Add to our stack.
			stackx[stackdepth] = x;
			stacky[stackdepth] = y;
			stackdepth++;
			UT_ASSERT(stackdepth < STACK_SIZE);
		    }
		}
	    }
	}
    }

    return true;
}

//
// Draw path using a distance function.
//
bool
MAP::drawPathStraight(int sx, int sy, int ex, int ey, bool lit)
{
    u16		dist, dp, dn;
    int		d[2];
    int		dir;
    int		s[2], e[2];
    int		rev = 0;
    
    buildDistance(ex, ey);
    UT_ASSERT(!"Got to draw...");

    // Check to see if a path is possible....
    if (ourDistanceMap[sy * MAP_WIDTH + sx] == 0xffff)
    {
	UT_ASSERT(!"Impossibly map!");
	return false;
    }

    s[0] = sx;
    s[1] = sy;
    e[0] = ex;
    e[1] = ey;
    
    // Initial random direction.
    dir = rand_choice(2);

    setPathTile(s[0], s[1], lit, false);
    
    while (s[0] != e[0] || s[1] != e[1])
    {
	// Determine which way is down in dir....
	dist = ourDistanceMap[s[1] * MAP_WIDTH + s[0]];

	d[!dir] = 0;
	dp = 0xffff;
	dn = 0xffff;
	if ((s[dir] + 1) < MAP_WIDTH)
	{
	    d[dir] = 1;
	    dp = ourDistanceMap[(s[1] + d[1]) * MAP_WIDTH + s[0] + d[0]];
	}
	if ((s[dir] - 1) >= 0)
	{
	    d[dir] = -1;
	    dn = ourDistanceMap[(s[1] + d[1]) * MAP_WIDTH + s[0] + d[0]];
	}
	if (dp >= dist && dn >= dist)
	{
	    // No one has a faster distance!  Change direction.
	    UT_ASSERT(!rev);
	    rev++;
	    dir = !dir;
	    continue;
	}
	if (dp < dist && dn < dist)
	{
	    // Both are shorter, this is a cusp which by definition
	    // cannot last.  Pick a random direction.
	    d[dir] = rand_sign();
	}
	else if (dp < dist)
	{
	    d[dir] = 1;
	}
	else
	{
	    UT_ASSERT(dn < dist);
	    // Dn is smallest.
	    d[dir] = -1;
	}
	rev = 0;

	// Go in this direction until it is unprofitable...
	while (s[0] != e[0] || s[1] != e[1])
	{
	    setPathTile(s[0], s[1], lit, false);
	    s[0] += d[0];
	    s[1] += d[1];

	    // Check if next tile is good...
	    dist = ourDistanceMap[s[1] * MAP_WIDTH + s[0]];

	    if (s[dir] + d[dir] < 0)
		break;
	    if (s[dir] + d[dir] >= MAP_WIDTH)
		break;
	    if (ourDistanceMap[(s[1]+d[1]) * MAP_WIDTH + s[0] + d[0]] >= dist)
	    {
		// No longer profitable, stop moving this way!
		break;
	    }
	}
    }
    setPathTile(s[0], s[1], lit, false);
    return true;
}

//
// Draw a path from the given location to the given location.
// This will avoid walls.
// It will return if it failed.  It shouldn't, but shit happens.
//
bool
MAP::drawPath(int sx, int sy, int ex, int ey, bool lit)
{
    int		d[2];
    int		i, dir, wallhit;
    int		s[2], e[2], n[2];
    int		len = 0;
    int		ncnt = 0;

    s[0] = sx;
    s[1] = sy;
    e[0] = ex;
    e[1] = ey;
    
    // Initial random direction.
    dir = rand_choice(2);

    setPathTile(s[0], s[1], lit, false);
    
    while (s[0] != e[0] || s[1] != e[1])
    {
	// Find our current direction to go...
	for (i = 0; i < 2; i++)
	    d[i] = SIGN(e[i] - s[i]);

	// Switch our direction.
	dir = !dir;
	
	// If we are aligned in that direction, try the opposite.
	if (!d[dir])
	    dir = !dir;

	// Move in the direction dir until a random chance fails,
	// or we get aligned with a destination.
	wallhit = 0;
	while (1)
	{
	    if (rand_chance(40))
	    {
		// Stop digging!
		break;
	    }

	    // Alignment...  This only counts for straight moves,
	    // if bounced off a wall we don't want to trigger this.
	    if (!wallhit && s[dir] == e[dir])
		break;

	    // Calculate the next pos...
	    n[0] = s[0];
	    n[1] = s[1];
	    n[dir] += d[dir];
	    // Check if our current dig direction is valid...
	    ncnt = 0;
	    while (isMapWall(n[0], n[1]))
	    {
		// Not a valid dig direction!  Rotate 90 degrees
		// and go in a random direction...
		// As we never build walls adjacent and they are square,
		// this guarantees a non-wall.
		// Except it doesn't.  WTF?
		dir = !dir;
		if (!d[dir])
		    d[dir] = rand_sign();
		n[0] = s[0];
		n[1] = s[1];
		n[dir] += d[dir];
		wallhit = 1;
		ncnt++;
		if (ncnt > 100)
		{
		    // THis fails when we hit the bottom most wall,
		    // turn to the left,
		    // and then hit the left wall prior to deciding to reset
		    // NOte that alignment will never trigger as wallhit
		    // is true, and we could have been going the wrong
		    // way anyways.
		    // Our abort here is reasonable enough.
		    return false;
		}
	    }
	    // Move in the desired direction...
	    s[dir] += d[dir];
	    // Write out the path tile...
	    setPathTile(s[0], s[1], lit, false);

	    len++;
	    if (len > 100)
		return false;
	}
    }
    return true;
}

void
MAP::drawPath_old(int sx, int sy, int ex, int ey, bool lit)
{
    int		dx = 0, dy = 0, dir, dirlen;
    
    dir = 0;		// 1 is y, -1 is x.
    while (sx != ex || sy != ey)
    {
	// Choose our direction...
	dx = rand_sign();
	if (sx < ex)
	    dx = 1;
	else if (sx > ex)
	    dx = -1;
	dy = rand_sign();
	if (sy < ey)
	    dy = 1;
	else if (sy > ey)
	    dy = -1;

	// Every so often the builder is drunk...
	if (0) // rand_chance(30))
	{
	    dx *= -1;
	    dy *= -1;
	}

	if (dir != 0)
	    dir *= -1;
	else
	{
	    if (dx && dy)
	    {
		dir = rand_sign();
	    }
	    else if (dx)
		dir = -1;
	    else
		dir = 1;
	}

	if (dir < 0)
	    dy = 0;
	else
	    dx = 0;

	// Find max len...
	if (dir < 0)
	    dirlen = abs(ex - sx);
	else
	    dirlen = abs(ey - sy);

	if (dirlen > 3)
	{
	    // Move at least 3, but choose a randome length otherwise...
	    dirlen = rand_range(3, dirlen);
	}

	// Move dirlen or a wall, whatever is first...
	while (dirlen)
	{
	    setPathTile(sx, sy, lit, false);
	    if (isMapWall(sx + dx, sy + dy))
	    {
		// Don't want to tunnel, so quit this path.
		if (!(dx && dy))
		    return;
		break;
	    }
	    // Move forward...
	    sx += dx;
	    sy += dy;
	    dirlen--;
	}
    }
    // Hit the end, draw the last square...
    setPathTile(ex, ey, lit, false);
}

bool
MAP::drawCorridor(int sx, int sy, int sdx, int sdy,
	     int ex, int ey, int edx, int edy,
	     bool lit)
{
    bool		success;
    
    success = drawPath(sx+sdx, sy+sdy, ex+edx, ey+edy, lit);
    
    // Draw our doors.  We only do this if our path creation
    // was successful.
    if (success)
    {
	setPathTile(sx, sy, true, false);
	setPathTile(ex, ey, true, false);
    }
    return success;
}

//
// Checks to see if we can build the specified room.
// The room itself must be filled with void, for one space
// around the room we should have no walls (don't want touching rooms
// as that will annoy the path builder)
// The boundary of the map counts as a wall for this purpose.
//
bool
MAP::checkRoomFits(const ROOM &room)
{
    int		x, y;

    for (y = room.l.y; y <= room.h.y; y++)
        for (x = room.l.x; x <= room.h.x; x++)
	    if (getTile(x, y) != SQUARE_EMPTY)
		return false;
    
    // Test the border...
    for (x = room.l.x-1; x <= room.h.x+1; x++)
    {
	if (isMapWall(x, room.l.y-1))
	    return false;
	if (isMapWall(x, room.h.y+1))
	    return false;
    }
    // And the x sides...
    for (y = room.l.y-1; y <= room.h.y+1; y++)
    {
	if (isMapWall(room.l.x-1, y))
	    return false;
	if (isMapWall(room.h.x+1, y))
	    return false;
    }

    // Everything passes...
    return true;
}

const ROOM_DEF *
MAP::chooseRandomRoom(int depth)
{
    int		 prob[NUM_ALLROOMDEFS];
    const ROOM_DEF	*def;
    int		 i, totalprob = 0, offset;

    for (i = 0; i < NUM_ALLROOMDEFS; i++)
    {
	def = glb_allroomdefs[i];

	prob[i] = def->rarity;

	if (def->minlevel >= 0 &&
	    def->minlevel > depth)
	    prob[i] = 0;
	if (def->maxlevel >= 0 &&
	    def->maxlevel < depth)
	    prob[i] = 0;

	// Mandatory rooms are always built on any level that
	// they are legal.
	if (prob[i] && def->mandatory)
	    return def;

	totalprob += prob[i];
    }
    
    offset = rand_choice(totalprob);
    for (i = 0; i < NUM_ALLROOMDEFS; i++)
    {
	if (offset < prob[i])
	    break;
	offset -= prob[i];
    }
    if (i >= NUM_ALLROOMDEFS)
    {
	UT_ASSERT(!"Could not find a room!");
	i = NUM_ALLROOMDEFS-1;
    }

    return glb_allroomdefs[i];
}

void
MAP::buildRandomRoom(ROOM &room)
{
    // We don't let the room lie on a map boundary as then if it gets
    // an exit on that side, the corridor won't be able to propogate.
    room.l.x = rand_range(1, MAP_WIDTH-4);
    room.l.y = rand_range(1, MAP_HEIGHT-4);
    room.h.x = rand_range(room.l.x + 3, MIN(room.l.x + 8, MAP_WIDTH-2));
    room.h.y = rand_range(room.l.y + 3, MIN(room.l.y + 8, MAP_HEIGHT-2));
    room.definition = 0;
}

void
MAP::buildRandomRoomFromDefinition(ROOM &room, const ROOM_DEF *def)
{
    UT_ASSERT((int)def);
    if (!def)
    {
	buildRandomRoom(room);
	return;
    }

    int			width, height;

    room.rotation = rand_choice(8);

    if (room.rotation & ROOM_FLOP)
    {
	width = def->size.y;
	height = def->size.x;
    }
    else
    {
	width = def->size.x;
	height = def->size.y;
    }
    
    // We do a bit of a border on these rooms to ensure
    // that we can always force exits on the maze levels.
    room.l.x = rand_range(3, MAP_WIDTH - 3 - width);
    room.l.y = rand_range(3, MAP_HEIGHT - 3 - height);

    // If the room overrides x or y, override the value.
    if (def->x >= 0)
	room.l.x = def->x;
    if (def->y >= 0)
	room.l.y = def->y;
    
    // Note room's are INCLUSIVE coords, while size defines
    // exclusive.
    room.h.x = room.l.x + width - 1;
    room.h.y = room.l.y + height - 1;

    UT_ASSERT(room.l.x >= 0);
    UT_ASSERT(room.h.x < MAP_WIDTH);
    UT_ASSERT(room.l.y >= 0);
    UT_ASSERT(room.h.y < MAP_HEIGHT);

    room.definition = def;
}

void
MAP::closeUnusedExits(const ROOM &room, bool force)
{
    if (room.definition)
    {
	// Only close on special rooms.
	int		x, y, rx, ry;
	int		lastx = 0, lasty = 0;
	int		lastdx = 0, lastdy = 0;
	SQUARE_NAMES	lastsquare = SQUARE_FLOOR;

	for (x = room.l.x; x <= room.h.x; x++)
	{
	    mapToRoomCoords(rx, ry, room, x, room.l.y);
	    if (isRoomExit(room, rx, ry))
	    {
		lastx = x;	lasty = room.l.y;
		lastdx = 0;	lastdy = -1;
		lastsquare = getTile(x, room.l.y);
		if (!isMapPathway(x, room.l.y-1))
		    setTile(x, room.l.y, 
			    getTile(x-1, room.l.y));
		else
		    force = false;
	    }
	    mapToRoomCoords(rx, ry, room, x, room.h.y);
	    if (isRoomExit(room, rx, ry))
	    {
		lastx = x;	lasty = room.h.y;
		lastdx = 0;	lastdy = 1;
		lastsquare = getTile(x, room.h.y);
		if (!isMapPathway(x, room.h.y+1))
		    setTile(x, room.h.y, 
			    getTile(x-1, room.h.y));
		else
		    force = false;
	    }
	}
	for (y = room.l.y; y <= room.h.y; y++)
	{
	    mapToRoomCoords(rx, ry, room, room.l.x, y);
	    if (isRoomExit(room, rx, ry))
	    {
		lastx = room.l.x;	lasty = y;
		lastdx = -1;		lastdy = 0;
		lastsquare = getTile(room.l.x, y);
		if (!isMapPathway(room.l.x-1, y))
		    setTile(room.l.x, y, 
			    getTile(room.l.x, y-1));
		else
		    force = false;
	    }
	    mapToRoomCoords(rx, ry, room, room.h.x, y);
	    if (isRoomExit(room, rx, ry))
	    {
		lastx = room.h.x;	lasty = y;
		lastdx = 1;		lastdy = 0;
		lastsquare = getTile(room.h.x, y);
		if (!isMapPathway(room.h.x+1, y))
		    setTile(room.h.x, y, 
			    getTile(room.h.x, y-1));
		else
		    force = false;
	    }
	}

	// TODO: Have option to wipe room if doesn't work out
	// nicely.

	if (force)
	{
	    // We must force at least one exit.
	    // We dig continuously in the direction given by our last exit
	    // until we find a way out.
	    UT_ASSERT(lastdx || lastdy);
	    if (lastdx || lastdy)
	    {
		bool			found = false;
		SQUARE_NAMES		square = SQUARE_WALL;
		int			pass;
		int			curdx = 0, curdy = 0;

		for (pass = 0; pass < 3; pass++)
		{
		    found = true;

		    // Determine initial x & curdx & curdy.
		    switch (pass)
		    {
			case 0:		// Straight
			    curdx = lastdx;
			    curdy = lastdy;
			    break;
			case 1:		// Turn
			    curdx = !lastdx;
			    curdy = !lastdy;
			    break;
			case 2:		// Other turn.
			    curdx = -!lastdx;
			    curdy = -!lastdy;
			    break;
		    }
		    // Always move in last delta direction because
		    // that is the one that will get us outside of the
		    // container.
		    x = lastx+lastdx;
		    y = lasty+lastdy;

		    // Only prefetch this if it has a chance of being valid.
		    if (isMapPathway(x, y))
			square = getTile(x, y);
		    while (!isMapPathway(x, y))
		    {
			if (x < 0 || x >= MAP_WIDTH ||
			    y < 0 || y >= MAP_HEIGHT)
			{
			    // complete failure.
			    found = false;
			    break;
			}
			// Check adjacent squares to see if this opened up.
			if (isMapPathway(x + !curdx, y + !curdy))
			{
			    square = getTile(x + !curdx, y + !curdy);
			    break;
			}
			if (isMapPathway(x - !curdx, y - !curdy))
			{
			    square = getTile(x - !curdx, y - !curdy);
			    break;
			}
			x += curdx;
			y += curdy;
			if (isMapPathway(x, y))
			    square = getTile(x, y);
		    }

		    if (found)
		    {
			lastx += lastdx;
			lasty += lastdy;
			break;
		    }
		}
		
		if (found)
		{
		    // We found a direction!  Fill it in!
		    while (x != lastx || y != lasty)
		    {
			setTile(x, y, square);
			x -= curdx;
			y -= curdy;
		    }
		    setTile(x, y, square);

		    // Restore the last square.
		    {
			x -= lastdx;
			y -= lastdy;
			setTile(x, y, lastsquare);
		    }
		}
	    }
	}
    }
}

void
MAP::populateRoom(const ROOM &room)
{
    if (!room.definition)
	return;

    // It is very important we apply the room flags before we test
    // if loading as we want reloaded rooms to have the same flags
    // Otherwise populate traps will behave differently as it will
    // think it can make holes (and players can zap wands all of a 
    // sudden)

    // Apply the flags from the room definition.
    if (!room.definition->allowgeneration)
	myGlobalFlags &= ~MAPFLAG_NEWMOBS;
    if (!room.definition->allowdig)
	myGlobalFlags &= ~MAPFLAG_DIG;
    if (!room.definition->allowitems)
	myGlobalFlags |= MAPFLAG_NOITEMS;
    if (!room.definition->allowtel)
	myGlobalFlags |= MAPFLAG_NOTELE;

    // If we are loading from disk, we've already populated the room
    // from disk.
    if (ourIsLoading)
	return;

    RAND_STATEPUSH	saverng;

    int		x, y;
    const PT2	*pt;
    MOB		*mob;
    ITEM	*item;
    MOBSTACK	 roommobs;

    // Create listed mobs.
    for (pt = room.definition->moblist; pt->data != MOB_NONE; pt++)
    {
	mob = MOB::create((MOB_NAMES) pt->data);
	
	roomToMapCoords(x, y, room, pt->x, pt->y);
	mob->move(x, y, true);
	registerMob(mob);
	roommobs.append(mob);
    }

    int			i;
    MOB			*leader = 0;

    // Create mobs of the given level.
    for (pt = room.definition->moblevellist; pt->data != MOBLEVEL_NONE; pt++)
    {
	int			threat;

	// If there is already a mob here, uniquify it.
	if (pt->data == MOBLEVEL_UNIQUE)
	{
	    roomToMapCoords(x, y, room, pt->x, pt->y);
	    mob = getMob(x, y);
	    if (mob)
	    {
		mob->makeUnique();
		continue;
	    }
	}

	threat = getThreatLevel();
	threat += glb_mobleveldefs[pt->data].maxlevel;

	mob = MOB::createNPC(threat);

	if (pt->data == MOBLEVEL_UNIQUE)
	    mob->makeUnique();
	
	roomToMapCoords(x, y, room, pt->x, pt->y);
	mob->move(x, y, true);
	registerMob(mob);
	roommobs.append(mob);
    }

    // Apply intrinsics to any mobs:
    for (pt = room.definition->intrinsiclist; pt->data != INTRINSIC_NONE; pt++)
    {
	MOB		*mob;

	roomToMapCoords(x, y, room, pt->x, pt->y);
	mob = getMob(x, y);
	if (mob)
	{
	    mob->setIntrinsic((INTRINSIC_NAMES) pt->data);
	    if (pt->data == INTRINSIC_LEADER)
	    {
		leader = mob;
	    }
	}
    }

    // If there is a leader, tame all the room's mobs to it.
    // Ideally we'd do something tad smarter to allow multiple
    // groups to be made.
    if (leader)
    {
	for (i = 0; i < roommobs.entries(); i++)
	{
	    // Don't self tame :>
	    if (roommobs(i) == leader)
		continue;

	    roommobs(i)->makeSlaveOf(leader);
	}
    }

    // Create the listed items.
    for (pt = room.definition->itemlist; pt->data != ITEM_NONE; pt++)
    {
	item = ITEM::create((ITEM_NAMES) pt->data);

	roomToMapCoords(x, y, room, pt->x, pt->y);
	acquireItem(item, x, y, 0);
    }
    
    // Create the listed item types.
    for (pt = room.definition->itemtypelist; pt->data != ITEMTYPE_NONE; pt++)
    {
	item = ITEM::createRandomType((ITEMTYPE_NAMES) pt->data);

	roomToMapCoords(x, y, room, pt->x, pt->y);
	acquireItem(item, x, y, 0);
    }

    // Create the signposts
    for (pt = room.definition->signpostlist; pt->data != SIGNPOST_NONE; pt++)
    {
	roomToMapCoords(x, y, room, pt->x, pt->y);
	addSignPost((SIGNPOST_NAMES) pt->data, x, y);
    }
}

void
MAP::addSignPost(SIGNPOST_NAMES post, int x, int y)
{
    if (!mySignList)
	mySignList = new SIGNLIST;

    mySignList->append(post, x, y);
}

void
MAP::chooseRandomExit(int *entrance, int *delta, const ROOM &room)
{
    if (room.definition)
    {
	// We need to find valid door squares on the boundary
	// only these can be exits.
	int		x, y;
	int		*estack, *dstack;
	int		numexit = 0, choice;

	// Maximum size of the stacks is the perimeter of the room.
	estack = new int[ 2 * (room.h.x - room.l.x + 1) +
			  2 * (room.h.y - room.l.y + 1) ];
	dstack = new int[ 2 * (room.h.x - room.l.x + 1) +
			  2 * (room.h.y - room.l.y + 1) ];

	for (x = room.l.x; x <= room.h.x; x++)
	{
	    if (isMapExit(x, room.l.y))
	    {
		estack[numexit*2] = x;
		estack[numexit*2+1] = room.l.y;
		dstack[numexit*2] = 0;
		dstack[numexit*2+1] = -1;
		numexit++;
	    }
	    if (isMapExit(x, room.h.y))
	    {
		estack[numexit*2] = x;
		estack[numexit*2+1] = room.h.y;
		dstack[numexit*2] = 0;
		dstack[numexit*2+1] = 1;
		numexit++;
	    }
	}
	for (y = room.l.y; y <= room.h.y; y++)
	{
	    if (isMapExit(room.l.x, y))
	    {
		estack[numexit*2] = room.l.x;
		estack[numexit*2+1] = y;
		dstack[numexit*2] = -1;
		dstack[numexit*2+1] = 0;
		numexit++;
	    }
	    if (isMapExit(room.h.x, y))
	    {
		estack[numexit*2] = room.h.x;
		estack[numexit*2+1] = y;
		dstack[numexit*2] = 1;
		dstack[numexit*2+1] = 0;
		numexit++;
	    }
	}

	UT_ASSERT(numexit);
	choice = rand_choice(numexit);

	entrance[0] = estack[choice*2];
	entrance[1] = estack[choice*2+1];
	delta[0] = dstack[choice*2];
	delta[1] = dstack[choice*2+1];
	
	delete [] estack;
	delete [] dstack;

	return;
    }
    
    switch (rand_choice(4))
    {
	case 0:
	    entrance[0] = room.l.x;
	    entrance[1] = rand_range(room.l.y+1, room.h.y-1);
	    delta[0] = -1;
	    delta[1] = 0;
	    break;
	case 1:		
	    entrance[0] = room.h.x;
	    entrance[1] = rand_range(room.l.y+1, room.h.y-1);
	    delta[0] = 1;
	    delta[1] = 0;
	    break;
	case 2:
	    entrance[0] = rand_range(room.l.x+1, room.h.x-1);
	    entrance[1] = room.l.y;
	    delta[0] = 0;
	    delta[1] = -1;
	    break;
	case 3:		
	    entrance[0] = rand_range(room.l.x+1, room.h.x-1);
	    entrance[1] = room.h.y;
	    delta[0] = 0;
	    delta[1] = 1;
	    break;
    }
}

void
MAP::buildBigRoom()
{
    ROOM		room;
    int			x, y;

    room.l.x = 1;
    room.l.y = 1;
    room.h.x = 30;
    room.h.y = 30;
    room.definition = 0;
    drawRoom(room, true, false, false);

    // Build random up & down stair cases.
    while (1)
    {
	x = rand_range(2, 29);
	y = rand_range(2, 29);

	if (getTile(x, y) == SQUARE_FLOOR)
	{
	    setTile(x, y, SQUARE_LADDERUP);
	    // setTile(x-1, y, SQUARE_LADDERDOWN);
	    break;
	}
    } 
    while (1)
    {
	x = rand_range(2, 29);
	y = rand_range(2, 29);

	if (getTile(x, y) == SQUARE_FLOOR)
	{
	    setTile(x, y, SQUARE_LADDERDOWN);
	    break;
	}
    } 
}

void
MAP::buildRoguelike()
{
    // Draw some rooms...
    ROOM	rooms[20];
    int		numroom = 0;
    int		fails = 0;
    int		i;
    int		src;
    int		room, x, y;
    int		maxdark, numdark;
    bool	isdark;

    // Track number of dark rooms.
    numdark = 0;
    // Only have up to depth - 1 dark rooms.  This prevents
    // start levels from having lots of dark rooms.
    maxdark = myDepth-1;
    
    // Build as many rooms as fits, up to twenty.
    while (fails < 40 && numroom < 20)
    {
	if (!numroom)
	{
	    buildRandomRoomFromDefinition(rooms[numroom],
			      chooseRandomRoom(myDepth));
	}
	else
	{
	    buildRandomRoom(rooms[numroom]);
	}
	if (checkRoomFits(rooms[numroom]))
	{
	    isdark = false;
	    // Decide if it should be dark.
	    if (numdark < maxdark)
	    {
		isdark = rand_chance(10);
	    }
	    if (isdark)
		numdark++;

	    drawRoom(rooms[numroom], !isdark, false, false);
	    numroom++;
	}
	else
	    fails++;
    }

    // Now, build corridors...
    // We want every room to be connected.
    // Thus, we have a pool of i connected rooms.  We each
    // round take i+1 and connect it to one of our connect rooms
    // at random.

    // If we fail to connect 40 times, we declare this dungeon sucky
    // and go again...
    fails = 0;
    for (i = 0; i < numroom - 1; i++)
    {
	int		cs[2], ce[2], ds[2], de[2];
	src = rand_range(0, i);
	
	chooseRandomExit(cs, ds, rooms[src]);
	chooseRandomExit(ce, de, rooms[i+1]);
	
	// Draw a corridor between these two rooms.
	if (!drawCorridor(cs[0], cs[1], ds[0], ds[1],
			  ce[0], ce[1], de[0], de[1]))
	{
	    // Failed to draw a corridor, try again.
	    i--;
	    fails++;
	    if (fails > 40)
		break;
	    
	    continue;
	}
    }
    if (fails > 40)
    {
	UT_ASSERT(!"Failed to build a dungeon, retrying!");
	build(true);
	return;
    }

    // Now we close any unused exits in our special rooms...
    for (room = 0; room < numroom; room++)
    {
	closeUnusedExits(rooms[room], false);
	populateRoom(rooms[room]);
    }
    
    // Now, we pick a random up and down stair case...
    while (1)
    {
	room = rand_choice(numroom);

	// If there was only one room, we trivially allow so we 
	// can't get an infinite loop if said rooms doesn't allow stairs.
	if (numroom > 1 &&
	    rooms[room].definition && !rooms[room].definition->allowstairs)
	    continue;
	
	x = rand_range(rooms[room].l.x+1, rooms[room].h.x-1);
	y = rand_range(rooms[room].l.y+1, rooms[room].h.y-1);

	if (getTile(x, y) == SQUARE_FLOOR ||
	    getTile(x, y) == SQUARE_CORRIDOR)
	{
	    setTile(x, y, SQUARE_LADDERDOWN);
	    break;
	}
    } 
    while (1)
    {
	room = rand_choice(numroom);

	if (numroom > 1 &&
	    rooms[room].definition && !rooms[room].definition->allowstairs)
	    continue;
	
	x = rand_range(rooms[room].l.x+1, rooms[room].h.x-1);
	y = rand_range(rooms[room].l.y+1, rooms[room].h.y-1);

	if (getTile(x, y) == SQUARE_FLOOR ||
	    getTile(x, y) == SQUARE_CORRIDOR)
	{
	    setTile(x, y, SQUARE_LADDERUP);
	    break;
	}
    } 
}

static int
findCell(int *cells, int c)
{
    int		n;

    n = cells[c];
    if (n == c)
	return c;

    n = findCell(cells, n);
    cells[c] = n;
    return n;
}

void
MAP::smooth(SQUARE_NAMES floor, SQUARE_NAMES wall)
{
    int			x, y;
    int			n;

    for (y = 1; y < MAP_HEIGHT-1; y++)
    {
	for (x = 1; x < MAP_WIDTH-1; x++)
	{
	    if (getTile(x, y) == floor)
	    {
		// If we have 3 non floor neighbours, delete.
		n = 0;
		if (getTile(x-1, y) == wall)
		    n++;
		if (getTile(x+1, y) == wall)
		    n++;
		if (getTile(x, y-1) == wall)
		    n++;
		if (getTile(x, y+1) == wall)
		    n++;

		if (n >= 3)
		{
		    setTile(x, y, wall);
		}
	    }
	}
    }
}

void
MAP::fillWithTile(int x, int y, SQUARE_NAMES newtile, SQUARE_NAMES oldtile)
{
    int			stackpos;
#define STACKSIZE 500
    int			stack[STACKSIZE];

    stackpos = 0;
    stack[stackpos++] = y * MAP_WIDTH + x;
    while (stackpos)
    {
	stackpos--;
	x = stack[stackpos] & 31;
	y = stack[stackpos] >> 5;

	if (getTile(x, y) == oldtile)
	{
	    setTile(x, y, newtile);

	    if ((x > 0) && getTile(x-1, y) == oldtile)
		stack[stackpos++] = (x-1) + (y * MAP_WIDTH);
	    if (stackpos >= STACKSIZE)
		continue;
	    if ((x < MAP_WIDTH-1) && getTile(x+1, y) == oldtile)
		stack[stackpos++] = (x+1) + (y * MAP_WIDTH);
	    if (stackpos >= STACKSIZE)
		continue;
	    if ((y > 0) && getTile(x, y-1) == oldtile)
		stack[stackpos++] = x + (y - 1) * MAP_WIDTH;
	    if (stackpos >= STACKSIZE)
		continue;
	    if ((y < MAP_HEIGHT-1) && getTile(x, y+1) == oldtile)
		stack[stackpos++] = x + (y + 1) * MAP_WIDTH;
	    if (stackpos >= STACKSIZE)
		continue;
	}
    }
}

void
MAP::drawQixWall(int sx, int sy, int dx, int dy, bool islit)
{
    int		x, y;
    int		range;

    // It is possible that we were given an out of range start
    // value.  This is not a problem, but we should not go
    // writing.
    if (!compareTile(sx, sy, SQUARE_FLOOR))
	return;

    range = rand_range(5, 20);

    x = sx;
    y = sy;
    while (range)
    {
	setTile(x, y, SQUARE_WALL);

	// See if now adjacent.
	// These are intentionally crossed over!
	if (!compareTile(x+dy, y+dx, SQUARE_FLOOR))
	    break;
	if (!compareTile(x-dy, y-dx, SQUARE_FLOOR))
	    break;
	if (!compareTile(x+dx, y+dy, SQUARE_FLOOR))
	    break;

	// Keep building.
	x += dx;
	y += dy;
	range--;
    }

    // If we didn't hit anything, rotate.
    if (!range)
    {
	// Rotate...
	x -= dx;
	y -= dy;
	if (dx)
	{
	    dy = dx;
	    dx = 0;
	}
	else
	{
	    dx = -dy;
	    dy = 0;
	}
	x += dx;
	y += dy;
	drawQixWall(x, y, dx, dy, islit);
    }
}

void
MAP::buildQix(bool islit)
{
    int			x, y, cx, cy, dx, dy;
    int			i;
    ROOM		room;
    int			numwalls, tries, dir;

    // Build a drunken cavern to use as our guide.
    buildDrunkenCavern(islit, false, false, false);

    smooth(SQUARE_CORRIDOR, SQUARE_EMPTY);

    // Set the entire map to normal ground:
    // Boundary becomes walls
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    int		oldtile;

	    oldtile = getTile(x, y);
	    if (oldtile == SQUARE_CORRIDOR)
		setTile(x, y, SQUARE_FLOOR);
	    else if (oldtile == SQUARE_EMPTY)
		setTile(x, y, SQUARE_WALL);

	    // For debugging:
	    // setFlag(x, y, SQUAREFLAG_MAPPED);
	}
    }

    // Create the special room:
    buildRandomRoomFromDefinition(room,
			  chooseRandomRoom(myDepth));
    drawRoom(room, true, true, false);
    populateRoom(room);

    numwalls = rand_range(17, 28);
    while (numwalls--)
    {
	// Find a valid wall position.
	tries = 100;
	while (tries--)
	{
	    cx = rand_range(1, MAP_WIDTH-1);
	    cy = rand_range(1, MAP_HEIGHT-1);

	    // Ensure we are not in the special room.
	    if (cx >= room.l.x && cx <= room.h.x &&
		cy >= room.l.y && cy <= room.h.y)
	    {
		continue;
	    }

	    if (getTile(cx, cy) != SQUARE_FLOOR)
		continue;
	    
	    // Verify we are not adjacent to a wall.
	    if (getTile(cx, cy+1) != SQUARE_FLOOR)
		continue;
	    if (getTile(cx, cy-1) != SQUARE_FLOOR)
		continue;
	    if (getTile(cx+1, cy) != SQUARE_FLOOR)
		continue;
	    if (getTile(cx-1, cy) != SQUARE_FLOOR)
		continue;

	    // Valid wall, done.
	    break;
	}

	// Couldn't find a valid space.
	if (!tries)
	    break;

	dir = rand_choice(4);
	getDirection(dir, dx, dy);
	
	// Now draw walls in two directions until they hit a wall
	// or become adjacent to a wall
	drawQixWall(cx, cy, dx, dy, islit);

	dir += rand_range(1, 3);
	dir &= 3;
	getDirection(dir, dx, dy);
	
	drawQixWall(cx+dx, cy+dy, dx, dy, islit);
    }

    int		*cutout = ourCutOuts;
    int		*cell = ourCells;
    int		numcuts = 0, c1, c2;
    
    // Find all possible cuts
    for (y = 1; y < MAP_HEIGHT-1; y++)
    {
	for (x = 1; x < MAP_WIDTH-1; x++)
	{
	    // Ensure we are not in the special room.
	    if (x >= room.l.x && x <= room.h.x &&
		y >= room.l.y && y <= room.h.y)
	    {
		continue;
	    }

	    // To be a cut, we must be a wall.
	    if (getTile(x, y) != SQUARE_WALL)
		continue;
	
	    // To be a valid cut, we need to have a floor
	    // on either side and walls on the opposite sides.
	    if ((getTile(x-1, y) == SQUARE_FLOOR) &&
		(getTile(x+1, y) == SQUARE_FLOOR) &&
		(getTile(x, y-1) == SQUARE_WALL) &&
		(getTile(x, y+1) == SQUARE_WALL))
	    {
		cutout[numcuts++] = y * MAP_WIDTH + x;
	    }
	    if ((getTile(x-1, y) == SQUARE_WALL) &&
		(getTile(x+1, y) == SQUARE_WALL) &&
		(getTile(x, y-1) == SQUARE_FLOOR) &&
		(getTile(x, y+1) == SQUARE_FLOOR))
	    {
		cutout[numcuts++] = y * MAP_WIDTH + x;
	    }
	}
    }
    UT_ASSERT(numcuts < 512);

    // Initialize cell array:
    for (i = 0; i < 1024; i++)
	cell[i] = i;

    // Connect anything already connected
    for (y = 1; y < MAP_HEIGHT-1; y++)
    {
	for (x = 1; x < MAP_WIDTH-1; x++)
	{
	    if (getTile(x, y) != SQUARE_FLOOR)
		continue;

	    c1 = findCell(cell, y * MAP_WIDTH + x);

	    if (getTile(x+1, y) == SQUARE_FLOOR)
	    {
		c2 = findCell(cell, y * MAP_WIDTH + (x + 1));
		if (c1 < c2)
		    cell[c2] = c1;
		else if (c1 > c2)
		    cell[c1] = c2;
	    }

	    if (getTile(x, y+1) == SQUARE_FLOOR)
	    {
		c2 = findCell(cell, (y+1) * MAP_WIDTH + x);
		if (c1 < c2)
		    cell[c2] = c1;
		else if (c1 > c2)
		    cell[c1] = c2;
	    }
	}
    }
    
    // Now, randomize our cut list:
    rand_shuffle(cutout, numcuts);

    for (i = 0; i < numcuts; i++)
    {
	// Determine direction of cut
	x = cutout[i] % MAP_WIDTH;
	y = cutout[i] / MAP_WIDTH;

	// Find direction.
	if ((getTile(x+1, y) == SQUARE_WALL) ||
	    (getTile(x-1, y) == SQUARE_WALL))
	{
	    dx = 0;
	    dy = 1;
	}
	if ((getTile(x, y+1) == SQUARE_WALL) ||
	    (getTile(x, y-1) == SQUARE_WALL))
	{
	    dx = 1;
	    dy = 0;
	}

	// Check to see if the cells are different.
	c1 = findCell(cell, (y+dy)*MAP_WIDTH+(x+dx));
	c2 = findCell(cell, (y-dy)*MAP_WIDTH+(x-dx));

	if (c1 != c2)
	{
	    setTile(x, y, SQUARE_DOOR);
	    if (c1 < c2)
		cell[c2] = c1;
	    else if (c1 > c2)
		cell[c1] = c2;
	    
	    // Add the door itself to the include list.
	    c1 = findCell(cell, y * MAP_WIDTH + x);
	    if (c1 < c2)
		cell[c2] = c1;
	    else if (c1 > c2)
		cell[c1] = c2;
	}
	else if (rand_chance(5))
	{
	    setTile(x, y, SQUARE_SECRETDOOR);
	}
    }

    // Now destroy any walls that are unconnected.
    int		c1cnt, c2cnt;
    c1cnt = 0;
    c1 = -1;
    for (y = 0; y < MAP_WIDTH; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    if (x >= room.l.x && x <= room.h.x &&
		y >= room.l.y && y <= room.h.y)
	    {
		continue;
	    }

	    if (getTile(x, y) == SQUARE_FLOOR || getTile(x, y) == SQUARE_DOOR)
	    {
		if (c1 < 0)
		{
		    c1 = findCell(cell, y * MAP_WIDTH + x);
		    c1cnt = 0;
		    for (i = 0; i < 1024; i++)
		    {
			if (findCell(cell, i) == c1)
			    c1cnt++;
		    }
		}
		
		c2 = findCell(cell, y * MAP_WIDTH + x);
		if (c1 != c2)
		{
		    if (c2 == y * MAP_WIDTH + x)
		    {
			c2cnt = 0;
			for (i = 0; i < 1024; i++)
			{
			    if (findCell(cell, i) == c2)
				c2cnt++;
			}

			if (c2cnt > c1cnt)
			{
			    c1 = c2;
			    c1cnt = c2cnt;
			    // Restart so we rescan earlyer
			    // parts.
			    y = 0;
			    x = 0;
			    continue;
			}
		    }
		
		    // Isolated region!  Reject!
		    setTile(x, y, SQUARE_WALL);
		}
	    }
	}
    }

    closeUnusedExits(room, true);

    // Now, we pick a random up and down stair case...
    while (1)
    {
        findRandomLoc(x, y, MOVE_WALK,
			true, false, false, false, false, false);

	if (!room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_FLOOR)
	{
	    setTile(x, y, SQUARE_LADDERUP);
	    break;
	}
    }

    while (1)
    {
        findRandomLoc(x, y, MOVE_WALK,
			true, false, false, false, false, false);
	
	if (!room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_FLOOR)
	{
	    setTile(x, y, SQUARE_LADDERDOWN);
	    break;
	}
    }
}

void
MAP::buildMaze(int stride)
{
    // A 32x32 room as 16x16 horizontal cuts and 16x16 vertical cuts,
    // for a max cut list of 512.
    int			*cutouts;
    // The cell tracks connected sets.  Each cell points to the
    // cell it is connected to, or to itself if it is the root of
    // the tree.
    int			*cell;
    int			numcuts;
    int			x, y, cx, cy, dx, dy;
    int			i;
    int			cut;
    int			c1, c2;
    ROOM		room;

    cell = ourCells;
    cutouts = ourCutOuts;

    // Set the entire map to normal walls:
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    setTile(x, y, SQUARE_WALL);
	    // It would be very cruel not to light the maze.
	    setFlag(x, y, SQUAREFLAG_LIT);

	    // For debugging:
	    // setFlag(x, y, SQUAREFLAG_MAPPED);
	}
    }

    // We want every other square to be a hole.
    for (y = 1; y < MAP_HEIGHT-stride; y += stride + 1)
    {
	for (x = 1; x < MAP_WIDTH-stride; x += stride + 1)
	{
	    for (dy = 0; dy < stride; dy++)
	    {
		for (dx = 0; dx < stride; dx++)
		{
		    setTile(x+dx, y+dy, SQUARE_FLOOR);
		}
	    }
	}
    }

    // Create the special room:
    buildRandomRoomFromDefinition(room,
			  chooseRandomRoom(myDepth));
    drawRoom(room, true, true, false);
    populateRoom(room);

    // We also want to initialize the cutout array here (which
    // is a list of all possible wall cuts.)
    // We do not include any cuts that will bridge the room - ie, either
    // side of the cut is inside of the room.
    numcuts = 0;
    for (y = 1; y < MAP_HEIGHT-stride; y += stride+1)
    {
	for (x = 1; x < MAP_WIDTH-stride; x += stride+1)
	{
	    for (dx = 0; dx < stride; dx++)
	    {
		if (x+stride < MAP_WIDTH-2)
		    if (x+stride+1 < room.l.x || x > room.h.x ||
			y+dx < room.l.y || y+dx > room.h.y)	
			cutouts[numcuts++] = ((y+dx) * MAP_WIDTH) + x + stride;
		if (y+stride < MAP_HEIGHT-2)
		    if (x+dx < room.l.x || x+dx > room.h.x ||
			y+stride+1 < room.l.y || y > room.h.y)	
			cutouts[numcuts++] = ((y+stride) * MAP_WIDTH) + x + dx;
	    }
	}
    }
    UT_ASSERT(numcuts < 512);

    // Initialize cell array:
    for (i = 0; i < 256; i++)
	cell[i] = i;
    
    // Now, randomize our cut list:
    rand_shuffle(cutouts, numcuts);

    // And cut all the cutouts that won't join an existing pair...
    for (i = 0; i < numcuts; i++)
    {
	cut = cutouts[i];

	y = cut / MAP_WIDTH;
	x = cut - y * MAP_WIDTH;
	cx = (x-1) / (stride+1);
	cy = (y-1) / (stride+1);
	c1 = findCell(cell, cy * (MAP_WIDTH / 2) + cx);
	if (cut & 1)
	{
	    // Vertical cut...
	    c2 = findCell(cell, (cy+1) * (MAP_WIDTH / 2) + cx);
	}
	else
	{
	    // Horizontal cut...
	    c2 = findCell(cell, cy * (MAP_WIDTH / 2) + cx + 1);
	}

	// We only connect if they are different.  This ensures
	// we have a fully branching corridor.   It would also
	// prevent any loop backs and make for lots of backtracking.
	// Thus, to be kind, we add some extras...
	if (c1 != c2 || rand_chance(10))
	{
	    // We have two different cells that would be joined
	    // by this cut, so make it.
	    setTile(x, y, SQUARE_FLOOR);
	    // And mark the cells the same:
	    cell[c2] = c1;
	}
    }

    closeUnusedExits(room, true);

    // Now, we pick a random up and down stair case...
    while (1)
    {
        findRandomLoc(x, y, MOVE_WALK,
			true, false, false, false, false, false);

	if (!room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_FLOOR)
	{
	    setTile(x, y, SQUARE_LADDERUP);
	    break;
	}
    }

    while (1)
    {
        findRandomLoc(x, y, MOVE_WALK,
			true, false, false, false, false, false);
	
	if (!room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_FLOOR)
	{
	    setTile(x, y, SQUARE_LADDERDOWN);
	    break;
	}
    }
}

void
MAP::buildDrunkenCavern(bool islit, bool allowlakes, bool allowroom, bool buildstaircase)
{
    // This time the drunk in the title refers to the algorithm, not
    // my state when writing it.

    // int			numdrunks, i;
    int			rum, j;
    int			x, y;
    int			dx, dy;
    int			numcuts = 0;
    SQUARE_NAMES	tunneltype;

    if (islit)
    {
	for (y = 0; y < MAP_HEIGHT; y++)
	{
	    for (x = 0; x < MAP_WIDTH; x++)
	    {
		setFlag(x, y, SQUAREFLAG_LIT);
		// For debugging:
		// setFlag(x, y, SQUAREFLAG_MAPPED);
	    }
	}
    }

    ROOM			room;
    room.definition = 0;

    if (allowroom)
    {
	buildRandomRoomFromDefinition(room,
			  chooseRandomRoom(myDepth));
	drawRoom(room, islit, false, false);
	populateRoom(room);
    }
    
    // 10-30 drunken walks.
    //numdrunks = rand_range(20, 30);
    //rum = rand_range(20, 30);
    //numdrunks = 1;
    //rum = 900;

    // We want numdrunks + sqrt(rum) == 20
    // The sqrt is because each drunk will move a distance on average
    // of sqrt(rum).

    // We may want to:
    // 1) Bias the birth of drunkards near tunnel terminuses
    // 2) Turn on back tracking to allow creation of deadends
    // 3) Tunnel until a certain percentage is execavated.
    /*
    numdrunks = rand_range(1, 17);
    rum = 20 - numdrunks;
    rum *= rum;
    */
    rum = 300;
    
    //BUF		buf;
    //buf.sprintf("Drunk: %d, Len: %d.  ", numdrunks, rum);
    //msg_report(buf);
    //for (i = 0; i < numdrunks; i++)
    while (numcuts < (MAP_WIDTH * MAP_HEIGHT / 2))
    {
	// Pick a random drunk starting location.
	//if (i)
	if (numcuts)
	{
	    findRandomLoc(x, y, MOVE_STD_SWIM,
			    true, false, false, false, false, false);
	    if (isMapWall(x, y))
		continue;
	    tunneltype = SQUARE_CORRIDOR;
	}
	else
	{
	    while (1)
	    {
		x = rand_range(3, MAP_WIDTH-3);
		y = rand_range(3, MAP_HEIGHT-3);
		if (isMapWall(x, y))
		    continue;
		if (getTile(x, y) == SQUARE_EMPTY)
		    break;
	    }

	    // We get free cuts corresponding to the size of the room.
	    if (allowroom)
		numcuts += (room.h.x - room.l.x) * (room.h.y - room.l.y);

	    if (allowlakes)
	    {
		tunneltype = SQUARE_WATER;
	    }
	    else
		tunneltype = SQUARE_CORRIDOR;
	}

	// Now start carving out the dungeon...
	// Initial dig direction:
	rand_direction(dx, dy);
	for (j = 0; j < rum; j++)
	{
	    // Dig out this square...
	    if (getTile(x, y) == SQUARE_EMPTY)
		numcuts++;

	    // Don't cut out special rooms.
	    if (getTile(x, y) != SQUARE_CORRIDOR &&
		getTile(x, y) != SQUARE_EMPTY &&
		getTile(x, y) != SQUARE_WATER)
	    {
		break;
	    }
	    
	    if (getTile(x, y) != SQUARE_WATER)
		setTile(x, y, tunneltype);

	    // Move drunkenly...
	    x += dx;
	    y += dy;

	    // Stumble against the walls.
	    if (x < 1)
		x = 1;
	    if (x >= MAP_WIDTH-2)
		x = MAP_WIDTH-2;
	    if (y < 1)
		y = 1;
	    if (y >= MAP_HEIGHT-2)
		y = MAP_HEIGHT-2;

	    // Change our direction...
	    switch (rand_choice(3))
	    {
		case 0:
		    // Unchanged.
		    break;
		case 1:
		    // Turn, +1
		    dx = !dx;
		    dy = !dy;
		    break;
		case 2:
		    // Turn -1
		    dx = -!dx;
		    dy = -!dy;
		    break;
	    }
	}
    }

    if (allowroom)
	closeUnusedExits(room, true);

    // Build the staircases..
    while (buildstaircase)
    {
        findRandomLoc(x, y, MOVE_STD_SWIM,
			true, false, false, false, false, false);
	
	if (room.definition &&
	    !room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_CORRIDOR || getTile(x, y) == SQUARE_WATER)
	{
	    setTile(x, y, SQUARE_LADDERUP);
	    break;
	}
    }

    while (buildstaircase)
    {
        findRandomLoc(x, y, MOVE_STD_SWIM,
			true, false, false, false, false, false);
	
	if (room.definition &&
	    !room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_CORRIDOR || getTile(x, y) == SQUARE_WATER)
	{
	    setTile(x, y, SQUARE_LADDERDOWN);
	    break;
	}
    }
}

void
MAP::buildRiverRoom()
{
    int		upx = 0, upy;
    int		downx = 0, downy;
    int		minx, maxx;
    int		rx, ry;
    int		i;

    do 
    {
	// Empty the map...
	clear();

	// Clear out our wall map.
	memset(ourWallMap, 0, MAP_WIDTH * MAP_HEIGHT);

	buildDrunkenCavern(false, false, false, true);

	// Find the up and down ladders, try to cut through them with a river.
	if (!findTile(SQUARE_LADDERUP, upx, upy))
	    continue;
	if (!findTile(SQUARE_LADDERDOWN, downx, downy))
	    continue;
    } while (abs(upx - downx) <= 7);

    // We are now guaranteed that we have a 5 space room for our river.
    if (upx < downx)
    {
	minx = upx;
	maxx = downx;
    }
    else
    {
	minx = downx;
	maxx = upx;
    }
    minx += 3;
    maxx -= 3;

    rx = (upx + downx) / 2;
    for (ry = 0; ry < MAP_HEIGHT; ry++)
    {
	// Move rx randomly to left or right.
	rx += rand_choice(3) - 1;
	if (rx < minx)
	    rx = minx;
	if (rx > maxx)
	    rx = maxx;

	// Draw the river here.
	for (i = -2; i <= 2; i++)
	{
	    // We only want to draw over corridor and normal walls.
	    // Otherwise we could write over a special room.
	    switch (getTile(rx+i, ry))
	    {
		case SQUARE_CORRIDOR:
		case SQUARE_EMPTY:
		    setTile(rx+i, ry, SQUARE_WATER);
		    break;
		default:
		    break;
	    }
	}
    }
}

void
MAP::buildSurfaceWorld()
{
    int		elevation[MAP_HEIGHT+1][MAP_WIDTH+1];
    int		scale, step, average, e;
    int		x, y, pass;
    SQUARE_NAMES	tile;

    // Set our flags appropraitel
    myGlobalFlags &= ~MAPFLAG_DIG;
    myGlobalFlags &= ~MAPFLAG_NEWMOBS;
    myGlobalFlags |= MAPFLAG_NOITEMS;

    for (scale = 5; scale >= 0; scale--)
    {
	step = 1 << scale;

	for (pass = 0; pass < 2; pass++)
	{
	    for (y = 0; y <= MAP_HEIGHT; y += step)
	    {
		for (x = 0; x <= MAP_WIDTH; x += step)
		{
		    // Clamp boundaries:
		    if (!x || !y || (x == MAP_WIDTH) || (y == MAP_HEIGHT))
			elevation[y][x] = 256;
		    else if (x == MAP_WIDTH/2 && y == MAP_HEIGHT/2)
			elevation[y][x] = 0;
		    else
		    {
			// First pass we want only edges
			// Second pass the centers.
			if (pass)
			{
			    if (!(x & step) || !(y & step))
				continue;

			    average = elevation[y - step][x] +
				      elevation[y + step][x] +
				      elevation[y][x - step] +
				      elevation[y][x + step];
			    average = (average + 2) / 4;
			}
			else
			{
			    if (!((x & step) ^ (y & step)))
				continue;

			    if (x & step)
			    {
				average = elevation[y][x - step] +
					  elevation[y][x + step];
				average = (average + 1) / 2;
			    }
			    else
			    {
				average = elevation[y - step][x] +
					  elevation[y + step][x];
				average = (average + 1) / 2;
			    }
			}

			average += rand_range(-step*10, step*10);

			elevation[y][x] = average;
		    }
		}
	    }
	}		// for pass
    }

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    e = elevation[y][x];

	    if (e < 50)
		tile = SQUARE_GRASS;
	    else if (e < 100)
		tile = SQUARE_WATER;
	    else if (e < 150)
		tile = SQUARE_GRASS;
	    else if (e < 200)
		tile = SQUARE_FOREST;
	    else if (e < 240)
		tile = SQUARE_HILLS;
	    else
		tile = SQUARE_MOUNTAIN;

	    setTile(x, y, tile);
	    setFlag(x, y, SQUAREFLAG_LIT);
	}
    }

    // One upstairs in the center...
    setTile(MAP_WIDTH/2, MAP_HEIGHT/2, SQUARE_LADDERDOWN);

    // Warning sign post
    setTile(MAP_WIDTH/2+2, MAP_HEIGHT/2, SQUARE_SIGNPOST);
    addSignPost(SIGNPOST_WON, MAP_WIDTH/2+2, MAP_HEIGHT/2);

    // Elder
    y = MAP_HEIGHT/2;

    for ( ; y < MAP_HEIGHT; y++)
    {
	x = MAP_WIDTH/2;
	for ( ; x < MAP_WIDTH; x++)
	{
	    if (!canMove(x, y, MOVE_STD_SWIM))
		break;
	}
	if (canMove(x-1,y, MOVE_WALK))
	{
	    x--;
	    break;
	}
    }
    if (y >= MAP_HEIGHT)
    {
	// Failed to place far away
	x = MAP_WIDTH/2-1;
	y = MAP_HEIGHT/2;
    }
    // If we are loading from disk, we've already populated the room
    // from disk.
    if (!ourIsLoading)
    {
	MOB *mob;

	mob = MOB::create(MOB_ELDER);

	// interlevell so no traps
	// but what do we do about failure???
	// currently move never fails with interlevel.
	if (mob->move(x, y, true))
	    registerMob(mob);
	else
	    delete mob;
    }
}

void
MAP::buildTutorial()
{
    // Find the appropriate room...
    int			i, nummatch, match = -1;

    nummatch = 0;
    for (i = 0; i < NUM_ALLROOMDEFS; i++)
    {
	if (!strcmp(glb_allroomdefs[i]->name, "tutorial"))
	{
	    nummatch++;
	    if (!rand_choice(nummatch))
		match = i;
	}
    }

    if (match == -1)
    {
	// No matching god!
	buildRoguelike();
	return;
    }

    // Only have to build the room.  It is assumed to have
    // the required crap in it.
    ROOM		room;

    buildRandomRoomFromDefinition(room, glb_allroomdefs[match]);
    drawRoom(room, true, false, false);
    populateRoom(room);
}

void
MAP::buildQuest(GOD_NAMES god)
{
    // Set our flags appropraitely
#if 0
    myGlobalFlags &= ~MAPFLAG_DIG;
    myGlobalFlags &= ~MAPFLAG_NEWMOBS;
    myGlobalFlags |= MAPFLAG_NOITEMS;
#endif

    // Find the appropriate room...
    int			i, nummatch, match = -1;

    nummatch = 0;
    for (i = 0; i < NUM_ALLROOMDEFS; i++)
    {
	if (glb_allroomdefs[i]->quest == god)
	{
	    nummatch++;
	    if (!rand_choice(nummatch))
		match = i;
	}
    }

    if (match == -1)
    {
	// No matching god!
	buildRoguelike();
	return;
    }

    // Only have to build the room.  It is assumed to have
    // the required crap in it.
    ROOM		room;

    buildRandomRoomFromDefinition(room, glb_allroomdefs[match]);
    drawRoom(room, true, false, false);
    populateRoom(room);
}

class CIRCLE
{
public:
    CIRCLE() {}
    CIRCLE(int r)
    {
	CIRCLE::r = r;
    }
    int		x, y;
    int		r;

    bool	overlaps(const CIRCLE &circle) const;
};

bool
CIRCLE::overlaps(const CIRCLE &circle) const
{
    int		dx, dy, r2, cr2;

    dx = circle.x - x;
    dy = circle.y - y;
    
    r2 = dx * dx + dy * dy;
    cr2 = circle.r + r + 2;	// Extra two is to get a wall between them.
    cr2 *= cr2;

    // See if combined radii of circle is within r2.
    if (cr2 >= r2)
	return true;

    return false;
}

typedef PTRSTACK<CIRCLE> CIRCLELIST;

bool
circleFits(int x, int y, int r, const CIRCLELIST &list)
{
    int		i, n;
    CIRCLE	test;

    test.x = x;
    test.y = y;
    test.r = r;

    n = list.entries();
    for (i = 0; i < n; i++)
    {
	if (test.overlaps(list(i)))
	{
	    return false;
	}
    }
    return true;
}

bool
createCircle(CIRCLE &circle, int r, const CIRCLELIST &list)
{
    int		x, y, numchoice = 0;

    for (y = r+1; y < MAP_HEIGHT - (r+1); y++)
    {
	for (x = r+1; x < MAP_WIDTH - (r+1); x++)
	{
	    if (circleFits(x, y, r, list))
	    {
		numchoice++;
		if (!rand_choice(numchoice))
		{
		    circle.x = x;
		    circle.y = y;
		    circle.r = r;
		}
	    }
	}
    }
    if (numchoice)
	return true;
    return false;
}

MOB *
MAP::createGoldenTridudeLair(int x, int y)
{
    MOB		*leader, *mob;
    ITEM	*artifact;
    int		 i;
    const int	 numslaves = 9;
    int		 placedx[numslaves] =
    { 0, 0, 0, 1, 2, 3, -1, -2, -3 }; 
    int		 placedy[numslaves] =
    { 2, 3, 4,-1,-2,-3, -1, -2, -3 };
    MOB_NAMES	 placetype[numslaves] =
    { 
	MOB_REDTRIDUDE, MOB_REDTRIDUDE, MOB_REDTRIDUDE,
	MOB_BLUETRIDUDE, MOB_BLUETRIDUDE, MOB_BLUETRIDUDE,
	MOB_PURPLETRIDUDE, MOB_PURPLETRIDUDE, MOB_PURPLETRIDUDE
    };

    leader = MOB::create(MOB_GOLDTRIDUDE);
    leader->setIntrinsic(INTRINSIC_LEADER);
    leader->move(x, y, true);
    registerMob(leader);

    // The leader gets a tasty artifact.
    artifact = ITEM::createRandomType(ITEMTYPE_ARTIFACT);
    acquireItem(artifact, x, y, 0);

    for (i = 0; i < numslaves; i++)
    {
	mob = MOB::create(placetype[i]);
	mob->makeSlaveOf(leader);

	mob->move(x + placedx[i], y + placedy[i], true);
	registerMob(mob);
    }

    return leader;
}

void
MAP::createColouredTridudeLair(int x, int y, MOB_NAMES tridude, MOB *trueleader)
{
    MOB		*leader, *mob;
    int		 i;
    const int	 numslaves = 6;
    int		 placedx[numslaves] =
    { 0, -1,  1, 0, -2,  2};
    int		 placedy[numslaves] =
    { 1, -1, -1, 2, -2, -2};

    leader = MOB::create(tridude);
    leader->makeUnique();
    leader->setIntrinsic(INTRINSIC_LEADER);
    leader->move(x, y, true);
    if (trueleader)
	leader->makeSlaveOf(trueleader);
    registerMob(leader);

    for (i = 0; i < numslaves; i++)
    {
	mob = MOB::create(tridude);
	mob->makeSlaveOf(leader);

	mob->move(x + placedx[i], y + placedy[i], true);
	registerMob(mob);
    }
}

void
MAP::buildSpaceShip()
{
    int			x, y, r, dx, dy, i;
    SQUARE_NAMES	square;
    CIRCLELIST		list;
    CIRCLE		circle;

    // Set our appropriate flags.
    myGlobalFlags &= ~MAPFLAG_DIG;
    // We'll do our own populating.
    myGlobalFlags &= ~MAPFLAG_NEWMOBS;

    // This is random placement so we try until we get the required mandatory
    // rooms
    while (1)
    {
	bool		created = false;
	clear();

	list.clear();
	
	while (1)
	{
	    // Dimension door.
	    if (!createCircle(circle, 2, list))
		break;
	    drawCircle(circle.x, circle.y, circle.r, SQUARE_METALFLOOR, SQUAREFLAG_LIT);
	    list.append(circle);
	    setTile(circle.x, circle.y, SQUARE_DIMDOOR);

	    // Giant tridude
	    if (!createCircle(circle, 5, list))
		break;
	    list.append(circle);
	    drawCircle(circle.x, circle.y, circle.r, SQUARE_METALFLOOR, SQUAREFLAG_LIT);

	    // Tri coloured tridude lairs
	    for (i = 0; i < 3; i++)
	    {
		if (!createCircle(circle, 4, list))
		    break;
		drawCircle(circle.x, circle.y, circle.r, SQUARE_METALFLOOR, SQUAREFLAG_LIT);
		list.append(circle);
	    }
	    if (i != 3)
		break;

	    // Some bonus rooms.
	    // We don't care if these fail.
	    for (i = 0; i < 6; i++)
	    {
		if (!createCircle(circle, 2+rand_choice(2), list))
		    break;
		drawCircle(circle.x, circle.y, circle.r, SQUARE_METALFLOOR, SQUAREFLAG_LIT);
		list.append(circle);
	    }
	    
	    created = true;
	    break;
	}

	if (created)
	    break;
    }

    // Populate rooms.
    if (!ourIsLoading)
    {
	RAND_STATEPUSH		saverng;
	const int		numlair = 3;
	MOB			*goldtridude;
	MOB_NAMES		lairtype[numlair] =
	{
	    MOB_REDTRIDUDE,
	    MOB_BLUETRIDUDE,
	    MOB_PURPLETRIDUDE
	};
	// Create the golden lair.
	goldtridude = createGoldenTridudeLair(list(1).x, list(1).y);
	// Rooms 2, 3, 4 are all tridude lairs
	for (i = 0; i < numlair; i++)
	{
	    // Each lair is a feudal system.  Knock out the local
	    // leader and they will fight the other packs.
	    createColouredTridudeLair(list(i+2).x, list(i+2).y, lairtype[i],
				    goldtridude);
	}

	// Now a smattering of regular tridudes.
	for (i = 0; i < 10; i++)
	{
	    MOB		*mob;
	    
	    // Abort if no more room for some reason.
	    if (!findRandomLoc(x, y, MOVE_WALK,
			    false, false, false, false, false, false))
		break;
	    
	    mob = MOB::create(MOB_TRIDUDE);
	    
	    mob->move(x, y, true);
	    // These all owe allegiance to the giant tridude.
	    mob->makeSlaveOf(goldtridude);
	    registerMob(mob);
	}
    }

    // Connect all rooms.
    // We have two sets: connected, and unconnected.
    // We find the closest pair between the two sets and connect them
    // then add the new room to the connected set.
    for (i = 1; i < list.entries(); i++)
    {
	int		j, k;
	int		minr2 = MAP_WIDTH*MAP_WIDTH*4;
	int		minid1 = i, minid2 = 0;

	for (j = 0; j < i; j++)
	{
	    for (k = i; k < list.entries(); k++)
	    {
		dx = list(j).x - list(k).x;
		dy = list(j).y - list(k).y;
		r = dx * dx + dy * dy;
		if (r < minr2)
		{
		    minid2 = j;
		    minid1 = k;
		    minr2 = r;
		}
	    }
	}
	
	// Draw a line between the centers.
	drawLine(list(minid1).x, list(minid1).y,
		 list(minid2).x, list(minid2).y,
		 SQUARE_METALFLOOR);

	// Min pair is minid2 and minid1.
	// Swap i with minid1 to mark it connected.
	circle = list(i);
	list.set(i, list(minid1));
	list.set(minid1, circle);
    }

    // Outline all rooms with walls.
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    if (getTile(x, y) == SQUARE_EMPTY)
	    {
		// If a eight way neighbour is not empty and not
		// a metal wall, convert.
		for (dy = -1; dy <= 1; dy++)
		{
		    for (dx = -1; dx <= 1; dx++)
		    {
			if (!dx && !dy)
			    continue;
			if (x+dx < 0 || y + dy < 0)
			    continue;
			if (x+dx >= MAP_WIDTH || y + dy >= MAP_HEIGHT)
			    continue;
			square = getTile(x+dx, y+dy);
			if (square != SQUARE_EMPTY && square != SQUARE_METALWALL)
			{
			    setTile(x, y, SQUARE_METALWALL);
			    dx = dy = 2;
			}
		    }
		}
	    }
	}
    }
    
    // Create viewports anywhere which is bordered on one half by
    // stars and the other by non-stars and is a metalwall.
    for (y = 1; y < MAP_HEIGHT-1; y++)
    {
	for (x = 1; x < MAP_WIDTH-1; x++)
	{
	    const int viewportchance = 15;

	    if (getTile(x, y) != SQUARE_METALWALL)
		continue;

	    if (getTile(x-1, y) != SQUARE_METALWALL &&
		getTile(x+1, y) != SQUARE_METALWALL)
	    {
		if (getTile(x-1, y) == SQUARE_EMPTY ||
		    getTile(x+1, y) == SQUARE_EMPTY)	
		{
		    if (rand_chance(viewportchance))
			setTile(x, y, SQUARE_VIEWPORT);
		}
	    }
	    if (getTile(x, y-1) != SQUARE_METALWALL &&
		getTile(x, y+1) != SQUARE_METALWALL)
	    {
		if (getTile(x, y-1) == SQUARE_EMPTY ||
		    getTile(x, y+1) == SQUARE_EMPTY)	
		{
		    if (rand_chance(viewportchance))
			setTile(x, y, SQUARE_VIEWPORT);
		}
	    }
	}
    }
    
    const int		numstars = 5;

    // Mark everything lit & having a random star.
    // We also turn off the mob generation inside stars
    // to stop you from being teleported there.
    const SQUARE_NAMES starlist[numstars] =
    {
	SQUARE_STARS1,
	SQUARE_STARS2,
	SQUARE_STARS3,
	SQUARE_STARS4,
	SQUARE_STARS5
    };
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    setFlag(x, y, SQUAREFLAG_LIT);
	    // For debugging:
    	    // setFlag(x, y, SQUAREFLAG_MAPPED);

	    if (getTile(x, y) == SQUARE_EMPTY)
	    {
		setTile(x, y, starlist[rand_choice(numstars)]);
		setFlag(x, y, SQUAREFLAG_NOMOB);
	    }
	}
    }
}

void
MAP::buildHello()
{
    int			x, y, r;
    int			numcirc;
    SQUARE_NAMES	square;
    SQUAREFLAG_NAMES	flag;

    // Set our appropriate flags.
    myGlobalFlags &= ~MAPFLAG_DIG;

    clear();

    ROOM			room;

    // I'm just typing this to note I am currently flying first class
    // Woohoo!
    buildRandomRoomFromDefinition(room,
		      chooseRandomRoom(myDepth));
    drawRoom(room, true, false, false);
    populateRoom(room);
    
    for (numcirc = 0; numcirc < 20; numcirc++)
    {
	x = rand_range(1, MAP_WIDTH);
	y = rand_range(1, MAP_HEIGHT);
	r = rand_range(1, 6);

	switch (rand_choice(3))
	{
	    case 0:
		square = SQUARE_CORRIDOR;
		flag = SQUAREFLAG_NONE;
		break;
	    case 1:
		square = SQUARE_LAVA;
		flag = SQUAREFLAG_LIT;
		break;
	    case 2:
		square = SQUARE_WATER;
		flag = SQUAREFLAG_NONE;
		break;
	    default:
		UT_ASSERT(0);
		square = SQUARE_EMPTY;
		flag = SQUAREFLAG_NONE;
		break;
	}

	drawCircle(x, y, r, square, flag);
    }

    closeUnusedExits(room, true);
    
    // Build the staircases..
    while (1)
    {
        findRandomLoc(x, y, MOVE_STD_SWIM,
			true, false, false, false, false, false);
	
	if (!room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_CORRIDOR || getTile(x, y) == SQUARE_WATER
	    || getTile(x, y) == SQUARE_LAVA)
	{
	    setTile(x, y, SQUARE_LADDERUP);
	    break;
	}
    }

    // There is no down stairs from hello.
#if 0
    while (1)
    {
        findRandomLoc(x, y, MOVE_STD_SWIM,
			true, false, false, false, false, false);
	
	if (!room.definition->allowstairs &&
	     x >= room.l.x && x <= room.h.x &&
	     y >= room.l.y && y <= room.h.y)
	    continue;
	
	if (getTile(x, y) == SQUARE_CORRIDOR || getTile(x, y) == SQUARE_WATER
	    || getTile(x, y) == SQUARE_LAVA)
	{
	    setTile(x, y, SQUARE_LADDERDOWN);
	    break;
	}
    }
#endif
}

void
MAP::drawLine(int x1, int y1, int x2, int y2,
		SQUARE_NAMES square,
		bool overwrite)
{
    int		cx, cy, x, y, acx, acy;

    cx = x2 - x1;
    cy = y2 - y1;
    acx = abs(cx);
    acy = abs(cy);


    // Start and beginning are done separately as this code
    // is stolen from the LOS code that ignore start and end.
    if (overwrite || getTile(x1, y1) == SQUARE_EMPTY)
	setTile(x1, y1, square);
    if (overwrite || getTile(x2, y2) == SQUARE_EMPTY)
	setTile(x2, y2, square);
    
    if (acx > acy)
    {
	int		dx, dy, error;

	dx = SIGN(cx);
	dy = SIGN(cy);

	error = 0;
	error = acy;
	y = y1;
	for (x = x1 + dx; x != x2; x += dx)
	{
	    if (error >= acx)
	    {
		error -= acx;
		if (overwrite || getTile(x, y) == SQUARE_EMPTY)
		    setTile(x, y, square);
		y += dy;
	    }

	    error += acy;
	    if (overwrite || getTile(x, y) == SQUARE_EMPTY)
		setTile(x, y, square);
	}
    }
    else
    {
	int		dx, dy, error;

	dx = SIGN(cx);
	dy = SIGN(cy);

	error = 0;
	error = acx;
	x = x1;
	for (y = y1 + dy; y != y2; y += dy)
	{
	    if (error >= acy)
	    {
		error -= acy;
		if (overwrite || getTile(x, y) == SQUARE_EMPTY)
		    setTile(x, y, square);
		x += dx;
	    }

	    error += acx;
	    if (overwrite || getTile(x, y) == SQUARE_EMPTY)
		setTile(x, y, square);
	}
    }

}

void
MAP::drawCircle(int cx, int cy, int rad, 
		SQUARE_NAMES square, SQUAREFLAG_NAMES flag,
		bool overwrite)
{
    int		x, y, r2;

    // Expand radius by .5 so we don't get pimply circles.
    r2 = (int) ((rad + 0.5) * (rad + 0.5));

    for (y = cy - rad; y <= cy + rad; y++)
    {
	if (y < 0)
	    continue;
	// Leave blank tile
	if (y >= MAP_HEIGHT-1)
	    break;

	for (x = cx - rad; x <= cx + rad; x++)
	{
	    if (x < 0)
		continue;
	    // Leave blank tile
	    if (x >= MAP_WIDTH-1)
		break;

	    // Only overwrite earth tiles.
	    if (!overwrite && getTile(x, y) != SQUARE_EMPTY)
		continue;

	    // Determine if this is in range.
	    // My world for a bressenham circle algorithm!
	    // Okay, not my world.
	    // I've written enough of them that this code is ar esult
	    // of extreme laziness/drunkness, rather than a lack of awareness
	    // of a perfectly fine and fast appraoch.
	    if ((y - cy) * (y - cy) + (x - cx) * (x - cx) < r2)
	    {
		setTile(x, y, square);
		setRawFlag(x, y, flag);
	    }
	}
    }
}
