/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        map.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	The MAP structure.  This defines one levels information.  It
 *	contains all the items that are on the floor and all the mobs
 *	that are in the level.
 */

#ifndef __map_h__
#define __map_h__

//#define MAPSTATS

#include "assert.h"
#include "glbdef.h"
#include "gfxengine.h"
#include "smokestack.h"
#include "itemstack.h"
#include "signpost.h"

class MAP;
class MOB;
class ITEM;
class SRAMSTREAM;
class INTRINSIC_COUNTER;

extern MAP		*glbCurLevel;

struct PT2
{
    int x, y;
    int data;
};

struct ROOM_DEF
{
    PT2 size;
    const SQUARE_NAMES *squarelist;
    
    // Per square lists
    const PT2 *squareflaglist;
    const PT2 *itemlist;
    const PT2 *moblist;
    const PT2 *itemtypelist;
    const PT2 *moblevellist;
    const PT2 *intrinsiclist;
    const PT2 *signpostlist;

    // Global defines
    int		minlevel, maxlevel;
    u8		rarity;
    u8		mandatory;
    u8		onlyonce;
    u8		allowstairs;
    u8		allowgeneration;
    u8		allowdig;
    u8		allowitems;
    u8		allowtel;
    int		x, y;
    s8		quest;
    const char	*name;		// Specific room name from file name.
};

struct ROOM
{
    PT2			 l, h;
    const ROOM_DEF	*definition;
    int			 rotation;
};

#define ROOM_FLIP_X 1
#define ROOM_FLIP_Y 2
#define ROOM_FLOP 4

class MAP
{
public:
    MAP(int level, BRANCH_NAMES branch);
    virtual ~MAP();

    // Retrieves the random seed used to build this map.
    long		 getSeed() const { return mySeed; }

    // This deletes this map and all attached maps
    void		 deleteAllMaps();

    // This returns true if their is a path between (sx,sy) and
    // (ex,ey).  The next step is returned in (dx,dy).  This must
    // be stateless.
    // If two paths are equally cool (ie, cutting diagonals) picks
    // one at random.
    bool		 findPath(int sx, int sy,
				  int ex, int ey,
				  MOVE_NAMES movetype,
				  bool allowmob,
				  bool allowdoor,
				  int &dx, int &dy);

    // This updates the avatar smell at the given location.
    // The smell should be 0 where the avatar is and lower the farther
    // away.
    void		updateAvatarSmell(int x, int y, int smell);

    // Determines the level of smell at the given square.
    // Higher is smellier or closer to the avatar.  To make things
    // normalized, 65535 is at the avatar, 0 is as far as possible.
    int			getAvatarSmell(int x, int y) const;

    // Determines the direction closest to the avatar from the given point.
    // If no direction is closer, returns false.
    bool		getAvatarSmellGradient(MOB *mob, int x, int y, int &dx, int &dy) const;
    
    // This takes an empty map and adds people up to its desired
    // monster count.
    void		 populate();
    
    // This creates a monster appropriate for our level and puts
    // it somewhere on the level.
    // Returns true if successful
    bool		 createAndPlaceNPC(bool avoidfov);

    // Refresh will update the gfx engine from this map.
    // If preserve ray is turned on, any ray tiles in the
    // overly plane will be preserved.
    void		 refresh(bool preserve_ray = false);
    // This will clear out & rebuild the dynamic light lists.
    void		 updateLights();

    int			 getDesiredMobCount() const;
    int			 getThreatLevel() const;

    // This runs the level's heartbeat script.  It may birth mobs
    // if the world seems sparse.
    void		 doHeartbeat();

    // Digger can be null.  Otherwise, it reports the appropriate messages
    // about the possibility, and falls through the hole.
    void		 digHole(int x, int y, MOB *digger);

    // Digger can be null.  Otherwise, it reports the appropriate messages
    // about the possibility, and ends up in the pit.
    // Does not create holes if already pit.
    void		 digPit(int x, int y, MOB *digger);

    // Excavates a square.  Destroys boulders
    bool		 digSquare(int x, int y, MOB *digger);

    // Grows a nice forest here.
    void		 growForest(int x, int y, MOB *grower);

    // Torrential rain falls here
    void		 downPour(int x, int y, MOB *pourer);

    // This floods the pit with the given fluid type, such as SQUARE_LAVA
    // or SQUARE_WATER.
    void		 floodSquare(SQUARE_NAMES tile, int x, int y, 
			          MOB *flooder);

    // This maintaince function will drop all items on the given
    // square.  The revealer is optional and says who gets credit for the
    // discovery.  This does nothing if the square doesn't have depth.
    void		 dropItems(int x, int y, MOB *revealer);

    // This will try and fill the square that the item is on
    // with the item itself.  This plugs holes, fills water, etc.
    // filler is the person to blame if someone is buried alive.
    // This returns true if filling occured, in which case the item
    // was deleted.
    bool		 fillSquare(ITEM *boulder, MOB *filler, 
				    ITEM **newitem = 0);

    // This will drop all mobs on the given square into the holes,
    // provided they don't save via reflex.  They will also sink.
    // forcetrap negates the reflex save, but they may still save
    // due to flight, water walking, etc.  They may also save due
    // to the SKILL_EVADETRAP, unless noskillevade is set.
    // This returns false if a mob gets killed.
    bool		 dropMobs(int x, int y, bool forcetrap, MOB *revealer,
				    bool noskillevade=false);

    // Trapper can be null.  Otherwise the trapper SHOULD gain
    // responsibility for any deaths from the trap :>
    // Returns whether trap creation was successful.  It can fail
    // if the square is untrappable (Ie, not plain)
    bool		 createTrap(int x, int y, MOB *trapper);

    // This will apply a strong cold effect to the square,
    // turning water to ice, lava to rock.
    bool		 freezeSquare(int x, int y, MOB *freezer);

    // This will apply electricity to the square
    // If it is liquid, everyone in the liquid or touching it
    // is fried
    bool		 electrocuteSquare(int x, int y, MOB *shocker);

    // Places a moderate amount of water on the square.
    void		 douseSquare(int x, int y, bool holy, MOB *douser);

    // This will apply a strong heat to the square, melting ice.
    bool		 burnSquare(int x, int y, MOB *burner);

    // This will zap the square with electicity.
    bool		 electrifySquare(int x, int y, int points, 
					MOB *shocker);

    // This will bring all corpses/bones at given square back to life.
    // Returns true if something happens.
    bool		 resurrectCorpses(int x, int y, MOB *resser);

    // This creates a zombie out of the corpses.
    bool		 raiseZombie(int x, int y, MOB *raiser);

    // This creates a skeleton out of the bones.
    bool		 raiseSkeleton(int x, int y, MOB *raiser);

    // This moves all of the smoke on the level.
    void		 moveSmoke();

    // This queries our additional smoke structures that maintain the
    // type and ownership of smoke squares.
    SMOKE_NAMES		 getSmoke(int x, int y, MOB **owner=0) const;

    // Queries our signpost structure for the proper sign post data
    SIGNPOST_NAMES	 getSignPost(int x, int y) const;
    
    // This creates smoke at the given square.  It should be used
    // rather than trying to explicitly set the SQUAREFLAG values.
    // Owner is dude responsible for the effects.
    void		 setSmoke(int x, int y, SMOKE_NAMES smoke, MOB *owner);

    // Applies a suck-into-space effect at the given coordinate,
    // assumes a 4 way neighbour is space.
    void		 applySuction(int x, int y);

    // This moves all the fluids on the level, filling pits, etc.
    void		 moveFluids();

    void		 setWindDirection(int dx, int dy, int duration);

    // This only reports powerful winds such as the direct wind
    // spell.  It is assumed people can easily override the slow
    // currents that are present normally.
    void		 getWindDirection(int &dx, int &dy);

    // This reports transient zephyrs as well
    void		 getAmbientWindDirection(int &dx, int &dy);

    //
    // Access methods.  Explicitly inlined.
    //
    inline void		 setTile(u8 x, u8 y, SQUARE_NAMES tile)
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	ourMapDirty = true;
	mySquares[x + y * MAP_WIDTH] = tile;
    }
    inline SQUARE_NAMES	 getTile(u8 x, u8 y) const
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	return (SQUARE_NAMES) mySquares[x + y * MAP_WIDTH];
    }
    // Allows for out of bounds queries, returns SQUARE_EMPTY for those
    inline SQUARE_NAMES	 getTileSafe(u8 x, u8 y) const
    {
	if (x >= MAP_WIDTH || y >= MAP_HEIGHT)
	{
	    return SQUARE_EMPTY;
	}
	return (SQUARE_NAMES) mySquares[x + y * MAP_WIDTH];
    }
    inline u8		 compareTile(u8 x, u8 y, SQUARE_NAMES tile)
    {
	if (x >= MAP_WIDTH || y >= MAP_HEIGHT)
	    return false;
	return (getTile(x, y) == tile);
    }
    bool		 hasNeighbouringTile(int x, int y, SQUARE_NAMES tile,
				MOB *&srcmob);
    inline void		 setFlag(u8 x, u8 y, int flag, bool state=true)
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	if (!state)
	    clearFlag(x, y, flag);
	else
	    myFlags[x + y * MAP_WIDTH] |= flag;
    }
    inline void		 clearFlag(u8 x, u8 y, int flag)
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	myFlags[x + y * MAP_WIDTH] &= ~flag;
    }
    // Note this does *not* return bool, as it does
    // not return 0 or 1, but 0 or flag.  This is
    // for silly performance reasons.
    inline int		 getFlag(u8 x, u8 y, int flag) const
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	return myFlags[x + y * 32] & flag;
    }
    inline u8		 getRawFlag(u8 x, u8 y) const
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	return myFlags[x + y * 32];
    }
    inline void		 setRawFlag(u8 x, u8 y, int flag)
    {
	UT_ASSERT(x < MAP_WIDTH && y < MAP_HEIGHT);
	myFlags[x + y * 32] = flag;
    }

    int			 getDepth() const { return myDepth; }

    bool		 allowDiggingDown() const;
    bool		 allowMobGeneration() const;
    bool		 allowItemGeneration() const;
    bool		 allowTeleportation() const;

    // This marks as mapped everything within the given radius & los.
    void		 markMapped(int x, int y, int radx, int rady, 
				    bool uselos = true);

    // This updates the temporary light field.
    void		 applyTempLight(int x, int y, int radius);
    // This applies a map flag to everything in the given radius.
    void		 applyFlag(SQUAREFLAG_NAMES flag, 
				    int cx, int cy, int radius,
				    bool uselos, bool state);

    // Is it transparent, can we see through it?
    bool		 isTransparent(int x, int y) const;

    // Is lit, by some means or another?
    bool		 isLit(int x, int y) const;

    // Is digging not a noop?  Includes boulders, but not creatures
    bool		 isDiggable(int x, int y) const;

    // This builds a field of view table.  It sets the FOV temp flag
    // to true whereever it is within range of rx & ry.
    void		 buildFOV(int x, int y, int rx, int ry);

    bool		 hasFOV(int x, int y) const;

    // This determines if there is a straightline, +/- 1, between the
    // locations.
    bool		 hasLOS(int x1, int y1, int x2, int y2) const;
    // This is a crappy FP algorithm.
    bool		 hasSlowLOS(int x1, int y1, int x2, int y2) const;
    // This LOS algorithm was written while rather drunk.
    bool		 hasDrunkLOS(int x1, int y1, int x2, int y2) const;
    // This determines if there exists any manhatten walk between the two
    // locations.
    // Definitely one of my stupider ideas.
    bool		 hasManhattenLOS(int x1, int y1, int x2, int y2) const;

    void		 moveMob(MOB *mob, int ox, int oy, int nx, int ny);
    void		 registerMob(MOB *mob);
    void		 unregisterMob(MOB *mob);
    MOB			*getMobHead() const { return myMobHead; }

    // Crushes any surrounding breakable walls to make room for this
    void		 wallCrush(MOB *mob);

    // This finds all mobs interested in the given mob, which are also
    // adjacent to the staircase, and causes them to go downstairs.
    // The avatar is explicitly exempt from this.  Be careful as your
    // pets may chase people down stairs :>
    void		 chaseMobOnStaircase(SQUARE_NAMES destsquare,
			    int srcx, int srcy,
			    MOB *chasee,
			    MAP *nextmap);

    // Gives a brief listing of what is at the square...
    // blind tells us if the viewer is blind.
    bool		 describeSquare(int x, int y, bool blind, 
					bool includeself);

    // This fetches the maps below or above us.  If they don't exist,
    // they are created, unless they shouldn't exist when you will
    // get 0 back.
    MAP			*getMapUp(bool create = true);
    MAP			*peekMapUp() const;
    MAP			*getMapDown(bool create = true);
    MAP			*peekMapDown() const;

    // What branch getMapBranch() would be on - where this map
    // has a branch *to*.  BRANCH_NONE for no branch from here.
    BRANCH_NAMES	 offshootName() const;
    // The branch *this is on.  BRANCH_MAIN is, of course, the default.
    BRANCH_NAMES	 branchName() const;
    MAP			*getMapBranch(bool create = true);
    MAP			*peekMapBranch() const;

    // This is used internally to set up a cross reference...
    void		 setMapUp(MAP *map) { myMapUp = map; }
    void		 setMapDown(MAP *map) { myMapDown = map; }
    void		 setMapBranch(MAP *branch) { myMapBranch = branch; }

    // This finds the mob in the specified square.  Currently very slow.
    MOB			*getMob(int x, int y) const;
    // Does a full search for the mob, ignoring the cache.
    // Does not respect garguantuan for important reasons!
    MOB			*getMobPrecise(int x, int y) const;

    // Returns whoever is responsible for the status of this square
    MOB			*getGuiltyMob(int x, int y) const;
    void		 setGuiltyMob(int x, int y, MOB *mob);

    ITEM		*getItemHead() const { return myItemHead; }
    ITEM		*getItem(int x, int y) const;
    ITEM		*getItem(int x, int y, bool submerged) const;

    // This builds an itemstack of everything on the ground.  Submerged
    // check, if positive, determines if submerged status is used.
    // Note this just appends to the itemstack!
    // If checkmapped is true, it only adds items that have their
    // mapped bit set.
    int 		 getItemStack(ITEMSTACK &stack, int x, int y,
					int submergecheck = -1,
					bool checkmapped = false) const;

    // This builds a mob stack of all tamed mobs directly in the
    // control of the given mob.
    // Returns number of pets.
    // If sensable is true, only returns the pets that are sensable.
    int			 getPets(MOBSTACK &stack, MOB *owner, 
				 bool onlysensable) const;
					
    // This is the tile acquiring the item.
    // This sets newitem to the stack it gets merged to, otherwise itself.
    // Note item *can* be deleted here!
    // If we are to believe the item's grade status, set ignoregrade.
    // This is used for loading.
    // The item can *also* be dropped down a hole so end up on a
    // different level.  In this case, newitem is also set to 0.
    void		 acquireItem(ITEM *item, int x, int y, 
				     MOB *dropper,
				     ITEM **newitem = 0, 
				     bool ignoregrade = false);
    // This is the tile relieving & unlinking the item (as opposed to get)
    ITEM		*dropItem(int x, int y);
    void		 dropItem(ITEM *item);

    // If item is a bottle, fills with the liquid in tile x,y
    void		 fillBottle(ITEM *item, int x, int y) const;

    // This gets a random location on the map that fits the given profile.
    // It returns false if no such location was found.
    bool		 findRandomLoc(int &x, int &y, int movetype,
					bool allowmob,
					bool doublesize,
					bool avoidfov,
					bool avoidfire,
					bool avoidacid,
					bool avoidnomob);

    // This finds an instance of the given tile.
    // Returns false if no such tile exists.
    bool		 findTile(SQUARE_NAMES tile, int &x, int &y);
    
    // This finds the closest tile to the given x/y coordinate that
    // is legal according to the provided move mask & mob mask.
    // It does a spiral search, and ignores LOS.
    bool		 findCloseTile(int &x, int &y, MOVE_NAMES move,
					bool allowmob = false,
					int maxradius = MAP_WIDTH);

    // Does a simple search to see if this square is inside a room
    // with no non-hidden exits.  This lets us warn the player that
    // it isn't a dead end.
    bool		 isSquareInLockedRoom(int x, int y) const;

    // This will run the heartbeat & doAI on all the critters that
    // are not the avatar.
    void		 moveNPCs();

    // Determines if the given movetype can go into the given square.
    // Return false if it fails.
    bool		 canMove(int x, int y, MOVE_NAMES movetype,
				    bool allowmob = true,
				    bool allowdoor = false,
				    bool allowitem = true);

    // Determines if a ray can move into the given square.
    // If a mob there has reflection, it can't move there.
    // If reflect is false, no reflection checks occur.
    // didreflect is set to true if reflection occured.
    bool		 canRayMove(int x, int y, MOVE_NAMES movetype,
				    bool reflect, bool *didreflect = 0);

    // Throws an item in a certain direction.
    void		 throwItem(int x, int y, int dx, int dy,
				ITEM *item, ITEM *stack, ITEM *launcher, 
				MOB *thrower,
				const ATTACK_DEF *attack,
				bool ricochet);

    // This draws a ray through the environment, appropriate overlay
    // tiles.  If dobounce is true, it will bounce properly when
    // it hits something impassable according to movetype.
    // Each square it will trigger the raycallback, which can
    // return false if the ray should die.
    // The ray starts one square in the direction dx/dy.
    // One of dx & dy must be non zero.
    void		 fireRay(int ox, int oy, int dx, int dy,
				int length,
				MOVE_NAMES movetype, bool dobounce,
				bool raycallback(int x, int y, bool final, 
						void *data),
				void *data);

    // This draws a ball in the environment, appropriate overlay tiles.
    // Each square affected will trigger the ballcallback.  The result
    // code of the ball callback is unused.
    void		 fireBall(int ox, int oy, int rad, bool includecenter,
				bool ballcallback(int x, int y, bool final, 
						    void *data),
				void *data);

    // Performs a knockback with the (possibly) two creatures.
    // We specify the target of the knockback and the direction to apply
    // it in.  If infinitemass is true, the source isn't eligible to 
    // be knocked back.
    bool		 knockbackMob(int tx, int ty, int dx, int dy,
				    bool infinitemass,
				    bool ignoremob = false);

    // Save/load the global information to all maps
    static void		 saveGlobal(SRAMSTREAM &os);
    static void		 loadGlobal(SRAMSTREAM &is);
    // Save the map...
    void		 saveBranch(SRAMSTREAM &is, bool savebranch = true);
    bool		 save(SRAMSTREAM &os, bool savebranch = true);
    // And load...
    void		 loadBranch(SRAMSTREAM &is, bool checkbranch = true);
    bool		 load(SRAMSTREAM &is, bool checkbranch = true);

    bool		 verifyMob() const;
    bool		 verifyMobLocal() const;

    bool		 verifyCounterGone(INTRINSIC_COUNTER *counter) const;
    bool		 verifyCounterGoneLocal(INTRINSIC_COUNTER *counter) const;

    // The map can report messages.  These are eaten unless
    // the avatar can see (x, y).
    void		 reportMessage(const char *str, int x, int y) const;
    void		 reportMessage(BUF buf, int x, int y) const
			 { reportMessage(buf.buffer(), x, y); }

    int			 getItemCount() const { return myItemCount; }
    int			 getMobCount() const { return myMobCount; }

    // Returns the map which has the given mob, 0 if it shows up on
    // no map.
    MAP			*findMapWithMob(MOB *mob, bool usebranch=true) const;
    bool		 isMobOnMap(MOB *mob) const;

    // Collapses the dungeon by destroying all non-quest items.
    void		 collapseDungeon(bool dobranch = true);
    void		 collapseThisDungeon();
    

    // Flags that we want to (eventually) change the current level
    // but can't immediately as we are in some ugly code stack that
    // uses glbCurLevel
    static void		 delayedLevelChange(MAP *newcurrent);

    // Looks for stashed level changes and applies them.
    static void		 checkForAvatarMapChange();
    
    // This handles bookkeeping of glbCurLevel.  Do not set by hand!
    static void		 changeCurrentLevel(MAP *newcurrent);

    // This will rebuild the pointer maps to be accurate for this.
    // Should be glbCurLevel->refreshPtrMaps whenever glbCurLevel is changed.
    void		 refreshPtrMaps();

    //
    // Build methods.
    // These methods are defined in build.cpp
    // They are used to construct the map.
    //

    // Generic build method, does different things depending on level,
    // random chance, etc.
    // The alreadysavedseed is a flag to ensure we don't resave the seed,
    // as that would cause the random number seed to be changed when
    // we try and force an exact rebuild by pre-setting the seed.
    void		 build(bool alreadysavedseed);

    static bool		 isLoading() { return ourIsLoading; }

    // Is this map unlocked?
    bool		 isUnlocked() const;

protected:
    // This wipes out the map and fills it with a blank map.
    // This will also delete all mobs and all creatures in the map.
    void		 clear();
    
    // Specific map builders.
    void		 buildRoguelike();
    void		 buildQuest(GOD_NAMES god);
    void		 buildBigRoom();
    void		 buildDrunkenCavern(bool islit, bool allowlakes, bool allowroom, bool buildstaircase);
    void		 buildMaze(int stride);

    // Attempts to smooth a map removing small dead end nooks.
    void		 smooth(SQUARE_NAMES floor, SQUARE_NAMES wall);

    // Does a flood fill with the given tile type, replacing the
    // given tile
    void		 fillWithTile(int x, int y, SQUARE_NAMES newtile, SQUARE_NAMES oldtile);
    // Qix style level, Bunch of walls subdividing the level.
    void		 drawQixWall(int x, int y, int dx, int dy, bool islit);
    void		 buildQix(bool islit);
    // A cavern bisected by a river.
    void		 buildRiverRoom();
    void		 buildSurfaceWorld();
    void		 buildTutorial();
    // Builds the tridude spaceship
    void		 buildSpaceShip();
    // Builds Baezl'bub's lair - lots of water, lava, and caverns.
    void		 buildHello();

    void		 drawCircle(int cx, int cy, int rad,
				    SQUARE_NAMES tile,
				    SQUAREFLAG_NAMES flag,
				    bool overwrite = false);

    void		 drawLine(int x1, int y1, int x2, int y2,
				SQUARE_NAMES tile,
				bool overwrite = false);

    // Specific type of move building
    bool		 buildMoveDistance(int px, int py, MOVE_NAMES move);
    // This builds the distance function given the given walk function...
    bool		 buildDistance(int px, int py);

    // Builder functions.
    bool		 drawPath(int sx, int sy, int ex, int ey, 
				 bool lit = false);
    bool		 drawPathStraight(int sx, int sy, int ex, int ey, 
				 bool lit = false);
    void		 drawPath_old(int sx, int sy, int ex, int ey, 
				 bool lit = false);
    bool		 drawCorridor(int sx, int sy, int sdx, int sdy,
				 int ex, int ey, int edx, int edy,
				 bool lit = false);
    void		 createColouredTridudeLair(int x, int y, MOB_NAMES tridude, MOB *trueleader);
    MOB			*createGoldenTridudeLair(int x, int y);
    static const ROOM_DEF *chooseRandomRoom(int depth);
    void		 buildRandomRoom(ROOM &room);
public:
    void		 buildRandomRoomFromDefinition(ROOM &room,
					const ROOM_DEF *def);
    void		 populateRoom(const ROOM &room);
    void		 drawRoom(const ROOM &room,
				 bool lit,
				 bool mazetype,
				 bool caverntype);
protected:
    void		 chooseRandomExit(int *enter, int *delta,
				 const ROOM &room);
    void		 closeUnusedExits(const ROOM &room, bool force);
    void		 roomToMapCoords(int &mx, int &my,
				const ROOM &room, int rx, int ry);
    void		 mapToRoomCoords(int &rx, int &ry,
				const ROOM &room, int mx, int my);

    // Builder query functions, these aren't necessarily valid outside
    // the building code.
    bool		 isTileWall(int tile) const;
    bool		 isMapWall(int x, int y) const;
    bool		 isMapExit(int x, int y) const;
    bool		 isRoomExit(const ROOM &room, int x, int y) const;
    bool		 isMapPathway(int x, int y) const;
    bool		 checkRoomFits(const ROOM &room);

    // This draws a path element, or a door if the destinatin is a wall.
    void		 setPathTile(int x, int y, bool lit, bool usefloor);

    void		 addSignPost(SIGNPOST_NAMES post, int x, int y);

public:
#ifdef MAPSTATS
    static void		 printStatsHeader(FILE *fp);
    bool		 printStats(FILE *fp);
#endif

private:    
    // Data...
    long		 mySeed;		// Random number seed used.
    int			 myGlobalFlags;		// Global flags for this map.
    
    // Indices into SQUARE.
    u8			*mySquares;
    // These are the state flags for each world square...
    u8			*myFlags;
    // Stashed coords of the FOV map.
    s8			 myFOVX, myFOVY;
    // What branch we are on.
    u8			 myBranch;
    // How deep this map is.
    s8			 myDepth;

    // Indices into SQUARE, TILE_VOID if blank.
    // These are temporary arrays for efficient map refreshing.
    static u16		*ourItemMap;
    static u16		*ourMobMap;
    // This is 0 if not a wall, 1 if a wall.
    static u8		*ourWallMap;
    // This is how far each square is from the desired destination,
    // thus allowing gradiant computatin and path finding.
    static u16		*ourDistanceMap;
    static u16		*ourAvatarSmell;
    static u16		 ourAvatarSmellWatermark;
    // These track our cell positions & cutout values  used by the
    // build functions.
    static int		*ourCells;
    static int		*ourCutOuts;

    // These are pointers to the first mob or item on each square.
    // They are only valid for the current level.
    static MOBREF	*ourMobRefMap;
    static ITEM		**ourItemPtrMap;

    // This tracks who is responsible for each square
    // It is not saved per level - changing level loses responsibility
    static MOBREF	*ourMobGuiltyMap;

    // This is the mob list..
    int			 myMobCount;
    int			 myItemCount;
    MOB			*myMobHead;
    // If we unregister this mob, we will autobump it.
    MOB			*myMobSafeNext;
    // And the item list...
    ITEM		*myItemHead;

    // Pointers to the double linked list of maps...
    // Zero means not yet built.
    MAP			*myMapUp;
    MAP			*myMapDown;
    // My branch and myself are on the same "level",
    // myMapBranch->myMapBranch == this.
    MAP			*myMapBranch;

    // Bleh... List of all valid smoke on the level.
    // Entries may be SMOKE_NONE, in which case they are cleared out
    // and waiting compression.
    SMOKESTACK		 mySmokeStack;

    SIGNLIST		*mySignList;

    // Flags whether we are currently loading or not.
    static bool		 ourIsLoading;

    // Flags the current wind direction and count down.
    // Shared by all levels.  If count down is out, wind direction
    // is random
    static s8		 ourWindDX, ourWindDY;
    static int		 ourWindCounter;
    static bool		 ourMapDirty;
    static MAP		*ourDelayedLevelChange;
};

#endif
