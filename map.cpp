/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        map.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Routines to handle the map data structure.
 *
 *	Note that code relevant to the creation of levels is in build.cpp.
 */

#include <stdio.h>
#include <math.h>
#include "mygba.h"
#include "map.h"
#include "msg.h"
#include "gfxengine.h"
#include "rand.h"
#include "assert.h"
#include "creature.h"
#include "item.h"
#include "itemstack.h"
#include "sramstream.h"
#include "piety.h"
#include "victory.h"
#include "ptrlist.h"

MAP	*glbCurLevel = 0;
int	 glbMapCount = 0;

u16 	*MAP::ourItemMap = 0;
u16	*MAP::ourMobMap = 0;
// Temporary map used to mark if stuff are walls or not.
u8	*MAP::ourWallMap = 0;
// Used while building our levels.
u16	*MAP::ourDistanceMap = 0;
int	*MAP::ourCells = 0;
int	*MAP::ourCutOuts = 0;

// Used for smelling..
u16	*MAP::ourAvatarSmell = 0;
u16	 MAP::ourAvatarSmellWatermark = 64;

MOBREF	*MAP::ourMobRefMap = 0;
ITEM	**MAP::ourItemPtrMap = 0;

MOBREF	*MAP::ourMobGuiltyMap = 0;

bool	 MAP::ourIsLoading = false;

s8	 MAP::ourWindDX = 0;
s8	 MAP::ourWindDY = 0;
int	 MAP::ourWindCounter = 0;

bool	 MAP::ourMapDirty = true;

// Marks if we have a pending level change.
MAP	*MAP::ourDelayedLevelChange = 0;

u32	 glbBlockingTiles[32] =
{ 0, 0, 0, 0,
  0, 0, 0, 0,  
  0, 0, 0, 0,  
  0, 0, 0, 0,  

  0, 0, 0, 0,
  0, 0, 0, 0,  
  0, 0, 0, 0,  
  0, 0, 0, 0 };

u32	 glbWalkTiles1[32] =
{ 0, 0, 0, 0,
  0, 0, 0, 0,  
  0, 0, 0, 0,  
  0, 0, 0, 0,  

  0, 0, 0, 0,
  0, 0, 0, 0,  
  0, 0, 0, 0,  
  0, 0, 0, 0 };

u32	 glbWalkTiles2[32] =
{ 0, 0, 0, 0,
  0, 0, 0, 0,  
  0, 0, 0, 0,  
  0, 0, 0, 0,  

  0, 0, 0, 0,
  0, 0, 0, 0,  
  0, 0, 0, 0,  
  0, 0, 0, 0 };


static int
sign(int d)
{
    if (d == 0)
	return 0;
    else if (d < 0)
	return -1;
    else
	return 1;
}

MAP::MAP(int level, BRANCH_NAMES branch)
{
    glbMapCount++;
    mySquares = new u8[MAP_WIDTH * MAP_HEIGHT];
    myFlags = new u8[MAP_WIDTH * MAP_HEIGHT];

    // Default abilities for all levels.
    myGlobalFlags = 0;
    myGlobalFlags |= MAPFLAG_DIG;
    myGlobalFlags |= MAPFLAG_NEWMOBS;

    if (!ourItemMap)
	ourItemMap = new u16[MAP_WIDTH * MAP_HEIGHT];
    if (!ourMobMap)
	ourMobMap = new u16[MAP_WIDTH * MAP_HEIGHT];
    if (!ourWallMap)
	ourWallMap = new u8[MAP_WIDTH * MAP_HEIGHT];
    if (!ourDistanceMap)
	ourDistanceMap = new u16[MAP_WIDTH * MAP_HEIGHT];
    if (!ourCells)
	ourCells = new int[1024];
    if (!ourCutOuts)
	ourCutOuts = new int[512];

    if (!ourAvatarSmell)
	ourAvatarSmell = new u16[MAP_WIDTH * MAP_HEIGHT];

    if (!ourMobRefMap)
	ourMobRefMap = new MOBREF[MAP_WIDTH * MAP_HEIGHT];
    if (!ourItemPtrMap)
	ourItemPtrMap = new ITEM*[MAP_WIDTH * MAP_HEIGHT];

    if (!ourMobGuiltyMap)
	ourMobGuiltyMap = new MOBREF[MAP_WIDTH * MAP_HEIGHT];
    
    myMobCount = 0;
    myItemCount = 0;

    myMapDown = 0;
    myMapUp = 0;
    myMapBranch = 0;
    myItemHead = 0;
    myMobHead = 0;
    myMobSafeNext = 0;

    mySignList = 0;

    clear();

    myDepth = level;
    myBranch = branch;

    myFOVX = myFOVY = -1;
}

MAP::~MAP()
{
    delete [] mySquares;
    delete [] myFlags;

    delete mySignList;

    glbMapCount--;
}

void
MAP::deleteAllMaps()
{
    MOB		*mob;
    ITEM	*item;
    
    // Clear out our mob refs.
    if (this == glbCurLevel)
    {
	int		 x, y;

	for (y = 0; y < MAP_HEIGHT; y++)
	    for (x = 0; x < MAP_WIDTH; x++)
	    {
		ourMobRefMap[(y) * MAP_WIDTH + x].setMob(0);
		ourMobGuiltyMap[(y) * MAP_WIDTH + x].setMob(0);
	    }
    }

    for (mob = getMobHead(); mob; mob = getMobHead())
    {
	myMobHead = mob->getNext();
	mob->setNext(0);
	delete mob;    
    }

    for (item = getItemHead(); item; item = getItemHead())
    {
	myItemHead = item->getNext();
	item->setNext(0);
	delete item;
    }
    
    if (myMapUp)
    {
	myMapUp->myMapDown = 0;
	myMapUp->deleteAllMaps();
    }

    if (myMapDown)
    {
	myMapDown->myMapUp = 0;
	myMapDown->deleteAllMaps();
    }

    if (myMapBranch)
    {
	// Break the branch.
	myMapBranch->myMapBranch = 0;
	myMapBranch->deleteAllMaps();
    }

    myMapUp = myMapDown = myMapBranch = 0;

    delete this;
    
    changeCurrentLevel(0);
}

bool
MAP::findPath(int sx, int sy,
	      int ex, int ey,
	      MOVE_NAMES movetype,
	      bool allowmob,
	      bool allowdoor,
	      int &dx, int &dy)
{
    int			 x, y, iterations;
    u32			 mask, val, endmask, diff;
    u32			*curtile, *lasttile, *tmp;

    UT_ASSERT(sx >= 0 && sx < 32);
    UT_ASSERT(sy >= 0 && sy < 32);
    UT_ASSERT(ex >= 0 && ex < 32);
    UT_ASSERT(ey >= 0 && ey < 32);
    
    if (sx == ex && sy == ey)
    {
	// Already there, trivial.
	dx = dy = 0;
	return true;
    }

    // Initialize the blocking tiles.
    for (y = 0; y < 32; y++)
    {
	val = 0;
	mask = 1;
	for (x = 0; x < 32; x++)
	{
	    if (canMove(x, y, movetype, allowmob, allowdoor))
		val |= mask;
	    mask <<= 1;
	}
	glbBlockingTiles[y] = val;
    }

    // Initialize the current array to the destination location.
    memset(glbWalkTiles1, 0, sizeof(u32) * 32);
    glbWalkTiles1[ey] = (1 << ex);

    // Now, walk until we get a match...
    curtile = glbWalkTiles1;
    lasttile = glbWalkTiles2;

    endmask = (1 << sx);

    // We explicitly allow walking to the end square.  If allowmob
    // is false, for example, we may falsely report we can't get to the
    // mob.
    glbBlockingTiles[sy] |= endmask;

    iterations = 0;
    while (1)
    {
	// Write from curtile into lasttile the effect of one step.
	for (y = 0; y < 32; y++)
	{
	    // Include effects of stepping left and right.
	    lasttile[y] = curtile[y] | (curtile[y] << 1) | (curtile[y] >> 1);
	}

	// Now, merge in vertical steps.
	for (y = 1; y < 31; y++)
	{
	    lasttile[y] |= curtile[y-1] | curtile[y+1];
	}

	// Special cases.
	lasttile[0] |= curtile[1];
	lasttile[31] |= curtile[30];

	// Now, clear out the relevant mask.  At the same time,
	// we track if we have any differences.  If there are no
	// differences, we can't reach our destination.
	diff = 0;
	for (y = 0; y < 32; y++)
	{
	    lasttile[y] &= glbBlockingTiles[y];
	    diff |= lasttile[y] ^ curtile[y];
	}

	if (!diff)
	{
#if 0
	    BUF		buf;
	    buf.sprintf(buf, "Same effect at %d.  ", iterations);
	    msg_report(buf);
#endif
	    return false;
	}

	// Swap last & cur.
	tmp = curtile;
	curtile = lasttile;
	lasttile = tmp;
	
	// If the curtile now has the end bit set, we are done.
	if (curtile[sy] & endmask)
	    break;

	iterations++;
    }

    if (0)
    {
	BUF		buf;
	buf.sprintf("Found end at %d.  ", iterations);
	msg_report(buf);
    }
    
    // We have successfully built a path to the start.  To determine
    // the best path, we check the previous step.  All the neighbouring
    // squares of the previous step are, by definition, closer.  Further,
    // at least one must be set.
    int			numdir, dir, dirlist[4];

    numdir = 0;
    for (dir = 0; dir < 4; dir++)
    {
	getDirection(dir, dx, dy);
	x = sx + dx;
	y = sy + dy;
	if (x >= 0 && x < 32 && y >= 0 && y < 32)
	{
	    if (lasttile[y] & (1 << x))
	    {
		dirlist[numdir++] = dir;
	    }
	}
    }

    if (!numdir)
    {
	UT_ASSERT(!"No direction found?");
	return false;
    }

    // Pick a direction at random...
    dir = rand_choice(numdir);
    dir = dirlist[dir];

    getDirection(dir, dx, dy);
    return true;
}

void
MAP::updateAvatarSmell(int x, int y, int smell)
{
    int		cursmell;

    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // Move to top of current watermark.
    smell += ourAvatarSmellWatermark;

    // Get current smell at this square.
    cursmell = ourAvatarSmell[y * MAP_WIDTH + x];

    // Update if better...
    if (cursmell < smell || cursmell > ourAvatarSmellWatermark)
	ourAvatarSmell[y * MAP_WIDTH + x] = smell;
}

int
MAP::getAvatarSmell(int x, int y) const
{
    u16		usmell;		// You smell
    int		ismell;		// I smell
    // char	wiismell;	// We smell.

    usmell = ourAvatarSmell[y * MAP_WIDTH + x];
    // If we are farther than the watermark, it is zero.
    if (usmell > ourAvatarSmellWatermark)
	return 0;
    // Find negative smell.  Lower is less smelly, but is centered
    // with 0 being closest.
    ismell = (int) usmell - (int)ourAvatarSmellWatermark;
    // Normalize.
    ismell += 65535;
    
    return ismell;
}

bool
MAP::getAvatarSmellGradient(MOB *mob, int x, int y, int &dx, int &dy) const
{
    int		maxsmell = 0, smell;
    int		ddx, ddy;

    FORALL_8DIR(ddx, ddy)
    {
	if (mob && mob->canMove(x+ddx, y+ddy, true))
	{
	    // Possible smelly square to move to.
	    smell = getAvatarSmell(x+ddx, y+ddy);
	    if (smell > maxsmell)
	    {
		dx = ddx;
		dy = ddy;
		maxsmell = smell;
	    }
	}
    }

    // See if we got a good direction...
    if (maxsmell)
	return true;
    return false;
}

void
MAP::populate()
{
    int		 desiredcount;
    MOB		*mob;
    int		 x, y;
    int		 fails;
    int		 i, n;

    desiredcount = getDesiredMobCount();

    // desiredcount = 100;

    fails = 0;
    while (myMobCount < desiredcount && (fails < 20))
    {
	// No worry about FOV here.
	if (!createAndPlaceNPC(false))
	    fails++;
    }

    // Create a dimensional portal on the correct level
    if (myDepth == 20 && branchName() == BRANCH_MAIN)
    {
	int		tries = 20;
	while (tries--)
	{
	    if (findRandomLoc(x, y, MOVE_STD_SWIM, true, false, false, false, false, false))
	    {
		// Make sure this isn't an invulnerable square.
		SQUARE_NAMES		square;

		square = getTile(x, y);
		if (!glb_squaredefs[square].invulnerable)
		{
		    setTile(x, y, SQUARE_DIMDOOR);
		    // Dimensional door glows!
		    setFlag(x, y, SQUAREFLAG_LIT);
		    break;
		}
	    }
	}
	UT_ASSERT(tries);
    }

    // Check if we are a special level..
    if (myDepth == 25 && branchName() == BRANCH_MAIN)
    {
	// Add the daemon...
	mob = MOB::create(MOB_BAEZLBUB);
	if (findRandomLoc(x, y, mob->getMoveType(), false, 
		    mob->getSize() >= SIZE_GARGANTUAN,
		    false, false, true, true))
	{
	    if (!mob->move(x, y, true))
	    {
		UT_ASSERT(!"COOL MOB INSTA KILLED");
	    }
	    else
	    {
		registerMob(mob);
		// We want this to be a global broadcast.
		msg_report("You sense your doom approaching!  ");
	    }
	}
	else
	{
	    UT_ASSERT(!"Couldn't place cool mob!");
	    delete mob;
	}
    }

    // Populate the floor of the dungeon.
    n = 10 + rand_choice(myDepth);

    // Do not put anything on the wilderness map.
    if (!allowItemGeneration())
	n = 0;
    for (i = 0; i < n; i++)
    {
	ITEM		*item;
	// Don't generate items in nomob locations such as star fields.
	if (findRandomLoc(x, y, MOVE_WALK, true, false, false, false, false, true))
	{
	    item = ITEM::createRandom();
#if 0
	    item = ITEM::create(ITEM_BOULDER);
#endif
	    acquireItem(item, x, y, 0);
	}
    }
}

bool		 
MAP::createAndPlaceNPC(bool avoidfov)
{
    MOB	*mob;
    int	 x, y;
    bool	avoidfire = true, avoidacid = true;
    
    mob = MOB::createNPC(getThreatLevel());
    if (rand_chance(1))
	mob->makeUnique();

    if (mob->hasIntrinsic(INTRINSIC_RESISTFIRE) &&
	!mob->hasIntrinsic(INTRINSIC_VULNFIRE))
	avoidfire = false;

    if ((mob->getMoveType() & MOVE_FLY))
	avoidacid = false;

    if (mob->hasIntrinsic(INTRINSIC_RESISTACID) &&
	!mob->hasIntrinsic(INTRINSIC_VULNACID))
	avoidacid = false;

    // We don't want the new dude to show up in the FOV.
    if (findRandomLoc(x, y, mob->getMoveType(), false,
		    mob->getSize() >= SIZE_GARGANTUAN,
		    avoidfov, avoidfire, avoidacid, true))
    {
	// Do an interlevel move to skip trap checking.
	// Still check for move death.
	if (mob->move(x, y, true))
	{
	    registerMob(mob);
	    return true;
	}
    }
    // Failed to place the mob, ignore.
    delete mob;
    return false;
}

void
MAP::refresh(bool preserve_ray)
{
    int		 x, y, tile, mobtile;
    MOB		*mob;
    ITEM	*item;
    bool	 avatarblind = false;
    bool	 avatarinvis = false;
    bool	 avatarinpit = false;
    bool	 avatarintree = false;
    bool	 avatarsubmerged = false;
    bool	 forcelit;
    
    memset(ourItemMap, TILE_VOID, sizeof(u16) * MAP_HEIGHT * MAP_WIDTH);
    memset(ourMobMap, TILE_VOID, sizeof(u16) * MAP_HEIGHT * MAP_WIDTH);
    
    // Cycle through all MOBs...
    MOB		*avatar = MOB::getAvatar();
    int		 ax = 0, ay = 0;

    if (avatar)
    {
	avatarblind = avatar->hasIntrinsic(INTRINSIC_BLIND);
	avatarinpit = avatar->hasIntrinsic(INTRINSIC_INPIT);
	avatarintree = avatar->hasIntrinsic(INTRINSIC_INTREE);
	avatarsubmerged = avatar->hasIntrinsic(INTRINSIC_SUBMERGED);
	ax = avatar->getX();
	ay = avatar->getY();
    }
    
    // Cycle through all items...
    for (item = getItemHead(); item; item = item->getNext())
    {
	int		ix, iy;

	ix = item->getX();
	iy = item->getY();

	// Items that are below grade (in pits, or under water)
	// an only be seen by avatars that are below grade,
	// in pits or underwater.
	if (item->isBelowGrade())
	{
	    if (avatar)
	    {
		// One's in a pit are only seen by avatars on that
		// square.
		if (avatarinpit)
		{
		    if (ix != ax || iy != ay)
		    {
			// Can't see, not in our own pit.
			continue;
		    }
		}
		// If the avatar is underwater, can see all below
		// grade stuff (if it has LOS, of course :>)
		else if (!avatarsubmerged)
		{
		    // Not in a pit, not underwater.
		    continue;
		}
	    }
	}
	else
	{
	    if (avatarinpit || avatarsubmerged)
	    {
		// Can't see anything above ground if we are
		// underground.
		continue;
	    }
	    
	    // Can't see anything under tree canopy when in trees.
	    if (avatarintree && glb_squaredefs[getTile(ix,iy)].isforest)
		continue;
	}

	// Figure out rejection criteria from expensive to cheap...
	if (item->isMapped())
	{
	    // Trivially true.
	    ourItemMap[iy * MAP_WIDTH + ix] = item->getTile();
	    continue;
	}
	if (!getFlag(ix, iy, SQUAREFLAG_MAPPED))
	{
	    // Trivially false...
	    continue;
	}
	// Now, check if ix, iy is visible to us.  Our square is always
	// visible, even if blind.  Otherwise, if we can see, we use
	// LOS.
	if (ix == ax && iy == ay)
	{
	    item->markMapped();
	    ourItemMap[iy * MAP_WIDTH + ix] = item->getTile();
	    continue;
	}
	// Check if avatar is blind, that is trivial no see.
	if (avatarblind)
	    continue;
	
	// Check if square is lit.  If not, can't see.
	// We can always feel around to neighbouring squares if it
	// is merely dark.
	forcelit = false;
	if ((ix - ax >= -1) && (ix - ax <= 1) &&
	    (iy - ay >= -1) && (iy - ay <= 1))
	    forcelit = true;

	if (!forcelit && !isLit(ix, iy))
	    continue;

	// Finally, check LOS.  Have LOS, can see.
	UT_ASSERT(ix >= 0 && ix < MAP_WIDTH);
	UT_ASSERT(iy >= 0 && iy < MAP_HEIGHT);
	if (hasLOS(ax, ay, ix, iy))
	{
	    item->markMapped();
	    ourItemMap[iy * MAP_WIDTH + ix] = item->getTile();
	}
    }

    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	if (avatar ? avatar->canSense(mob)
		   : true)
	{
	    if (avatar && avatar->getSenseType(mob) == SENSE_WARN)
	    {
		// MOB tile gets changed to a ?
		ourMobMap[mob->getY() * MAP_WIDTH + mob->getX()] =
							    TILE_UNKNOWN;
		if (mob->getSize() >= SIZE_GARGANTUAN
		    && mob->getX() < MAP_WIDTH - 1
		    && mob->getY() < MAP_HEIGHT - 1)
		{
		    ourMobMap[(mob->getY()+1) * MAP_WIDTH + mob->getX()] =
								TILE_UNKNOWN;
		    ourMobMap[(mob->getY()+1) * MAP_WIDTH + mob->getX()+1] =
								TILE_UNKNOWN;
		    ourMobMap[mob->getY() * MAP_WIDTH + mob->getX()+1] =
								TILE_UNKNOWN;
		}
	    }
	    else
	    {
		ourMobMap[mob->getY() * MAP_WIDTH + mob->getX()] =
							    mob->getTile();
		if (mob->getSize() >= SIZE_GARGANTUAN
		    && mob->getX() < MAP_WIDTH - 1
		    && mob->getY() < MAP_HEIGHT - 1)
		{
		    ourMobMap[(mob->getY()+1) * MAP_WIDTH + mob->getX()] =
							    mob->getTileLL();
		    ourMobMap[(mob->getY()+1) * MAP_WIDTH + mob->getX()+1] =
							    mob->getTileLR();
		    ourMobMap[mob->getY() * MAP_WIDTH + mob->getX()+1] =
							    mob->getTileUR();
		}
	    }
	}
	else if (mob->isAvatar())
	{
	    // Can't see oneself!
	    avatarinvis = true;
	}
    }

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    if (getFlag(x, y, SQUAREFLAG_MAPPED))
		tile = glb_squaredefs[mySquares[y * MAP_WIDTH + x]].tile;
	    else
		tile = TILE_VOID;

	    mobtile = TILE_VOID;
	    if (ourItemMap[y * MAP_WIDTH + x] != TILE_VOID)
		mobtile = ourItemMap[y * MAP_WIDTH + x];
	    if (ourMobMap[y * MAP_WIDTH + x] != TILE_VOID)
		mobtile = ourMobMap[y * MAP_WIDTH + x];

	    // Clear out underneath the mob if opaque tiles are on.
	    if (mobtile != TILE_VOID && glbOpaqueTiles)
		tile = TILE_VOID;

	    gfx_settile(x, y, tile);
	    gfx_setmoblayer(x, y, mobtile);

	    // See if we force the tile as lit due to avatar being
	    // dead or this being next to avatar.
	    forcelit = false;
	    if (!avatar)
		forcelit = true;
	    else if (!avatarblind)
	    {
		int	diffx, diffy;
		diffx = ax - x;
		diffy = ay - y;
		if (diffx >= -1 && diffx <= 1 &&
		    diffy >= -1 && diffy <= 1)
		{
		    forcelit = true;
		}
	    }

	    // If the avatar is blind, everything is shadow.
	    if (!avatarblind && hasFOV(x, y) && (forcelit || isLit(x, y)))
	    {
		// It isn't shadow.  It might be a smokey square though.
		if (getFlag(x, y, SQUAREFLAG_SMOKE))
		{
		    SMOKE_NAMES		smoke;

		    smoke = getSmoke(x, y);

		    tile = glb_smokedefs[smoke].tile;
		}
		else
		    tile = TILE_VOID;
	    }
	    else
		tile = TILE_SHADOW;

	    // Exception: If this is a mob, we want to reveal how
	    // we saw the mob.   
	    // We have to apply this to all mobs.  If a mob is in a lit
	    // area but we only sense with hearing, we want to put
	    // the hearing overlay on.
	    if ((tile == TILE_SHADOW || tile == TILE_VOID) && 
		ourMobMap[y * MAP_WIDTH + x] != TILE_VOID)
	    {
		if (avatar)
		{
		    switch (avatar->getSenseType(getMob(x, y)))
		    {
			case SENSE_SIGHT:
			{
			    MOB		*mob = getMob(x, y);

			    tile = TILE_VOID;
			    if (mob)
			    {
				// If the creature is burning, draw special
				if (mob->hasIntrinsic(INTRINSIC_AFLAME))
				{
				    tile = TILE_AFLAME;
				}
				else if (mob->hasIntrinsic(INTRINSIC_TAME) &&
					mob->isSlave(MOB::getAvatar()))
				{
				    tile = TILE_TAMEMOB;
				}
				else if (mob->hasIntrinsic(INTRINSIC_UNIQUE))
				{
				    tile = TILE_UNIQUEMOB;
				}
				else if (mob->hasIntrinsic(INTRINSIC_ASLEEP))
				{
				    tile = TILE_ASLEEP;
				}
			    }
			    break;
			}

			case SENSE_NONE:
			    // Should not happen.
			    // Maybe black tile?
			    tile = TILE_SHADOW;
			    break;

			case SENSE_HEAR:
			    tile = TILE_SHADOW_HEAR;
			    break;

			case SENSE_WARN:
			    tile = TILE_SHADOW_ESP;
			    break;

			case SENSE_ESP:
			    tile = TILE_SHADOW_ESP;
			    break;
			    
			case NUM_SENSES:
			    UT_ASSERT(!"Invalid sense!");
			    break;
		    }
		}
	    }

	    // Exception: If this is the avatar's location, and the
	    // avatar isn't displayed due to being invisible, draw
	    // the invisible avatar overlay tile.
	    if (avatarinvis)
	    {
		if (x == ax && y == ay)
		{
		    tile = TILE_INVISIBLEAVATAR;
		}
	    }

	    // Exception: If there is no mob here, but there is
	    // an item which is an artifact, flag as such.
	    if (tile == TILE_VOID && mobtile != TILE_VOID &&
		ourMobMap[y * MAP_WIDTH + x] == TILE_VOID)
	    {
		// Returns topmost item quickly.
		item = getItem(x, y);
		if (item && item->isArtifact())
		{
		    // Okay, a bit misnamed now.
		    tile = TILE_UNIQUEMOB;
		}
	    }

	    // If preserve ray is turned on, we check the current
	    // overlay tile and see if we have manually updated
	    // it into a ray tile.  If so, we don't want to
	    // overwrite it.
	    // Alternatively, we may want to preserve anything that
	    // is *not* one of the standard shadow tiles?
	    if (preserve_ray)
	    {
		int		raytile;

		raytile = gfx_getoverlay(x, y);

		switch (raytile)
		{
		    case TILE_RAYSLASH:
		    case TILE_RAYPIPE:
		    case TILE_RAYDASH:
		    case TILE_RAYBACKSLASH:
			// Preseve the tile
			tile = raytile;
			break;
		    default:
			// Leave tile unchanged.
			break;
		}
	    }

	    gfx_setoverlay(x, y, tile);
	}
    }
}

MOB *
MAP::getGuiltyMob(int x, int y) const
{
    if (this != glbCurLevel)
	return 0;

    return ourMobGuiltyMap[y*MAP_WIDTH+x].getMob();
}

void
MAP::setGuiltyMob(int x, int y, MOB *mob)
{
    if (this != glbCurLevel)
	return;

    ourMobGuiltyMap[y*MAP_WIDTH+x].setMob(mob);
}

void
MAP::moveMob(MOB *mob, int ox, int oy, int nx, int ny)
{
    int		dx, dy;
    
    // Note that the mob may not be in its old position, it may
    // have already been overwritten.
    // Note also that the old position may be invalid.

    if (this != glbCurLevel)
	return;

    if (ox >= 0 && ox < MAP_WIDTH &&
	oy >= 0 && oy < MAP_HEIGHT)
    {
	// Clear old spot.
	if (ourMobRefMap[oy * MAP_WIDTH + ox].getMob() == mob)
	    ourMobRefMap[oy * MAP_WIDTH + ox].setMob(0);

	if (mob->getSize() >= SIZE_GARGANTUAN)
	{
	    for (dx = 0; dx < 2; dx++)
	    {
		for (dy = 0; dy < 2; dy++)
		{
		    if (ourMobRefMap[(oy + dy) * MAP_WIDTH + (ox+dx)].getMob() == mob)
			ourMobRefMap[(oy + dy) * MAP_WIDTH + (ox+dx)].setMob(0);
		}
	    }
	}
    }

    if (nx >= 0 && nx < MAP_WIDTH &&
	ny >= 0 && ny < MAP_HEIGHT)
    {
	// Mark new spot.
	ourMobRefMap[ny * MAP_WIDTH + nx].setMob(mob);

	if (mob->getSize() >= SIZE_GARGANTUAN)
	{
	    for (dx = 0; dx < 2; dx++)
	    {
		for (dy = 0; dy < 2; dy++)
		{
		    ourMobRefMap[(ny + dy) * MAP_WIDTH + (nx+dx)].setMob(mob);
		}
	    }
	}
    }
}


void		 
MAP::wallCrush(MOB *mob)
{
    UT_ASSERT(mob != 0);
    if (!mob)
	return;

    if (mob->getSize() < SIZE_GARGANTUAN)
	return;

    int		dx, dy, x, y;
    MOB		*movemob;
    x = mob->getX();
    y = mob->getY();
    for (dy = 0; dy < 2; dy++)
    {
	for (dx = 0; dx < 2; dx++)
	{
	    if (!canMove(x+dx, y+dy, MOVE_STD_FLY))
	    {
		if (glb_squaredefs[getTile(x+dx, y+dy)].invulnerable)
		{
		    // Don't destroy invulernable walls.
		    mob->formatAndReport("%U <crush> the wall, but the wall withstands the impact!");
		}
		else
		{
		    mob->formatAndReport("%U <crush> the walls!");
		    setTile(x+dx, y+dy, SQUARE_CORRIDOR);
		}
	    }
	    movemob = getMobPrecise(x+dx, y+dy);
	    if (movemob && (movemob != mob))
	    {
		movemob->formatAndReport("%U <be> pushed out of the way.");
		int		nx, ny;
		nx = movemob->getX();
		ny = movemob->getY();
		if (findCloseTile(nx, ny, movemob->getMoveType(), false))
		{
		    movemob->move(nx, ny);
		}
		else
		{
		    movemob->formatAndReport("But there is no where to move!");
		}
	    }
	}
    }
}

void
MAP::registerMob(MOB *mob)
{
    mob->setNext(myMobHead);
    mob->setDLevel(myDepth);
    myMobHead = mob;
    myMobCount++;

    if (glbCurLevel == this)
    {
	ourMobRefMap[mob->getY() * MAP_WIDTH + mob->getX()].setMob(mob);
	if (mob->getSize() >= SIZE_GARGANTUAN)
	{
	    int		dx, dy;
	    for (dx = 0; dx < 2; dx++)
		for (dy = 0; dy < 2; dy++)
		    ourMobRefMap[(mob->getY() + dy) * MAP_WIDTH + mob->getX() + dx].setMob(mob);
	}
    }
}

void
MAP::unregisterMob(MOB *mob)
{
    // TODO: If this is deadly, a double linked list would get us
    // O(1) removal...
    MOB		*cur;

    mob->setDLevel(-1);
    mob->clearReferences();
    
    if (glbCurLevel == this)
    {
	ourMobRefMap[mob->getY() * MAP_WIDTH + mob->getX()].setMob(0);
	if (mob->getSize() >= SIZE_GARGANTUAN)
	{
	    ourMobRefMap[(mob->getY()+1) * MAP_WIDTH + mob->getX()].setMob(0);
	    ourMobRefMap[(mob->getY()+1) * MAP_WIDTH + mob->getX()+1].setMob(0);
	    ourMobRefMap[mob->getY() * MAP_WIDTH + mob->getX()+1].setMob(0);
	}
    }

    // Ensure any safe mob loops don't crash.
    if (myMobSafeNext == mob)
	myMobSafeNext = mob->getNext();

    for (cur = getMobHead(); cur; cur = cur->getNext())
    {
	if (cur != mob)
	    cur->alertMobGone(mob);
    }

    if (mob == myMobHead)
    {
	myMobHead = myMobHead->getNext();
	mob->setNext(0);
	myMobCount--;
	return;
    }
    for (cur = myMobHead; cur->getNext(); cur = cur->getNext())
    {
	if (mob == cur->getNext())
	{
	    cur->setNext(mob->getNext());
	    mob->setNext(0);
	    myMobCount--;
	    return;
	}
    }
    UT_ASSERT(!"Removal of non-present mob");
}

MOB *
MAP::getMob(int x, int y) const
{
    MOB		*cur;

    if (this == glbCurLevel)
    {
	if (x < 0 || x >= MAP_WIDTH ||
	    y < 0 || y >= MAP_HEIGHT)
	    return 0;

	return ourMobRefMap[y * MAP_WIDTH + x].getMob();
    }

    for (cur = getMobHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	    return cur;
	if (cur->getSize() >= SIZE_GARGANTUAN)
	{
	    if ( (cur->getX()+1 == x || cur->getX() == x) &&
		 (cur->getY()+1 == y || cur->getY() == y) )
	    {
		return cur;
	    }
	} 
    }	    
    return 0;
}

MOB *
MAP::getMobPrecise(int x, int y) const
{
    MOB		*cur;

    for (cur = getMobHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	    return cur;
    }	    
    return 0;
}

void
MAP::chaseMobOnStaircase(SQUARE_NAMES destsquare,
		    int srcx, int srcy, 
		    MOB *chasee, MAP *nextmap)
{
    MOB		*safenext, *chaser;
    int		 range;
    bool	 follow;
    int		 x, y;

    for (chaser = getMobHead(); chaser; chaser = safenext)
    {
	safenext = chaser->getNext();
	
	range = MAX(abs(chaser->getX() - srcx),
		    abs(chaser->getY() - srcy));

	follow = false;
	
	if (range <= 1)
	{
	    // Within range, this dude can make it to the staircase.
	    // However, he may not be interested.

	    // Avatar is never moved implicitly.
	    if (chaser->isAvatar())
		continue;

	    // Double square mobs won't find correct spots on otherside
	    // of the stair case, so don't chase for now.
	    if (chaser->getSize() >= SIZE_GARGANTUAN)
	    {
		continue;
	    }

	    if (chaser->isSlave(chasee))
	    {
		// Our owner went downstairs, we follow
		follow = true;
	    }

	    if (chaser->getAITarget() == chasee)
	    {
		// Someone we want to kill went downstiars, follow.
		follow = true;
	    }
	}

	if (follow)
	{
	    if (nextmap->findTile(destsquare, x, y) &&
		nextmap->findCloseTile(x, y, chaser->getMoveType()))
	    {
		// We have a valid destination, unlink and move!
		unregisterMob(chaser);
		chaser->move(x, y, true);
		nextmap->registerMob(chaser);
	    }
	}
    }
}

bool
MAP::findRandomLoc(int &ox, int &oy, int movetype, bool allowmob, bool doublesize, bool avoidfov, bool avoidfire, bool avoidacid, bool avoidnomob)
{
    // TODO: This is 2k we could save.
    static u16		validlist[MAP_WIDTH * MAP_HEIGHT];
    int			numvalid = 0;
    int			x, y, tile, i;
    ITEM		*item;

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    // Don't allow spots with FOV set if this is enabled.
	    if (avoidfov && hasFOV(x, y))
		continue;

	    if (avoidnomob && getFlag(x, y, SQUAREFLAG_NOMOB))
		continue;
	    
	    // Check if this tile is valid.
	    tile = getTile(x, y);

	    if (avoidfire && tile == SQUARE_LAVA)
		continue;
	    if (avoidacid && tile == SQUARE_ACID)
		continue;
	    
	    if (glb_squaredefs[tile].movetype & movetype)
	    {
		// Check if it has a mob...
		if (allowmob || !getMob(x, y))
		{
		    // Check to see if there is an immovable item.
		    item = getItem(x, y);
		    if (item && !item->isPassable())
			continue;
		    
		    if (doublesize)
		    {
			if (x < MAP_WIDTH - 1 &&
			    y < MAP_HEIGHT - 1)
			{
			    if ((glb_squaredefs[getTile(x+1,y)].movetype & movetype) &&
			        (glb_squaredefs[getTile(x+1,y+1)].movetype & movetype) &&
			        (glb_squaredefs[getTile(x,y+1)].movetype & movetype))
			    {
				if (allowmob ||
				    (!getMob(x+1,y) && 
				     !getMob(x+1,y+1) &&
				     !getMob(x,y+1)))
				{
				    validlist[numvalid++] = x + y * MAP_WIDTH;
				}
			    }
			}
		    }
		    else
			validlist[numvalid++] = x + y * MAP_WIDTH;
		}
	    }
	}
    }
    // If valid list is empty, we have no valid squares.
    if (!numvalid)
    {
	// This can happen in places like the Hell Level if a lot
	// of creatures are spawned (as found in the stress test)
	//UT_ASSERT(!"Failed to find valid square");
	return false;
    }

    // Now, we pick an entry at random...
    i = rand_range(0, numvalid-1);
	    
    ox = validlist[i] & (MAP_WIDTH-1);
    oy = validlist[i] / MAP_WIDTH;
    return true;
}

void
MAP::moveNPCs()
{
    MOB		*mob;

    for (mob = getMobHead(); mob; mob = myMobSafeNext)
    {
	myMobSafeNext = mob->getNext();

	// What if next is killed by doAI?
	//  - handled by using myMobSafeNext (hack)
	// What if mob is killed by doAI?
	//  - ignored since is last stage of processing
	// What if mob is killed by heartbeat?
	//  - handled by returning false from doHeartbeat (unrobust)
	if (mob->isAvatar())
	    continue;			// Ignore the PC.
	// Heartbeat returns true if AI can run.
	if (mob->doHeartbeat())
	    if (mob->doMovePrequel())
		mob->doAI();
    }
}

int
MAP::getThreatLevel() const
{
    int		threatlevel = myDepth;

    if (glbStressTest && !glbMapStats)
	return 50;

    // Surface world doesn't have monsters.
    if (!threatlevel)
	return threatlevel;

    if (MOB::getAvatar())
    {
	MOB		*avatar = MOB::getAvatar();
	
	if (avatar->isPolymorphed())
	{
	    avatar = avatar->getBaseType();
	    if (!avatar)
	    {
		UT_ASSERT(!"No base type!");
		avatar = MOB::getAvatar();
	    }
	}
	threatlevel += avatar->getHitDie() + avatar->getMagicDie();
    }

    return threatlevel;
}

int
MAP::getDesiredMobCount() const
{
    if (!allowMobGeneration())
	return 0;

    return getThreatLevel() / 2;
}

void
MAP::doHeartbeat()
{
    // In all cases, update our smell watermark so we can get proper
    // gradients.
    ourAvatarSmellWatermark++;
    if (ourAvatarSmellWatermark == 65535)
    {
	// We overflowed our watermark.  We clear our map and set it
	// to the defualt distance of 64.  I wonder if anyone will notice
	// that every 65k moves all creatures become confused?
	ourAvatarSmellWatermark = 64;
	memset(ourAvatarSmell, 0xff, sizeof(u16) * MAP_WIDTH * MAP_HEIGHT);
    }

    // Determine if we are in a proper phase.
    if (!speed_isheartbeatphase())
	return;

    // Determine if our number of mobs makes sense for our
    // threat level...
    int		spawnchance = 0;
    int		desiredcount;

    // Our probability of spawning will never go to zero.  It will
    // attempt to maintain the proper number of mobs for this threat
    // level.  If we have less than our desired number, we spawn
    // at 10% (one every 10 turns).  If we have ten more than that number,
    // we spawn at 1%.  Between that is interpolated.
    // The desired number is threatlevel / 2, maxed at 20.
    desiredcount = getDesiredMobCount();

    if (desiredcount > 20) desiredcount = 20;
    if (myMobCount < desiredcount)
	spawnchance = 10;
    else if (myMobCount - 10 < desiredcount)
	spawnchance = 10 - (desiredcount - myMobCount);
    else
	spawnchance = 1;

    if (!desiredcount)
	spawnchance = 0;

//  if (1)
    if (rand_chance(spawnchance))
    {
	createAndPlaceNPC(true);
    }

    // Run the heartbeat on all the items...
    ITEM		*cur, *next;

    for (cur = getItemHead(); cur; cur = next)
    {
	next = cur->getNext();

	if (!cur->doHeartbeat(glbCurLevel, 0))
	{
	    dropItem(cur);
	    delete cur;
	}
    }
}

ITEM *
MAP::getItem(int x, int y) const
{
    ITEM	*cur, *result = 0;

    if (this == glbCurLevel)
    {
	// Use the item map.
	if (x < 0 || x >= MAP_WIDTH ||
	    y < 0 || y >= MAP_HEIGHT)
	    return 0;
	return ourItemPtrMap[y * MAP_WIDTH + x];
    }

    // Because the drawn item is the top item, it is also the guy we
    // return here.
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	    result = cur;
    }
    return result;
}

ITEM *
MAP::getItem(int x, int y, bool submerged) const
{
    ITEM	*cur, *result = 0;

    if (this == glbCurLevel)
    {
	// Use the item map.
	if (x < 0 || x >= MAP_WIDTH ||
	    y < 0 || y >= MAP_HEIGHT)
	    return 0;

	cur = ourItemPtrMap[y * MAP_WIDTH + x];
	if (cur && cur->isBelowGrade())
	{
	    if (submerged)
		result = cur;
	}
	else
	{
	    if (!submerged)
		result = cur;
	}

	// TODO: Doesn't work for water walking on top of submerged
	// stuff!
	return result;
    }

    // Because the drawn item is the top item, it is also the guy we
    // return here.
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	{
	    if (cur->isBelowGrade())
	    {
		if (submerged)
		    result = cur;
	    }
	    else
	    {
		if (!submerged)
		    result = cur;
	    }
	}
    }
    return result;
}

int
MAP::getItemStack(ITEMSTACK &stack, int x, int y, int submergecheck,
		    bool checkmapped) const
{
    int			 numadded = 0;
    ITEM		*item;
    bool		 subboolean = (submergecheck ? true : false);
    
    // Because the drawn item is the top item, it is also the guy we
    // return here.
    for (item = getItemHead(); item; item = item->getNext())
    {
	if (item->getX() == x && item->getY() == y)
	{
	    if (submergecheck < 0 || (subboolean == item->isBelowGrade()))
	    {
		if (checkmapped && !item->isMapped())
		    continue;
		stack.append(item);
		numadded++;
	    }
	}
    }
    
    return numadded;
}

int
MAP::getPets(MOBSTACK &stack, MOB *owner, bool onlysensable) const
{
    MOB		*mob;

    stack.clear();

    if (!owner)
	return 0;

    for (mob = myMobHead; mob; mob = mob->getNext())
    {
	if (mob == owner)
	    continue;

	if (mob->getMaster() == owner)
	{
	    // Verify we can sense the mob.
	    if (onlysensable)
	    {
		if (!owner->canSense(mob))
		    continue;
	    }
	    stack.append(mob);
	}
    }

    return stack.entries();
}

// If this is a bottle, and if it dropped into liquid, we want
// to fill it with the liquid.
// We preserve the blessed/curse status of the bottle.
void
MAP::fillBottle(ITEM *item, int x, int y) const
{
    int			 tile;

    if (item->getDefinition() != ITEM_BOTTLE)
	return;

    tile = getTile(x, y);
    switch (tile)
    {
	case SQUARE_WATER:
	{
	    // Convert to water.
	    item->formatAndReport("%U <fill> with water.");
	    item->setDefinition(ITEM_WATER);
	    break;
	}
	case SQUARE_LAVA:
	{
	    // Fill with lava.
	    item->formatAndReport("%U <be> infused with fire.");
	    item->setDefinition(ITEM::lookupMagicItem(MAGICTYPE_POTION,
						    POTION_GREEKFIRE));
	    break;
	}
	case SQUARE_ACID:
	{
	    // Fill with acid.
	    item->formatAndReport("%U <fill> with acid.");
	    item->setDefinition(ITEM::lookupMagicItem(MAGICTYPE_POTION,
						    POTION_ACID));
	    break;
	}
	default:
	    // Safe to do nothing.
	    break;
    }
}

void
MAP::acquireItem(ITEM *item, int x, int y, MOB *dropper, ITEM **newitem, bool ignoregrade)
{
    ITEM		*last, *cur, *oldtopitem;
    bool		 settopitem = false;
    bool		 falldown = false;
    bool		 dropinliquid = false;
    BUF			 buf;
    int			 tile, trap;

    if (newitem)
	*newitem = item;

    // The item should already be removed from its own list.
    UT_ASSERT(!item->getNext());

    myItemCount++;
    oldtopitem = 0;
    if (this == glbCurLevel)
    {
	// Impassable items remain on the top of the heap.
	if (!ourItemPtrMap[y * MAP_WIDTH + x] ||
	     ourItemPtrMap[y * MAP_WIDTH + x]->getStackOrder() <= item->getStackOrder())
	{
	    settopitem = true;
	    oldtopitem = ourItemPtrMap[y * MAP_WIDTH + x];
	    ourItemPtrMap[y * MAP_WIDTH + x] = item;
	}
    }

    // Determine if the item is going below grade.  If so,
    // generate the flavour text.
    tile = getTile(x, y);
    trap = glb_squaredefs[tile].trap;
    switch ((TRAP_NAMES) trap)
    {
	case TRAP_HOLE:
	{
	    // Item falls to the next level.
	    item->setPos(x, y);
	    item->markMapped(false);
	    if (item->fallInHole(this, false, dropper))
	    {
		// Clear out the boulder from our top list.
		if (settopitem)
		{
		    ourItemPtrMap[y * MAP_WIDTH + x] = oldtopitem;
		}
		// Callers should be warned we no longer exist.
		if (newitem)
		    *newitem = 0;
		return;
	    }
	    break;
	}
	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	    // Item falls into a pit.
	    falldown = true;	    
	    buf = MOB::formatToString("%U <fall> into a %B1.",
		    0, item, 0, 0,
		    glb_trapdefs[trap].name);
	    break;
	case TRAP_TELEPORT:
	{
	    item->setPos(x, y);
	    
	    if (item->randomTeleport(this, false, dropper))
	    {
		// That's the end of the story. The item's new position is
		// handled by its own call to acquireItem.
		if (settopitem)
		{
		    ourItemPtrMap[y * MAP_WIDTH + x] = oldtopitem;
		}
		return;
	    }
	    break;
	}
	case TRAP_SMOKEVENT:
	case TRAP_POISONVENT:
	case TRAP_NONE:
	case NUM_TRAPS:
	    // Do nothing.
	    break;
    }
    // Switch on floor type:
    switch (tile)
    {
	case SQUARE_WATER:
	    falldown = true;
	    dropinliquid = true;
	    buf = MOB::formatToString("%U <fall> into the water.",
			0, item, 0, 0);
	    if (item->douse())
	    {
		delete item;

		if (newitem)
		    *newitem = 0;

		// Our top item map is now invalid.
		refreshPtrMaps();
		return;
	    }
	    break;
	case SQUARE_ACID:
	{
	    // If we can dissolve: Good bye item!
	    item->setPos(x, y);
	    if (item->dissolve())
	    {
		delete item;

		if (newitem)
		    *newitem = 0;

		// Our top item map is now invalid.
		refreshPtrMaps();
		return;
	    }	    
	    falldown = true;
	    dropinliquid = true;
	    buf = MOB::formatToString("%U <fall> into the acid.",
			0, item, 0, 0);
	    break;
	}
	case SQUARE_LAVA:
	    // If we can burn: Good bye item!
	    item->setPos(x, y);
	    if (item->ignite())
	    {
		delete item;

		if (newitem)
		    *newitem = 0;

		// Our top item map is now invalid.
		refreshPtrMaps();
		return;
	    }	    
	    falldown = true;
	    dropinliquid = true;
	    buf = MOB::formatToString("%U <fall> into the lava.",
			0, item, 0, 0);
	    break;
    }

    if (dropinliquid)
    {
	if (item->hasIntrinsic(INTRINSIC_WATERWALK))
	    falldown = false;
    }

    if (!ignoregrade && falldown)
    {
	// Only report the fall down message if we are not
	// below grade.
	if (!item->isBelowGrade())
	{
	    item->markBelowGrade(true);
	
	    reportMessage(buf, x, y);
	}

	if (dropinliquid)
	{
	    fillBottle(item, x, y);
	}
    }
    else if (!ignoregrade)
    {
	item->markBelowGrade(false);
    }

    // Check to see if we should merge with an existing pile.
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	{
	    if (cur->canMerge(item))
	    {
		cur->mergeStack(item);
		delete item;
		if (newitem)
		    *newitem = cur;
		// Account for our over estimate:
		myItemCount--;
		if (settopitem)
		{
		    ourItemPtrMap[y * MAP_WIDTH + x] = oldtopitem;
		}
		return;
	    }
	}
    }

    // We add the item where it fits according to its sort order.
    last = 0;
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getStackOrder() > item->getStackOrder())
	    break;
	last = cur;
    }

    // We wish to insert before cur.  Note cur & last may be null.
    item->setNext(cur);
    if (!last)
    {
	// Insert at beginning.
	myItemHead = item;
    }
    else
    {
	// Insert before cur.
	last->setNext(item);
	UT_ASSERT(last != item);
    }
    
    item->setPos(x, y);

    // Now, check to see if we have to fill a pit, or similar.  NOTE:
    // this may delete the item!
    fillSquare(item, dropper, newitem);
}

ITEM *
MAP::dropItem(int x, int y)
{
    ITEM		*item;

    item = getItem(x, y);
    if (item)
	dropItem(item);
    return item;
}

void
MAP::dropItem(ITEM *item)
{
    ITEM		*cur, *prev = 0;

    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur == item)
	{
	    break;
	}
	prev = cur;
    }

    UT_ASSERT(cur && cur == item);
    if (!cur || cur != item)
	return;

    myItemCount--;
    if (prev)
	prev->setNext(item->getNext());
    else
	myItemHead = item->getNext();

    item->markBelowGrade(false);
    item->setNext(0);

    if (this == glbCurLevel &&
	ourItemPtrMap[item->getY() * MAP_WIDTH + item->getX()] == cur)
    {
	// TODO: Find next on stack!
	refreshPtrMaps();
    }
}

void
MAP::buildFOV(int ox, int oy, int radx, int rady)
{
    int		sx, ex;
    int		sy, ey;
    int		x, y, r;

    // Reset FOV stash location...
    myFOVX = myFOVY = -1;
    
    sx = ox - radx;
    if (sx < 0) sx = 0;
    ex = ox + radx;
    if (ex >= MAP_WIDTH) ex = MAP_WIDTH-1;

    sy = oy - rady;
    if (sy < 0) sy = 0;
    ey = oy + rady;
    if (ey >= MAP_HEIGHT) ey = MAP_HEIGHT-1;

    for (y = 0; y < sy; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	    clearFlag(x, y, SQUAREFLAG_FOV);
    }
    for (; y <= ey; y++)
    {
	for (x = 0; x < sx; x++)
	    clearFlag(x, y, SQUAREFLAG_FOV);
	for (; x <= ex; x++)
	{
	    if (hasLOS(ox, oy, x, y))
	    {
		setFlag(x, y, SQUAREFLAG_FOV, true);
		// Update our distance table...
		r = ABS(x - ox) + ABS(y - oy);
		updateAvatarSmell(x, y, -r);
	    }
	    else
		clearFlag(x, y, SQUAREFLAG_FOV);
	}
	for (; x < MAP_WIDTH; x++)
	    clearFlag(x, y, SQUAREFLAG_FOV);
    }
    for (; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	    clearFlag(x, y, SQUAREFLAG_FOV);
    }
    myFOVX = ox;
    myFOVY = oy;
}

bool
MAP::hasFOV(int x, int y) const
{
    UT_ASSERT(x >= 0  && y >= 0);
    UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
    return getFlag(x, y, SQUAREFLAG_FOV) ? true : false;
}

bool
MAP::allowDiggingDown() const
{
    if (myGlobalFlags & MAPFLAG_DIG)
	return true;
    return false;
}

bool
MAP::allowMobGeneration() const
{
    if (myGlobalFlags & MAPFLAG_NEWMOBS)
	return true;
    return false;
}

bool
MAP::allowItemGeneration() const
{
    if (myGlobalFlags & MAPFLAG_NOITEMS)
	return false;
    return true;
}

bool
MAP::allowTeleportation() const
{
    if (myGlobalFlags & MAPFLAG_NOTELE)
	return false;
    return true;
}

void
MAP::markMapped(int ox, int oy, int radx, int rady, bool uselos)
{
    int		sx, ex;
    int		sy, ey;
    int		x, y;
    bool	forcelit;
    
    sx = ox - radx;
    if (sx < 0) sx = 0;
    ex = ox + radx;
    if (ex >= MAP_WIDTH) ex = MAP_WIDTH-1;

    sy = oy - rady;
    if (sy < 0) sy = 0;
    ey = oy + rady;
    if (ey >= MAP_HEIGHT) ey = MAP_HEIGHT-1;

    for (y = sy; y <= ey; y++)
    {
	for (x = sx; x <= ex; x++)
	{
	    forcelit = false;
	    if (uselos)
	    {
		int		diffx, diffy;
		diffx = ox - x;
		diffy = oy - y;
		if (diffx >= -1 && diffx <= 1 && diffy >= -1 && diffy <= 1)
		{
		    forcelit = true;
		}
	    }
	    if (!uselos || (hasLOS(ox, oy, x, y) && (forcelit || isLit(x, y))))
		setFlag(x, y, SQUAREFLAG_MAPPED);
	}
    }
}

bool
MAP::isTransparent(int x, int y) const
{
    SQUARE_NAMES	square;

    // Read squares definition to learn if it blocks LOS.
    // This is more complicated than if it blocks movement!
    square = getTile(x, y);

    bool	forcetrans = false;

    // If the avatar is in a tree trees don't block sight.
    // This is wrong as it lets creatures see through the trees
    // as soon as the avatar does.  However, creatures still can't
    // see the avatar due to the in-tree rules for visibility, so this
    // just affects mob/mob and mob/pet which hopefully people don't
    // notice.  Pet/mob is retconned by the avatar directing the pets.
    if (square == SQUARE_FOREST || square == SQUARE_FORESTFIRE)
    {
	if (MOB::getAvatar() && MOB::getAvatar()->hasIntrinsic(INTRINSIC_INTREE))
	{
	    forcetrans = true;
	}
    }
    if (!forcetrans && !glb_squaredefs[square].transparent)
	return false;

    // Smokey squares block LOS.
    if (getFlag(x, y, SQUAREFLAG_SMOKE))
	return false;

    return true;
}

bool
MAP::hasLOS(int x1, int y1, int x2, int y2) const
{
    // Check to see if the start, (x1,y1), is the stashed FOV location
    // If so, we use the prebuilt FOV table.
    if (x1 == myFOVX && y1 == myFOVY)
    {
	return hasFOV(x2, y2);
    }
    else if (x2 == myFOVX && y2 == myFOVY)
    {
	// While hasLOS is not symmetric, we force it to be
	// in the case of FOV as that avoids hockeystick
	// paradoxes.  (And is more efficient)
	return hasFOV(x1, y1);
    }
    
    int		cx, cy, x, y, acx, acy;

    cx = x2 - x1;
    cy = y2 - y1;
    acx = abs(cx);
    acy = abs(cy);
    
    // Check for trivial LOS (and divide by zero)
    if (acx <= 1 && acy <= 1)
	return true;

    if (acx > acy)
    {
	int		dx, dy, error;

	dx = sign(cx);
	dy = sign(cy);

	error = 0;
	error = acy;
	y = y1;
	for (x = x1 + dx; x != x2; x += dx)
	{
	    if (error >= acx)
	    {
		error -= acx;
		y += dy;
	    }
	    if (!isTransparent(x, y))
		return false;

	    error += acy;
	}
    }
    else
    {
	int		dx, dy, error;

	dx = sign(cx);
	dy = sign(cy);

	error = 0;
	error = acx;
	x = x1;
	for (y = y1 + dy; y != y2; y += dy)
	{
	    if (error >= acy)
	    {
		error -= acy;
		x += dx;
	    }
	    if (!isTransparent(x, y))
		return false;

	    error += acx;
	}
    }

    // Successful LOS.
    return true;
}

bool
MAP::hasSlowLOS(int x1, int y1, int x2, int y2) const
{
    int		cx, cy, dx, dy, x, y, acx, acy;

    cx = x2 - x1;
    cy = y2 - y1;
    acx = abs(cx);
    acy = abs(cy);
    
    // Check for trivial LOS (and divide by zero)
    if (acx <= 1 && acy <= 1)
	return true;

    if (acx > acy)
    {
	float		truey;
	float		deltay;
	int		fy;

	deltay = cy / (float) abs(cx);
	dx = sign(cx);
	truey = y1 + deltay;
	
	for (x = x1 + dx; x != x2; x += dx)
	{
	    fy = (int)floor(truey + 0.5f);
	    if (!isTransparent(x, fy))
	    {
		return false;
	    }
	    truey += deltay;
	}
    }
    else
    {
	float		truex;
	float		deltax;
	int		fx;

	deltax = cx / (float) abs(cy);
	dy = sign(cy);
	truex = x1 + deltax;
	
	for (y = y1 + dy; y != y2; y += dy)
	{
	    fx = (int)floor(truex+0.5f);
	    if (!isTransparent(fx, y))
	    {
		return false;
	    }
	    truex += deltax;
	}
    }

    // Successful LOS.
    return true;
}

bool
MAP::hasDrunkLOS(int x1, int y1, int x2, int y2) const
{
    int		x, y;
    float	truex, truey;
    
    // This does somewhat more correct LOS by drawing a straight line
    // between ourselves and the destintation...
    // You have the direct floating point LOS line between two points.
    // An LOS exists if EITHER of the integer solutions of said line
    // are valid.

    // Trivial cases...
    if (x1 == x2 && y1 == y2)
    {
	return true;
    }	

    // Step through...
    x = x1;
    y = y1;
    if (abs(x2 - x1) > abs(y2 - y1))
    {
	// X major, use truey.
	while (x != x2)
	{
	    x += sign(x2 - x1);
	    truey = (y2 - y1) * ( (float)(x - x1) / (x2 - x1) ) + y1;
	    if (!isTransparent(x, (int)(truey)) &&
		!isTransparent(x, (int)(truey)+1))
	    {
		return false;
	    }
	}
	return true;
    }
    else
    {
	// Y major, use truex.
	while (y != y2)
	{
	    y += sign(y2 - y1);
	    truex = (x2 - x1) * ( (float)(y - y1) / (y2 - y1) ) + x1;
	    if (!isTransparent((int)(truex), y) &&
		!isTransparent((int)(truex)+1, y))
	    {
		return false;
	    }
	}
	return true;
    }
}

bool
MAP::hasManhattenLOS(int x1, int y1, int x2, int y2) const
{
    int		dx, dy;

    // NOTE: The dominance matrix approach won't gain us much here
    // as it is basically a prebuilt left/right/diagonal LUT that
    // is only efficient if we restrict ourselves to 4x4.

    // A square is visible if it has a LOS to the source.
    // We are saying anything with walk/swim/fly is visible.
    // We consider any manhatten walk between origin and dest a valid
    // LOS.
    
    // Trivial...
    if (x1 == x2 && y1 == y2)
    {
	return true;
    }

    // Beside is trivial...
    if (abs(x1 - x2) == 1 && y1 == y2)
	return true;
    if (abs(y1 - y2) == 1 && x1 == x2)
	return true;

    // Find the direction...
    dx = sign(x2 - x1);
    dy = sign(y2 - y1);

    if (dx)
    {
	// Check if visible in dx direction...
	if (isTransparent(x1 + dx, y1))
	{
	    if (hasManhattenLOS(x1 + dx, y1, x2, y2))
		return true;
	}
    }

    if (dy)
    {
	// Check if visible in dy direction...
	if (isTransparent(x1, y1 + dy))
	{
	    if (hasManhattenLOS(x1, y1 + dy, x2, y2))
		return true;
	}
    }

    // Failed in all directions.
    return false;
}

bool
MAP::isLit(int x, int y) const
{
    if (getFlag(x, y, SQUAREFLAG_LIT) ||
	getFlag(x, y, SQUAREFLAG_TEMPLIT))
    {
	return true;
    }
    
    return false;
}

bool
MAP::isDiggable(int x, int y) const
{
    switch (getTile(x, y))
    {
	case SQUARE_EMPTY:
	case SQUARE_WALL:
	case SQUARE_SECRETDOOR:
	case SQUARE_VIEWPORT:
	    return true;
    }

    ITEM		*item;

    item = glbCurLevel->getItem(x, y);
    if (item && item->getDefinition() == ITEM_BOULDER)
    {
	return true;
    }
    
    return false;
}

void
MAP::updateLights()
{
    int		 x, y;
    MOB		*mob;
    ITEM	*item;

    // First, clear out all dynamic lights.
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    // Set the temp lit according to the square's status
	    int		tile;

	    tile = getTile(x, y);
	    if (glb_squaredefs[tile].islit)
		setFlag(x, y, SQUAREFLAG_TEMPLIT);
	    else
		clearFlag(x, y, SQUAREFLAG_TEMPLIT);
	}
    }

    // Now run through our object & mob list looking for lights...
    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	if (mob->isLight())
	{
	    applyTempLight(mob->getX(), mob->getY(), mob->getLightRadius());
	}
    }

    for (item = getItemHead(); item; item = item->getNext())
    {
	if (item->isLight())
	{
	    applyTempLight(item->getX(), item->getY(), item->getLightRadius());
	}
    }
}

void
MAP::applyTempLight(int x, int y, int radius)
{
    applyFlag(SQUAREFLAG_TEMPLIT, x, y, radius, true, true);
}

void
MAP::applyFlag(SQUAREFLAG_NAMES flag, int cx, int cy, int radius, 
	       bool uselos, bool state)
{
    int		x, y;
    
    for (y = cy - radius; y <= cy + radius; y++)
    {
	if (y < 0 || y >= MAP_WIDTH)
	    continue;
	for (x = cx - radius; x <= cx + radius; x++)
	{
	    if (x < 0 || x >= MAP_WIDTH)
		continue;
	    if (!uselos || hasLOS(cx, cy, x, y))
	    {
		setFlag(x, y, flag, state);
	    }
	}
    }
}

bool
MAP::isSquareInLockedRoom(int x, int y) const
{
    int		nx, ny;
    bool	founddoor = false;

    // Note this room might be non square.
    // Thus we want do do a flood fill of flyable squares,
    // looking for an open door.
    bool	*checkedsquare;

    checkedsquare = new bool [MAP_HEIGHT * MAP_WIDTH];

    memset(checkedsquare, 0, MAP_HEIGHT * MAP_WIDTH);

    PTRLIST<int>	stack;

    stack.append(x + y * MAP_WIDTH);
    checkedsquare[x+y*MAP_WIDTH] = true;

    while (!founddoor && stack.entries())
    {
	int		idx = stack.pop(), idxtest;
	SQUARE_NAMES	tile;

	nx = idx & (MAP_WIDTH-1);
	ny = idx / MAP_HEIGHT;

	tile = getTile(nx, ny);

	if (!isTileWall(tile))
	{
	    // an empty door is a door.
	    if (tile == SQUARE_CORRIDOR)
		founddoor = true;

	    // Spread out in 4 directions.
	    if (nx)
	    {
		idxtest = idx-1;
		if (!checkedsquare[idxtest])
		{
		    checkedsquare[idxtest] = true;
		    stack.append(idxtest);
		}
	    }
	    if (nx < MAP_WIDTH-1)
	    {
		idxtest = idx+1;
		if (!checkedsquare[idxtest])
		{
		    checkedsquare[idxtest] = true;
		    stack.append(idxtest);
		}
	    }
	    if (ny)
	    {
		idxtest = idx-MAP_WIDTH;
		if (!checkedsquare[idxtest])
		{
		    checkedsquare[idxtest] = true;
		    stack.append(idxtest);
		}
	    }
	    if (ny < MAP_HEIGHT-1)
	    {
		idxtest = idx+MAP_WIDTH;
		if (!checkedsquare[idxtest])
		{
		    checkedsquare[idxtest] = true;
		    stack.append(idxtest);
		}
	    }
	}
	else
	{
	    // We stop here.  But if it is a door, we are done.
	    switch (tile)
	    {
		case SQUARE_DOOR:
		case SQUARE_BLOCKEDDOOR:
		case SQUARE_OPENDOOR:
		    founddoor = true;
		    break;
		default:
		    break;
	    }
	}
    }

    delete [] checkedsquare;
    return !founddoor;
}

bool
MAP::findTile(SQUARE_NAMES tile, int &ox, int &oy)
{
    int		x, y;

    // First, we find an instance of the given tile...
    for (y = 0; y < MAP_HEIGHT; y++)
    {
        for (x = 0; x < MAP_WIDTH; x++)
	{
	    if (getTile(x, y) == tile)
	    {
		// Success.
		ox = x;
		oy = y;
		return true;
	    }
	}
    }

    // Nothing worked, leave x & y unchanged and return false...
    return false;
}

bool
MAP::findCloseTile(int &x, int &y, MOVE_NAMES move, bool allowmob,
		    int maxradius)
{
    int		rad = 0;
    int		ox, oy, d;

    ox = x;
    oy = y;

    // Search in ever increasing circles until we get a hit.
    for (rad = 0; rad < maxradius; rad++)
    {
	for (d = -rad; d <= rad; d++)
	{
	    x = ox + rad;
	    y = oy + d;
	    if (x >= 0 && x < MAP_WIDTH &&
		y >= 0 && y < MAP_HEIGHT)
		if (canMove(x, y, move, allowmob))
		    return true;

	    x = ox - rad;
	    y = oy + d;
	    if (x >= 0 && x < MAP_WIDTH &&
		y >= 0 && y < MAP_HEIGHT)
		if (canMove(x, y, move, allowmob))
		    return true;

	    x = ox + d;
	    y = oy + rad;
	    if (x >= 0 && x < MAP_WIDTH &&
		y >= 0 && y < MAP_HEIGHT)
		if (canMove(x, y, move, allowmob))
		    return true;

	    x = ox + d;
	    y = oy - rad;
	    if (x >= 0 && x < MAP_WIDTH &&
		y >= 0 && y < MAP_HEIGHT)
		if (canMove(x, y, move, allowmob))
		    return true;
	}
    }
    
    // Nothing found, fail.
    x = ox;
    y = oy;
    return false;
}

MAP *
MAP::peekMapUp() const
{
    return myMapUp;
}

MAP *
MAP::getMapUp(bool create)
{
    if (myMapUp)
	return myMapUp;
    
    if (!create)
	return 0;

    if (myDepth == 0)
	return 0;			// Nothing up there!

    myMapUp = new MAP(myDepth - 1, branchName());
    
    myMapUp->setMapDown(this);

    myMapUp->build(false);
    myMapUp->populate();

    return myMapUp;
}

MAP *
MAP::peekMapDown() const
{
    return myMapDown;
}

MAP *
MAP::getMapDown(bool create)
{
    if (myMapDown)
	return myMapDown;

    if (!create)
	return 0;

    myMapDown = new MAP(myDepth + 1, branchName());

    myMapDown->setMapUp(this);
    
    myMapDown->build(false);
    myMapDown->populate();

    return myMapDown;
}

MAP *
MAP::peekMapBranch() const
{
    return myMapBranch;
}

BRANCH_NAMES
MAP::offshootName() const
{
    // THis is way more complicated than it should be.
    const int		tridudedepth = 20;
    switch (branchName())
    {
	case BRANCH_MAIN:
	    if (getDepth() == tridudedepth)
		return BRANCH_TRIDUDE;
	case BRANCH_TRIDUDE:
	    if (getDepth() == tridudedepth)
		return BRANCH_MAIN;
	    break;

	case BRANCH_NONE:
	case NUM_BRANCHS:
	    UT_ASSERT(!"Invalid source branch name");
	    break;
    }
    return BRANCH_NONE;
}

BRANCH_NAMES
MAP::branchName() const
{
    return (BRANCH_NAMES) myBranch;
}

MAP *
MAP::getMapBranch(bool create)
{
    if (myMapBranch)
	return myMapBranch;

    if (!create)
	return 0;

    BRANCH_NAMES		branch;

    branch = offshootName();
    if (branch == BRANCH_NONE)
    {
	return 0;
    }

    myMapBranch = new MAP(getDepth(), branch);

    myMapBranch->setMapBranch(this);
    
    myMapBranch->build(false);
    myMapBranch->populate();

    return myMapBranch;
}
    

bool
MAP::describeSquare(int x, int y, bool blind, bool includeself)
{
    bool		describe, started;
    BUF			buf;
    ITEMSTACK		stack;
    MOB			*mob;
    const char		*prefix;
    const char		*here;
    bool		ownsquare = false;
    bool		checksubmerged;
    int			tile;
    int			i, n;

    if (x == MOB::getAvatar()->getX() && y == MOB::getAvatar()->getY())
    {
	here = " here.  ";
	// We also want to explicitly map this square.  Mapping may
	// occur after walking, so this avoids getting a know nothing
	// if you teleport, etc.
	markMapped(x, y, 0, 0, false);	
	ownsquare = true;
    }
    else
	here = " there.  ";

    checksubmerged = false;
    if (ownsquare)
	if (MOB::getAvatar()->hasIntrinsic(INTRINSIC_INPIT))
	    checksubmerged = true;

    if (MOB::getAvatar()->hasIntrinsic(INTRINSIC_SUBMERGED))
	checksubmerged = true;

    describe = false;
    started = false;

    // MUST check mobs first as they don't chain!

    // Check all MOBs.  Do not report the avatar as we are the one doing
    // the describing..
    mob = getMob(x, y);
    if (mob && !mob->isAvatar() && MOB::getAvatar()->canSense(mob))
    {
	buf.strcat(mob->getDescription());
	describe = false;
	started = true;
    }
    else if (includeself && mob && mob->isAvatar())
    {
	if (blind || 
		(mob->hasIntrinsic(INTRINSIC_INVISIBLE) && 
		 !mob->hasIntrinsic(INTRINSIC_SEEINVISIBLE)))
	    buf.strcat("You feel yourself");
	else
	    buf.strcat("You see yourself");
	describe = true;
	started = true;
    }

    // We do not care if the items are mapped if they are on our own
    // square.  Indeed, the act of stepping on them will trigger them
    // to be mapped.
    getItemStack(stack, x, y, checksubmerged, !ownsquare);
    n = stack.entries();

    if (ownsquare)
    {
	// Map all the items.
	for (i = 0; i < n; i++)
	{
	    stack(i)->markMapped();
	}
    }

    // Print out all the items.  We cap at 5 items and just
    // report many items.
    for (i = 0; i < n; i++)
    {
	if (!describe)
	{
	    if (!ownsquare && (!hasFOV(x, y) || !isLit(x, y) || blind))
	    {
		// We are working from memory.
		buf.strcat("You recall ");
	    }
	    else if (blind)
	    {
		buf.strcat("You feel ");
	    }
	    else
	    {
		buf.strcat("You see ");
	    }
	}
	else
	{
	    buf.strcat(", ");
	}
	describe = true;

	if (n > 5)
	{
	    buf.strcat("many items");
	    break;
	}
	buf.strcat(stack(i)->getName());
    }

    // Check if the square is mapped...
    // If not, the only thing we could have is the MOB report or
    // any known items.
    if (!getFlag(x, y, SQUAREFLAG_MAPPED))
    {
	if (!describe)
	{
	    if (!started)
		buf.sprintf("You know nothing of this spot.  ");
	}
	else
	{
	    if (blind)
	    {
		buf.strcat(here);
	    }
	    else
		buf.strcat(".  ");
	}
	    
	msg_report(buf);
	return true;
    }
    
    // Check for special tiles...
    if (!describe)
    {
	if (!ownsquare && (!hasFOV(x, y) || !isLit(x, y) || blind))
	    prefix = "You recall ";
	else if (blind)
	    prefix = "You feel ";
	else
	    prefix = "You see ";
    }
    else
	prefix = ", ";

    SMOKE_NAMES	smoke = getSmoke(x, y);

    if (smoke != SMOKE_NONE)
    {
	buf.strcat(prefix);
	buf.strcat(glb_smokedefs[smoke].name);
	prefix = " and ";
	describe = true;
    }

    tile = getTile(x, y);
    if (glb_squaredefs[tile].description)
    {
	buf.strcat(prefix);
	buf.strcat(glb_squaredefs[tile].description);
	describe = true;
    }

    if (describe)
    {
	if (blind)
	    buf.strcat(here);
	else
	    buf.strcat(".  ");
	msg_report(buf);
    }
    else if (started)
    {
	msg_report(buf);
    }

    return describe || started;
}

bool
MAP::canMove(int x, int y, MOVE_NAMES movetype, 
	     bool allowmob, bool allowdoor, bool allowitem)
{
    SQUARE_NAMES		square;

    // Cannot move outside of the map.
    if (x < 0 || x >= MAP_WIDTH ||
	y < 0 || y >= MAP_HEIGHT)
	return false;

    if (!allowmob)
    {
	if (getMob(x, y))
	    return false;
    }

    if (!allowitem)
    {
	ITEM		*item;

	item = getItem(x, y);
	if (item && !item->isPassable())
	    return false;
    }
    
    square = getTile(x, y);

    if (allowdoor)
    {
	if (square == SQUARE_DOOR)
	    return true;
    }
    if (!(glb_squaredefs[getTile(x, y)].movetype & movetype))
    {
	return false;
    }
    return true;
}

bool
MAP::canRayMove(int x, int y, MOVE_NAMES movetype, bool reflect,
		bool *didreflect)
{
    MOB		*mob;

    if (didreflect)
	*didreflect = false;

    // All rays terminate at map boundaries.
    if (x < 0 || x >= MAP_WIDTH)
	return false;
    if (y < 0 || y >= MAP_HEIGHT)
	return false;
    
    if (!(glb_squaredefs[getTile(x, y)].movetype & movetype))
    {
	return false;
    }

    // Non-reflecting beams ignore reflection.
    if (!reflect)
	return true;

    // Check for mobs with reflection...
    mob = getMob(x, y);
    if (mob)
    {
	if (mob->hasIntrinsic(INTRINSIC_REFLECTION))
	{
	    if (didreflect)
		*didreflect = true;
	    return false;
	}
    }
    return true;
}

void		 
MAP::throwItem(int x, int y, int dx, int dy,
		ITEM *missile, ITEM *stack, ITEM *launcher, 
		MOB *thrower,
		const ATTACK_DEF *attack,
		bool ricochet)
{
    int 		 cansee;
    bool		 didhit;
    MOB			*mob;
    int			 range;
    int			 oldoverlay;
    BUF			 buf;

    x += dx;
    y += dy;

    range = attack->range;
    oldoverlay = -1;
    
    didhit = false;
    while (canMove(x, y, MOVE_STD_FLY) && range > 0)
    {
	cansee = glbCurLevel->getFlag(x, y, SQUAREFLAG_FOV);
	
	// Erase old overlay...
	if (oldoverlay >= 0)
	{
	    gfx_setoverlay(x - dx, y - dy, oldoverlay);
	}
	// And put the item in as the new overlay.
	if (cansee)
	{
	    oldoverlay = gfx_getoverlay(x, y);
	    gfx_setoverlay(x, y, missile->getTile());
	}
	else
	    oldoverlay = -1;

	if (cansee)
	{
	    gfx_sleep(6);
	}

	// Determine if we have a hit...
	mob = getMob(x, y);

	if (mob)
	{
	    mob->receiveAttack(attack, thrower, missile,
				launcher,
				ATTACKSTYLE_THROWN, &didhit);

	    if (didhit)
	    {
		range = 0;
		break;
	    }
	}
	
	range--;
	if (range)
	{
	    // Check for ricochet
	    if (ricochet && !canMove(x+dx, y+dy, MOVE_STD_FLY))
	    {
		bool	didbounce = false;
		int	ox = x, oy = y;

		// Diagonal test, only obvious bounce.
		if (dx && dy)
		{
		    if (canMove(x+dx,y,MOVE_STD_FLY)
			&& !canMove(x,y+dy,MOVE_STD_FLY))
		    {
			y += dy;
			dy = -dy;
			didbounce = true;
		    }
		    else if (!canMove(x+dx,y,MOVE_STD_FLY)
			&& canMove(x,y+dy,MOVE_STD_FLY))
		    {
			x += dx;
			dx = -dx;
			didbounce = true;
		    }
		}
		else if (dx)
		{
		    // Horizontal, bounce into diagonal provided
		    // a path open.
		    if (canMove(x, y+1, MOVE_STD_FLY))
		    {
			didbounce = true;
			dy = 1;
		    }
		    if (canMove(x, y-1, MOVE_STD_FLY) && (!didbounce || rand_choice(2)))
		    {
			didbounce = true;
			dy = -1;
		    }
		    
		    if (didbounce)
		    {
			x += dx;
			dx = -dx;
		    }
		}
		else if (dy)
		{
		    // Vertical, bounce into diagonal provided
		    // a path open.
		    if (canMove(x+1, y, MOVE_STD_FLY))
		    {
			didbounce = true;
			dx = 1;
		    }
		    if (canMove(x-1, y, MOVE_STD_FLY) && (!didbounce || rand_choice(2)))
		    {
			didbounce = true;
			dx = -1;
		    }
		    
		    if (didbounce)
		    {
			y += dy;
			dy = -dy;
		    }
		}
		
		if (didbounce)
		{
		    buf = MOB::formatToString("%U <ricochet>.", 0, missile, 0, 0);
		    reportMessage(buf, x+dx, y+dy);

		    // Clear out the old position.
		    if (oldoverlay >= 0)
		    {
			gfx_setoverlay(ox, oy, oldoverlay);
			oldoverlay = -1;
		    }
		}
	    }
	    x += dx;
	    y += dy;
	}
    }

    if (range)
    {
	// Stopped short, undo last step so we don't leave it in a wall.
	x -= dx;
	y -= dy;
    }
    
    // Clean up...  We always have our old position here...
    if (oldoverlay >= 0)
    {
	gfx_setoverlay(x, y, oldoverlay);
    }

    // Always get a break at the end of a throw.
    if (missile->doBreak(10, thrower, 0, this, x, y))
	return;

    // If we did not break it, there is a chance we ided it.
    if (didhit && thrower && thrower->isAvatar())
    {
	if (!missile->isKnownEnchant())
	{
	    int		skill, chance;

	    skill = thrower->getWeaponSkillLevel(missile, ATTACKSTYLE_THROWN);
	    chance = 5;
	    if (skill >= 1)
		chance += 5;
	    if (skill >= 2)
		chance += 10;
		
	    // About 20 hits to figure it out with 0 skill, 10 for
	    // one level skill, and 5 for 2 levels skill.
	    if (rand_chance(chance))
	    {
		missile->markEnchantKnown();
		if (stack)
		    stack->markEnchantKnown();
		buf = MOB::formatToString("%U <know> %r %Iu better.  ", thrower, 0, 0, missile);
		// This is important enough to be broadcast.
		msg_announce(gram_capitalize(buf));
	    }
	}
    }

    // Drop the item on the ground.
    acquireItem(missile, x, y, thrower);
}


void
MAP::fireRay(int ox, int oy, int dx, int dy, int length,
	     MOVE_NAMES movetype, bool dobounce, 
	     bool raycallback(int x, int y, bool final, void *data),
	     void *data)
{
    if (length <= 0)
    {
	if (raycallback)
	    raycallback(ox, oy, true, data);
	return;
    }
    
    // Technically a bool, but deprecated for
    // performance.
    int		cansee;
    bool	validreport = true;
    bool	didreflect;
    
    UT_ASSERT(dx || dy);
    if (!(dx || dy))
    {
	if (raycallback)
	    raycallback(ox, oy, true, data);
	return;
    }

    // See if we can pass...
    int		x, y;

    x = ox + dx;
    y = oy + dy;
    
    // Determine if we can see the new ray square.
    // This could be out of bounds so adjust appropriately.
    if (x < 0 || x >= MAP_WIDTH ||
	y < 0 || y >= MAP_HEIGHT)
    {
	// Cansee is determined by old position.  This may still
	// be out of bounds.
	if (ox >= 0 && ox < MAP_WIDTH && oy >= 0 && oy < MAP_HEIGHT)
	{
	    cansee = false;
	    validreport = true;
	}
	else
	{
	    cansee = false;
	    validreport = false;
	}
    }
    else
    {
	cansee = getFlag(x, y, SQUAREFLAG_FOV);
	// We always report to ox/oy
	if (ox < 0 || ox >= MAP_WIDTH ||
	    oy < 0 || oy >= MAP_HEIGHT)
	{
	    validreport = false;
	}
	else
	    validreport = cansee;
    }

    if (!canRayMove(x, y, movetype, dobounce, &didreflect))
    {
	// Can't move in this direction.
	if (!dobounce)
	{
	    // Blow up in previous place!
	    if (raycallback)
		raycallback(ox, oy, true, data);
	    return;		// Ray is done.
	}

	if (validreport)
	    reportMessage("The ray bounces.", ox, oy);

	// If it reflected due to reflection, and we have an attacker,
	// make the appropriate note.
	// TODO: This isn't currently possible :<

	// If the ray is in one direction, we merely negate the direction
	// and resend from the new position.  This will then rehit
	// ox & oy.  We decrement length here, resulting in the
	// bounce costing one hit.
	if (!dx || !dy)
	{
	    // Report the message..
	    fireRay(x, y, -dx, -dy, length-1,
		    movetype, dobounce, raycallback, data);
	    return;
	}

	// We have a complicated bounce...
	bool		canx, cany;

	canx = canRayMove(ox+dx, oy, movetype, dobounce);
	cany = canRayMove(ox, oy+dy, movetype, dobounce);

	if ((canx && cany) || (!canx && !cany))
	{
	    // We have a corner, inside or outside, so bounce straight
	    // back.
	    fireRay(x, y, -dx, -dy, length-1,
		    movetype, dobounce, raycallback, data);
	    return;
	}

	// We can have a bounce.  We want to move into the square we
	// can move to.  Thus, if it is canx, we'd like to go to the
	// new y pos, with the old x, reversing dy.  Complicated?  Yep.
	if (canx)
	    fireRay(ox, y, dx, -dy, length-1,
		    movetype, dobounce, raycallback, data);
	else	// MUST be cany...
	    fireRay(x, oy, -dx, dy, length-1,
		    movetype, dobounce, raycallback, data);	

	// Did something, so return...
	return;
    }

    // We are not blocked!  Stash the old overlay, drop in the ray,
    // call the callback, and pause.  Then progress...
    int			oldoverlay = TILE_VOID;

    if (cansee)
    {
	oldoverlay = gfx_getoverlay(x, y);

	if (!dx)
	    gfx_setoverlay(x, y, TILE_RAYPIPE);
	else if (!dy)
	    gfx_setoverlay(x, y, TILE_RAYDASH);
	else if (dx * dy > 0)
	    gfx_setoverlay(x, y, TILE_RAYBACKSLASH);
	else
	    gfx_setoverlay(x, y, TILE_RAYSLASH);
    }

    if (raycallback)
    {
	if (!raycallback(x, y, false, data))
	{
	    // Ray requested to end at this person.
	    return;
	}
    }

    // Only sleep if the avatar can see the ray!
    if (cansee)
        gfx_sleep(6);

    // Fire to the next level...
    fireRay(x, y, dx, dy, length-1,
	    movetype, dobounce, raycallback, data);
    
    // Restore the screen...
    if (cansee)
	gfx_setoverlay(x, y, oldoverlay);
}

void
MAP::fireBall(int ox, int oy, int rad, bool includeself,
		bool ballcallback(int x, int y, bool final, void *data),
		void *data)
{
    int		 dx, dy, diam, x, y;
    int		*oldoverlay;
    bool	 canbeseen = false;

    UT_ASSERT(rad >= 0);

    diam = rad * 2 + 1;
    oldoverlay = new int[diam * diam];

    // Draw the overlay...
    for (dx = -rad; dx <= rad; dx++)
    {
	x = ox + dx;
	if (x < 0) continue;
	if (x >= MAP_WIDTH) continue;
	for (dy = -rad; dy <= rad; dy++)
	{
	    y = oy + dy;
	    if (y < 0) continue;
	    if (y >= MAP_HEIGHT) continue;

	    oldoverlay[dx + rad + (dy + rad) * diam] = gfx_getoverlay(x, y);

	    // Only draw the ball effect if we can see it.
	    if (!hasFOV(x, y))
		continue;

	    if (!dx && !dy)
		continue;

	    canbeseen = true;

	    if (!dx)
		gfx_setoverlay(x, y, TILE_RAYDASH);
	    else if (!dy)
		gfx_setoverlay(x, y, TILE_RAYPIPE);
	    else if (dx * dy > 0)
		gfx_setoverlay(x, y, TILE_RAYSLASH);
	    else
		gfx_setoverlay(x, y, TILE_RAYBACKSLASH);
	}
    }

    // Apply the effect.
    for (dx = -rad; dx <= rad; dx++)
    {
	x = ox + dx;
	if (x < 0) continue;
	if (x >= MAP_WIDTH) continue;
	for (dy = -rad; dy <= rad; dy++)
	{
	    y = oy + dy;
	    if (y < 0) continue;
	    if (y >= MAP_HEIGHT) continue;

	    if (!includeself && !dx && !dy)
		continue;

	    if (ballcallback)
		ballcallback(x, y, false, data);
	}
    }
    
    // Sleep for the effect to be visible.
    if (canbeseen)
	gfx_sleep(15);
    
    // Restore the overlay.
    for (dx = -rad; dx <= rad; dx++)
    {
	x = ox + dx;
	if (x < 0) continue;
	if (x >= MAP_WIDTH) continue;
	for (dy = -rad; dy <= rad; dy++)
	{
	    y = oy + dy;
	    if (y < 0) continue;
	    if (y >= MAP_HEIGHT) continue;

	    if (!dx && !dy)
		continue;

	    gfx_setoverlay(x, y, oldoverlay[dx + rad + (dy + rad) * diam]);
	}
    }

    delete [] oldoverlay;
}

bool
MAP::knockbackMob(int tx, int ty, int dx, int dy, bool infinitemass,
		    bool ignoremob)
{
    // We now want to knock back the critter.  This
    // may also knock back ourselves if we are smaller.
    // Note both myself and the critter may be dead now.
    MOB		*caster, *target;

    caster = glbCurLevel->getMob(tx - dx, ty - dy);
    target = glbCurLevel->getMob(tx, ty);

    if (infinitemass)
	caster = 0;

    // If we have both a caster and a target, knockback
    // is possible.  If either died, or they are the same
    // person (think Baazl'bub here) no knock back occurs.
    if ((infinitemass && target) ||
	(caster && target && (caster != target)))
    {
	MOB			*knockee;
	MOVE_NAMES		 move;

	// Determine who flies back and in what direction.
	// The larger critter stays put.  In a tie,
	// it is random.
	// However, if only one creature can
	// knockback, that creature moves,

	// Determine if target can fly back.
	move = target->getMoveType();
	// We want to allow knocking into water because
	// we are very cruel.
	move = (MOVE_NAMES) (move | MOVE_SWIM);
	
	if (!glbCurLevel->canMove(tx+dx, ty+dy, move, 
				 ignoremob, false, false))
	{
	    target = 0;
	}

	if (caster)
	{
	    // Determine if caster can knock back
	    move = caster->getMoveType();
	    // We want to allow knocking into water because
	    // we are very cruel.
	    move = (MOVE_NAMES) (move | MOVE_SWIM);
	    
	    if (!glbCurLevel->canMove(tx-2*dx, ty-2*dy, move, 
				     ignoremob, false, false))
	    {
		caster = 0;
	    }
	}
	
	// Choose which one will knock back, ie, the
	// knockee.
	if (target &&
	    (!caster ||
	     (caster->getSize() > target->getSize()) ||
	     (caster->getSize() == target->getSize() 
		    && rand_choice(2)))
	   )
	{
	    // The target is sent flying in dx/dy.
	    knockee = target;
	}
	else
	{
	    // The caster is sent backwards.
	    knockee = caster;
	    dx = -dx;
	    dy = -dy;
	}

	// If knockee was false, no knockback is
	// possible
	if (knockee)
	{
	    // A valid move!  Yay!
	    tx = knockee->getX() + dx;
	    ty = knockee->getY() + dy;

	    knockee->formatAndReport("%U <be> knocked back.");

	    knockee->move(tx, ty);

	    return true;
	}
    }

    // Failed knockback
    return false;
}

void
MAP::saveGlobal(SRAMSTREAM &os)
{
    int		x, y;

    os.write(ourWindCounter, 32);
    if (ourWindCounter)
    {
	os.write(ourWindDX, 8);
	os.write(ourWindDY, 8);
    }

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    ourMobGuiltyMap[y*MAP_WIDTH+x].save(os);
	}
    }
}

void
MAP::loadGlobal(SRAMSTREAM &is)
{
    int		val;
    int		x, y;

    is.read(val, 32);
    ourWindCounter = val;
    if (ourWindCounter)
    {
	is.read(val, 8);
	ourWindDX = val;
	is.read(val, 8);
	ourWindDY = val;
    }

    // We may be loading globals before we load any particular map.
    if (!ourMobGuiltyMap)
	ourMobGuiltyMap = new MOBREF[MAP_WIDTH * MAP_HEIGHT];
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    ourMobGuiltyMap[y*MAP_WIDTH+x].load(is);
	}
    }
}

void
MAP::loadBranch(SRAMSTREAM &is, bool checkbranch)
{
    MAP		*map;
    int		 cont;

    // Read ourself.
    load(is, checkbranch);

    // Read parents.
    map = this;
    while (1)
    {
	is.uread(cont, 8);
	if (cont)
	    break;
	map->setMapUp(new MAP(0, BRANCH_NONE));
	map->getMapUp()->setMapDown(map);
	map = map->getMapUp(false);

	map->load(is);
    }

    // And the children
    map = this;
    while (1)
    {
	is.uread(cont, 8);
	if (cont)
	    break;
	map->setMapDown(new MAP(0, BRANCH_NONE));
	map->getMapDown()->setMapUp(map);
	map = map->getMapDown(false);
	map->load(is);
    }
}

void
MAP::saveBranch(SRAMSTREAM &os, bool savebranch)
{
    MAP		*map;

    save(os, savebranch);
    
    for (map = getMapUp(false); map; map = map->getMapUp(false))
    {
	os.write(0, 8);
	map->save(os);
    }

    os.write(1, 8);
    for (map = getMapDown(false); map; map = map->getMapDown(false))
    {
	os.write(0, 8);
	map->save(os);
    }

    os.write(1, 8);
}

bool
MAP::save(SRAMSTREAM &os, bool savebranch)
{
    int		x, y, tile;
    ITEM	*item;
    MOB		*mob;
    MAP		*comp;

    // While we aren't exactly loading, we don't want the build to
    // trigger creation.
    ourIsLoading = true;

    // Save our private data...
    os.write(myDepth, 8);
    os.write(mySeed, 32);
    os.write(myGlobalFlags, 8);
    os.write(myBranch, 8);

    mySmokeStack.save(os);

    comp = new MAP(myDepth, branchName());
    rand_setseed(mySeed);
    comp->build(true);

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    tile = getTile(x, y);
	    tile ^= comp->getTile(x, y);
	    os.write(tile, 8);
	}
    }
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    tile = getRawFlag(x, y);
	    // Clear out temporary flags that we don't care about,
	    // as we will be rebuilding them anyways.
	    tile &= ~ (SQUAREFLAG_TEMPLIT | SQUAREFLAG_FOV);
	    tile ^= comp->getRawFlag(x, y);
	    os.write(tile, 8);
	}
    }

    // Write out our signposts.  While we could likely compress well
    // against the comparison map, the only signpost heavy level
    // is the tutorial we don't care about compressing.
    if (mySignList)
	mySignList->save(os);
    else
	os.write(0, 8);

    delete comp;

    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	// Write that there is a mob...
	os.write(0, 8);
	// And the mob data...
	mob->save(os);
    }
    // Write no more mobs...
    os.write(1, 8);

    for (item = getItemHead(); item; item = item->getNext())
    {
	// Write that there is an item...
	os.write(0, 8);
	// And the item data
	item->save(os);
    }
    // Write no more items...
    os.write(1, 8);

    ourIsLoading = false;

    // Now, once we are done, check for a branch.
    if (savebranch)
    {
	if (getMapBranch(false))
	{
	    MAP		*map = getMapBranch(false);
	    os.write(1, 8);

	    map->saveBranch(os, false);
	}
	else
	{
	    os.write(0, 8);
	}
    }

    return true;
}

bool
MAP::load(SRAMSTREAM &is, bool checkbranch)
{
    int		x, y, tile, val;

    ourIsLoading = true;

    // Load private data...
    is.read(val, 8);
    myDepth = val;
    is.uread(val, 32);
    mySeed = val;
    is.uread(val, 8);
    myGlobalFlags = val;
    is.uread(val, 8);
    myBranch = val;

    mySmokeStack.load(is);

    // Rebuild according to our settings...
    rand_setseed(mySeed);
    build(true);

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    is.uread(tile, 8);
	    setTile(x, y, (SQUARE_NAMES) (tile ^ getTile(x, y)));
	}
    }
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    is.uread(tile, 8);
	    setRawFlag(x, y, tile ^ getRawFlag(x, y));
	}
    }

    // Load the signposts.
    delete mySignList;
    mySignList = new SIGNLIST;
    mySignList->load(is);
    if (!mySignList->entries())
    {
	// Don't bother to keep empty signpost lists.
	delete mySignList;
	mySignList = 0;
    }

    // Load mobs...
    MOB		*mob;
    int		 cont;
    while (1)
    {
	is.uread(cont, 8);
	if (cont)
	    break;
	mob = MOB::load(is);
	registerMob(mob);
    }

    // Load items...
    ITEM	*item;
    while (1)
    {
	is.uread(cont, 8);
	if (cont)
	    break;
	item = ITEM::load(is);
	acquireItem(item, item->getX(), item->getY(), 0, 0, true);
    }
    
    ourIsLoading = false;

    // Conservative...
    ourMapDirty = true;

    // Having finished loading ourself, we check for a branch
    // and load that.
    if (checkbranch)
    {
	is.uread(val, 8);
	if (val)
	{
	    MAP		*branch;

	    branch = new MAP(0, BRANCH_NONE);
	    setMapBranch(branch);
	    branch->setMapBranch(this);
	    branch->loadBranch(is, false);
	}
    }
    
    return true;
}

bool
MAP::verifyMobLocal() const
{
    MOB		*mob;
    ITEM	*item;
    bool	 valid = true;

    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	valid &= mob->verifyMob();
    }

    for (item = getItemHead(); item; item = item->getNext())
    {
	valid &= item->verifyMob();
    }

    return valid;
}

bool
MAP::verifyMob() const
{
    // Recurse in both directions, up and down.
    const MAP		*map;
    bool	 valid = true;

    for (map = this; map; map = map->peekMapUp())
    {
	valid &= map->verifyMobLocal();
    }
    for (map = peekMapDown(); map; map = map->peekMapDown())
    {
	valid &= map->verifyMobLocal();
    }

    return valid;
}

bool
MAP::verifyCounterGoneLocal(INTRINSIC_COUNTER *counter) const
{
    MOB		*mob;
    ITEM	*item;
    bool	 valid = true;

    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	valid &= mob->verifyCounterGone(counter);
    }

    for (item = getItemHead(); item; item = item->getNext())
    {
	valid &= item->verifyCounterGone(counter);
    }

    return valid;
}

bool
MAP::verifyCounterGone(INTRINSIC_COUNTER *counter) const
{
    // Recurse in both directions, up and down.
    const MAP		*map;
    bool	 valid = true;

    for (map = this; map; map = map->peekMapUp())
    {
	valid &= map->verifyCounterGoneLocal(counter);
    }
    for (map = peekMapDown(); map; map = map->peekMapDown())
    {
	valid &= map->verifyCounterGoneLocal(counter);
    }

    return valid;
}

void
MAP::digHole(int x, int y, MOB *digger)
{
    SQUARE_NAMES		tile, newtile;
	
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // If we don't allow digging in this level, prohibit it.
    if (!allowDiggingDown())
    {
	if (digger)
	{
	    digger->reportMessage("The ground on this level is to hard to dig deep!");
	}
	digPit(x, y, digger);
	return;
    }

    // Determine type at this square...
    tile = getTile(x, y);
    switch (tile)
    {
	case SQUARE_FLOOR:
	case SQUARE_FLOORPIT:
	    newtile = SQUARE_FLOORHOLE;
	    break;
	case SQUARE_CORRIDOR:
	case SQUARE_PATHPIT:
	    newtile = SQUARE_PATHHOLE;
	    break;
	case SQUARE_FLOORHOLE:
	case SQUARE_PATHHOLE:
	{
	    // Already a hole here!
	    if (digger)
	    {
		digger->formatAndReport("%U slightly <enlarge> the existing hole.");
	    }
	    return;
	}
	default:
	    newtile = tile;
	    break;
    }

    if (newtile != tile)
    {
	// You actuallly dig...
	setTile(x, y, newtile);
	setGuiltyMob(x, y, digger);
	// drop the items before falling to the new level, in case avatar is
	// the digger
	dropItems(x, y, digger);
	if (digger)
	{
	    digger->formatAndReport("%U <dig> a hole in the floor.");

	    // See if the digger should fall through...
	    MAP		*nextmap;
	    int		 nx, ny;
	    nextmap = getMapDown();

	    if (nextmap &&
		nextmap->isUnlocked() &&
		nextmap->findRandomLoc(nx, ny, digger->getMoveType(), false,
				    digger->getSize() >= SIZE_GARGANTUAN,
				    false, false, false, true))
	    {
		digger->formatAndReport("%U <fall> through the hole in the ground!");

		unregisterMob(digger);

		if (!digger->move(nx, ny, true))
		    return;
	    
		nextmap->registerMob(digger);

		if (digger->isAvatar())
		{
		    changeCurrentLevel(nextmap);
		}
	    }
	    else
	    {
		digger->formatAndReport("%U <find> the bottom of the hole blocked.");
	    }
	}

    }
    else
    {
	// Can't dig here.
	if (digger)
	{
	    digger->formatAndReport("%U <dig> in vain.");
	}
    }
}

void
MAP::digPit(int x, int y, MOB *digger)
{
    SQUARE_NAMES		tile, newtile;
	
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // Determine type at this square...
    tile = getTile(x, y);
    switch (tile)
    {
	case SQUARE_FLOOR:
	    newtile = SQUARE_FLOORPIT;
	    break;
	case SQUARE_CORRIDOR:
	case SQUARE_GRASS:
	    newtile = SQUARE_PATHPIT;
	    break;
	case SQUARE_FLOORPIT:
	case SQUARE_PATHPIT:
	{
	    // Already a hole here!
	    setGuiltyMob(x, y, digger);
	    if (digger)
	    {
		digger->formatAndReport("%U slightly <enlarge> the existing pit.");

		// If we are in the pit, we sink with it.
		if (digger->getX() == x && digger->getY() == y)
		{
		    digger->setIntrinsic(INTRINSIC_INPIT);
		}

		// Anyone on the square will fall down again due to
		// the widening...
		dropMobs(x, y, true, digger);
	    }
	    return;
	}
	default:
	    newtile = tile;
	    break;
    }

    if (newtile != tile)
    {
	// You actuallly dig...
	setTile(x, y, newtile);
	setGuiltyMob(x, y, digger);
	if (digger)
	{
	    digger->formatAndReport("%U <dig> a pit in the floor.");

	    if (digger->getX() == x && digger->getY() == y)
	    {
		digger->setIntrinsic(INTRINSIC_INPIT);
	    }
	}

	dropItems(x, y, digger);
	dropMobs(x, y, true, digger);
    }
    else
    {
	// Can't dig here.
	if (digger)
	{
	    digger->formatAndReport("%U <dig> in vain.");
	}
    }
}

bool		 
MAP::digSquare(int x, int y, MOB *digger)
{
    SQUARE_NAMES		tile;
	
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // Determine type at this square...
    tile = getTile(x, y);
    switch (tile)
    {
	case SQUARE_EMPTY:
	case SQUARE_WALL:
	{
	    setTile(x, y, SQUARE_CORRIDOR);
	    break;
	}
	
	case SQUARE_SECRETDOOR:
	{
	    reportMessage(
		    "Stone falls away, revealing a secret door.",
		    x, y);
	    setTile(x, y, SQUARE_DOOR);
	    return false;
	}

	case SQUARE_VIEWPORT:
	{
	    reportMessage("The glass wall shatters.", x, y);

	    setTile(x, y, SQUARE_BROKENVIEWPORT);

	    // Ends the bolt
	    return false;
	}

	case SQUARE_DOOR:
	case SQUARE_BLOCKEDDOOR:
	{
	    // Can't tunnel through these!
	    return false;
	}

	default:
	    break;
    }

    ITEM		*item;

    item = getItem(x, y);
    if (item && item->getDefinition() == ITEM_BOULDER)
    {
	item->formatAndReport("%U <disintegrate>.");
	// Destroy the item.
	dropItem(item);
	delete item;
    }

    // Can still keep tunneling.
    return true;
}

void
MAP::growForest(int x, int y, MOB *grower)
{
    SQUARE_NAMES		tile;
	
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // Determine type at this square...
    tile = getTile(x, y);
    if (glb_squaredefs[tile].invulnerable)
    {
	if (grower)
	    grower->formatAndReport("The trees cannot gain a roothold and quickly collapse.");
	return;
    }

    if (glb_squaredefs[tile].movetype == MOVE_SWIM)
    {
	if (grower)
	    grower->formatAndReport("The trees lack ground for their roots and quickly collapse.");
	return;
    }
    
    if (glb_squaredefs[tile].movetype != MOVE_WALK)
    {
	if (grower)
	    grower->formatAndReport("The trees lack room to grow and quickly collapse.");
	return;
    }

    // Check to see if it is already a forest
    if (tile == SQUARE_FOREST)
    {
	if (grower)
	    grower->formatAndReport("The forest appears a bit denser.");
	return;
    }

    if (tile == SQUARE_FORESTFIRE)
    {
	if (grower)
	    grower->formatAndReport("More fuel is added to the fire.");
	return;
    }
    
    // You actuallly grow...
    setTile(x, y, SQUARE_FOREST);
    setGuiltyMob(x, y, grower);
}

void
MAP::douseSquare(int x, int y, bool holy, MOB *douser)
{
    MOB		*victim = getMob(x, y);
    bool	 didanything = false;
    MOBREF	 dref;

    dref.setMob(douser);

    // Check to see if we upconvert smoke into acid.
    if (getSmoke(x, y) == SMOKE_ACID)
    {
	reportMessage("The water reacts violently with the acid.", x, y);
	setSmoke(x, y, SMOKE_STEAM, douser);
    }

    if (victim)
	victim->submergeItemEffects(SQUARE_WATER);

    // Wet everything on this square.
    {
	ITEMSTACK	items;
	int		i;

	getItemStack(items, x, y);
	for (i = 0; i < items.entries(); i++)
	{
	    if (items(i)->douse(dref.getMob()))
	    {
		dropItem(items(i));
		delete items(i);
	    }
	}
    }

    if (victim && victim->getDefinition() == MOB_FIREELEMENTAL)
    {
	victim->receiveDamage(ATTACK_DOUSEFIREELEMENTAL,
			dref.getMob(), 0, 0, ATTACKSTYLE_MISC);
	// Create steam.
	glbCurLevel->setSmoke(x, y, SMOKE_STEAM, dref.getMob());
	didanything = true;
    }

    if (!didanything && victim && victim->getMobType() == MOBTYPE_UNDEAD)
    {
	if (holy)
	{
	    // Holy water burns undead.
	    victim->receiveDamage(ATTACK_HOLYWATER,
			    dref.getMob(), 0, 0, ATTACKSTYLE_MISC);
	    didanything = true;
	}
    }

    // Put out fire.  Ben Shadwick actually tried this,
    // and failed.  Shame accrues with me!
    if (!didanything && victim && victim->hasIntrinsic(INTRINSIC_AFLAME))
    {
	victim->formatAndReport("The water puts out %R flames.");
	victim->clearIntrinsic(INTRINSIC_AFLAME);
	didanything = true;
    }

    // Hmm... Should water put out a forest fire?
    // If we decide not to, make sure we freezeSquare inside downPour
    if (glbCurLevel->getTile(x, y) == SQUARE_FORESTFIRE)
    {
	glbCurLevel->reportMessage("The water douses the forest fire.",
				    x, y);
	glbCurLevel->setTile(x, y, SQUARE_FOREST);
	setGuiltyMob(x, y, dref.getMob());
	glbCurLevel->setSmoke(x, y, SMOKE_STEAM, dref.getMob());
	didanything = true;
    }

    // If this was a lava square, steam will be generated.
    if (glbCurLevel->getTile(x, y) == SQUARE_LAVA)
    {
	glbCurLevel->reportMessage("The water vaporizes on contact with the lava.",
				    x, y);
	glbCurLevel->setSmoke(x, y, SMOKE_STEAM, dref.getMob());
	didanything = true;
    }

    // Soak the creature.
    if (victim && !didanything)
    {
	victim->formatAndReport("%U <be> soaked with water.");
    }
}

void
MAP::downPour(int x, int y, MOB *pourer)
{
    SQUARE_NAMES		 tile;
    BUF				 buf;
    MOBREF			 dref;
	
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    dref.setMob(pourer);

    // Directly apply water to the square, this will damage fire eles,
    // etc.
    douseSquare(x, y, false, dref.getMob());

    // Determine type at this square...
    tile = getTile(x, y);

    if (tile == SQUARE_LAVA)
    {
	// While we don't let normal water extinguish lava, this is
	// a lot of water.
	freezeSquare(x, y, dref.getMob());
    }

    switch ((TRAP_NAMES) glb_squaredefs[tile].trap)
    {
	case TRAP_TELEPORT:
	{
	    int		tx, ty;
	    
	    reportMessage("Water disappears into the teleporter.", x, y);
	    if (findRandomLoc(tx, ty, MOVE_STD_FLY,
			    true, false, true, true, true, false))
	    {
		// We prohibit landing on a teleporter as I'm paranoid
		// of infinite recursion
		if (glb_squaredefs[getTile(tx,ty)].trap != TRAP_TELEPORT)
		{
		    downPour(tx, ty, dref.getMob());
		}
	    }
	    break;
	}
	case TRAP_HOLE:
	    reportMessage("Water flows down the hole.", x, y);
	    // TODO downpour on next level!
	    break;
	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	    floodSquare(SQUARE_WATER, x, y, dref.getMob());
	    break;
	case TRAP_SMOKEVENT:
	case TRAP_POISONVENT:
	case TRAP_NONE:
	case NUM_TRAPS:
	    // Do nothing.
	    break;
    }

}

void
MAP::floodSquare(SQUARE_NAMES newtile, int x, int y, MOB *flooder)
{
    SQUARE_NAMES		 tile;
    MOB				*mob;
    BUF				 buf;
	
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // Determine type at this square...
    tile = getTile(x, y);

    if (newtile != tile)
    {
	const char		*desc;
	const char		*liquid = "UNKNOWN LIQUID";

	desc = glb_squaredefs[tile].description;
	if (!desc)
	    desc = "the floor";

	switch (newtile)
	{
	    case SQUARE_LAVA:
		liquid = "Lava";
		break;
	    case SQUARE_WATER:
		liquid = "Water";
		break;
	    case SQUARE_ACID:
		liquid = "Acid";
		break;
	    default:
		UT_ASSERT(!"Unknown liquid");
		break;
	}

	// Determine if the flood is successful.  It fails
	// if the current tile is invulnerable.
	if (glb_squaredefs[tile].invulnerable)
	{
	    newtile = tile;
	    buf.sprintf("%s floods %s, leaving it unchanged.", liquid, desc);
	}
	else
	{
	    // You actuallly flood...
	    setTile(x, y, newtile);
	    setGuiltyMob(x, y, flooder);
	    buf.sprintf("%s floods %s.", liquid, desc);
	}
	reportMessage(buf, x, y);
    }

    // Finish submerging.  We recheck newtile == tile
    // as it may have been made equal by a failed flood.
    if (newtile != tile)
    {
	// Ensure anyone with IN_PIT intrinsic gets SUBMERGED.
	mob = getMob(x, y);
	if (mob)
	{
	    if (mob->hasIntrinsic(INTRINSIC_INPIT))
	    {
		mob->clearIntrinsic(INTRINSIC_INPIT);
		mob->setIntrinsic(INTRINSIC_SUBMERGED);
	    }
	}

	// Apply drop mobs to anyone still standing.
	dropMobs(x, y, true, flooder, true);
	dropItems(x, y, flooder);
    }
}


void
MAP::dropItems(int x, int y, MOB *revealer)
{
    int		tile, trap;
    bool	drop = false;
    bool	dropintoliquid = false;
    bool	dropintolava = false;
    bool	dropintoacid = false;
    bool	dropintowater = false;
    bool	dropintohole = false;
    const char *dropname = "";
    ITEM       *item, *next;
    ITEM       *squarefiller = 0;
    BUF		buf;

    tile = getTile(x, y);
    trap = glb_squaredefs[tile].trap;
    switch ((TRAP_NAMES) trap)
    {
	case TRAP_HOLE:
	    drop = true;
	    dropintohole = true;
	    break;
	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	    drop = true;
	    dropname = glb_trapdefs[trap].name;
	    break;
	case TRAP_TELEPORT:
            // Random teleport is handled in acquireItem.
	    break;
	case TRAP_POISONVENT:
	case TRAP_SMOKEVENT:
	case TRAP_NONE:
	case NUM_TRAPS:
	    // Nothing to do.
	    break;
    }

    switch (tile)
    {
	case SQUARE_WATER:
	    drop = true;
	    dropname = "water";
	    dropintoliquid = true;
	    dropintowater = true;
	    break;

	case SQUARE_ACID:
	    drop = true;
	    dropname = "acid";
	    dropintoliquid = true;
	    dropintoacid = true;
	    break;

	case SQUARE_LAVA:
	    drop = true;
	    dropname = "lava";
	    dropintoliquid = true;
	    dropintolava = true;
	    break;
    }

    if (!drop)
	return;

    // Now, drop all the items here into the pit:
    for (item = getItemHead(); item; item = next)
    {
	next = item->getNext();
	if (item->getX() == x && item->getY() == y)
	{
	    // If we are dealing with acid, we should dissolve what
	    // we can.
	    if ((dropintoacid && item->dissolve()) ||
		(dropintolava && item->ignite()) ||
		(dropintowater && item->douse()))
	    {
		// Yes, this also axes waterwalk items as they dissolve
		// as they float in the acid!
		// Only exception would be flying items.
		dropItem(item);
		delete item;
		continue;
	    }
	    
	    // If the item is a boulder, mark that we should fill the square
	    // as well.
	    if (item->getDefinition() == ITEM_BOULDER)
		squarefiller = item;
	    else if (dropintohole && item->fallInHole(this, true, revealer))
	    {
		continue;
	    }

	    // If the item is submerged, you discover it.
	    // If not, it falls in.
	    if (item->isBelowGrade())
	    {
		if (dropintoliquid && item->hasIntrinsic(INTRINSIC_WATERWALK))
		{
		    item->formatAndReport("%U <float> to the surface.");
		    item->markBelowGrade(false);
		}
		else
		{
		    if (revealer)
			buf = MOB::formatToString("%U <discover> %IU!",
				    revealer, 0, 0, item);
		    else
			buf = MOB::formatToString("%U <be> revealed.",
					0, item, 0, 0);
		    reportMessage(buf, x, y);
		    // Check to see if we have to fill the item with
		    // new liquid
		    fillBottle(item, x, y);
		}
	    }
	    else
	    {
		// The item falls into the pit.
		// If it is going into water and the item waterwalks,
		// it floats.
		if (dropintoliquid && item->hasIntrinsic(INTRINSIC_WATERWALK))
		{
		    // Do nothing.
		}
		else
		{
		    buf = MOB::formatToString("%U <fall> into the %B1.",
				0, item, 0, 0,
				dropname, 0);
		    reportMessage(buf, x, y);
		    item->markBelowGrade(true);
		    // Check to see if we have to fill the item with
		    // new liquid
		    fillBottle(item, x, y);
		}
	    }
	}
    }

    if (squarefiller)
    {
	// Note the revealer also becomes guilty of filling.
	fillSquare(squarefiller, revealer);
    }
}

bool
MAP::fillSquare(ITEM *boulder, MOB *filler, ITEM **newitem)
{
    if (newitem)
	*newitem = boulder;

    // Check this is a fillable item.
    if (boulder->getDefinition() != ITEM_BOULDER)
	return false;

    // Check the square if fillable.
    SQUARE_NAMES		 tile, newtile;
    TRAP_NAMES			 trap;
    bool			 dofill = false;
    bool			 dropintoliquid = false;
    const char			*fillname = 0;
    BUF				 buf;
    MOB				*mob;

    tile = getTile(boulder->getX(), boulder->getY());
    newtile = tile;
    trap = (TRAP_NAMES) glb_squaredefs[tile].trap;

    switch (trap)
    {
	case TRAP_HOLE:
	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	    fillname = glb_trapdefs[trap].name;
	    newtile = (SQUARE_NAMES) glb_squaredefs[tile].disarmsquare;
	    dofill = true;
	    break;

	case TRAP_TELEPORT:
	    // Do nothing here, since the boulder would have already
	    // teleported in acquireItem, or this is an impossible situation
	    // for a teleport to exist in (called here during water->pit
	    // transformation, for example). I hope.
	    break;

	case TRAP_SMOKEVENT:
	case TRAP_POISONVENT:
	case TRAP_NONE:
	case NUM_TRAPS:
	    break;
    }

    switch (tile)
    {
	case SQUARE_WATER:
	    dofill = true;
	    fillname = "water";
	    dropintoliquid = true;
	    newtile = SQUARE_CORRIDOR;
	    break;
	case SQUARE_ACID:
	    dofill = true;
	    fillname = "acid";
	    dropintoliquid = true;
	    newtile = SQUARE_CORRIDOR;
	    break;
	case SQUARE_LAVA:
	    dofill = true;
	    fillname = "lava";
	    dropintoliquid = true;
	    newtile = SQUARE_CORRIDOR;
	    break;
	default:
	    break;
    }

    if (!dofill)
	return false;

    UT_ASSERT(fillname != 0);
    if (!fillname)
	fillname = "ERROR";

    // And now do the appropriate message.
    if (dropintoliquid)
	buf = MOB::formatToString("%U <splash> into the %B1, filling it.",
			0, boulder, 0, 0,
			fillname, 0);
    else
	buf = MOB::formatToString("%U <plug> the %B1.",
			0, boulder, 0, 0,
			fillname, 0);

    reportMessage(buf, boulder->getX(), boulder->getY());

    setGuiltyMob(boulder->getX(), boulder->getY(), filler);

    // Chnage the tile:
    setTile(boulder->getX(), boulder->getY(), newtile);

    // Transmute any buried warhammers.
    if (getItem(boulder->getX(), boulder->getY()))
    {
	ITEMSTACK	items;
	int		i;

	// Only fetch buried items...
	getItemStack(items, boulder->getX(), boulder->getY(), true);
	for (i = 0; i < items.entries(); i++)
	{
	    ITEM *item = items(i);
	    if (item->getDefinition() == ITEM_WARHAMMER)
	    {
		reportMessage("A yellow glow shines briefly through the ground.", boulder->getX(), boulder->getY());
		item->setDefinition(ITEM_EARTHHAMMER);

		// If the boulder is an artifact tranmute the hammer
		if (boulder->isArtifact() && !item->isArtifact())
		{
		    item->makeArtifact(boulder->getPersonalName());
		}

	    }
	}
    }

    // Ensure anyone with IN_PIT intrinsic gets SUBMERGED.
    mob = getMob(boulder->getX(), boulder->getY());
    if (mob)
    {
	if (mob->hasIntrinsic(INTRINSIC_INPIT))
	{
	    mob->clearIntrinsic(INTRINSIC_INPIT);
	    mob->setIntrinsic(INTRINSIC_SUBMERGED);
	}
    }
    
    // Now, we want to delete the boulder as it was be consumed.
    dropItem(boulder);
    delete boulder;

    if (newitem)
	*newitem = 0;

    return true;
}

bool
MAP::dropMobs(int x, int y, bool forcetrap, MOB *revealer, bool noskillevade)
{
    SQUARE_NAMES	 tile;
    TRAP_NAMES		 trap;
    BUF			 buf;
    MOB	       		*mob;

    // Assume only one mob per square.
    mob = getMob(x, y);
    if (!mob)
	return true;

    tile = getTile(x, y);
    trap = (TRAP_NAMES) glb_squaredefs[tile].trap;
    
    // Drop us out of trees if they cease to exist.
    if (!glb_squaredefs[tile].isforest && mob->hasIntrinsic(INTRINSIC_INTREE))
    {
	mob->formatAndReport("%U <fall> to the ground.");
	mob->clearIntrinsic(INTRINSIC_INTREE);
    }

    if (mob->getSize() >= SIZE_GARGANTUAN)
    {
	// Giant mobs can't fall.
	return true;
    }

    if (trap != TRAP_NONE &&
	!(glb_trapdefs[trap].moveevade & mob->getMoveType()))
    {
	// Note that rangers never trigger traps.  They are just
	// too cool looking.
	if (!mob->hasIntrinsic(INTRINSIC_DRESSED_RANGER) &&
	    (noskillevade || !mob->hasIntrinsic(INTRINSIC_SKILL_EVADETRAP)) &&
	     (forcetrap || rand_chance(glb_trapdefs[trap].triggerchance)))
	{
	    // Trigger the trap!
	    switch (trap)
	    {
		case TRAP_HOLE:
		{
		    MAP		*nextmap;
		    int		 x, y;

		    nextmap = getMapDown();
		    if (!nextmap || !nextmap->isUnlocked())
		    {
			mob->formatAndReport("%R fall <be> stopped by an invisible barrier.");
			break;
		    }

		    // See if there is room...
		    // We just go somewhere random (but moveable!)
		    if (!nextmap->findRandomLoc(x, y, mob->getMoveType(), false,
						mob->getSize() >= SIZE_GARGANTUAN,
						false, false, false, true))
		    {
			mob->formatAndReport("%U <find> the hold blocked.");
			break;
		    }

		    // We must report this message before unregistering
		    // or the avatar won't see things fall into holes.
		    mob->formatAndReport("%U <fall> into the hole.");

		    // Move to it...
		    unregisterMob(mob);
		    if (!mob->move(x, y, true))
			return false;
		    nextmap->registerMob(mob);

		    if (mob->isAvatar())
		    {
			MAP::changeCurrentLevel(nextmap);
		    }

		    // Any further falling should occur on the new level.
		    return true;
		}

		case TRAP_PIT:
		case TRAP_SPIKEDPIT:
		{
		    ATTACK_NAMES		attack;

		    // If we are already in the pit, not much to do!
		    if (mob->hasIntrinsic(INTRINSIC_INPIT))
			break;

		    attack = (ATTACK_NAMES) glb_trapdefs[trap].attack;
		    // Inform the user they fell into the pit.
		    buf = MOB::formatToString("%U <fall> into the %B1.",
				mob, 0, 0, 0,
				glb_trapdefs[trap].name, 0);
		    mob->reportMessage(buf);

		    // Set our in pit flag.
		    // We do this prior to damage so on death we
		    // get the proper intrinsics.
		    mob->setIntrinsic(INTRINSIC_INPIT);

		    MOB		*attacker;
		    attacker = getGuiltyMob(x, y);
		    if (!attacker)
			attacker = mob;

		    // Take the damage...
		    if (!mob->receiveDamage(attack, 
					    attacker, 
					    0, 0,
					    ATTACKSTYLE_MISC))
		    {
			return false;
		    }

		    break;
		}

		case TRAP_SMOKEVENT:
		case TRAP_POISONVENT:
		{
		    if (trap == TRAP_SMOKEVENT)
			reportMessage("Smoke billows from the vent.", x, y);
		    else
			reportMessage("Poison smoke billows from the vent.", x, y);
			
		    setSmoke(x, y, ((trap == TRAP_SMOKEVENT) 
					    ? SMOKE_SMOKE
					    : SMOKE_POISON), 
			    getGuiltyMob(x, y));
		    break;
		}

		case TRAP_TELEPORT:
		{
		    mob->formatAndReport("%U <trigger> a teleport trap.");
		    mob->actionTeleport();
		    // The mob is no longer at x,y so further
		    // drop invocation doesn't make sense
		    // In any case, it might have died as a result of
		    // the teleport.
		    return true;
		}
		
		case TRAP_NONE:
		case NUM_TRAPS:
		    UT_ASSERT(!"BAD CODE PATH WITH TRAPS");
		    break;
	    }
	}
	else
	{
	    // Evade the trap.
	    // Report that you evade it.
	    // If you are already in a pit, don't report.
	    // Note that this is incorrect for evading holes.  However,
	    // I hope you can't have the inpit flag set whilst on a hole.
	    if (!mob->hasIntrinsic(INTRINSIC_INPIT))
	    {
		buf = MOB::formatToString("%U <evade> the %B1.",
			mob, 0, 0, 0,
			glb_trapdefs[trap].name, 0);
		mob->reportMessage(buf);
	    }
	}
    }

    // Determine special swimming effects.
    switch (tile)
    {
	case SQUARE_LAVA:
	case SQUARE_WATER:
	case SQUARE_ACID:
	{
	    if (mob->hasIntrinsic(INTRINSIC_SUBMERGED))
	    {
		// Already underwater, nothing to do.
		break;
	    }

	    // These movetypes allow one to negate sinking.
	    if (mob->getMoveType() & (MOVE_FLY | MOVE_PHASE))
	    {
		break;
	    }

	    if (mob->hasIntrinsic(INTRINSIC_WATERWALK))
	    {
		// Water walkers can't sink.
		break;
	    }

	    // Chance of sinking!  10% chance in lava.
	    // 50% chance in water.
	    int		 sink_chance;
	    const char	*liquid;

	    switch (tile)
	    {
		case SQUARE_LAVA:
		    sink_chance = 10;
		    liquid = "lava";
		    break;
		case SQUARE_WATER:
		    sink_chance = 50;
		    liquid = "water";
		    break;
		case SQUARE_ACID:
		    sink_chance = 50;
		    liquid = "acid";
		    break;

		default:
		    UT_ASSERT(!"UNHANDLED SQUARE TYPE");
		    liquid = "INVALID LIQUID";
		    sink_chance = 0;
		    break;
	    }

	    if (!forcetrap && !rand_chance(sink_chance))
	    {
		// You manage to keep your head above the liquid.
		break;
	    }

	    buf = MOB::formatToString("%U <sink> below the %B1.",
			mob, 0, 0, 0,
			liquid, 0);
	    mob->reportMessage(buf);
	    mob->setIntrinsic(INTRINSIC_SUBMERGED);
	    break;
	}

	default:
	{
	    // You, by definition are no longer in water.  If stepped
	    // out, say so.
	    if (mob->hasIntrinsic(INTRINSIC_SUBMERGED))
	    {
		// We can't say we stepped out of the liquid as we may
		// have escaped from being underground!
		mob->formatAndReport("%U <reach> the surface.");
		mob->clearIntrinsic(INTRINSIC_SUBMERGED);
	    }
	    break;
	}
    }

    return true;
}

bool
MAP::createTrap(int x, int y, MOB *trapper)
{
    SQUARE_NAMES		tile, newtile;

    // If spot is outside of map, it always fails
    // (Zapping wands of create trap doesn't bounds check, nor
    // should it)
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
	return false;
    
    tile = getTile(x, y);
    newtile = tile;

    // Determine if it is a valid tile.
    if (tile == SQUARE_CORRIDOR || tile == SQUARE_FLOOR)
    {
	// Determine possible traps, which is modulated by
	// the dungeon level.
	TRAP_NAMES		traps[NUM_TRAPS];
	int			numtraps = 0, i;
	
	for (i = 0; i < NUM_TRAPS; i++)
	{
	    if (glb_trapdefs[i].level <= myDepth)
		traps[numtraps++] = (TRAP_NAMES) i;
	}

	UT_ASSERT(numtraps);
	if (!numtraps)
	    return false;

	// Switch on the possible trap types:
	switch (traps[rand_choice(numtraps)])
	{
	    case TRAP_SPIKEDPIT:
		if (tile == SQUARE_CORRIDOR)
		    newtile = SQUARE_SECRETPATHSPIKEDPIT;
		else
		    newtile = SQUARE_SECRETFLOORSPIKEDPIT;
		break;
	    case TRAP_HOLE:
		// If we prohibit holes, we fall through to 
		// pit traps.
		if (allowDiggingDown())
		{
		    if (tile == SQUARE_CORRIDOR)
			newtile = SQUARE_SECRETPATHHOLE;
		    else
			newtile = SQUARE_SECRETFLOORHOLE;
		    break;
		}
		// FALL THROUGH
	    case TRAP_PIT:
		if (tile == SQUARE_CORRIDOR)
		    newtile = SQUARE_SECRETPATHPIT;
		else
		    newtile = SQUARE_SECRETFLOORPIT;
		break;
	    case TRAP_SMOKEVENT:
		if (tile == SQUARE_CORRIDOR)
		    newtile = SQUARE_SECRETPATHSMOKEVENT;
		else
		    newtile = SQUARE_SECRETFLOORSMOKEVENT;
		break;
	    case TRAP_POISONVENT:
		if (tile == SQUARE_CORRIDOR)
		    newtile = SQUARE_SECRETPATHPOISONVENT;
		else
		    newtile = SQUARE_SECRETFLOORPOISONVENT;
		break;
	    case TRAP_TELEPORT:
		if (tile == SQUARE_CORRIDOR)
		    newtile = SQUARE_SECRETPATHTELEPORTER;
		else
		    newtile = SQUARE_SECRETFLOORTELEPORTER;
		break;

	    case TRAP_NONE:
	    case NUM_TRAPS:
		break;
	}
	// Done.
	if (newtile != tile)
	{
	    setTile(x, y, newtile);
	    setGuiltyMob(x, y, trapper);
	    if (trapper)
		trapper->pietyCreateTrap();
	    return true;
	}
    }
    return false;
}

bool
MAP::freezeSquare(int x, int y, MOB *freezer)
{
    SQUARE_NAMES	tile, newtile;

    tile = getTile(x, y);

    switch (tile)
    {
	case SQUARE_WATER:
	    reportMessage("The water freezes.", x, y);
	    newtile = SQUARE_ICE;

	    // Any maces so embedded are frostified
	    if (getItem(x, y))
	    {
		ITEMSTACK	items;
		int		i;

		// Only fetch buried items...
		getItemStack(items, x, y, true);
		for (i = 0; i < items.entries(); i++)
		{
		    ITEM *item = items(i);
		    if (item->getDefinition() == ITEM_MACE)
		    {
			reportMessage("A blue glow shines briefly through the ice.", x, y);
			item->setDefinition(ITEM_ICEMACE);
		    }
		}
	    }
	    break;

	case SQUARE_LAVA:
	    reportMessage("The lava hardens.", x, y);
	    newtile = SQUARE_CORRIDOR;
	    break;

	case SQUARE_FORESTFIRE:
	    reportMessage("The forest fire is extinguished.", x, y);
	    newtile = SQUARE_FOREST;
	    break;

	case SQUARE_ACID:	// Acid doesn't freeze.
	default:
	    // Nothing happens.
	    newtile = tile;
	    break;
    }

    if (newtile != tile)
    {
	setTile(x, y, newtile);
	setGuiltyMob(x, y, freezer);
	return true;
    }
    return false;
}

bool
MAP::electrocuteSquare(int x, int y, MOB *shocker)
{
    SQUARE_NAMES	tile;

    tile = getTile(x, y);

    switch (tile)
    {
	case SQUARE_WATER:
	case SQUARE_ACID:	// Dissolved ions implies conductivity.
	    if (tile == SQUARE_WATER)
		reportMessage("Electricty arcs through the water.", x, y);
	    else
		reportMessage("Electricty arcs through the acid.", x, y);

	    // TODO:
	    // Apply an attack to all connected tiles of same type,
	    // water acid barrier for some reason acts as insulator
	    break;

	default:
	    // Nothing happens.
	    break;
    }

    return false;
}

bool
MAP::burnSquare(int x, int y, MOB *burner)
{
    SQUARE_NAMES	tile, newtile;
    bool	hadeffect = false;

    tile = getTile(x, y);

    switch (tile)
    {
	case SQUARE_ICE:
	    reportMessage("The ice melts.", x, y);
	    newtile = SQUARE_WATER;
	    hadeffect = true;
	    break;
	case SQUARE_WATER:
	    reportMessage("The water boils.", x, y);
	    // No change to the tile.
	    // should fill with adjacent water.
	    newtile = SQUARE_PATHPIT;
	    hadeffect = true;
	    // Generate smoke.
	    setSmoke(x, y, SMOKE_STEAM, burner);
	    break;
	case SQUARE_ACID:
	    reportMessage("The acid boils.", x, y);
	    newtile = SQUARE_PATHPIT;
	    hadeffect = true;
	    // Generate smoke.
	    setSmoke(x, y, SMOKE_ACID, burner);
	    break;
	case SQUARE_FOREST:
	    reportMessage("The forest catches on fire.", x, y);
	    newtile = SQUARE_FORESTFIRE;
	    hadeffect = true;
	    break;
	default:
	    // Nothing happens.
	    newtile = tile;
	    break;
    }

    if (newtile != tile)
    {
	setTile(x, y, newtile);
	setGuiltyMob(x, y, burner);
	dropItems(x, y, burner);
	// We want to demand a reaction throw.
	dropMobs(x, y, true, burner);
	// TODO: Submerged mobs should become inpit!
	return true;
    }
    return hadeffect;
}

bool
MAP::electrifySquare(int x, int y, int points, MOB *shocker)
{
    SQUARE_NAMES	tile, newtile;
    bool	hadeffect = false;

    tile = getTile(x, y);

    switch (tile)
    {
	// There are currently no tile transitions.
	default:
	    // Nothing happens.
	    newtile = tile;
	    break;
    }

    if (newtile != tile)
    {
	setTile(x, y, newtile);
	setGuiltyMob(x, y, shocker);
	dropItems(x, y, shocker);
	// We want to demand a reaction throw.
	dropMobs(x, y, true, shocker);
	// TODO: Submerged mobs should become inpit!
	return true;
    }

    // Affect any items
    if (getItem(x, y))
    {
	ITEMSTACK	items;
	ITEM		*item;
	int		i;

	// Don't want to charge underwater items
	getItemStack(items, x, y, false);
	
	for (i = 0; i < items.entries(); i++)
	{
	    item = items(i);
	    if (item->electrify(points, shocker, 0))
	    {
		dropItem(item);
		delete item;
	    }
	}
    }
    
    return hadeffect;
}

bool
MAP::resurrectCorpses(int x, int y, MOB *resser)
{
    ITEM		*cur;
    
    // Can't res if a mob is already there.
    if (getMob(x, y))
	return false;

    // Find all items on this square & res them.
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	{
	    if (cur->resurrect(this, resser, x, y))
	    {
		dropItem(cur);
		delete cur;

		return true;
	    }
	}
    }

    return false;
}

bool
MAP::raiseZombie(int x, int y, MOB *resser)
{
    ITEM		*cur;
    
    // Can't res if a mob is already there.
    if (getMob(x, y))
	return false;

    // Find all items on this square & res them.
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	{
	    if (cur->raiseZombie(this, resser))
	    {
		dropItem(cur);
		delete cur;

		return true;
	    }
	}
    }

    return false;
}

bool
MAP::raiseSkeleton(int x, int y, MOB *resser)
{
    ITEM		*cur;
    
    // Can't res if a mob is already there.
    if (getMob(x, y))
	return false;

    // Find all items on this square & res them.
    for (cur = getItemHead(); cur; cur = cur->getNext())
    {
	if (cur->getX() == x && cur->getY() == y)
	{
	    if (cur->raiseSkeleton(this, resser))
	    {
		dropItem(cur);
		delete cur;

		return true;
	    }
	}
    }

    return false;
}

void
MAP::moveSmoke()
{
    int		x, y, nx, ny;
    int		ex, ey, idx, idy;
    int		wdx, wdy;
    
    // Determine if we are in a proper phase.
    if (!speed_isheartbeatphase())
	return;

    if (ourWindCounter > 0)
    {
	wdx = ourWindDX;
	wdy = ourWindDY;
	ourWindCounter--;

	if (!ourWindCounter)
	{
	    // The wind has changed!  Global alert!
	    msg_report("The wind shifts.  ");
	}
    }
    else
    {
	// We only have a 1 in 3 chance to want to move the smoke.
	if (!rand_choice(3))
	{
	    wdx = rand_choice(3)-1;
	    wdy = rand_choice(3)-1;
	}
	else
	{
	    wdx = 0;
	    wdy = 0;
	}
    }


    if (!mySmokeStack.entries())
	return;
    
    // We want to update in a direction that ensures we make room
    // for the exitting smoke.
    // Ie, positive wdx means we should update right to left.
    
    // wdx & wdy are the wind direction.
    // Each unit of smoke has a 1 in 10 chance of dissipating.
    if (wdy < 0)
    {
	y = 0;
	ey = MAP_HEIGHT;
	idy = 1;
    }
    else
    {
	y = MAP_HEIGHT-1;
	ey = -1;
	idy = -1;
    }

    for (; y != ey; y += idy)
    {
	if (wdx < 0)
	{
	    x = 0;
	    ex = MAP_WIDTH;
	    idx = 1;
	}
	else
	{
	    x = MAP_WIDTH-1;
	    ex = -1;
	    idx = -1;
	}
	for (; x != ex; x += idx)
	{
	    if (getFlag(x, y, SQUAREFLAG_SMOKE))
	    {
		MOB			*owner;
		SMOKE_NAMES		 smoke;

		smoke = getSmoke(x, y, &owner);

		// The probability of the smoke decaying depends on the
		// type of the smoke.
		if (rand_chance(glb_smokedefs[smoke].decayrate))
		{
		    setSmoke(x, y, SMOKE_NONE, 0);
		}
		else
		{
		    // We want to move it in the correct direction.
		    // Check to make sure new square is moveable &
		    // doesn't have smoke already.
		    nx = x + wdx;
		    ny = y + wdy;
		    if (nx >= 0 && nx < MAP_WIDTH &&
			ny >= 0 && ny < MAP_HEIGHT)
		    {
			if (!getFlag(nx, ny, SQUAREFLAG_SMOKE) &&
			    (glb_squaredefs[getTile(nx, ny)].movetype &
				    MOVE_STD_FLY))
			{
			    // Move the smoke.
			    setSmoke(x, y, SMOKE_NONE, 0);
			    setSmoke(nx, ny, smoke, owner);
			}
		    }
		}
	    }
	}
    }

    // Remove now empty entries:
    mySmokeStack.collapse();
}

SMOKE_NAMES
MAP::getSmoke(int x, int y, MOB **owner) const
{
    if (owner)
	*owner = 0;
    
    if (x < 0 || x >= MAP_WIDTH)
	return SMOKE_NONE;
    if (y < 0 || y >= MAP_HEIGHT)
	return SMOKE_NONE;

    // Verify we have smoke according to the bitflag.
    if (getFlag(x, y, SQUAREFLAG_SMOKE))
    {
	// Find the exact smoke type:
	return mySmokeStack.get(x, y, owner);
    }

    return SMOKE_NONE;
}

SIGNPOST_NAMES
MAP::getSignPost(int x, int y) const
{
    if (x < 0 || x >= MAP_WIDTH)
	return SIGNPOST_NONE;
    if (y < 0 || y >= MAP_HEIGHT)
	return SIGNPOST_NONE;

    // Ensure this is a proper signpost.
    if (mySignList && getTile(x, y) == SQUARE_SIGNPOST)
    {
	return mySignList->find(x, y);
    }

    return SIGNPOST_NONE;
}

void
MAP::setSmoke(int x, int y, SMOKE_NAMES smoke, MOB *owner)
{
    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_WIDTH);

    // Add the square flag so we know this is a smoke square.
    if (smoke == SMOKE_NONE)
    {
	// We early exit as a no-op if we are requested to clear
	// a tile where there is no smoke.
	if (!getFlag(x, y, SQUAREFLAG_SMOKE))
	    return;

	clearFlag(x, y, SQUAREFLAG_SMOKE);
    }
    else
	setFlag(x, y, SQUAREFLAG_SMOKE);

    // Add the specific type to our smoke stack.
    mySmokeStack.set(x, y, smoke, owner);
}

bool
MAP::hasNeighbouringTile(int x, int y, SQUARE_NAMES tile, MOB *&srcmob)
{
    if (x > 0)
	if (getTile(x-1, y) == tile)
	{
	    srcmob = getGuiltyMob(x-1, y);
	    return true;
	}
    if (x < MAP_WIDTH-1)
	if (getTile(x+1, y) == tile)
	{
	    srcmob = getGuiltyMob(x+1, y);
	    return true;
	}
    if (y > 0)
	if (getTile(x, y-1) == tile)
	{
	    srcmob = getGuiltyMob(x, y-1);
	    return true;
	}
    if (y < MAP_HEIGHT-1)
	if (getTile(x, y+1) == tile)
	{
	    srcmob = getGuiltyMob(x, y+1);
	    return true;
	}

    return false;
}

void
MAP::applySuction(int x, int y)
{
    int		dx, dy;
    int		odx, ody;
    int		mx, my;
    int		mnx, mny;
    int		s, t;
    MOB		*mob;
    ITEMSTACK	itemlist;

    FORALL_4DIR(dx, dy)
    {
	if (glb_squaredefs[getTile(x+dx,y+dy)].isstars)
	    break;
    }
    odx = !dx;
    ody = !dy;
    
    for (s = 3; s >= -3; s--)
    {
	for (t = -ABS(s); t <= ABS(s); t++)
	{
	    // Current suction square
	    mx = x + dx*s + odx*t;
	    my = y + dy*s + ody*t;
	    // Where we suck to.
	    mnx = mx + dx;
	    mnx += SIGN(s*t) * odx;
	    mny = my + dy;
	    mny += SIGN(s*t) * ody;

	    if (mnx < 0 || mnx >= MAP_WIDTH ||
		mny < 0 || mny >= MAP_HEIGHT)
		continue;
	    if (mx < 0 || mx >= MAP_WIDTH ||
		my < 0 || my >= MAP_HEIGHT)
		continue;

	    mob = getMob(mx, my);
	    if (mob && mob->canMove(mnx, mny, true))
	    {
		mob->move(mnx, mny);
	    }

	    itemlist.clear();
	    getItemStack(itemlist, mx, my);
	    
	    ITEM		*item;
	    for (int i = 0; i < itemlist.entries(); i++)
	    {
		item = itemlist(i);
		dropItem(item);
		item->markMapped(false);
		acquireItem(item, mnx, mny, 0);
	    }

	    if (getFlag(mx, my, SQUAREFLAG_SMOKE) &&
		!getFlag(mnx, mny, SQUAREFLAG_SMOKE))
	    {
		// Move the smoke over..
		MOB			*owner;
		SMOKE_NAMES		 smoke;

		smoke = getSmoke(mx, my, &owner);

		if (glb_squaredefs[getTile(mnx, mny)].movetype & MOVE_STD_FLY)
		{
		    setSmoke(mx, my, SMOKE_NONE, 0);
		    setSmoke(mnx, mny, smoke, owner);
		}
	    }
	}
    }
}

void
MAP::moveFluids()
{
    // Determine if we are in a proper phase.
    if (!speed_isheartbeatphase())
	return;

    if (!ourMapDirty)
	return;

    // Update all map tiles with the following rules:
    // 1) If a pit, and posseses neighbouring liquid, fill in.
    // 2) If lava, and possesses neighbouring water, steam & dry out.
    // 3) If ice, and neighbour is lava, turn to water.
    // We don't want instant chain reactions, so the order of
    // operation is important.

    int			x, y;
    u8			newtiles[MAP_HEIGHT][MAP_WIDTH];
    int			numnew = 0;
    MOB			*srcmob;

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    newtiles[y][x] = NUM_SQUARES;
	    srcmob = 0;

	    switch (getTile(x, y))
	    {
		case SQUARE_BROKENVIEWPORT:
		    // Auto-repair technology is in effect.
		    if (rand_chance(5))
		    {
			newtiles[y][x] = SQUARE_VIEWPORT;
			reportMessage("The glass wall mends itself.", x, y);
		    }
		    else
		    {
			applySuction(x, y);
		    }
		    numnew++;
		    break;

		case SQUARE_FORESTFIRE:
		    if (hasNeighbouringTile(x, y, SQUARE_WATER, srcmob))
		    {
			if (rand_chance(20))
			    newtiles[y][x] = SQUARE_FOREST;
		    }
		    if (rand_chance(10))
		    {
			newtiles[y][x] = SQUARE_CORRIDOR;
		    }
		    numnew++;
		    if (rand_chance(20))
			setSmoke(x, y, SMOKE_SMOKE, 0);
		    break;
		case SQUARE_FOREST:
		    if (hasNeighbouringTile(x, y, SQUARE_LAVA, srcmob))
		    {
			newtiles[y][x] = SQUARE_FORESTFIRE;
			// forest grower guilty if fire source unowned.
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    if (hasNeighbouringTile(x, y, SQUARE_FORESTFIRE, srcmob))
		    {
			if (rand_chance(30))
			{
			    newtiles[y][x] = SQUARE_FORESTFIRE;
			    // forest grower guilty if fire source unowned.
			    if (srcmob)
				setGuiltyMob(x, y, srcmob);
			}
			numnew++;
		    }
		    break;
		case SQUARE_ICE:
		    if (hasNeighbouringTile(x, y, SQUARE_LAVA, srcmob))
		    {
			newtiles[y][x] = SQUARE_WATER;
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    break;

		case SQUARE_LAVA:
		    if (hasNeighbouringTile(x, y, SQUARE_WATER, srcmob))
		    {
			newtiles[y][x] = SQUARE_CORRIDOR;
			// entombment goes to lava maker first
			if (!getGuiltyMob(x, y))
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    break;

		case SQUARE_WATER:
		    if (hasNeighbouringTile(x, y, SQUARE_LAVA, srcmob))
		    {
			newtiles[y][x] = SQUARE_PATHPIT;
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    break;

		case SQUARE_ACID:
		    if (hasNeighbouringTile(x, y, SQUARE_LAVA, srcmob))
		    {
			newtiles[y][x] = SQUARE_PATHPIT;
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    break;

		case SQUARE_FLOORPIT:
		case SQUARE_PATHPIT:
		case SQUARE_FLOORSPIKEDPIT:
		case SQUARE_PATHSPIKEDPIT:
		    // Lava first so water takes priority, as it
		    // flows faster.
		    if (hasNeighbouringTile(x, y, SQUARE_LAVA, srcmob))
		    {
			newtiles[y][x] = SQUARE_LAVA;
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    if (hasNeighbouringTile(x, y, SQUARE_ACID, srcmob))
		    {
			newtiles[y][x] = SQUARE_ACID;
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    if (hasNeighbouringTile(x, y, SQUARE_WATER, srcmob))
		    {
			newtiles[y][x] = SQUARE_WATER;
			if (srcmob)
			    setGuiltyMob(x, y, srcmob);
			numnew++;
		    }
		    break;
		default:
		    break;
	    }
	}
    }

    // Early exit for no-op.
    if (!numnew)
    {
	ourMapDirty = false;
	return;
    }

    // Process all the tiles.
    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    if (newtiles[y][x] == NUM_SQUARES)
		continue;

	    // Because we propagated guilt already, we can use our own guilt
	    // for source.
	    // This system built at very high altitude likely over the Pacific
	    // (the map feature is broken on this plane, the only 
	    // entertainment I like :<)
	    // Hopefully it holds together at sea level.
	    switch (getTile(x, y))
	    {
		case SQUARE_ICE:
		    burnSquare(x, y, getGuiltyMob(x, y));
		    break;

		case SQUARE_LAVA:
		    freezeSquare(x, y, getGuiltyMob(x, y));
		    break;

		case SQUARE_WATER:
		    burnSquare(x,y, getGuiltyMob(x, y));
		    break;

		case SQUARE_ACID:
		    // WTF?  Why burn?
		    burnSquare(x,y, getGuiltyMob(x, y));
		    break;

		case SQUARE_FOREST:
		    // Only transition is to burn.
		    burnSquare(x, y, getGuiltyMob(x, y));
		    break;

		case SQUARE_BROKENVIEWPORT:
		    if (newtiles[y][x] == SQUARE_VIEWPORT)
			setTile(x, y, SQUARE_VIEWPORT);
		    break;

		case SQUARE_FORESTFIRE:
		    // May have burned out or been doused.
		    if (newtiles[y][x] == SQUARE_CORRIDOR)
		    {
			// Directly extinguish
			setTile(x, y, SQUARE_CORRIDOR);
			// Drop from trees.
			dropMobs(x, y, true, getGuiltyMob(x, y), true);
		    }
		    else
		    {
			// Freeze is same as douse.
			freezeSquare(x, y, getGuiltyMob(x, y));
		    }
		    break;
				   

		case SQUARE_FLOORPIT:
		case SQUARE_PATHPIT:
		case SQUARE_FLOORSPIKEDPIT:
		case SQUARE_PATHSPIKEDPIT:
		    floodSquare((SQUARE_NAMES)newtiles[y][x], x, y, 
				getGuiltyMob(x, y));
		    break;
		default:
		    break;
	    }
	}
    }
}

void
MAP::reportMessage(const char *str, int x, int y) const
{
    if (!str)
	return;

    // Ignore non-level messages.
    if (this != glbCurLevel)
	return;

    UT_ASSERT(x >= 0 && x < MAP_WIDTH);
    UT_ASSERT(y >= 0 && y < MAP_HEIGHT);

    // Ignore out of bound messages.
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
	return;

    // The FOV check is guaranteed to be the same as the hasLOS for
    // the avatar.  
    if (MOB::getAvatar() &&
	!hasLOS(MOB::getAvatar()->getX(), MOB::getAvatar()->getY(),
		      x, y))
    {
	// Eat the message.
	return;
    }
    
    msg_report(gram_capitalize(str));
    // We always trail with some spaces, this will not cause line wrap,
    // but will ensure successive messages are all froody.
    msg_append("  ");
}

bool
MAP::isMobOnMap(MOB *mob) const
{
    MOB	*m;

    for (m = myMobHead; m; m = m->getNext())
    {
	if (m == mob)
	    return true;
    }
    return false;
}

MAP *
MAP::findMapWithMob(MOB *mob, bool usebranch) const
{
    MAP		*map, *branch;

    if (isMobOnMap(mob))
	return (MAP *)this;

    if (usebranch && peekMapBranch())
    {
	map = peekMapBranch()->findMapWithMob(mob, false);
	if (map)
	    return map;
    }

    for (map = peekMapUp(); map; map = map->peekMapUp())
    {
	if (map->isMobOnMap(mob))
	    return map;

	if (map->peekMapBranch())
	{
	    branch = map->peekMapBranch()->findMapWithMob(mob, false);
	    if (branch)
		return branch;
	}
    }

    for (map = peekMapDown(); map; map = map->peekMapDown())
    {
	if (map->isMobOnMap(mob))
	    return map;

	if (map->peekMapBranch())
	{
	    branch = map->peekMapBranch()->findMapWithMob(mob, false);
	    if (branch)
		return branch;
	}
    }

    // Couldn't find the mob anywhere...
    return 0;
}

void
MAP::collapseDungeon(bool dobranch)
{
    MAP		*map;

    // Rough guestimate of memory usage:
    // items: 20 bytes each
    // intrinscs, 167 -> 21 bytes
    // mobs: 48 + 21 = 69 bytes each
    // maps: 2k each
    // Need 27 maps, 54k
    // In 200k, 2968 mobs or 10,240 items fit.
    // Estimate ~500 mobs in bad game, say 72 bytes each, say 32bytes for
    // items gives 5,275 items.  Seems like I'm still missing somethiing
    // big.
    // Static maps: itemmap, 2k, mobmap 2k, wallmap 1k, distmap 2k, smell 2k
    // celllist 4k, cutouts 4k, refmap 2k, itemmap 4k, all told is 23k.
    // That cuts it to 4,539 items :>
    
    if (dobranch)
    {
	if (peekMapBranch())
	{
	    peekMapBranch()->collapseThisDungeon();
	    peekMapBranch()->collapseDungeon(false);
	}
    }

    for (map = peekMapUp(); map; map = map->peekMapUp())
    {
	map->collapseThisDungeon();
    }
    for (map = peekMapDown(); map; map = map->peekMapDown())
    {
	map->collapseThisDungeon();
    }
}

void
MAP::collapseThisDungeon()
{
    // Destroy all items that are destroyable..
    ITEM		*item, *next;

    for (item = getItemHead(); item; item = next)
    {
	next = item->getNext();

	if (!item->defn().isquest)
	{
	    dropItem(item);
	    delete item;
	}
    }

}

void
MAP::delayedLevelChange(MAP *newcurrent)
{
    ourDelayedLevelChange = newcurrent;
}

void
MAP::checkForAvatarMapChange()
{
    if (ourDelayedLevelChange)
    {
	changeCurrentLevel(ourDelayedLevelChange);
	ourDelayedLevelChange = 0;
    }
}


void
MAP::changeCurrentLevel(MAP *newcurrent)
{
    int		x, y;

    // Whatever it was is now invalid.
    ourDelayedLevelChange = 0;

    glbCurLevel = newcurrent;
    if (glbCurLevel)
	glbCurLevel->refreshPtrMaps();

    // Reset our smell map...
    memset(ourAvatarSmell, 0xff, sizeof(u16) * MAP_WIDTH * MAP_HEIGHT);
    // Reset our watermark
    ourAvatarSmellWatermark = 64;

    // Ensure we start looking for water and stuff to move
    ourMapDirty = true;

    // Reset our guilty map
    for (y = 0; y < MAP_HEIGHT; y++)
	for (x = 0; x < MAP_WIDTH; x++)
	    ourMobGuiltyMap[(y) * MAP_WIDTH + x].setMob(0);

    // Output any level feelings...
    if (newcurrent && newcurrent->branchName() == BRANCH_MAIN)
    {
	switch (newcurrent->getDepth())
	{
	    case 20:
		msg_announce("><0|V|: Expand your horizons!  ");
		break;
	    case 21:
		msg_announce("Belweir: Welcome to my library!  ");
		break;
	    case 22:
		msg_announce("Quizar: Find your way through!  ");
		break;
	    case 23:
		msg_announce("H'ruth: Prove your worth in my combat arena!  ");
		break;
	    case 24:
		msg_announce("Klaskov: This village is under attack from orcs!  ");
		break;
	    case 25:
		msg_report("You have entered the fiery pits of hell!  ");
		break;
	}
    }
}

void
MAP::refreshPtrMaps()
{
    MOB		*mob;
    ITEM	*item;
    int		 x, y;

    for (y = 0; y < MAP_HEIGHT; y++)
	for (x = 0; x < MAP_WIDTH; x++)
	    ourMobRefMap[(y) * MAP_WIDTH + x].setMob(0);

    memset(ourItemPtrMap, 0, sizeof(ITEM *) * MAP_WIDTH * MAP_HEIGHT);
    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	ourMobRefMap[mob->getY() * MAP_WIDTH + mob->getX()].setMob(mob);
	if (mob->getSize() >= SIZE_GARGANTUAN)
	{
	    int		dx, dy;
	    for (dx = 0; dx < 2; dx++)
		for (dy = 0; dy < 2; dy++)
		    ourMobRefMap[(mob->getY() + dy) * MAP_WIDTH + mob->getX() + dx].setMob(mob);
	}
    }
    for (item = getItemHead(); item; item = item->getNext())
    {
	int		offset = item->getY() * MAP_WIDTH + item->getX();
	
	// We want to auto-sort to ensure the right people are on top.
	if (!ourItemPtrMap[offset] ||
	     ourItemPtrMap[offset]->getStackOrder() <= item->getStackOrder())
	    ourItemPtrMap[offset] = item;
    }
}

void
MAP::setWindDirection(int dx, int dy, int duration)
{
    ourWindCounter = duration;
    ourWindDX = dx;
    ourWindDY = dy;
}

void
MAP::getWindDirection(int &dx, int &dy)
{
    if (ourWindCounter)
    {
	dx = ourWindDX;
	dy = ourWindDY;
    }
    else
    {
	dx = 0;
	dy = 0;
    }
}

void
MAP::getAmbientWindDirection(int &dx, int &dy)
{
    if (ourWindCounter)
    {
	dx = ourWindDX;
	dy = ourWindDY;
    }
    else
    {
	dx = rand_choice(3)-1;
	dy = rand_choice(3)-1;
    }
}

bool
MAP::isUnlocked() const
{
#ifdef iPOWDER
    if (hamfake_isunlocked())
	return true;

    if (getDepth() > 15)
	return false;
    return true;
#else
    return true;
#endif
}

#ifdef MAPSTATS
void
MAP::printStatsHeader(FILE *fp)
{
    fprintf(fp, "Seed, Walkable, LadderDist, NumMob, TotalMobLevel, WallRatio\n");
}

bool
MAP::printStats(FILE *fp) 
{
    fprintf(fp, "%d, ", mySeed);

    int		numwalk = 0;
    int		difficulty = 0;
    int		nummob = 0;
    int		numwall = 0;
    int		x, y;
    MOB		*mob;

    for (y = 0; y < MAP_HEIGHT; y++)
    {
	for (x = 0; x < MAP_WIDTH; x++)
	{
	    if (canMove(x, y, MOVE_STD_FLY,
			true, true, true))
	    {
		int		dx, dy;

		numwalk++;
		FORALL_4DIR(dx, dy)
		{
		    if (canMove(x+dx, y+dy, MOVE_STD_FLY,
			true, true, true))
		    {
			numwall++;
		    }
		}
	    }
	}
    }
    fprintf(fp, "%d, ", numwalk);

    for (mob = getMobHead(); mob; mob = mob->getNext())
    {
	if (mob->isAvatar())
	    continue;

	difficulty += mob->getExpLevel();
	nummob++;
    }

    
    int		ux = 0, uy = 0, dx = 0, dy = 0;
    bool	founda, foundb;
    founda = findTile(SQUARE_LADDERUP, ux, uy);
    foundb = findTile(SQUARE_LADDERDOWN, dx, dy);
    UT_ASSERT(founda && foundb);
    buildMoveDistance(dx, dy, MOVE_STD_FLY);

    int		maxdist = 256;
    int dist = ourDistanceMap[uy * MAP_WIDTH + ux];
    if (dist > maxdist)
	dist = maxdist;
    fprintf(fp, "%d, ", dist);

    fprintf(fp, "%d, ", nummob);
    fprintf(fp, "%d, ", difficulty);
    fprintf(fp, "%f, ", ((float)numwall) / numwalk);
    fprintf(fp, "\n");

    dist--;
#if 1
    if (dist == maxdist)
    {
	for (y = 0; y < MAP_HEIGHT; y++)
	{
	    for (x = 0; x < MAP_WIDTH; x++)
	    {
		int dist = ourDistanceMap[y * MAP_WIDTH + x];

		if (getTile(x, y) == SQUARE_LADDERUP)
		    fprintf(fp, "z");
		else if (getTile(x, y) == SQUARE_LADDERDOWN)
		    fprintf(fp, "y");
		else if (dist >= maxdist)
		{
		    if (canMove(x, y, MOVE_STD_FLY,
				true, true, true))
		    {
			fprintf(fp, "+");
		    }
		    else if (getTile(x, y) == SQUARE_SECRETDOOR ||
				getTile(x, y) == SQUARE_DOOR ||
				getTile(x, y) == SQUARE_BLOCKEDDOOR)
		    {
			fprintf(fp, "/");
		    }
		    else
		    {
			fprintf(fp, "#");
		    }
		}
		else
		    fprintf(fp, ".");
	    }
	    fprintf(fp, "\n");
	}
    }
#endif

    return dist == maxdist;
}

#endif
