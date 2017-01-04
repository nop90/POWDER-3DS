/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        creature.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	This provides the definition of all mob types.  Individual
 * 	instances are copied out of the main ROM table so they can store
 * 	their non-static data.
 */

#ifndef __creature_h__
#define __creature_h__

#include "grammar.h"

#include "glbdef.h"
#include "intrinsic.h"
#include "mobref.h"
#include "name.h"
#include "speed.h"
#include "map.h"

class ITEM;
class SRAMSTREAM;
class MOB;

// One time mob initialization.
void mob_init();

// AI initialization, resetting, saving, loading
void ai_init();
void ai_reset();

// invoked by save/loadGlobal
void ai_save(SRAMSTREAM &os);
void ai_load(SRAMSTREAM &is);

//
// Used for the drag and drop menu system to go to and from u8 and
// our action/spell values.
// These are usually tied to actions, hence their presence here.
//
u8 action_packStripButton(ACTION_NAMES action);
u8 action_packStripButton(SPELL_NAMES spell);
void action_unpackStripButton(u8 value, ACTION_NAMES &action, SPELL_NAMES &spell);

// The grey value will be 0 if this sprite isn't useable, so should be grey
// to 256 for a fully charged sprite.
SPRITE_NAMES action_spriteFromStripButton(u8 value, int *grey = 0);
void action_indexToOverlayPos(int index, int &x, int &y);



// Kill counters.
extern u16 glbKillCount[NUM_MOBS];

// Inventory size...
#define MOBINV_WIDTH	12
#define MOBINV_HEIGHT	8

// AI State variables.  These are volatile
#define AI_DIRTY_INVENTORY	0x8000
#define AI_HAS_THROWABLE	0x4000
#define AI_HAS_ATTACKWAND	0x2000
#define AI_HAS_EDIBLE		0x1000
#define AI_LAST_HIT_LOC		0x03ff

// AI FSM Modes.  This is non-volatile.
#define AI_FSM_NORMAL		0
#define AI_FSM_ATTACK		1
#define AI_FSM_GUARD		2
#define AI_FSM_STAY		3
#define AI_FSM_JUSTBORN		4

// Intrinsic source flags
#define INTRINSIC_FROM_ITEM		1
#define INTRINSIC_FROM_PERMAMENT	2
#define INTRINSIC_FROM_TEMPORARY	4

class INTRINSIC_COUNTER
{
public:
#ifdef STRESS_TEST
    ~INTRINSIC_COUNTER()
    {
	// Verify that we are no longer referenced by anyone.
	if (glbCurLevel)
	    glbCurLevel->verifyCounterGone(this);
    }
#endif
    INTRINSIC_NAMES	 myIntrinsic;
    int		 	 myTurns;
    INTRINSIC_COUNTER	*myNext;
    // Tracks who put this counter on!
    MOBREF		 myInflictor;
};

// Instance of a mob.
// Do not make this virtual or you will have misunderstood the 
// point of the exercise!
class MOB
{
private:
    MOB();		// Create mobs with ::create please.
public:
    ~MOB();

    // Resets all the mob counts, such as the number of creatures slain,
    // etc.
    static void	 init();

    // Loads & saves the global state.
    static void	 saveGlobal(SRAMSTREAM &os);
    static void	 loadGlobal(SRAMSTREAM &is);

    // The avatar is the special mob through which everything is viewed.
    // It also gets the pronoun "you".
    static MOB	*getAvatar();
    static void  setAvatar(MOB *mob);

    static void  saveAvatar(SRAMSTREAM &os);
    static void	 loadAvatar(SRAMSTREAM &is);

    // Mobs can belong to lists...
    MOB		*getNext()		{ return myNext; }
    void	 setNext(MOB *mob)	{ myNext = mob; }

    bool	 isAvatar() const { return getAvatar() == this; }

    static bool	 isAvatarOnFreshSquare() { return ourAvatarOnFreshSquare; }
    static void	 setAvatarOnFreshSquare(bool fresh) 
		 { ourAvatarOnFreshSquare = fresh; }

    // Are we currently polymorphed?
    bool	 isPolymorphed() const { return !myBaseType.isNull(); }
    // Returns the base creature type.  This is what you are
    // before polymorphing.
    MOB		*getBaseType() const { return myBaseType.getMob(); }

    // Get our MOBREF.  Pass by reference to minimize reconstruction.
    // DO NOT USE THIS.  Instead, use MOBREF.setMob(this), which has
    // the advantage of dealing with the MOB * being null.
    const MOBREF &getMobRef() const { return myOwnRef; }

    // This alert tells us to invalidate any references to the given mob.
    void	 alertMobGone(MOB *mob);
    // This clears all the references on this mob as a result of it
    // being unregistered from a map.
    void	 clearReferences();
    
    static MOB	*create(MOB_NAMES definition);

    // Equips self with an ascension worthy set of equipment/abilities.
    void	 buildAscensionKit();
    
    // Creates a new mob, possibly updating the avatar.
    static MOB	*load(SRAMSTREAM &is);

    // Writes out mob data...
    void	 save(SRAMSTREAM &os);

    // Verifies our MOBREFs are valid.
    // Reports 0 if nulls, ! if bad link.
    bool	 verifyMob() const;

    // Verifies that we don't see the given INTRINISC_COUNTER anywhere.
    bool	 verifyCounterGone(INTRINSIC_COUNTER *counter) const;

    // Chooses an NPC def given a threat level
    static MOB_NAMES chooseNPC(int threatlevel);

    // Creates an NPC given a threat level.
    static MOB  *createNPC(int threatlevel);

    // Turn this mob into a unique.
    // This should be done just after create!
    void	 makeUnique();

    // Is the mob physically capable of being on the given square.
    // checksafety will prohibit squares that will damage the
    // critter.
    bool	 canMove(int nx, int ny, bool checkitem, 
			bool allowdoor = false,
			bool checksafety = false) const;

    // Checks if the mob can move through a diagonal passage.  We
    // prohibit kiddie-corner style moves.  If dx and dy are not both
    // non-zero, turns into a normal canMove.
    // In case of diagonal, the orthogonal passages are only tested
    // for MOVE flags, not for bolders, mobs, safety, etc.
    bool	 canMoveDelta(int dx, int dy, bool checkitem,
			bool allowdoor = false,
			bool checksafety = false) const;

    // This returns true if this is aware of mob.  This means that:
    // A) this cannot see, has telepathy, and mob has a mind.
    // B) this can see, mob is visible, has los, and mob is lit
    // C) this can see, can see invisible, mob is invisible, has los, and lit.
    // D) if either party is in a pit, the distance is 1.
    // E) both parties are same submerge state.
    // F) not deaf, noiselevel of other critter is high enough
    // G) has warning, other creature in range.
    // The LOS check is skipped if checklos = false.
    // If only sight is true, only sight checks are made.
    bool	 canSense(const MOB *mob, bool checklos = true, bool onlysight = false) const;

    // Returns *how* you can sense the given mob.
    SENSE_NAMES	 getSenseType(const MOB *mob) const;

    // This is the movement mask we currently can do, modified by items,
    // etc.
    MOVE_NAMES	 getMoveType() const;

    int		 getX() const { return myX; }
    int		 getY() const { return myY; }
    int		 getDLevel() const { return myDLevel; }
    void	 setDLevel(int d) { myDLevel = d; }
    int		 getTile() const;		// This is Upper left.
    int		 getTileLL() const;
    int		 getTileLR() const;
    int		 getTileUR() const;

    MOB_NAMES	 getDefinition() const { return (MOB_NAMES) myDefinition; }
    void	 setOrigDefinition(MOB_NAMES origdefn) { myOrigDefinition = origdefn; }

    // Changes our internal & original type.  If reset, we wipe
    // out our mp/hp type stats.
    void	 changeDefinition(MOB_NAMES newdefn, bool reset);

    MOBTYPE_NAMES getMobType() const { return (MOBTYPE_NAMES) glb_mobdefs[myDefinition].mobtype; }

    const MOB_DEF &defn() const { return defn(getDefinition()); }
    static const MOB_DEF &defn(MOB_NAMES mob) { return glb_mobdefs[mob]; }

    int		 getHP() const { return myHP; }
    int		 getMaxHP() const { return myMaxHP; }
    void	 incrementMaxHP(int amount);
    void	 setMinimalHP() { myHP = 1; }
    void	 setMaximalHP() { myHP = myMaxHP; }
    void	 forceHP(int hp) { myMaxHP = myHP = hp; }
    int		 getHitDie() const { return myHitDie / 2; }
    void	 incrementHitDie(int amount) { myHitDie += amount*2; }
    int		 getExpLevel() const { return myExpLevel; }
    int		 getMP() const { return myMP; }
    int		 getMaxMP() const { return myMaxMP; }
    void	 incrementMaxMP(int amount);
    int		 getMagicDie() const { return myMagicDie / 2; }
    void	 incrementMagicDie(int amount) { myMagicDie += amount*2; }
    int		 getExp() const { return myExp; }
    
    int		 getStrength() const 
			    { return glb_mobdefs[myDefinition].strength; }
    int		 getSmarts() const 
			    { return glb_mobdefs[myDefinition].smarts; }
    SIZE_NAMES	 getSize() const { return (SIZE_NAMES) 
					    glb_mobdefs[myDefinition].size; }

    // This gets the maximum AC of the creature.
    int		 getAC() const;

    // Computes the score, only really sensible for the avatar
    int		 calcScore(bool haswon) const;
    
    // This properly rolls the AC to find out what it is for this
    // particular hit.
    int		 rollAC(MOB *attacker) const;

    MATERIAL_NAMES getMaterial() const
		 { return (MATERIAL_NAMES) glb_mobdefs[myDefinition].material; }

    bool	 hasBreathAttack() const;
    ATTACK_NAMES getBreathAttack() const;
    bool	 isBreathAttackCharged() const;
    int		 getBreathRange() const;
    const char  *getBreathSubstance() const;

    bool	 isTalkative() const;

    void	 submergeItemEffects(SQUARE_NAMES tile);

    bool	 hasItem(ITEM_NAMES def) const;

    // This finds out if the mob emits light, and if so, how far.
    bool	 isLight() const;
    int		 getLightRadius() const;

    // Determines if we are allowed to move on diagonals
    // (obscure reference to diabolizing matrices)
    bool	 canMoveDiabolically() const;

    bool	 canResurrectFromCorpse() const;
    bool	 isBoneless() const;
    bool	 isBloodless() const;

    void	 setAIFSM(int fsm) { myAIFSM = fsm; }
    void	 setAITarget(MOB *mob) { myAITarget.setMob(mob); }
    void	 clearAITarget() { myAITarget.setMob(0); }
    MOB		*getAITarget() const { return myAITarget.getMob(); }

    // Returns the master of this mob.  This is who the mob
    // immediately reports to
    MOB		*getMaster() const;

    // Returns true if this mob is at some level a slave of the given
    // critter.
    // You are not a slave to yourself.
    bool	 isSlave(const MOB *owner) const;

    // Makes this a slave of the given master.
    void	 makeSlaveOf(const MOB *owner);

    // Returns true if we have a common master.
    bool	 hasCommonMaster(const MOB *other) const;

    bool	 hasIntrinsic(INTRINSIC_NAMES intrinsic, bool allowitem=true) const;

    // Return bit field showing all sources of the intrinsic
    int		 intrinsicFromWhat(INTRINSIC_NAMES intrinsic) const;

    // Return a string giving the identifier for the intrinsic source
    const char	*intrinsicFromWhatLetter(INTRINSIC_NAMES intrinsic) const;

    // These just chain to the relevant intrinsic, but allow for
    // more legible code.
    bool	 hasSkill(SKILL_NAMES skill, bool allowitem=true, bool ignoreamnesia=false) const;
    bool	 hasSpell(SPELL_NAMES spell, bool allowitem=true, bool ignoreamnesia=false) const;

    // Rolls to see if the given skill meets its proc chance
    bool	 skillProc(SKILL_NAMES skill) const;

    // These determine if you have the necessary prereqs.
    // If explain is true, the list of failed prereqs is given.
    bool	 canLearnSkill(SKILL_NAMES skill, bool explain=false) const;
    int		 getUsedSkillSlots() const;
    int		 getFreeSkillSlots() const;
    bool	 canLearnSpell(SPELL_NAMES spell, bool explain=false) const;
    int		 getUsedSpellSlots() const;
    int		 getFreeSpellSlots() const;

    void	 clearDeathIntrinsics(bool silent);

    bool	 canDigest(ITEM *item) const;
    bool	 safeToDigest(ITEM *item) const;

    bool	 canFly() const;

    // Return true if anything we are wearing is made of that material.
    bool	 isWearing(MATERIAL_NAMES material) const;

    // Sorts our linked list invenroy to match the expected slot
    // order
    void	 sortInventoryBySlot();

    // This sets the intrinsic permamently & instantly.
    void	 setIntrinsic(INTRINSIC_NAMES intrinsic);
    // Merges into our intrinsic all those in the given string
    void	 mergeIntrinsicsFromString(const char *rawstr);
    // This removes the intrinsic.  It does so instantly & permamently.
    void	 clearIntrinsic(INTRINSIC_NAMES intrinsic, bool silent=false);
    // This sets the given intrinsic for the given number of turns.
    // Turns are eaten every heartbeat.
    // If the intrinsic is already on permamently, it doesn't affect
    // anything.
    // We are not guaranteed to return a counter.
    // If quiet is set, we never announce the gain/loss
    void	 setTimedIntrinsic(MOB *inflictor,
				   INTRINSIC_NAMES intrinsic, int turns,
				   bool quiet = false);

    int		 getAttackBonus(const ATTACK_DEF *attack, ATTACKSTYLE_NAMES style);

    // Returns the probability, 0..100 inclusive, that the second
    // weapon should be allowed to proc.
    // This is zero if there is no second weapon, etc.
    int		 getSecondWeaponChance() const;
    
    // This gets THIS mobs attitude towards APPRAISEE.
    ATTITUDE_NAMES getAttitude(const MOB *appraisee) const;

    AI_NAMES	 getAI() const 
		 { return (AI_NAMES) glb_mobdefs[myDefinition].ai; }

    // Move the mob into the given tile, updating the gfx map &
    // applying any relevant callbacks.
    // Note the new mob x/y is not necessarily nx ny!
    // Returns false if the creature is now dead.
    bool	 move(int nx, int ny, bool interlevel = false);

    // This runs all the every-turn things this mob should do.  This will
    // cause the mob to age, consume food, regain magic & hits, etc.  This
    // is often in the MOBs time frame, not the world time frame, so usually
    // occurs before the mob's AI frame (so faster mobs consume faster)
    // This returns true if the mob can still perform an action.  It
    // returns false if the mob's ai should be skipped (eg: the mob died)
    bool	 doHeartbeat();

    // This finds a random dx,dy pair for which canMove returns true.
    // The move will not move into the owner.
    void	 findRandomValidMove(int &dx, int &dy, MOB *owner);

    // Determines if the creature is confused.  It then will adjust
    // the dx, dy, and dz values appropriately.  It does the random
    // chance to keep the original decision.
    // Care must be taken to not end up with double jeopardy.
    void	 applyConfusion(int &dx, int &dy);
    void	 applyConfusionNoSelf(int &dx, int &dy);
    void	 applyConfusion(int &dx, int &dy, int &dz);

    // Returns number of foes that are melee adjacent,
    // does not count friends, sleeping, or paralysed.
    int		 calculateFoesSurrounding() const;

    // Returns true if this mob should doAI on the given phase.
    bool	 validMovePhase(PHASE_NAMES phase);

    // This does the prequel actions prior to the creature getting its
    // move.  It's move would either be the user action or the doAI
    // action.
    // true is returned if the user should act, false if the prequel
    // ate their action.
    bool	 doMovePrequel();

    // AI: All ai prefixed functions, along with doAI, are found in
    // ai.cpp.
    //
    // This runs the AI script on this MOB, which will cause it to run
    // some action or another.
    void	 doAI();

    // Returns true if we successfully move in the given direction.
    // We will try and jump if possible.
    // actionBump will be used, but blockers will be avoided.
    // if target mob blocks, an attack will still be made.
    // This can handle dx & dy both being non zero, in which case it
    // picks one of them.
    bool	 aiMoveInDirection(int dx, int dy, int distx, int disty,
				    MOB *target);

    // Decays knowledge of us.
    void	 aiDecayKnowledgeBase();

    // Returns true if we know that the given target has a resistance
    // to the element.  This means we should likely not bother with
    // an attack based on that element.
    bool	 aiCanTargetResist(MOB *target, ELEMENT_NAMES element) const;

    // Tries to note that the given target has the given intrinsic,
    // internally tranforms into a call to CanResist.
    void	 aiNoteThatTargetHasIntrinsic(MOB *target, INTRINSIC_NAMES intrinsic);
    // Tries to note that the given target can resist the given element
    void	 aiNoteThatTargetCanResist(MOB *target, ELEMENT_NAMES element);
    // Internal function when we've passed the tests.
    void	 aiTrulyNoteThatTargetCanResist(MOB *target, ELEMENT_NAMES element);

    // Returns true if acted.
    // Searches inventory for useful items and uses them.
    bool	 aiUseInventory();

    // Picks up whatever is on the ground.
    bool	 aiBePackrat();

    // Searches through one's invenotry for yummy stuff and eats it.
    bool	 aiEatStuff();

    // Tries to leave melee combat through teleportation, etc.
    // Only does so if it decides it's unsafe (low hp, etc)
    bool	 aiDoFleeMelee(MOB *target);

    // Looks for any battle preperations it can do.  Ie, casting
    // buffs, etc.
    // The dx, dy is the offset to our target.
    bool	 aiDoBattlePrep(int diffx, int diffy);

    // Heal, cure, etc.
    bool	 aiDoUrgentMoves();

    // Specific action ai:
    
    // Attacks: These will attack in the given direction at the
    // given range.
    bool	 aiDoRangedAttack(int dx, int dy, int range);
    bool	 aiDoThrownAttack(int dx, int dy, int range);
    bool	 aiDoWandAttack(int dx, int dy, int range);
    bool	 aiDoSpellAttack(int dx, int dy, int range);

    // Heals: These attempt to restore health/cure poison.
    bool	 aiDoHealSelf();
    bool	 aiDoCurePoisonSelf();

    // This attempts to immerse onself in water.
    bool	 aiDoDouseSelf();

    // Attempts to freeze the square I stand on
    bool	 aiDoFreezeMySquare();

    // Try to get off my current square onto a safe square
    bool	 aiDoEscapeMySquare();

    // This will attempt to stop stoning, provided we care about it.
    bool	 aiDoStopStoningSelf();

    // Specific AI behaviours:

    // Mice run around with the right hand rule getting into mischief.
    bool	 aiDoMouse();

    // Testing functions.
    bool	 aiIsItemCooler(ITEM *item, ITEMSLOT_NAMES slot);
    bool	 aiIsItemThrowable(ITEM *item, MOB *target = 0) const;
    bool	 aiIsItemZapAttack(ITEM *item) const;

    bool	 aiIsPoisoned() const;

    bool	 aiWillOpenDoors() const;

    // Actions...  This is where all the creature smarts go.  At the
    // root is the actionBump - this is a request by a creature to go
    // in a certain direction.  What it means is creature dependent...
    // Actions return true if they consumed a timeslot, and false if
    // they failed.

    // All of the "action" prefixed functions can be found in action.cpp.	
    bool	 actionBump(int dx, int dy);

    // The creature uses its breath attack in the given direction.
    bool	 actionBreathe(int dx, int dy);

    // Note all these callbacks take a MOBREF * inside data!
    static bool	 meltRocksCBStatic(int x, int y, bool final, void *data);

    // The attack is in ourEffectAttack.
    static bool	 areaAttackCBStatic(int x, int y, bool final, void *data);

    // Spell is in ourZapSpell.
    static bool  zapCallbackStatic(int x, int y, bool final, void *data);

    // This tries to swap with a creature in the given direction.
    bool	 actionSwap(int dx, int dy);
    
    // Talk to someone or something
    bool	 actionTalk(int dx, int dy);

    // Puts the mob to sleep for 50 turns.
    bool	 actionSleep();

    // This tries to push on the given direction.
    bool	 actionPush(int dx, int dy);

    // This will move in the given direction.  It does no intelligence
    // checks!
    bool	 actionWalk(int dx, int dy, bool spacewalk = false);

    // This will jump in the given direction.  This will move you
    // two squares.  It can jump over things, but not through walls.
    // Destination must be visible and lit.
    bool	 actionJump(int dx, int dy);

    // Applies the magic door effect, only valid for avatars.
    // Coords are absolute location of magical door.
    bool	 actionMagicDoorEffect(int x, int y);

    // This will try to open the object in the given direction.
    bool	 actionOpen(int dx, int dy);

    // THis will try to close the object.
    bool	 actionClose(int dx, int dy);

    // This causes this MOB to search the given square, spamming
    // the appropriate discovery and revealling it to the world.
    // It returns true if anything was found.
    // The isaction controls if it was purposeful or not.  This affects
    // the probability in dark areas.  Darkness penalty only applies
    // to purposeful search attempts.  (Finding stuff accidentally is
    // bad, so should be encouraged :>)
    bool	 searchSquare(int x, int y, bool isaction);
    
    // This searches at the square we are now on.
    bool	 actionSearch();

    // This will attack what is in the given direction.
    bool	 actionAttack(int dx, int dy, int multiplierbonus = 0);

    // This handles attempts to climb things.
    bool	 actionClimb();
    bool	 actionClimbUp();
    bool	 actionClimbDown();

    bool	 actionDig(int dx, int dy, int dz, int range, bool magical);

    // Picks up whats on the floor.
    // The specified item is which item to pickup.
    // Note that picking up an item may destroy it when it merges
    // with a stack, newitem will be set to the resulting item.
    bool	 actionPickUp(ITEM *item, ITEM **newitem = 0);
    // Tries to drop the given item.
    bool	 actionDrop(int ix, int iy);

    // Sorts the inventory items.
    bool	 actionSort();

    // Tries to eat.
    // This doubles as drinking.
    bool	 actionEat(int ix, int iy);

    // Tries to throw.
    bool	 actionThrow(int ix, int iy, int dx, int dy, int dz);

    // Throws a quivered item.
    bool	 actionFire(int dx, int dy, int dz);
    
    // Puts the specified object into the quiver.
    bool	 actionQuiver(int ix, int iy);

    // Verifies you can zap
    bool	 ableToZap(int ix, int iy, int dx, int dy, int dz, bool quiet);
    
    // Tries to zap.
    bool	 actionZap(int ix, int iy, int dx, int dy, int dz);

    // Tries to cast.
    bool	 actionCast(SPELL_NAMES spell, int dx, int dy, int dz);

    // Verifies if you can read.
    bool	 ableToRead(int ix, int iy, bool quiet);

    // Tries to read.
    bool	 actionRead(int ix, int iy);

    // The potion is what to dip into, the i() is what to dip.
    bool	 actionDip(int potionx, int potiony, int ix, int iy);

    // This will cause you to query the gods as to your current
    // status.
    bool	 actionPray();

    // Take control of given mob, only really works for avatar.
    bool	 actionPossess(MOB *mob, int duration);

    // Removes our possession and returns ourself to our original
    // body, provided it exists.
    bool	 actionReleasePossession(bool systemshock = false);

    // This will perform a random teleport, unless the creature has
    // teleport control, in which case it will use its own AI (or
    // user input) for the destination.
    // allowcontrol will enable teleport control to be tested.
    // givecontrol will grant the creature telecontrol for this action.
    // Xloc and yloc give the teleport destination, if present.
    bool	 actionTeleport(bool allowcontrol=true, bool givecontrol=false,
				int xloc=-1, int yloc=-1);

    bool	 actionForget(SPELL_NAMES spell, SKILL_NAMES skill);

    // UPdates our tile according to what we are wearing.
    void	 rebuildAppearance();

    // Update our worn intrinsic according to what we have equipped.
    void	 rebuildWornIntrinsic();

    // Determines how well dressed the creature is.
    // per god amounts optionally stored in dressiness.   0 is nothing
    // nice, no consistent maximum.
    void	 determineClassiness(int *dressiness = 0);

    // Verifies you can equip the item.
    bool	 ableToEquip(int ix, int iy, ITEMSLOT_NAMES slot, bool quiet);

    // Tries to equip the item in the given inventory slot.
    bool	 actionEquip(int ix, int iy, ITEMSLOT_NAMES slot, bool quiet=false);

    // Verifies you can remove the item.
    bool	 ableToDequip(ITEMSLOT_NAMES slot, bool quiet);

    // Tries to take off the item in the given slot.
    bool	 actionDequip(ITEMSLOT_NAMES slot, bool quiet = false);

    // Returns the total skill the creature has with the given
    // armour or weapon.
    int		 getWeaponSkillLevel(const ITEM *weapon, 
				     ATTACKSTYLE_NAMES attackstyle) const;
    int		 getArmourSkillLevel(const ITEM *armour) const;

    // This records that src tried to attack us.
    // We may choose to disregard this if we feel it is friendly fire,
    // or currently have someone we hate more.
    void	 noteAttacker(MOB *src);

    // Receptor functions...
    // The attacking item may be null, in which case the mob attacks
    // alone.
    // These return false if the creature dies.
    // The actuallyhit is set to true if any attack landed.
    // It is NOT set to false otherwise!
    bool	 receiveAttack(ATTACK_NAMES attack, MOB *src, ITEM *weapon,
				ITEM *launcher,
				ATTACKSTYLE_NAMES style,
				bool *actuallyhit = 0, int multiplier = 0);
    bool	 receiveAttack(const ATTACK_DEF *attack, MOB *src, ITEM *weapon,
				ITEM *launcher,
				ATTACKSTYLE_NAMES style,
				bool *actuallyhit = 0, int multiplier = 0);
    bool	 receiveDamage(ATTACK_NAMES attack, MOB *src,
			    ITEM *weapon, 
			    ITEM *launcher,
			    ATTACKSTYLE_NAMES style,
			    int multiplier = 1,
			    int damage_reduction = 0);
    bool	 receiveDamage(const ATTACK_DEF *attack, MOB *src, 
			    ITEM *weapon,
			    ITEM *launcher,
			    ATTACKSTYLE_NAMES style,
			    int multiplier = 1,
			    int damage_reduction = 0);
    void	 receiveDeathExp(int explevel);
    void	 receiveExp(int exp);
    // Clears all experience: Ouch!
    void	 wipeExp();

    // Enchant a random piece of armour by the given amounts.
    // ITEMSLOT_NAMES -1 means a random armour piece.
    void	 receiveArmourEnchant(ITEMSLOT_NAMES slot, int bonus, bool shudder);

    // Enchant your weapon.
    void	 receiveWeaponEnchant(int bonus, bool shudder);

    // Curses random item that isn't already cursed, starting with
    // your equipped items.
    // onlyequip true will ensure only equipped items will be cursed.
    // forceany prevents the code from preferring equipped items.
    void	 receiveCurse(bool onlyequip, bool forceany);

    // Uncurses random cursed item, starting with equipped items.
    // Doall true means it will uncurse everything.
    void	 receiveUncurse(bool doall);
    
    // Only update HP through these methods to ensure consistent treatment
    // vampiric heals prevent Tlosh from growing angry.
    bool	 receiveHeal(int hp, MOB *src, bool vampiric = false);
    bool	 receiveMana(int mp, MOB *src);
    bool	 receiveCure();
    bool	 receiveSlowPoison();

    SPELL_NAMES	 findSpell(GOD_NAMES god, int *numspell = 0) const;
    SKILL_NAMES	 findSkill(GOD_NAMES god, int *numskill = 0) const;

    void	 learnSpell(SPELL_NAMES spell, int duration = -1);
    void	 learnSkill(SKILL_NAMES skill, int duration = -1);
    // Returns if we are able to cast the spell, ie have enough mana,
    // have the right intrinsic, etc.
    bool	 canCastSpell(SPELL_NAMES spell);
    // Returns how close we are to casting the spell.  0 means
    // not possible, 256 we can cast it this instant.
    int		 spellCastability(SPELL_NAMES spell);
    void	 gainLevel();

    // Undo casting cost of the spell.
    void	 cancelSpell(SPELL_NAMES spell);

    //
    // piety functions.  These are implemented in piety.cpp.
    // They control the implicity worship given to one or more gods
    // by just about any action.
    //
    // Currently only the avatar triggers these, but theoritically
    // that could be expanded.
    //
    void	 pietyGainLevel();

    void 	 pietyKill(MOB *mob, ATTACKSTYLE_NAMES style);
    void 	 pietyEat(ITEM *item, int foodval);
    void 	 pietyDress(INTRINSIC_NAMES intrinsic);
    void 	 pietyZapWand(ITEM *wand);
    void 	 pietySurrounded(int numenemy);
    void 	 pietyHeal(int amount, bool self, bool enemy, bool vampiric);
    void 	 pietyCastSpell(SPELL_NAMES spell);
    void 	 pietyCreateTrap();
    void	 pietyFindSecret();
    void 	 pietyAttack(MOB *mob, const ATTACK_DEF *attack, 
			      ATTACKSTYLE_NAMES style);
    // You made the given amount of noise.
    void	 pietyMakeNoise(int noise);
    // You identified something.
    void	 pietyIdentify(ITEM_NAMES itemdef);

    // Broke an item
    void 	 pietyBreak(ITEM *item);

    // Prints out a report of the current piety levels of all gods.
    void 	 pietyReport();

    // Announces the whim of xom if you are following ><0|V|
    void	 pietyAnnounceWhimOfXom();

    // Runs the god AI to determine what to do with the poor player.
    void 	 pietyRungods();

    // Used to actually apply piety points to a given god.
    void 	 pietyGrant(GOD_NAMES god, int amount, bool forbidden = false);

    // Applied when one performs a forbidden action.
    void 	 pietyTransgress(GOD_NAMES god);

    // Applied to use up some god points in doling out punishment.
    void 	 pietyPunish(GOD_NAMES god);

    // Applied to use up some god points in doling out rewards.
    void 	 pietyBoon(GOD_NAMES god);

    // Returns true if the player can benefit from the given boon.
    bool	 pietyBoonValid(BOON_NAMES boon);

    // What sort of crappy comment is this?
    //     "The string can take certain parameters..."
    void	 reportMessage(const char *str) const;
    inline void	 reportMessage(BUF buf) const
		 { reportMessage(buf.buffer()); }

    // This reports a message with mob specific connotations:
    // %U means the mob itself.
    // %u means the mob itself without any article.
    // %i means the item identified at class level
    // %R means the possessive of the mob. (full name!)
    // %r means the possessive, pronoun.
    // %A means the accusative of the mob.
    // %p means the pronoun.
    // %M? means the mob, using the same code as above.
    // %I? means the item, using the same code as above.
    // %I1? means the singular item, using the same code as above.
    // <foo> means to conjugate foo with this mob.
    // <I:..> and <M:..> conjugate respectively.
    // %B1, %B2, refer to an ancillary buffer with a specific
    // numbering order.
    void	 formatAndReport(const char *str);
    void	 formatAndReport(const char *str, const char *b1, const char *b2 = 0, const char *b3 = 0);
    inline void	 formatAndReport(const char *str, BUF b1)
    { formatAndReport(str, b1.buffer()); }
    void	 formatAndReport(const char *str, const MOB *mob, const char *b1 = 0, const char *b2 = 0, const char *b3 = 0);
    inline void	 formatAndReport(const char *str, const MOB *mob, BUF b1)
    { formatAndReport(str, mob, b1.buffer()); }
    void	 formatAndReport(const char *str, const ITEM *item, const char *b1 = 0, const char *b2 = 0, const char *b3 = 0);
    inline void	 formatAndReport(const char *str, const ITEM *item, BUF b1)
    { formatAndReport(str, item, b1.buffer()); }
    void	 formatAndReportExplicit(const char *str, const MOB *mob, const ITEM *item, const char *b1 = 0, const char *b2 = 0, const char *b3 = 0);
    inline void	 formatAndReportExplicit(const char *str, const MOB *mob, const ITEM *item, BUF b1)
    { formatAndReportExplicit(str, mob, item, b1.buffer()); }

    static BUF	 formatToString(const char *str, const MOB *self, const ITEM *selfitem, const MOB *mob, const ITEM *item, const char *b1 = 0, const char *b2 = 0, const char *b3 = 0);
    static BUF	 formatToString(const char *str, const MOB *self, const ITEM *selfitem, const MOB *mob, const ITEM *item, BUF b1, BUF b2, BUF b3);

    // Conjugates a verb with our own tense.  If this is an avatar,
    // that means "You", otherwise "It".
    BUF		 conjugate(const char *infinitive, bool past=false) const;

    // This gets the tense which this mob should be addressed with, ie,
    // me, you, he, she, it, they?
    VERB_PERSON	 getPerson() const;
    VERB_PERSON	 getGender() const;
    
    // you, it, he, she, they, etc.
    const char	*getPronoun() const;
    
    // your, its, his, her, their, etc.
    const char  *getPossessive() const;
    
    // yours, its, his, hers, theirs, etc.
    const char  *getOwnership() const;

    // yourself, itself, himself, herself, etc
    const char  *getReflexive() const;

    // you, it, him, her, etc.
    const char  *getAccusative() const;

    // dragon, the dragon, a dragon, etc.
    BUF		 getName(bool usearticle = true, bool usedefinite = true,
			bool forcevisible = false, bool neverpronoun = false) const;

    // The headless is in perfect health.  It would be a hard fight.
    BUF		 getDescription() const;

    // This displays the long description of this particular mob.
    // It will also provide all the other cool facts, like what they
    // are wearing, etc.
    void	 viewDescription() const;

    // Displays a character dump of the characters abilities.
    // Also saves to a file on the PC version.
    void	 characterDump(bool ondeath, bool didwin, bool savefile, int truetime = -1);

    void	 pageCharacterDump(bool ondeath, bool didwin, int truetime = -1);

    // Determines if this mob moved last turn.  Only works
    // for the avatar.
    bool	 hasMovedLastTurn() const;
    void	 getLastMoveDirection(int &dx, int &dy) const;
    void	 resetLastMoveDirection() const;
    void	 flagLastMoveAsOld() const;

    // Rename the MOB this name.
    void	 christen(const char *name);
    void	 christen(BUF buf)
		 { christen(buf.buffer()); }

    // Perform a smarts/str check against the given difficulty.
    // Result is:
    // -1 - Guaranteed fail
    //  0 - Failed a check.
    //  1 - Succeeded check.
    //  2 - Guaranteed success.
    // One should have different text for each so people know if they
    // should test again.
    int		 smartsCheck(int smarts);
    int		 strengthCheck(int str);

    // Picks up an item, assigns it into a slot.
    // Returns false if failed (item is there, or no room)
    // Note that this *CAN* delete item!  This occurs if it gets
    // merged into a stack in the inventory.
    bool	 acquireItem(ITEM *item, int sx, int sy, ITEM **newitem = 0);
    bool	 acquireItem(ITEM *item, ITEM **newitem = 0);
    // Finds first free item slot, returns false if none.
    bool	 findItemSlot(int &sx, int &sy) const;
    // Gets the item given a slot.  Possible null.
    ITEM	*getItem(int sx, int sy) const;
    // Returns a random item from our inventory, null if there are no
    // items.
    ITEM	*randomItem() const;
    // Unlinks & returns the item from the given slot.
    ITEM	*dropItem(int sx, int sy);

    // Destroys all items in this inventory.
    void	 destroyInventory();

    // Reverses the order of our inventory.  Theoritically this
    // should be a no-op, but our quiver uses this order.
    void	 reverseInventory();

    // Drops one of the rings the creature is wearing, provide
    // at least two rings are worn.
    void	 dropOneRing();

    // Gets the item from the specified equipped slot (same as
    // getItem(0, (int)name)
    ITEM	*getEquippedItem(ITEMSLOT_NAMES slot) const;

    // Determine if any item is equipped (faster than testing
    // each slot in turn...)
    bool	 hasAnyEquippedItems() const;

    bool	 hasSlot(ITEMSLOT_NAMES slot) const;
    // Note: May return 0 if there is no such slot!
    const char  *getSlotName(ITEMSLOT_NAMES slot) const;

    // Returns the first equipped item, if any, that grants the
    // given intrinsic.
    ITEM	*getSourceOfIntrinsic(INTRINSIC_NAMES intrinsic) const;

    // Returns the mob that inflicted the timed intrinsic, if any.
    MOB		*getInflictorOfIntrinsic(INTRINSIC_NAMES intrinsic) const;

    // This will create a new MOB for ourself.
    // All stashed mob references will be transfered to the new mob.
    // Polymorphing a morphed mob returns it to its original state.
    bool	 actionPolymorph(bool allowcontrol = true,
				 bool forcecontrol = false,
				 MOB_NAMES newdef = MOB_NONE);

    // This undoes the effect of polymorph.
    bool	 actionUnPolymorph(bool silent = false, 
				   bool systemshock = false);

    // This turns the creature into stone.
    bool	 actionPetrify();

    // This usually does nothing :>
    bool	 actionUnPetrify();

    HUNGER_NAMES getHungerLevel() const;

    void	 starve(int foodval);
    void	 feed(int foodval);

    // Applies system shock to this, 10% of maxhp lost
    void	 systemshock();

    // Attempts to save this creatures life.  Does not attempt
    // reverting to original form style saving, only INTRINSIC_LIFESAVED
    // type saving.
    // Returns true if critter saved, false if not saved.  Will destroy
    // source of saving.
    bool	 attemptLifeSaving(const char *deathmsg);

    // This marks this mob as having just died.  This will increment
    // the kill counts & also do proper victory stuff when the
    // user gets axed.
    // This does not mean any one gets the exp, or the mob won't be
    // back.  For example, kill/res loops will count, as will
    // petrifies.
    // becameitem flags if the monster died in a way in which it 
    // can come back
    void	 triggerAsDeath(ATTACK_NAMES attack, MOB *src, ITEM *weapon, bool becameitem);
    void	 triggerAsDeath(const ATTACK_DEF *attack, MOB *src, ITEM *weapon, bool becameitem);

    // This adds the given noise to this mob.
    void	 makeNoise(int noiselevel);
    // This adds any noise from the given slot.
    void	 makeEquipNoise(ITEMSLOT_NAMES slot);
    // This reveals the amount of noise this mob is making.
    // If the observer is null, it is the total noise.  If it is
    // a creature, it is a random number from 0 to max noise.  The trick
    // is that this will be the same for every call on this time slice
    // and with this pair of critters.
    int		 getNoiseLevel(const MOB *observer) const;

protected:    
    // Internal methods to handle intrinsic counters:
    INTRINSIC_COUNTER	*getCounter(INTRINSIC_NAMES intrinsic) const;
    void		 removeCounter(INTRINSIC_NAMES intrinsic);
    INTRINSIC_COUNTER	*addCounter(MOB *inflictor,
				INTRINSIC_NAMES intrinsic, int turns);

    // myDefinition is what we are now.  myOrigDefinition is
    // what we were when we were created.  Polymorph doesn't
    // create this difference, stuff like ghastify does.
    u8		 myDefinition, myOrigDefinition;
    u8		 myX, myY, myExpLevel;
    
    // These actually store twice our hit die to allow us to
    // gain fractional levels.
    u8		 myHitDie, myMagicDie;

    // This is the amount of noise the mob has made so far.  Not saved.
    u8		 myNoiseLevel;
    
    u8		 myDLevel;

    u8		 myGender;

    // Current state in AI FSM.  Stored to disk.
    u8		 myAIFSM;

    MOBREF	 myOwnRef;

    // When we are polymorphed, this is non-null and holds the base critter
    // type.
    MOBREF	 myBaseType;

    // Stores relevant AI state stuff.  Not saved.
    u16		 myAIState;

    // Food levels.
    u16		 myFoodLevel;
    
    // Note, HP had better be able to go negative.
    short	 myHP, myMaxHP;
    short	 myMP, myMaxMP;
    short	 myExp;
    MOB		*myNext;

    // AI related state information:
    MOBREF	 myAITarget;

    NAME	 myName;

    ITEM	*myInventory;

    INTRINSIC	 myIntrinsic;
    INTRINSIC	*myWornIntrinsic;

    INTRINSIC_COUNTER	*myCounters;

    static s8		ourAvatarDx, ourAvatarDy;

    static SPELL_NAMES	ourZapSpell;
    static ATTACK_NAMES	ourEffectAttack;
    static int		ourFireBallCount;
    static bool		ourAvatarOnFreshSquare;
    static bool		ourAvatarMoveOld;
};

#endif
