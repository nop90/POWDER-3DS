/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        item.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This provides the definition for all itmes.
 */

#ifndef __item_h__
#define __item_h__

#include "glbdef.h"
#include "grammar.h"
#include "mobref.h"
#include "name.h"

class SRAMSTREAM;

class MOB;
class INTRINSIC;
class INTRINSIC_COUNTER;
class MAP;
class ARTIFACT;

// One time initialization.
void item_init();

class ITEM
{
private:
    ITEM(); // Create items with ::create please.    
public:
    ~ITEM();

    static ITEM	*create(ITEM_NAMES definition, bool allowartifact = true,
			bool nocurse = false);

    // This returns the mundane item enum that goes with the exceptional
    // magic type.  ITEM_NONE is returned if it couldn't be found.
    static ITEM_NAMES lookupMagicItem(MAGICTYPE_NAMES magictype,
				int magicclass);

    static ITEM *createMagicItem(MAGICTYPE_NAMES magictype,
				 int magicclass,
				 bool allowartifact = true,
				 bool nocurse = false);

    // Creates a clone of this item.  All the local data will
    // match.  Note that the corpse mob will be cleared!  This is because
    // we have ownership of it, so bad stuff would happen if we cloned it.
    ITEM	*createClone() const;

    // Create any random item.
    static ITEM *createRandom(bool allowartifact = true);
    // Create a random of a given type.
    static ITEM *createRandomType(ITEMTYPE_NAMES type, 
				  bool allowartifact = true);
    static ITEM *createRandomFromTable(int *table, int max_prob,
				  bool allowartifact = true);

    // Creates a corpse from the given mob...
    static ITEM *createCorpse(MOB *mob);

    // Creates a statue from the given mob...
    static ITEM *createStatue(MOB *mob);
    
    static ITEM *load(SRAMSTREAM &is);

    static BUF	 buildAttackName(const ATTACK_DEF *attack);

    static void  loadGlobal(SRAMSTREAM &is);
    static void  saveGlobal(SRAMSTREAM &is);

    // This builds, or rebuilds all the itemtype tables....
    static void  	 init();

    void		 save(SRAMSTREAM &os);

    // Verifies any mob's that we may have attached.
    bool		 verifyMob() const;

    // Verifies the counter isn't present
    bool		 verifyCounterGone(INTRINSIC_COUNTER *counter) const;

    // This runs the heartbeat on the item.
    // Returns false if the item should be destroyed.
    bool		 doHeartbeat(MAP *map, MOB *owner);

    void		 setPos(int x, int y);
    int			 getX() const { return myX; }
    int			 getY() const { return myY; }
    int			 getEnchantment() const { return myEnchantment; }
    void		 enchant(int delta);

    // Turns this item into an artifact.
    void		 makeArtifact(const char *name = 0);

    // Cancels in the nethack sense into a plain-jane version
    // Vanilla, it should be noted, is hardly "ordinary", so this
    // is at best abuse of notation.
    void		 makeVanilla();

    // Raise, resurrect, etc:
    // Returns true if success
    // this is *not* deleted on success, so the caller should
    // get rid of it or there will be a soulless corpse
    // left behind.
    bool		 resurrect(MAP *map, MOB *resser, int x, int y);
    bool		 raiseZombie(MAP *map, MOB *raiser);
    bool		 raiseSkeleton(MAP *map, MOB *raiser);

    bool		 petrify(MAP *map, MOB *petrifier);
    bool		 unpetrify(MAP *map, MOB *unpetrifier);

    // Polymorphs this item.  Returns the newly created item on
    // success, 0 if failed.  Does not delete this on success!
    ITEM		*polymorph(MOB *polyer);

    // Tries to break this item.  The DC is a rough guess of
    // how strongly it should break.  The X/Y is the map location
    // where the break occurs.  The owner is null if no mob
    // currently owns the item.  If no owner, the item is assumed
    // to be in limbo.
    // The breaker is the mob responsible for breaking the item.
    // Returns true if the item is destroyed (It will be deleted then!)
    // If item is stacked, breaking it will only destroy one element
    // of the stack!  (And also not return true!)
    bool		 doBreak(int dc, MOB *breaker, MOB *owner,
				MAP *map, int x, int y);

    // Rename my base type.
    void		 renameType(const char *newname);

    // Rename myself.
    void		 rename(const char *newname);
    void		 rename(BUF buf);

    // Returns our own name, 0 if not assigned.
    const char		*getPersonalName() const;

    ITEM		*getNext() const { return myNext; }
    void		 setNext(ITEM *item) { myNext = item; }

    // Query methods to determine the nature of the item..
    MATERIAL_NAMES	 getMaterial() const;
    ATTACK_NAMES	 getAttackName() const;
    ATTACK_NAMES	 getThrownAttackName() const;
    const ATTACK_DEF	*getAttack() const;
    const ATTACK_DEF	*getThrownAttack() const;

    bool		 requiresLauncher() const;
    bool		 canLaunchThis(ITEM *launcher) const;

    SKILL_NAMES	 	 getAttackSkill() const;
    SKILL_NAMES		 getSpecialSkill() const;

    MOB			*getCorpseMob() const;
    MOB			*getStatueMob() const;

    SIZE_NAMES		 getSize() const;

    int			 getNoiseLevel() const;

    // This calculates average damage.  
    int			 calcAverageDamage() const;
    
    ITEM_NAMES		 getDefinition() const 
			 { return (ITEM_NAMES) myDefinition; }
    void		 setDefinition(ITEM_NAMES def)
			 { myDefinition = def; }
    ITEMTYPE_NAMES	 getItemType() const;

    const ITEM_DEF	&defn() const { return defn(getDefinition()); }
    static const ITEM_DEF &defn(ITEM_NAMES item) { return glb_itemdefs[item]; }

    // This returns whether the intrinsics of this item should
    // be granted even if it is just carried and not equipped.
    bool		 isCarryIntrinsic() const;

    // This returns true if the item is fully identified in every way.
    bool		 isFullyIdentified() const;
    // This returns whether the class is identified
    bool		 isIdentified() const;
    // This returns if the bless/cursed status is known.
    bool		 isKnownCursed() const;
    // Do we know if the item is not cursed?
    // Ie, have we tried it on and not gotten a stuck to our hands?
    bool		 isKnownNotCursed() const;
    // This returns if the enchantment is known.
    bool		 isKnownEnchant() const;
    // Return sif the charges is known.
    bool		 isKnownCharges() const;
    // Returns if poison status is known
    bool		 isKnownPoison() const;
    // Returns if class is known
    bool		 isKnownClass() const;
    // This marks both the specific item, and its class, as ided.
    // It identifies all aspects of this item.
    void		 markIdentified(bool silent = false);
    // This identifies only the class of the item (Ie; Wand of fireball)
    // rather than the entire thing (Ie: Wand of fireball (5))
    void		 markClassKnown(bool silent = false);
    // This marks that we now know the enchant status of this item.
    void		 markEnchantKnown();
    // This marks that we now know the cursed status of this item.
    void		 markCursedKnown();
    // Likewise, but we know it is merely not cursed.
    void		 markNotCursedKnown(bool isknown=true);
    // This marks we know the charge status.
    void		 markChargesKnown();
    void		 clearChargesKnown();
    // This means we know the poison status.
    void		 markPoisonKnown();

    // Determines if this is currently in some creatures pocket, or
    // on the world map.
    bool		 isInventory() const;
    void		 setInventory(bool isinventory);

    void		 makeCursed();
    void		 makeBlessed();
    bool		 isCursed() const;
    bool		 isBlessed() const;
    // Armor is all slots, so includes rings and amulets
    bool		 isArmor() const;
    bool		 isWeapon() const;
    bool		 isBoots() const;
    bool		 isHelmet() const;
    bool		 isShield() const;
    bool		 isJacket() const;
    bool		 isRing() const;
    bool		 isAmulet() const;
    bool		 isPassable() const;
    bool		 isQuivered() const;
    bool		 isFavorite() const;
    bool		 isMapped() const;
    bool		 isArtifact() const;
    // Returns the artifact data structure which describes this item.
    // Used internally to determine behaviour.
    const ARTIFACT	*getArtifact() const;

    // Dissolves this item, return true if it should be destroyed
    bool		 dissolve(MOB *dissolver = 0, bool *interesting = 0);

    // Determines if we can be dissolved by normal acid.
    bool		 canDissolve() const;

    // Ignites this item return true if it should be destroyed
    bool		 ignite(MOB *igniter = 0, bool *interesting = 0);

    // Electrifies this item
    bool		 electrify(int points, MOB *shocker, bool *interesting);

    // Determines if we will burn up by normal lava
    bool		 canBurn() const;

    // Douses this with water.
    // Returns true if should destroy item
    bool		 douse(MOB *douser = 0, bool *interesting = 0);
    
    // Returns the precedence the item should have for stacking on the
    // floor.  Low numbers go under high numbers.  Thus, impassables
    // such as Boulders should have a high stack, junk like corpses
    // low stack.
    int			 getStackOrder() const;
    void		 markMapped(bool mapstatus=true);
    void		 markQuivered(bool isquivered=true);
    void		 markFavorite(bool isfavorite=true);
    bool		 isBelowGrade() const;
    void		 markBelowGrade(bool belowgrade=true);

    void		 makePoisoned(POISON_NAMES poison, int charges);
    bool		 isPoisoned() const;
    void		 applyPoison(MOB *src, MOB *target, bool force=false);

    // This finds out if the item emits light, and if so, how far.
    bool	 	 isLight() const;
    int		 	 getLightRadius() const;
    TILE_NAMES		 getTile() const;
    MINI_NAMES		 getMiniTile() const;

    // Stack functions:  These manipulate stacks of items, which are
    // collections of identical items.
    int			 getStackCount() const { return myStackCount; }
    // Returns if we can merge with the given item.
    bool		 canMerge(ITEM *item) const;
    // Increments stack count.  Caller should likely delete item.  We
    // take the item as it's count size may be greater.  We may also
    // want to amortize some feature, such as a count down timer.
    void		 mergeStack(ITEM *item);
    // Pulls a substack off this stack.  The new ITEM * doesn't
    // belong anywhere, so should be acquired somewhere.  You can't
    // specify 0 or all items here - just move the pointer in that case!
    ITEM		*splitStack(int count=1);
    // Removes an item from this stack and immediately deletes the item
    // Does not handle the case of removing the last item!
    void		 splitAndDeleteStack(int count=1);

    // For those times when you want a specific number...
    // Do not use lightly.  Should only be done on item generation.
    void		setStackCount(int newcount) { myStackCount = newcount; }
    
    int			 getAC() const;
    int			 getCoolness() const;
    int			 getWeight() const;

    int			 getCharges() const;
    // This will automatically split the charges up among
    // the stack if this is a stacked item.
    void		 addCharges(int morecharges);
    void		 useCharge();
    void		 setAsPreserved();

    MAGICTYPE_NAMES	 getMagicType() const;
    int			 getMagicClass() const;

    // This merges into the given table all the intrinsics this item has.
    // It creates the table if it is 0 and there are intrinsics.
    void		 buildIntrinsicTable(INTRINSIC *&intrinsic) const;
    
    bool		 hasIntrinsic(INTRINSIC_NAMES intrinsic) const;

    // Equivalent to glb_itemdefs[item].name, except handles the
    // case of the item being identified by returning the identified name
    static const char	*getItemName(ITEM_NAMES item, bool forceid=false);
    
    // Returns the base name devoid of identifiers
    const char		*getRawName(bool idclass=false) const;
    BUF			 getName(bool article=true, bool shortname=false,
				 bool neverpronoun=false, 
				 bool idclass=false,
				 bool forcesingle=false) const;

    // Much like monster formatting.  
    // %U refers to the item.  owner isn't part of format string but
    // is used to determine who spams the message.
    void		 formatAndReport(MOB *owner, const char *str);
    void		 formatAndReport(const char *str);

    // Pages a list of all identified item types.
    static void		 viewDiscoveries();

    void		 viewDescription() const;
    // Send the description to the paging system.
    void		 pageDescription(bool brief) const;

    // Returns it, they, etc.
    const char		*getPronoun() const;

    // your, its, his, her, their, etc.
    const char  	*getPossessive() const;
    
    // Returns itself, themself, etc.
    const char 		*getReflexive() const;

    // Returns it, them, etc.
    const char		*getAccusative() const;

    BUF			 conjugate(const char *infinitive, bool past=false) const;

    // This gets the tense of this item, which includes the plurality of it.
    VERB_PERSON		 getPerson() const;

    // Actions.
    // Yep, items can do actions too.

    // This dips the dippee into this, with dipper being the entity
    // responsible.  Neither this nor dipper should be owned by anyone.
    // The result of the dip is two items which should be added back in,
    // NULL if they were destroyed.
    bool		 actionDip(MOB *dipper, ITEM *dippee,
				ITEM *&newpotion, ITEM *&newitem);

    // This zaps this, with zapper being the entity responsible.
    // this is assumed to be owned by the zapper.
    // true is returned if an action is consumed.
    // dx == dy == dz == 0 means zap self.
    // dz == +/-1 means ceiling/floor.
    // this MAY be destroyed by this function!
    bool		 actionZap(MOB *zapper,
			    int dx, int dy, int dz);

    // Zap callback methods
    bool		 zapCallback(int x, int y);
    static bool		 zapCallbackStatic(int x, int y, bool final, 
					    void *data);

    // Grenade callback methods
    bool		 grenadeCallback(int x, int y);
    static bool		 grenadeCallbackStatic(int x, int y, bool final, 
						void *data);

    // Ensure external invokers of grenadeCallback can get the proper
    // credit.
    static void		 setZapper(MOB *mob) { ourZapper.setMob(mob); }

    // Teleport the item to a randomly selected square.
    // Map is level to teleport on.
    bool 		 randomTeleport(MAP *map, bool mapownsitem, 
					MOB *teleporter);

    // Make the item fall into a hole and drop into the level below.
    // Pass the map in as the fall may be recursive, if you are unlucky.
    bool 		 fallInHole(MAP * curLevel, bool mapownsitem,
				    MOB *dropper);

protected:
    // This data must be kept as limitted as possible, as there
    // are a lot of items.
    // Make sure this is all initialized in ::create and copied in
    // ::createClone.
    u8			 myDefinition;
    u8			 myStackCount;
    u8			 myX, myY;
    s8			 myEnchantment;
    u8			 myCharges;
    u8			 myPoison;
    u8			 myPoisonCharges;
    ITEMFLAG1_NAMES	 myFlag1;

    MOBREF		 myCorpseMob;

    NAME		 myName;

    ITEM		*myNext;

    static MOBREF	 ourZapper;
};

#endif
