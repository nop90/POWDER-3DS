/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        item.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Implementation of the item manipulation routines
 */

#include <mygba.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "assert.h"
#include "artifact.h"
#include "item.h"
#include "itemstack.h"
#include "creature.h"
#include "gfxengine.h"
#include "msg.h"
#include "map.h"
#include "sramstream.h"
#include "mobref.h"
#include "victory.h"
#include "piety.h"
#include "encyc_support.h"

//
// ITEM defintions
//

// Global data:

// This is a list of what true magic item each mundane item will correspond
// to.  What type it refers to varies depending on the item's type, ie:
// potions will refer to POTION_.
u8		glb_magicitem[NUM_ITEMS];
// This tracks if each given item type has been ided.  This is different from
// a specic item has been ided.  A generically ided item will properly
// index its glb_magicitem class to give its type (acid potion rather than
// purple potion), a specific id will do this and its own state (bless vs curse)
bool		glb_itemid[NUM_ITEMS];

// This is the overloaded name someone has given this dude.
// It has been strduped so should be freed.
char		*glb_itemnames[NUM_ITEMS];

int 		 glbItemCount = 0;

// This is for the callback to track who did the zapping.
MOBREF		 ITEM::ourZapper;

void
item_init()
{
    memset(glb_itemnames, 0, sizeof(char *) * NUM_ITEMS);
}

ITEM::ITEM()
{
    myNext = 0;
    myName.setName(0);
    glbItemCount++;
}

ITEM::~ITEM()
{
    MOB		*mob;

    mob = myCorpseMob.getMob();

    if (mob)
    {
	// This MUST be safe, as we own it!
	// We want to punish the owner if we are a familiar.
	// This will catch the case of corpses decaying...
	// (Or being eaten!)
	// One problem is I think double jeapordy for smashing a statue
	// of your familiar.  But such evil scum that do that deserve
	// to be punished.
	if (mob->hasIntrinsic(INTRINSIC_FAMILIAR))
	{
	    MOB		*master;
	    master = mob->getMaster();
	    if (master)
	    {
		master->systemshock();

		master->formatAndReport("%U <feel> a great loss.");
	    }
	}

	delete mob;
    }

    glbItemCount--;
}

void
item_randomizetype(MAGICTYPE_NAMES magictype, int num)
{
    int			 i, j;
    u8			*lut;
    
    lut = new u8[num];
    for (i = 0; i < num; i++)
    {
	lut[i] = i;
    }
    // Now, mix randomly...
    rand_shuffle(lut, num);

    // Now, run through our items and assign the relevant type for each
    // potion, asserting we have enough!
    j = 0;
    for (i = 0; i < NUM_ITEMS; i++)
    {
	if (glb_itemdefs[i].magictype == magictype)
	{
	    glb_magicitem[i] = lut[j++];
	    UT_ASSERT(j <= num);
	}
    }
    delete [] lut;

    // Verify we gave away each item exactly once.
    UT_ASSERT(j == num);
}

void
ITEM::init()
{
    int		 i;

    // Clear the item names.
    for (i = 0; i < NUM_ITEMS; i++)
    {
	if (glb_itemnames[i])
	    free(glb_itemnames[i]);
	glb_itemnames[i] = 0;
    }
    
    // Build all the magic lookup tables...
    memset(glb_magicitem, 0, NUM_ITEMS);

    // Clear out the id flags.
    memset(glb_itemid, 0, NUM_ITEMS);

    // Now, for each of the special types, enumerate them...
    item_randomizetype(MAGICTYPE_POTION, NUM_POTIONS);
    item_randomizetype(MAGICTYPE_SCROLL, NUM_SCROLLS);
    item_randomizetype(MAGICTYPE_RING, NUM_RINGS);
    item_randomizetype(MAGICTYPE_HELM, NUM_HELMS);
    item_randomizetype(MAGICTYPE_BOOTS, NUM_BOOTSS);
    item_randomizetype(MAGICTYPE_AMULET, NUM_AMULETS);
    item_randomizetype(MAGICTYPE_WAND, NUM_WANDS);
    item_randomizetype(MAGICTYPE_SPELLBOOK, NUM_SPELLBOOKS);
    item_randomizetype(MAGICTYPE_STAFF, NUM_STAFFS);
}

ITEM *
ITEM::createClone() const
{
    ITEM	*clone;

    clone = ITEM::create((ITEM_NAMES) myDefinition);

    clone->myStackCount = myStackCount;
    clone->myX = myX;
    clone->myY = myY;
    clone->myName = myName;
    clone->myEnchantment = myEnchantment;
    clone->myCharges = myCharges;
    clone->myPoison = myPoison;
    clone->myPoisonCharges = myPoisonCharges;
    clone->myFlag1 = myFlag1;
    clone->myCorpseMob.setMob(0);

    return clone;
}

bool
ITEM::petrify(MAP *map, MOB *petrifier)
{
    // Mounds of flesh turn into boulders.
    // Things made of flesh become stones.

    if (myDefinition == ITEM_MOUNDFLESH)
    {
	myDefinition = ITEM_BOULDER;
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISBLOCKING);
	formatAndReport("The mound of flesh hardens into a boulder.");
	return true;
    }

    if (getMaterial() == MATERIAL_FLESH)
    {

	if (defn().isquest)
	{
	    // No petrification of quest items!
	    formatAndReport("%U <resist> the petrification!");
	    return false;
	}

	// This is made of flesh.  Should turn into a rock.
	formatAndReport("%U <harden> into a rock.");

	myDefinition = ITEM_ROCK;
	return true;
    }

    return false;
}

bool
ITEM::unpetrify(MAP *map, MOB *unpetrifier)
{
    if (myDefinition == ITEM_STATUE)
    {
	// This is at least possible
	MOB		*mob;

	mob = myCorpseMob.getMob();

	if (!mob)
	{
	    // Mobless statue.
	    formatAndReport("%U, being but a work of art, <collapse> into a meatball.");
	    myDefinition = ITEM_MEATBALL;
	    return true;
	}

	// Bring back to life
	mob->clearIntrinsic(INTRINSIC_DEAD);
	// Register on the map.
	mob->move(getX(), getY(), true);
	map->registerMob(mob);

	// We may have somehow been looted in the meantime.
	mob->rebuildAppearance();
	mob->rebuildWornIntrinsic();

	mob->formatAndReport("The statue of %U comes to life!");

	// Important so we don't delete ourselves :>
	myCorpseMob.setMob(0);
	map->dropItem(this);
	delete this;

	return true;
    }

    if (myDefinition == ITEM_BOULDER)
    {
	// Becomes a mound of flesh.
	formatAndReport("%U <collapse> into a mound of flesh.");
	myDefinition = ITEM_MOUNDFLESH;
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISBLOCKING);

	return true;
    }

    if (getMaterial() == MATERIAL_STONE)
    {
	if (defn().isquest)
	{
	    // No petrification of quest items!
	    formatAndReport("%U <remain> stony!");
	    return false;
	}

	// Transforms into a meatball.
	formatAndReport("%U <collapse>...");
	myDefinition = ITEM_MEATBALL;
	formatAndReport("into %U.");
	return true;
    }
    
    return false;
}

ITEM *
ITEM::polymorph(MOB *polyer)
{
    // Flavour text...
    formatAndReport(polyer, "A shimmering cloud engulfs %U.");

    // Check for poly-immune items.
    if (defn().isquest || hasIntrinsic(INTRINSIC_UNCHANGING) || isArtifact())
    {
	formatAndReport("%U <shudder>.");
	return 0;
    }

    // Create a new item of the same type.
    ITEMTYPE_NAMES		itemtype;

    itemtype = getItemType();

    // Note artifacts can never arise from polying.
    ITEM	*newitem;

    newitem = ITEM::createRandomType(itemtype, false);

    newitem->formatAndReport(polyer, "The cloud dissipates revealing %U.");

    return newitem;
}

bool
ITEM::resurrect(MAP *map, MOB *resser, int x, int y)
{
    if (myDefinition == ITEM_BONES || myDefinition == ITEM_CORPSE)
    {
	// This is at least possible
	MOB		*mob;

	mob = myCorpseMob.getMob();

	if (!mob)
	{
	    // This likely should be an assert: Why does a corpse/bones
	    // lack a mob?
	    resser->reportMessage("The spirit has travelled too far.");
	    
	    return false;
	}

	// See if we can find room in which to resurrect the mob.
	// The closest valid square is used.
	// We or in STD_SWIM here as we want creatures brought back
	// inside water to stay in the water.
	if (!map->findCloseTile(x, y, (MOVE_NAMES) (mob->getMoveType() | MOVE_STD_SWIM)))
	{
	    formatAndReport("%U <twitch> feebily.");
	    return false;
	}
	
	// Bring back to the verge of death.
	mob->setMinimalHP();
	if (mob->canResurrectFromCorpse())
	{
	    // Trolls get full hit points.
	    mob->setMaximalHP();
	}
	mob->clearIntrinsic(INTRINSIC_DEAD);

	// Register on the map.
	mob->move(x, y, true);
	map->registerMob(mob);
	
	// The mob may have been looted on death.  Thus, we rebuild
	// it's worn intrinisics.
	mob->rebuildWornIntrinsic();
	mob->rebuildAppearance();

	mob->formatAndReport("%U <return> from the dead!");

	// After being resurrected, one is, of course, thankful.
	mob->makeSlaveOf(resser);

	// Important so we don't delete ourselves :>
	myCorpseMob.setMob(0);

	// Drop back into whatever liquid we were in.  This is a bit
	// mean as pets that die in pits will insta die on resurrecting
	// as they will fall back into the pit.  However, you can easily
	// fix this by carrying the corpse out.
	map->dropMobs(mob->getX(), mob->getY(), true, resser);

	return true;
    }
    return false;
}

bool
ITEM::raiseZombie(MAP *map, MOB *raiser)
{
    if (myDefinition == ITEM_CORPSE)
    {
	// This is at least possible
	MOB		*mob, *zombie;

	// It doesn't matter if the mob is there, it just allows
	// us to get extra hits.
	mob = myCorpseMob.getMob();

	// Note we don't clear out myCorpseMob which means that
	// if the mob is a familiar we will be punished by zombifying
	// it.
	// On reflection, I think this is appropriate.

	zombie = MOB::create(MOB_ZOMBIE);
	zombie->destroyInventory();
	if (mob)
	{
	    zombie->forceHP(mob->getMaxHP());
	    zombie->setOrigDefinition(mob->getDefinition());
	}

	// Register on the map.
	zombie->move(getX(), getY(), true);
	map->registerMob(zombie);

	zombie->formatAndReport("%U <rise> from the dead!");

	// The zombie is marked as tame...
	zombie->makeSlaveOf(raiser);

	return true;
    }
    return false;
}

bool
ITEM::raiseSkeleton(MAP *map, MOB *raiser)
{
    if (myDefinition == ITEM_BONES)
    {
	// This is at least possible
	MOB		*mob, *skeleton;

	// It doesn't matter if the mob is there, it just allows
	// us to get extra hits.
	mob = myCorpseMob.getMob();

	skeleton = MOB::create(MOB_SKELETON);
	skeleton->destroyInventory();
	if (mob)
	{
	    skeleton->forceHP(mob->getMaxHP());
	    skeleton->setOrigDefinition(mob->getDefinition());
	}

	// Register on the map.
	skeleton->move(getX(), getY(), true);
	map->registerMob(skeleton);

	skeleton->formatAndReport("%U <rise> from the dead!");

	// The skeleton is marked as tame...
	skeleton->makeSlaveOf(raiser);

	return true;
    }
    return false;
}

bool
ITEM::doBreak(int dc, MOB *breaker, MOB *owner, MAP *map, int x, int y)
{
    bool		shouldbreak = false;
    bool		deleteself = false;
    
    // Determine if this DC is enough to break the item...
    
    // For now, potions break...
    if (getItemType() == ITEMTYPE_POTION)
    {
	shouldbreak = true;
    }

    if (getDefinition() == ITEM_BOTTLE)
	shouldbreak = true;

    if (getDefinition() == ITEM_MINDACIDHAND)
    {
	deleteself = true;
	shouldbreak = true;
    }

    // TODO: Build break chance into item properties,
    // can have Glass Swords then.
    if (getDefinition() == ITEM_ARROW || getDefinition() == ITEM_FIREARROW)
    {
	// Blessed arrows much less likely to break than cursed.
	int		chance;

	chance = 10;
	if (isCursed())
	    chance = 50;
	if (isBlessed())
	    chance = 1;

	// A bit more likely to break...
	if (getDefinition() == ITEM_FIREARROW)
	    chance *= 2;

	// If the breaker is a ranger, we don't break!
	if (breaker && breaker->hasIntrinsic(INTRINSIC_DRESSED_RANGER))
	    chance = 0;
	
	if (rand_chance(chance))
	{
	    deleteself = true;
	    shouldbreak = true;
	}
    }

    // Exit early if there is no breakage.
    if (!shouldbreak)
	return false;

    // Stash this before piety as piety may kill us.
    ourZapper.setMob(breaker);

    // Provide the breaker with piety...
    if (breaker)
	breaker->pietyBreak(this);

    // Switch on type & hence effect.
    switch (getItemType())
    {
	case ITEMTYPE_MISC:
	{
	    if (getDefinition() == ITEM_BOTTLE)
	    {
		// Empty bottles grenade...
		// FALL THROUGH
	    }
	    else
		break;
	}

	case ITEMTYPE_POTION:
	{
	    // Potions are always fully destroyed
	    deleteself = true;

	    // Give the appropriate dialog...
	    formatAndReport("%U <shatter>.");

	    if (map)
	    {
		map->fireBall(x, y, 1 + isBlessed() - isCursed(), true,
				grenadeCallbackStatic,
				this);
	    }

	    break;
	}

	case ITEMTYPE_AMULET:
	case ITEMTYPE_WEAPON:
	case ITEMTYPE_ARMOUR:
	case ITEMTYPE_RING:
	case ITEMTYPE_SCROLL:
	case ITEMTYPE_SPELLBOOK:
	case ITEMTYPE_WAND:
	case ITEMTYPE_STAFF:
	case NUM_ITEMTYPES:
	case ITEMTYPE_NONE:
	case ITEMTYPE_ANY:
	case ITEMTYPE_ARTIFACT:
	    break;
    }


    // Destroy our self if required.
    if (deleteself)
    {
	// If we are stacked, we can escape actually breaking...
	if (getStackCount() > 1)
	{
	    ITEM		*top;

	    // This is very round about.  The theory goes we may
	    // do something zany in splitStack or delete...
	    top = splitStack();
	    delete top;

	    // Well, it broke, but the ITEM * is still valid...
	    return false;
	}
	
	if (owner)
	{
	    owner->dropItem(getX(), getY());
	    owner->rebuildAppearance();
	    owner->rebuildWornIntrinsic();
	}
	delete this;
    }

    // Breakage occurred!
    return true;
}

void
ITEM::renameType(const char *newname)
{
    if (glb_itemnames[myDefinition])
	free(glb_itemnames[myDefinition]);
    if (newname && newname[0])
    {
	glb_itemnames[myDefinition] = strdup(newname);
    }
    else
    {
	glb_itemnames[myDefinition] = 0;
    }
}

void
ITEM::rename(const char *newname)
{
    myName.setName(newname);
}

void
ITEM::rename(BUF buf)
{
    rename(buf.buffer());
}

const char *
ITEM::getPersonalName() const
{
    return myName.getName();
}

ITEM *
ITEM::create(ITEM_NAMES definition, bool allowartifact, bool nocurse)
{
    ITEM		*item;

    item = new ITEM();

    item->myDefinition = definition;
    item->myStackCount = rand_dice(glb_itemdefs[item->myDefinition].stacksize);
    item->myX = item->myY = 0;
    item->myFlag1 = (ITEMFLAG1_NAMES)glb_itemdefs[item->myDefinition].flag1;
    item->myNext = 0;
    item->myEnchantment = 0;
    item->myCharges = 0;

    item->myPoison = POISON_NONE;
    item->myPoisonCharges = 0;

    // Determine cursed state...
    CURSECHANCE_NAMES	cursechance;

    cursechance = (CURSECHANCE_NAMES) 
		    glb_itemdefs[item->myDefinition].cursechance;
    // Do indirection on special magic types...
    switch ((MAGICTYPE_NAMES) glb_itemdefs[item->myDefinition].magictype)
    {
	case MAGICTYPE_POTION:
	    cursechance = (CURSECHANCE_NAMES) glb_potiondefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_SCROLL:
	    cursechance = (CURSECHANCE_NAMES) glb_scrolldefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_RING:
	    cursechance = (CURSECHANCE_NAMES) glb_ringdefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_HELM:
	    cursechance = (CURSECHANCE_NAMES) glb_helmdefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_BOOTS:
	    cursechance = (CURSECHANCE_NAMES) glb_bootsdefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_AMULET:
	    cursechance = (CURSECHANCE_NAMES) glb_amuletdefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_WAND:
	    cursechance = (CURSECHANCE_NAMES) glb_wanddefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_SPELLBOOK:
	    cursechance = (CURSECHANCE_NAMES) glb_spellbookdefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_STAFF:
	    cursechance = (CURSECHANCE_NAMES) glb_staffdefs[glb_magicitem[item->myDefinition]].cursechance;
	    break;
	case MAGICTYPE_NONE:
	    break;

	case NUM_MAGICTYPES:
	    UT_ASSERT(!"Unknown magic type!");
	    break;
    }

    // Probabilities of each.
    int		curse, normal, bless;

    curse = glb_cursechancedefs[cursechance].curse;
    normal = glb_cursechancedefs[cursechance].normal;
    bless = glb_cursechancedefs[cursechance].bless;

    UT_ASSERT(curse + normal + bless == 100);
    int		percent;
    percent = rand_choice(100);
    if (!nocurse)
    {
	if (percent < curse)
	    item->myFlag1 = (ITEMFLAG1_NAMES) (item->myFlag1 | ITEMFLAG1_ISCURSED);
	else if (percent < curse + bless)
	    item->myFlag1 = (ITEMFLAG1_NAMES) (item->myFlag1 | ITEMFLAG1_ISBLESSED);
    }

    // Determine enchantment.  Only certain types of items will
    // have a default enchantment.
    // You can enchant anything, however.
    if (item->isBoots() || item->isHelmet() || item->isShield() || 
	item->isJacket() || item->isWeapon())
    {
	// Armour enchant or weapon
	// We have a 10% chance of +/-, 50-50 for each.
	// This gives 1% for +2 or -2, 18% chance for +1 and 18% chance for -1.
	// Bah, not very good.  This makes 40% of items enchanted...
	// Thus, we hard code:
	// 1% +2
	// 9% +1
	// 9% -1
	// 1% -2
	// This is also silly, as we get limitted to +/-2.
	// Instead, just use a power distribution and have 10% less chance
	// for every bonus.
	// The major flaw with this is that we go from a 20% enchanted
	// to a 10% enchanted.  Thus, we encompsass the first test
	// with a 20% check.
	
	// Randomly determine if we will increase or decrease the enchantment.
	int			enchantdir;
	if (rand_choice(2))
	    enchantdir = 1;
	else
	    enchantdir = -1;

	item->myEnchantment = 0;

	if (rand_chance(20))
	{
	    item->enchant(enchantdir);

	    // Each additional level of enchantment has 10% chance of going
	    // through.
	    // This loop will terminate.  I hope.
	    // Call me paranoid...
	    int		maxenchant = 50;
	    while (rand_chance(10))
	    {
		item->enchant(enchantdir);
		maxenchant--;
		if (!maxenchant)
		{
		    UT_ASSERT(!"Excessive enchantment!");
		    break;
		}
	    }
	}

	// If the enchantment was negative, we get an extra 30% chance
	// of it being (more) cursed.
	if (item->myEnchantment < 0 && rand_chance(30))
	    item->makeCursed();
	// Otherwise, if it is positive, there is a chance for blessed.
	if (item->myEnchantment > 0 && rand_chance(30))
	    item->makeBlessed();
    }

    // Add in charges...
    if (item->getMagicType() == MAGICTYPE_WAND)
    {
	item->myCharges = rand_dice(glb_wanddefs[item->getMagicClass()].charges);
    }

    if (item->getMagicType() == MAGICTYPE_SPELLBOOK)
    {
	item->myCharges = rand_dice(glb_spellbookdefs[item->getMagicClass()].charges);
    }

    // One in a hundred mundanes is a real artifact!  Woot!
    if (allowartifact && rand_chance(1))
	item->makeArtifact();
	
    return item;
}

ITEM_NAMES
ITEM::lookupMagicItem(MAGICTYPE_NAMES magictype, int magicclass)
{
    int		itemtype;
    
    for (itemtype = 0; itemtype < NUM_ITEMS; itemtype++)
    {
	if (glb_itemdefs[itemtype].magictype == magictype)
	{
	    if (glb_magicitem[itemtype] == magicclass)
	    {
		return (ITEM_NAMES) itemtype;
	    }
	}
    }

    return ITEM_NONE;
}

ITEM *
ITEM::createMagicItem(MAGICTYPE_NAMES magictype, int magicclass,
		      bool allowartifact,
		      bool nocurse)
{
    ITEM_NAMES	item;

    item = lookupMagicItem(magictype, magicclass);
    if (item != ITEM_NONE)
        return ITEM::create(item, allowartifact, nocurse);

    return 0;
}

ITEM *
ITEM::createRandom(bool allowartifact)
{
    int		table[NUM_ITEMS];
    int		i, max;
    int		prob;

    max = 0;
    for (i = 0; i < NUM_ITEMS; i++)
    {
	if (glb_itemdefs[i].magictype == MAGICTYPE_SCROLL)
	    prob = glb_scrolldefs[glb_magicitem[i]].rarity;
	else
	    prob = glb_itemdefs[i].rarity;

	prob *= ((int)glb_itemtypedefs[glb_itemdefs[i].itemtype].rarity);
	table[i] = prob;
	max += prob;
    }
    
    return createRandomFromTable(table, max, allowartifact);
}

ITEM *
ITEM::createRandomType(ITEMTYPE_NAMES type, bool allowartifact)
{
    int		table[NUM_ITEMS];
    int		i, max;
    int		prob;

    // Special case item types.
    if (type == ITEMTYPE_ANY)
	return ITEM::createRandom(allowartifact);

    if (type == ITEMTYPE_ARTIFACT)
    {
	ITEM		*item;

	item = ITEM::createRandom(false);
	item->makeArtifact();
	return item;
    }

    max = 0;
    for (i = 0; i < NUM_ITEMS; i++)
    {
	if (glb_itemdefs[i].itemtype == type)
	{
	    if (glb_itemdefs[i].magictype == MAGICTYPE_SCROLL)
		prob = glb_scrolldefs[glb_magicitem[i]].rarity;
	    else
		prob = glb_itemdefs[i].rarity;
	}
	else
	    prob = 0;

	table[i] = prob;
	max += prob;
    }
    
    return createRandomFromTable(table, max, allowartifact);
}

ITEM *
ITEM::createRandomFromTable(int *table, int max_prob, bool allowartifact)
{
    int		item, i;

    if (!max_prob)
    {
	UT_ASSERT(!"No such items.");
	return 0;
    }

    item = rand_choice(max_prob);
    for (i = 0; i < NUM_ITEMS; i++)
    {
	item -= table[i];
	if (item < 0)
	    return create((ITEM_NAMES) i, allowartifact);
    }

    UT_ASSERT(!"Could not find item!");
    return 0;
}

ITEM *
ITEM::createCorpse(MOB *mob)
{
    ITEM		*item;

    item = ITEM::create(ITEM_CORPSE, false);
    item->myCorpseMob.setMob(mob);

    item->myCharges = 20;

    // Boneless critters get extra time as corpses as they have
    // no skeleton state.
    if (mob->isBoneless())
	item->myCharges += 20;

    return item;
}

ITEM *
ITEM::createStatue(MOB *mob)
{
    ITEM		*item;

    item = ITEM::create(ITEM_STATUE, false);
    item->myCorpseMob.setMob(mob);

    return item;
}

void
ITEM::setPos(int x, int y)
{
    myX = x;
    myY = y;
}

void
ITEM::enchant(int delta)
{
    int		newval;

    newval = myEnchantment + delta;

    if (newval < -99)
	newval = -99;
    if (newval > 99)
	newval = 99;

    myEnchantment = newval;
}

void
ITEM::makeArtifact(const char *name)
{
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISARTIFACT);

    if (!name)
	rename(artifact_buildname(getDefinition()));
    else
	rename(name);
}

void
ITEM::makeVanilla()
{
    // Ensure it isn't artifact.
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISARTIFACT);
    rename(0);
    makePoisoned(POISON_NONE, 0);
    myEnchantment = 0;
    myCharges = 0;

    // Ensure neither bless nor cursed.
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISBLESSED);
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISCURSED);
}

bool
ITEM::doHeartbeat(MAP *map, MOB *owner)
{
    // Determine if we are in a proper phase.
    if (!speed_isheartbeatphase())
	return true;

    // Run mind acid decay...
    if (myDefinition == ITEM_MINDACIDHAND)
    {
	if (myCharges)
	    myCharges--;
	else
	{
	    // Has fully decayed...
	    // The owner is confused.
	    if (owner)
	    {
		owner->formatAndReport("Mind-acid soaks into %R skin.");

		owner->setTimedIntrinsic(owner, INTRINSIC_CONFUSED,
					rand_dice(3, 5, 10));
	    }

	    // Request deletion.
	    return false;
	}
    }

    // Run decay...
    if (myDefinition == ITEM_CORPSE ||
	myDefinition == ITEM_BONES)
    {
	// Check for the "preserved" bit...
	if (myCharges == 255)
	    return true;
	    
	if (myCharges)
	    myCharges--;
	else
	{
	    // Has fully decayed...
	    if (myDefinition == ITEM_CORPSE)
	    {
		// We may want to resurrect ourselves.
		MOB		*mob;

		mob = myCorpseMob.getMob();
		if (mob)
		{
		    if (mob->canResurrectFromCorpse())
		    {
			// Trigger resurrection on self.
			if (owner)
			{
			    if (resurrect(glbCurLevel, mob, 
					    owner->getX(), owner->getY()))
			    {
				return false;
			    }
			}
			else
			{
			    if (resurrect(glbCurLevel, mob, 
					    getX(), getY()))
			    {
				return false;
			    }
			}
		    }
		    
		    if (mob->isBoneless())
		    {
			// Don't create bones...
			return false;
		    }
		}
		
		myDefinition = ITEM_BONES;
		myCharges = 20;
	    }
	    else
	    {
		// Erase.
		return false;
	    }
	}
    }

    return true;
}

MATERIAL_NAMES
ITEM::getMaterial() const
{
    // If we are a corpse, inherit from our base type.
    MOB		*corpse;

    corpse = getCorpseMob();
    if (corpse)
    {
	return corpse->getMaterial();
    }
    return (MATERIAL_NAMES) glb_itemdefs[myDefinition].material;
}

ATTACK_NAMES
ITEM::getAttackName() const
{
    return (ATTACK_NAMES) glb_itemdefs[myDefinition].attack;
}

ATTACK_NAMES
ITEM::getThrownAttackName() const
{
    return (ATTACK_NAMES) glb_itemdefs[myDefinition].thrownattack;
}

SKILL_NAMES
ITEM::getAttackSkill() const
{
    return (SKILL_NAMES) glb_itemdefs[myDefinition].attackskill;
}

SKILL_NAMES
ITEM::getSpecialSkill() const
{
    return (SKILL_NAMES) glb_itemdefs[myDefinition].specialskill;
}

const ATTACK_DEF *
ITEM::getAttack() const
{
    const ARTIFACT	*art;

    art = getArtifact();

    if (art && art->hasattack)
    {
	return &art->attack;
    }
    
    return &glb_attackdefs[glb_itemdefs[myDefinition].attack];
}

const ATTACK_DEF *
ITEM::getThrownAttack() const
{
    const ARTIFACT	*art;

    art = getArtifact();

    if (art && art->hasthrownattack)
    {
	return &art->thrownattack;
    }
    
    return &glb_attackdefs[glb_itemdefs[myDefinition].thrownattack];
}

bool
ITEM::requiresLauncher() const
{
    if (glb_itemdefs[myDefinition].launcher == ITEM_NONE)
	return false;

    return true;
}

bool
ITEM::canLaunchThis(ITEM *launcher) const
{
    if (!launcher)
	return false;

    if (launcher->getDefinition() == glb_itemdefs[myDefinition].launcher)
	return true;

    return false;
}

SIZE_NAMES
ITEM::getSize() const
{
    return (SIZE_NAMES) glb_itemdefs[myDefinition].size;
}

int
ITEM::getNoiseLevel() const
{
    return glb_itemdefs[myDefinition].noise;
}

MOB *
ITEM::getCorpseMob() const
{
    if (myDefinition != ITEM_CORPSE &&
	myDefinition != ITEM_BONES)
	return 0;
    return myCorpseMob.getMob();
}

MOB *
ITEM::getStatueMob() const
{
    if (myDefinition != ITEM_STATUE)
	return 0;

    return myCorpseMob.getMob();
}

int
ITEM::calcAverageDamage() const
{
    ATTACK_NAMES	 same;
    int			 dam = 0;
    const ATTACK_DEF	*attack, *sameattack;
    
    attack = getAttack();
    while (attack)
    {
	sameattack = attack;

	while (sameattack)
	{
	    dam += (sameattack->damage.myNumdie *
			(1 + sameattack->damage.mySides)) / 2;
	    dam += sameattack->damage.myBonus;
	    dam += getEnchantment();

	    // Non physical attacks are always cooler.
	    if (sameattack->element != ELEMENT_PHYSICAL)
		dam += 2;
	    
	    same = (ATTACK_NAMES)sameattack->sameattack;
	    if (same == ATTACK_NONE)
		sameattack = 0;
	    else
		sameattack = &glb_attackdefs[same];
	}
	if (attack->nextattack == ATTACK_NONE)
	    attack = 0;
	else
	    attack = &glb_attackdefs[attack->nextattack];
    }

    // Extra stuff: bonus for poisoned.
    if (isPoisoned())
    {
	dam += 4;
    }

    // Being made of silver is just cool.
    if (getMaterial() == MATERIAL_SILVER)
    {
	dam += 4;
    }
    return dam;
}

ITEMTYPE_NAMES
ITEM::getItemType() const
{
    return (ITEMTYPE_NAMES) glb_itemdefs[getDefinition()].itemtype;
}

bool
ITEM::isCarryIntrinsic() const
{
    const ARTIFACT		*art;

    // Artifacts may have carry intrinsics...
    art = getArtifact();
    if (art && art->iscarryintrinsic)
	return true;
    
    return glb_itemdefs[myDefinition].iscarryintrinsic;
}

bool
ITEM::isFullyIdentified() const
{
    return isIdentified() && isKnownCursed() &&
	   isKnownEnchant() && isKnownCharges() &&
	   isKnownPoison();
}

bool
ITEM::isIdentified() const
{
    return glb_itemid[myDefinition];
}

bool
ITEM::isKnownCursed() const
{
    return (myFlag1 & ITEMFLAG1_ISKNOWCURSE) ? true : false;
}

bool
ITEM::isKnownNotCursed() const
{
    return (myFlag1 & ITEMFLAG1_ISKNOWNOTCURSE) ? true : false;
}

bool
ITEM::isKnownEnchant() const
{
    return (myFlag1 & ITEMFLAG1_ISKNOWENCHANT) ? true : false;
}

bool
ITEM::isKnownCharges() const
{
    return (myFlag1 & ITEMFLAG1_ISKNOWCHARGES) ? true : false;
}

bool
ITEM::isKnownPoison() const
{
    return (myFlag1 & ITEMFLAG1_ISKNOWPOISON) ? true : false;
}

bool
ITEM::isKnownClass() const
{
    if (glb_itemid[myDefinition])
	return true;
    return false;
}

bool
ITEM::isInventory() const
{
    return (myFlag1 & ITEMFLAG1_ISINVENTORY) ? true : false;
}

bool
ITEM::isArtifact() const
{
    return (myFlag1 & ITEMFLAG1_ISARTIFACT) ? true : false;
}

const ARTIFACT *
ITEM::getArtifact() const
{
    if (!isArtifact())
	return 0;

    return artifact_buildartifact(myName.getName(), getDefinition());
}

void
ITEM::setInventory(bool isinventory)
{
    if (isinventory)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISINVENTORY);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISINVENTORY);
}

void
ITEM::markIdentified(bool silent) 
{
    markEnchantKnown();
    markCursedKnown();
    markChargesKnown();
    markPoisonKnown();

    markClassKnown(silent);
}

void
ITEM::markClassKnown(bool silent)
{
    // Avatar gets piety for this.
    if (!glb_itemid[myDefinition])
    {
	MOB		*avatar;

	avatar = MOB::getAvatar();
	if (avatar)
	{
	    // Only spam for interesting class changes.
	    if (!silent && getMagicType() != MAGICTYPE_NONE)
		avatar->formatAndReport("%U <learn> that %IU <I:be> %Ii.", this);
	    avatar->pietyIdentify((ITEM_NAMES) myDefinition);
	}
    }

    glb_itemid[myDefinition] = 1;
}

void
ITEM::markEnchantKnown()
{
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISKNOWENCHANT);
}

void
ITEM::markCursedKnown()
{
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISKNOWCURSE);
}

void
ITEM::markNotCursedKnown(bool newknown)
{
    if (newknown)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISKNOWNOTCURSE);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISKNOWNOTCURSE);
}

void
ITEM::markChargesKnown()
{
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISKNOWCHARGES);
}

void
ITEM::clearChargesKnown()
{
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & (~ITEMFLAG1_ISKNOWCHARGES));
}

void
ITEM::markPoisonKnown()
{
    myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISKNOWPOISON);
}

void
ITEM::makeCursed()
{
    if (myFlag1 & ITEMFLAG1_ISBLESSED)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISBLESSED);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISCURSED);

    // Regardless, anyone's theories about this are now wrong.
    // One could argue we should not clear this flag as if they
    // cursing was obvious we usually do a markCursedKnown, so if
    // that is not done it is truly the avatar that doesn't know
    // the change, and hence the player shouldn't as well.  
    // For now, I consider that logic too mean.
    markNotCursedKnown(false);
}

void
ITEM::makeBlessed()
{
    if (myFlag1 & ITEMFLAG1_ISCURSED)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISCURSED);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISBLESSED);
}

void
ITEM::makePoisoned(POISON_NAMES poison, int charges)
{
    myPoison = poison;
    myPoisonCharges = charges;
}

bool
ITEM::isPoisoned() const
{
    if (myPoison != POISON_NONE && myPoisonCharges)
	return true;
    return false;
}

void
ITEM::applyPoison(MOB *src, MOB *target, bool force)
{
    if (!force && rand_choice(glb_poisondefs[myPoison].modulus))
	return;

    if (!isPoisoned())
	return;

    // Is this a sensible time?
    target->setTimedIntrinsic( src, (INTRINSIC_NAMES) 
				glb_poisondefs[myPoison].intrinsic,
				rand_dice(3, 3, 0) );

    myPoisonCharges--;
}

bool
ITEM::isCursed() const
{
    return (myFlag1 & ITEMFLAG1_ISCURSED) ? true : false;
}

bool
ITEM::isBlessed() const
{
    return (myFlag1 & ITEMFLAG1_ISBLESSED) ? true : false;
}

bool
ITEM::isQuivered() const
{
    return (myFlag1 & ITEMFLAG1_ISQUIVERED) ? true : false;
}

void
ITEM::markQuivered(bool newquiver)
{
    if (newquiver)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISQUIVERED);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISQUIVERED);
    
}

bool
ITEM::isFavorite() const
{
    return (myFlag1 & ITEMFLAG1_ISFAVORITE) ? true : false;
}

void
ITEM::markFavorite(bool newfavorite)
{
    if (newfavorite)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISFAVORITE);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISFAVORITE);
    
}

bool
ITEM::isMapped() const
{
    return (myFlag1 & ITEMFLAG1_ISMAPPED) ? true : false;
}

void
ITEM::markMapped(bool val)
{
    if (val)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISMAPPED);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISMAPPED);
}

bool
ITEM::isBelowGrade() const
{
    return (myFlag1 & ITEMFLAG1_ISBELOWGRADE) ? true : false;
}

void
ITEM::markBelowGrade(bool val)
{
    if (val)
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 | ITEMFLAG1_ISBELOWGRADE);
    else
	myFlag1 = (ITEMFLAG1_NAMES) (myFlag1 & ~ITEMFLAG1_ISBELOWGRADE);
}

bool
ITEM::isWeapon() const
{
    // Not the simplest of computation as we can equip anything...
    if (getAttackName() != ATTACK_MISUSED)
	return true;
    return false;
}

bool
ITEM::isArmor() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) ? true : false;
}

bool
ITEM::isBoots() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) == ITEMFLAG1_ISBOOTS;
}

bool
ITEM::isHelmet() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) == ITEMFLAG1_ISHELMET;
}

bool
ITEM::isJacket() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) == ITEMFLAG1_ISJACKET;
}

bool
ITEM::isShield() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) == ITEMFLAG1_ISSHIELD;
}

bool
ITEM::isRing() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) == ITEMFLAG1_ISRING;
}

bool
ITEM::isAmulet() const
{
    return (myFlag1 & ITEMFLAG1_ARMORSLOTMASK) == ITEMFLAG1_ISAMULET;
}

bool
ITEM::isPassable() const
{
    return !(myFlag1 & ITEMFLAG1_ISBLOCKING);
}

int
ITEM::getStackOrder() const
{
    // This is used to force the sorting of items.  We want users
    // to quickly see if there are interesting drops so put corpses
    // on the bottom.  Likewise, impassable objects need to be known
    // or people won't know why they can't move there - this currently
    // only means boulders.
    // Finally, artifacts are put on the top to draw attention to them
    // as they are cool.
    if (myDefinition == ITEM_CORPSE ||
	myDefinition == ITEM_BONES)
    {
	return 0;
    }

    if (!isPassable())
	return 3;

    if (isArtifact())
	return 2;

    // Default level.
    return 1;
}

const char *
ITEM::getItemName(ITEM_NAMES item, bool forceid)
{
    const char		*rawname = 0;

    if (forceid || glb_itemid[item])
    {
	switch ((MAGICTYPE_NAMES) glb_itemdefs[item].magictype)
	{
	    case NUM_MAGICTYPES:
	    case MAGICTYPE_NONE:
		rawname = glb_itemdefs[item].name;
		break;
		
	    case MAGICTYPE_POTION:
		rawname = glb_potiondefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_SCROLL:
		rawname = glb_scrolldefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_RING:
		rawname = glb_ringdefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_HELM:
		rawname = glb_helmdefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_BOOTS:
		rawname = glb_bootsdefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_AMULET:
		rawname = glb_amuletdefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_WAND:
		rawname = glb_wanddefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_SPELLBOOK:
		rawname = glb_spellbookdefs[glb_magicitem[item]].name;
		break;

	    case MAGICTYPE_STAFF:
		rawname = glb_staffdefs[glb_magicitem[item]].name;
		break;
	}
    }
    else
    {
	rawname = glb_itemdefs[item].name;
    }

    return rawname;
}

const char *
ITEM::getRawName(bool idclass) const
{
    const char		*rawname = 0;

    rawname = ITEM::getItemName(getDefinition(), idclass);

    return rawname;
}

BUF
ITEM::getName(bool article, bool shortname, bool neverpronoun, bool idclass, bool forcesingle) const
{
    BUF			 rawname;
    bool		 ignorecurse = false;
    BUF			 result;

    rawname.reference(getRawName(idclass));

    // In some cases, the actual name comes from what mob it actually is.
    if (myDefinition == ITEM_BONES || myDefinition == ITEM_CORPSE ||
	myDefinition == ITEM_STATUE)    
    {
	MOB		*mob;

	mob = myCorpseMob.getMob();
	if (mob)
	{
	    BUF		buf, mobname;

	    mobname = mob->getName(false, false, true);

	    buf.sprintf("%s%s %s",
			(myCharges == 255 ? "preserved " : ""),
			mobname.buffer(), rawname.buffer());
	    rawname = buf;
	}
    }

    // We have a special case off the front here.  We don't want blessed
    // bottles of water to be "a holy bottle of water", but rather be
    // "a bottle of holy water".  This is because it is the water that
    // is affected, not the bottle.
    if (myDefinition == ITEM_WATER)
    {
	if (isKnownCursed())
	{
	    if (isCursed())
		rawname.reference("bottle of unholy water");
	    if (isBlessed())
		rawname.reference("bottle of holy water");

	    ignorecurse = true;
	}
    }
    
    // NB: This must handle every case which causes further tmp buffers
    // to be needed.
    if (!article && 
	(forcesingle || (getStackCount() == 1)) &&    
	(shortname || 
		((!isKnownCursed() || ignorecurse) && 
		 !isKnownEnchant() && !isKnownCharges() && 
		 !isKnownPoison() && !glb_itemnames[myDefinition] &&
		 !myName.getName())))
    {
	return rawname;
    }

    BUF			buf;

    if (shortname || 
	    ((!isKnownCursed() || ignorecurse) && !isKnownEnchant() && !isKnownCharges() && !isKnownPoison() && !glb_itemnames[myDefinition] && !myName.getName()))
    {
	// We require an article, but that is it.
	return gram_createcount(rawname, forcesingle ? 1 : getStackCount(), article);
    }
    else
    {
	// We first build the name without the article, as the prefix
	// may alter what the proper article is.  Eg: an evil sword.
	// The problem with this are cases where there should
	// be no article - in these cases, the presence of the prefix
	// may trick us into adding an unnecessary article, Eg:
	// an evil Baezl'bub's black heart.
	BUF		nonarticle;

	nonarticle.clear();
	
	if (isKnownCursed() && !ignorecurse)
	{
	    if (isCursed())
		nonarticle.strcat("evil ");
	    if (isBlessed())
		nonarticle.strcat("holy ");
	}

	if (isKnownPoison() && isPoisoned())
	{
	    nonarticle.strcat("poisoned ");
	}
	
	if (isKnownEnchant())
	{
	    // Don't waste space on +0 unless something where that
	    // counts.
	    if (getEnchantment() != 0 ||
		isBoots() || isHelmet() || isShield() || 
		isJacket() || isWeapon())
	    {
		BUF		tmp;

		tmp.sprintf("%+d ", getEnchantment());
		nonarticle.strcat(tmp);
	    }
	}

	nonarticle.strcat(rawname);
	
	if (article && isArtifact() && (forcesingle || (getStackCount() == 1)))
	{
	    buf.strcpy("the ");
	    buf.strcat(nonarticle);
	}
	else
	{
	    buf = gram_createcount(nonarticle, forcesingle ? 1 : getStackCount(), article);
	}

	// Now, we append the charge count if we know it, and the magictype
	// is something with charges.  Currently, that means wands or 
	// spellbooks.
	if (isKnownCharges() && 
	    (getMagicType() == MAGICTYPE_WAND ||
	     getMagicType() == MAGICTYPE_SPELLBOOK ||
	     getDefinition() == ITEM_LIGHTNINGRAPIER))
	{
	    BUF			tmp;

	    tmp.sprintf(" (%d)", myCharges);
	    buf.strcat(tmp);
	}

	if (myName.getName())
	{
	    // User named items are called.  Artifact items
	    // just *are* that name.  Ie: the long sword foobar.
	    if (isArtifact())
		buf.strcat(" ");
	    else
		buf.strcat(" called ");
	    buf.strcat(myName.getName());
	}
	else if (glb_itemnames[myDefinition])
	{
	    buf.strcat(" named ");
	    buf.strcat(glb_itemnames[myDefinition]);
	}
    }
    
    return buf;
}

void
ITEM::formatAndReport(const char *str)
{
    formatAndReport(0, str);
}
    
void
ITEM::formatAndReport(MOB *owner, const char *str)
{
    BUF		buf;

    if (!str)
	return;

    // Ignore if we are loading so we don't spam about re-wetting
    // sunk treasure.
    if (MAP::isLoading())
	return;

    buf = MOB::formatToString(str, 0, this, 0, 0);

    if (!owner && !isInventory())
    {
	// Spam our report to the map
	glbCurLevel->reportMessage(buf, getX(), getY());
    }
    else
    {
	if (owner)
	    owner->reportMessage(buf);
	else if (MOB::getAvatar())
	    MOB::getAvatar()->reportMessage(buf);
    }
}

void
ITEM::viewDiscoveries()
{
    ITEM_NAMES		item;
    int			num = 0;
    
    // Written while waiting for C967 to show up in the line
    // at the passport office.
    gfx_pager_addtext("Your discoveries:");
    gfx_pager_newline();
    gfx_pager_separator();

    FOREACH_ITEM(item)
    {
	// Check to see if this is a meaningful id
	// Unlike nethack, I don't think iding longswords is interesting.
	const char *basename, *idname;
	
	idname = ITEM::getItemName(item);
	basename = glb_itemdefs[item].name;
	// No need for strcmp here since we are using the 
	// prebuilt tables.
	if (idname != basename)
	{
	    BUF		buf;

	    buf.sprintf("%s:", basename);
	    gfx_pager_addtext(gram_capitalize(buf));
	    gfx_pager_newline();
	    buf.sprintf("  %s", idname);
	    gfx_pager_addtext(gram_capitalize(buf));
	    gfx_pager_newline();
	    num++;
	}
    }

    if (!num)
    {
	gfx_pager_addtext("You have learned nothing.");
	gfx_pager_newline();
    }

    gfx_pager_display();
}

void
ITEM::viewDescription() const
{
    pageDescription(false);

    gfx_pager_display();
}

BUF
ITEM::buildAttackName(const ATTACK_DEF *attack)
{
    BUF buf;

    buf.strcpy("");
    while (attack)
    {
	BUF		section;
	bool		skipbonus = false;

	if (attack->damage.myNumdie)
	{
	    if (attack->damage.mySides < 2)
	    {
		// Constant damage.  Fold in the bonus damage
		// and report just this.
		section.sprintf("%d",
			    attack->damage.myNumdie + attack->damage.myBonus);
		skipbonus = true;
		buf.strcat(section);
	    }
	    else
	    {
		section.sprintf("%d", 
			attack->damage.myNumdie);
		buf.strcat(section);
		section.sprintf("d%d", 
			attack->damage.mySides);
		buf.strcat(section);
	    }
	}
	if (!skipbonus && attack->damage.myBonus)
	{
	    section.sprintf("%+d", 
		attack->damage.myBonus);
	    buf.strcat(section);
	}
	if (attack->bonustohit)
	{
	    section.sprintf("[%+d]", attack->bonustohit);
	    buf.strcat(section);
	}
	section.sprintf(" (%s)", glb_elementdefs[attack->element].name);
	buf.strcat(section);

	if (attack->nextattack != ATTACK_NONE)
	{
	    buf.strcat(", {");

	    section = buildAttackName(&glb_attackdefs[attack->nextattack]);
	    buf.strcat(section);

	    buf.strcat("}");
	}
	
	if (attack->sameattack != ATTACK_NONE)
	{
	    buf.strcat(", ");
	    attack = &glb_attackdefs[attack->sameattack];
	}
	else
	    attack = 0;
    }

    return buf;
}

void
ITEM::pageDescription(bool brief) const
{
    BUF			buf;
    int			skilllevel;
    
    gfx_pager_addtext(gram_capitalize(getName()));
    gfx_pager_newline();

    // Add the identify info.
    const ARTIFACT		*art = getArtifact();
    if ((isIdentified() && !art) ||
	(isFullyIdentified() && art))
    {
	bool		hasany = false;
	INTRINSIC_NAMES	intrinsic;
	const char 	*prequel;

	if (!brief)
	{
	    gfx_pager_separator();
	}

	if (art)
	    prequel = "This artifact grants ";
	else
	    prequel = "This item grants ";

	for (intrinsic = INTRINSIC_NONE; intrinsic < NUM_INTRINSICS;
		intrinsic = (INTRINSIC_NAMES) (intrinsic + 1))
	{
	    if (hasIntrinsic(intrinsic))
	    {
		if (!hasany)
		{
		    gfx_pager_addtext(prequel);
		    hasany = true;
		}
		else
		    gfx_pager_addtext(", ");

		gfx_pager_addtext(glb_intrinsicdefs[intrinsic].name);
	    }
	}

	// Light radius:
	if (isLight())
	{
	    if (!hasany)
	    {
		gfx_pager_addtext(prequel);
		hasany = true;
	    }
	    else
		gfx_pager_addtext(", ");

	    buf.sprintf("light radius %d", getLightRadius());
	    gfx_pager_addtext(buf);
	}

	// Amour class
	if (getAC())
	{
	    if (!hasany)
	    {
		gfx_pager_addtext(prequel);
		hasany = true;
	    }
	    else
		gfx_pager_addtext(", ");

	    buf.sprintf("armour %d", getAC());
	    gfx_pager_addtext(buf);
	}

	// Finish off the special stuff list, and go to mandatory list.
	if (hasany)
	{
	    gfx_pager_addtext(".  ");
	    gfx_pager_newline();
	}

	if (isCarryIntrinsic())
	{
	    gfx_pager_addtext("Intrinsics are granted by carrying this item.");
	    gfx_pager_newline();
	}

	const ATTACK_DEF	*attack;
	
	gfx_pager_addtext("Melee: ");
	buf = buildAttackName(getAttack());
	gfx_pager_addtext(buf);
	gfx_pager_newline();

	gfx_pager_addtext("Thrown ");
	attack = getThrownAttack();

	buf.sprintf("%d: ", attack->range);
	gfx_pager_addtext(buf);
	
	buf = buildAttackName(attack);
	gfx_pager_addtext(buf);

	gfx_pager_newline();
    }

    // Skip out here if brief.
    if (brief)
	return;

    // Add the boring stats.
    gfx_pager_separator();

    buf.strcpy("Size: ");
    buf.strcat(gram_capitalize(glb_sizedefs[getSize()].name));
    gfx_pager_addtext(buf);
    gfx_pager_newline();

    buf.strcpy("Material: ");
    buf.strcat(gram_capitalize(glb_materialdefs[getMaterial()].name));
    gfx_pager_addtext(buf);
    gfx_pager_newline();

    buf.sprintf("Weight: %d",
	    glb_itemdefs[getDefinition()].weight);
    gfx_pager_addtext(buf);
    gfx_pager_newline();

    const char *noisetext;
    switch (getNoiseLevel())
    {
	case 0:
	    noisetext = "Silent";
	    break;
	case 1:
	    noisetext = "Quiet";
	    break;
	case 2:
	    noisetext = "Average";
	    break;
	case 3:
	    noisetext = "Loud";
	    break;
	default:
	case 4:
	    noisetext = "Very Loud";
	    break;
    }

    buf.sprintf("Noise: %s", noisetext);
    gfx_pager_addtext(buf);
    gfx_pager_newline();

    gfx_pager_addtext("Attack Style: ");
    buf.strcpy(glb_skilldefs[getAttackSkill()].name);
    // Remove first space.
    {
	char	*c;
	for (c = buf.evildata(); *c && !isspace(*c); c++);
	*c = '\0';
    }
    gfx_pager_addtext(buf);
    gfx_pager_newline();

    // Special skill, if any.
    if (getSpecialSkill() != SKILL_NONE)
    {
	gfx_pager_addtext("Special Skill: ");
	gfx_pager_addtext(glb_skilldefs[getSpecialSkill()].name);
	gfx_pager_newline();
    }

    // Calculate your attack ability.
    skilllevel = MOB::getAvatar()->getWeaponSkillLevel(this, ATTACKSTYLE_MELEE);
    if (skilllevel)
    {
	gfx_pager_addtext("Melee Skill: ");
	while (skilllevel--)
	{
	    gfx_pager_addtext("*");
	}
	gfx_pager_newline();
    }

    skilllevel = MOB::getAvatar()->getWeaponSkillLevel(this, ATTACKSTYLE_THROWN);
    if (skilllevel)
    {
	gfx_pager_addtext("Thrown Skill: ");
	while (skilllevel--)
	{
	    gfx_pager_addtext("*");
	}
	gfx_pager_newline();
    }

    skilllevel = MOB::getAvatar()->getArmourSkillLevel(this);
    if (skilllevel)
    {
	gfx_pager_addtext("Armour Skill: ");
	while (skilllevel--)
	{
	    gfx_pager_addtext("*");
	}
	gfx_pager_newline();
    }

    // Add encyclopedia description.
    if (encyc_hasentry("ITEM", getDefinition()))
    {
	gfx_pager_separator();
	encyc_pageentry("ITEM", getDefinition());
    }

    // Add the magical encylopedia entry
    if (isIdentified() && getMagicType() != MAGICTYPE_NONE)
    {
	if (encyc_hasentry(glb_magictypedefs[getMagicType()].cycname, getMagicClass()))
	{
	    gfx_pager_separator();
	    encyc_pageentry(glb_magictypedefs[getMagicType()].cycname, getMagicClass());
	}
    }
}

const char *
ITEM::getPronoun() const
{
    return gram_getpronoun(getPerson());
}

const char *
ITEM::getPossessive() const
{
    return gram_getpossessive(getPerson());
}

const char *
ITEM::getReflexive() const
{
    return gram_getreflexive(getPerson());
}

const char *
ITEM::getAccusative() const
{
    return gram_getaccusative(getPerson());
}

BUF
ITEM::conjugate(const char *infinitive, bool past) const
{
    return gram_conjugate(infinitive, getPerson(), past);
}

VERB_PERSON
ITEM::getPerson() const
{
    // Determine our plurality...
    if (myStackCount > 1)
    {
	// We are stacked, we are plural no matter what...
	return VERB_THEY;
    }

    const char		*name;

    // *cries*
    // Guess who called getPronoun inside of getName?
    name = getRawName();

    if (gram_isnameplural(name))
    {
	return VERB_THEY;
    }
    return VERB_IT;
}
    
bool
ITEM::isLight() const
{
    if (glb_itemdefs[myDefinition].lightradius)
	return true;

    // Check for ring of light...
    if (getMagicType() == MAGICTYPE_RING &&
	getMagicClass() == RING_LIGHT)
    {
	return true;
    }

    const ARTIFACT 	*art;
    art = getArtifact();

    if (art && art->lightradius)
	return true;

    return false;
}

int
ITEM::getLightRadius() const
{
    int		radius;

    radius = glb_itemdefs[myDefinition].lightradius;
    
    // Check for ring of light...
    if (getMagicType() == MAGICTYPE_RING &&
	getMagicClass() == RING_LIGHT)
    {
	if (radius < 3)
	    radius = 3;
    }

    const ARTIFACT		*art;

    art = getArtifact();
    if (art)
    {
	if (radius < art->lightradius)
	    radius = art->lightradius;
    }
    
    return radius;
}

TILE_NAMES
ITEM::getTile() const
{
    return (TILE_NAMES) glb_itemdefs[myDefinition].tile;
}

MINI_NAMES
ITEM::getMiniTile() const
{
    return (MINI_NAMES) glb_itemdefs[myDefinition].minitile;
}

//
// Stack Routines
//

bool
ITEM::canMerge(ITEM *item) const
{
    // Verify we match on all particulars.
    if (item->myDefinition != myDefinition)
	return false;

    // If either of our items have a name, we can't merge.
    if (item->myName.getName() || myName.getName())
    {
	// However, if both have a name
	if (item->myName.getName() && myName.getName())
	{
	    // And the names match
	    if (!strcmp(item->myName.getName(), myName.getName()))
	    {
		// We can merge!
	    }
	    else
		return false;
	}
	else
	    return false;
    }
    
    // Prohibit merging of artifacts unless both are artifacts.
    // Otherwise, naming an item the same as an artifact would allow
    // the creation of artifacts.
    if (item->isArtifact() != isArtifact())
	return false;

    // Prohibit merging of corpses!
    // This is required as we keep the mob * underneath them.
    // We also don't want to merge statues for the same reason.
    // Thus, if either item has a non-zero corpsemob, it is illegal.
    if (!item->myCorpseMob.isNull() || !myCorpseMob.isNull())
	return false;

    if (item->myEnchantment != myEnchantment)
	return false;

    if (item->myCharges != myCharges)
	return false;

    if (item->myPoison != myPoison)
	return false;

    if (item->myPoisonCharges != myPoisonCharges)
	return false;

    // Flags are a bit trickier, as we only care about some of them.

    if (item->isBlessed() != isBlessed())
	return false;
    if (item->isCursed() != isCursed())
	return false;

    // We don't want to reveal curse states by merging, so known items
    // won't merge with unknown.
    if (item->isKnownCursed() != isKnownCursed())
	return false;
    // We only care about enchantment amounts where it is armour or
    // weapon.
    if (getItemType() == ITEMTYPE_ARMOUR ||
	getItemType() == ITEMTYPE_WEAPON)
    {
	if (item->isKnownEnchant() != isKnownEnchant())
	    return false;
    }
    // We only care about charges with books and wands...
    if (getItemType() == ITEMTYPE_SPELLBOOK ||
	getItemType() == ITEMTYPE_WAND)
    {
	if (item->isKnownCharges() != isKnownCharges())
	    return false;
    }
    // We only care about poison if there is poison.  This does
    // let people id non-poisoned things but this is less of an
    // issue than the pain of detect curse not letting stuff stack.
    if (myPoisonCharges)
	if (item->isKnownPoison() != isKnownPoison())
	    return false;

    // Make sure a merge would not overflow our stack count.
    if (myStackCount + item->myStackCount > 99)
	return false;

    // These items are indistinguishable, merge them!
    return true;
}

void
ITEM::mergeStack(ITEM *item)
{
    UT_ASSERT(canMerge(item));
    UT_ASSERT(myStackCount + item->myStackCount <= 99);

    myStackCount += item->myStackCount;

    // Make sure we have the union of knowledge.
    // We don't want to lose any knowledge as a result of a merge,
    // even if that knowledge is deemed useless.
    if (item->isKnownCharges())
	markChargesKnown();
    if (item->isKnownEnchant())
	markEnchantKnown();
    if (item->isKnownPoison())
	markPoisonKnown();
}

ITEM *
ITEM::splitStack(int count)
{
    UT_ASSERT(count > 0);
    UT_ASSERT(count < myStackCount);

    ITEM		*item;

    item = createClone();
    item->myStackCount = count;
    myStackCount -= count;

    return item;
}

void
ITEM::splitAndDeleteStack(int count)
{
    ITEM		*split;

    split = splitStack(count);
    delete split;
}

int
ITEM::getAC() const
{
    int		ac;

    ac = glb_itemdefs[myDefinition].ac;

    // Enchantments...  Only counts if it is armour
    if (isBoots() || isHelmet() || isJacket() || isShield() || isAmulet())
    {
	ac += getEnchantment();
    }

    // Add in the artifact bonus.
    const ARTIFACT		*art;

    art = getArtifact();
    if (art)
	ac += art->acbonus;
    
    return ac;
}

int
ITEM::getCoolness() const
{
    int		cool = 0;

    // Find coolness from magic type.
    switch (getMagicType())
    {
	case MAGICTYPE_RING:
	    cool += glb_ringdefs[getMagicClass()].coolness;
	    break;
	case MAGICTYPE_AMULET:
	    cool += glb_amuletdefs[getMagicClass()].coolness;
	    break;
	default:
	    break;
    }

    return cool;
}

int
ITEM::getWeight() const
{
    int		weight;

    weight = glb_itemdefs[myDefinition].weight;

    return weight;
}

int
ITEM::getCharges() const
{
    int		charges;

    charges = myCharges;
    return charges;
}

void
ITEM::addCharges(int add)
{
    int		charges;

    // Divide the charges among the number of items.
    add += getStackCount() / 2;
    add /= getStackCount();
    // Because I am nice, always restore one charge
    if (add < 1)
	add = 1;

    charges = myCharges;
    charges += add;

    if (charges < 0)
	charges = 0;
    if (charges > 99)
	charges = 99;

    myCharges = charges;
}

void
ITEM::useCharge()
{
    if (myCharges)
	myCharges--;
}

void
ITEM::setAsPreserved()
{
    myCharges = 255;
}

MAGICTYPE_NAMES
ITEM::getMagicType() const
{
    return (MAGICTYPE_NAMES) glb_itemdefs[myDefinition].magictype;
}

int
ITEM::getMagicClass() const
{
    return glb_magicitem[myDefinition];
}

void
ITEM::buildIntrinsicTable(INTRINSIC *&intrinsic) const
{
    if (glb_itemdefs[myDefinition].intrinsic[0])
    {
	if (!intrinsic)
	    intrinsic = new INTRINSIC;
	intrinsic->mergeFromString(glb_itemdefs[myDefinition].intrinsic);
    }

    // Try the artifact class.
    const ARTIFACT		*art;
    
    art = getArtifact();
    if (art && art->intrinsics)
    {
	if (!intrinsic)
	    intrinsic = new INTRINSIC;
	intrinsic->mergeFromString(art->intrinsics);
    }

    // Try the magic item type...
    switch ((MAGICTYPE_NAMES) getMagicType())
    {
	case MAGICTYPE_RING:
	    if (glb_ringdefs[getMagicClass()].intrinsic[0])
	    {
		if (!intrinsic)
		    intrinsic = new INTRINSIC;
		intrinsic->mergeFromString(
			glb_ringdefs[getMagicClass()].intrinsic);
	    }
	    break;
	case MAGICTYPE_HELM:
	    if (glb_helmdefs[getMagicClass()].intrinsic[0])
	    {
		if (!intrinsic)
		    intrinsic = new INTRINSIC;
		intrinsic->mergeFromString(
			glb_helmdefs[getMagicClass()].intrinsic);
	    }
	    break;
	case MAGICTYPE_BOOTS:
	    if (glb_bootsdefs[getMagicClass()].intrinsic[0])
	    {
		if (!intrinsic)
		    intrinsic = new INTRINSIC;
		intrinsic->mergeFromString(
			glb_bootsdefs[getMagicClass()].intrinsic);
	    }
	    break;
	case MAGICTYPE_AMULET:
	    if (glb_amuletdefs[getMagicClass()].intrinsic[0])
	    {
		if (!intrinsic)
		    intrinsic = new INTRINSIC;
		intrinsic->mergeFromString(
			glb_amuletdefs[getMagicClass()].intrinsic);
	    }
	    break;
	case MAGICTYPE_STAFF:
	    if (glb_staffdefs[getMagicClass()].intrinsic[0])
	    {
		if (!intrinsic)
		    intrinsic = new INTRINSIC;
		intrinsic->mergeFromString(
			glb_staffdefs[getMagicClass()].intrinsic);
	    }
	    break;
	case MAGICTYPE_NONE:
	case MAGICTYPE_POTION:
	case MAGICTYPE_SPELLBOOK:
	case MAGICTYPE_SCROLL:
	case MAGICTYPE_WAND:
	    break;

	case NUM_MAGICTYPES:
	    UT_ASSERT(!"Unhandled MAGICTYPE");
	    break;
    }
}

bool
ITEM::hasIntrinsic(INTRINSIC_NAMES intrinsic) const
{
    // Check base item type...
    if (INTRINSIC::hasIntrinsic(glb_itemdefs[myDefinition].intrinsic,
				intrinsic))
	return true;

    // Try the artifact class.
    const ARTIFACT		*art;
    
    art = getArtifact();
    if (art)
    {
	if (INTRINSIC::hasIntrinsic(art->intrinsics, intrinsic))
	    return true;
    }

    // Try the magic item type...
    switch ((MAGICTYPE_NAMES) getMagicType())
    {
	case MAGICTYPE_RING:
	{
	    if (INTRINSIC::hasIntrinsic(glb_ringdefs[getMagicClass()].intrinsic,
					intrinsic) )
		return true;
	    break;
	}
	case MAGICTYPE_HELM:
	{
	    if (INTRINSIC::hasIntrinsic(glb_helmdefs[getMagicClass()].intrinsic,
					intrinsic) )
		return true;
	    break;
	}
	case MAGICTYPE_BOOTS:
	{
	    if (INTRINSIC::hasIntrinsic(glb_bootsdefs[getMagicClass()].intrinsic,
					intrinsic) )
		return true;
	    break;
	}
	case MAGICTYPE_AMULET:
	{
	    if (INTRINSIC::hasIntrinsic(glb_amuletdefs[getMagicClass()].intrinsic,
					intrinsic) )
		return true;
	    break;
	}
	case MAGICTYPE_STAFF:
	{
	    if (INTRINSIC::hasIntrinsic(glb_staffdefs[getMagicClass()].intrinsic,
					intrinsic) )
		return true;
	    break;
	}
	case MAGICTYPE_NONE:
	case MAGICTYPE_POTION:
	case MAGICTYPE_SPELLBOOK:
	case MAGICTYPE_SCROLL:
	case MAGICTYPE_WAND:
	{
	    // These guys all have no built in intrinsics
	    return false;
	}

	case NUM_MAGICTYPES:
	{
	    UT_ASSERT(!"Unhandled MAGICTYPE");
	    return false;
	}
    }
    return false;
}

bool
ITEM::dissolve(MOB *dissolver, bool *interesting)
{
    if (interesting)
	*interesting = true;
    if (canDissolve())
    {
	formatAndReport(dissolver, "%U <dissolve> in the acid!");
	return true;
    }
    if (interesting)
	*interesting = false;

    return false;
}

bool
ITEM::canDissolve() const
{
    bool		dodissolve = false;
    MATERIAL_NAMES	material;

    material = getMaterial();
    dodissolve = glb_materialdefs[material].soluble;

    // If we have resistance to acid, we never dissolve
    // If we have vulnerability, we always dissolve.
    // If we have both, it is as normal.
    if (hasIntrinsic(INTRINSIC_RESISTACID))
    {
	if (!hasIntrinsic(INTRINSIC_VULNACID))
	{
	    dodissolve = false;
	}
    }
    else if (hasIntrinsic(INTRINSIC_VULNACID))
    {
	dodissolve = true;
    }

    // Check for quest flag.  We don't want to dissolve those!
    if (glb_itemdefs[getDefinition()].isquest)
	dodissolve = false;

    return dodissolve;
}

bool
ITEM::ignite(MOB *igniter, bool *interesting)
{
    if (interesting)
	*interesting = true;

    // If we dip a sword, it becomes a flaming sword.
    if (getDefinition() == ITEM_LONGSWORD)
    {
	formatAndReport(igniter, "%U <catch> on fire!");

	myDefinition = ITEM_FLAMESWORD;
	return false;
    }

    if (getDefinition() == ITEM_ICEMACE)
    {
	formatAndReport(igniter, "%U <return> to room temperature.");
	myDefinition = ITEM_MACE;
	return false;
    }

    if (getDefinition() == ITEM_ARROW)
    {
	formatAndReport(igniter, "%U <catch> on fire!");
	myDefinition = ITEM_FIREARROW;
	return false;
    }

    // If we dip a club, it becomes a torch.
    if (getDefinition() == ITEM_CLUB)
    {
	formatAndReport(igniter, "%U <catch> on fire!");

	myDefinition = ITEM_TORCH;
	// Set our shield flag.
	myFlag1 = (ITEMFLAG1_NAMES) 
	    (myFlag1 | glb_itemdefs[ITEM_TORCH].flag1);
	return false;
    }

    // Check for burning up
    if (canBurn())
    {
	formatAndReport(igniter, "%U <burn> up.");
	return true;
    }

    // Got here, nothing interesting happened.
    if (interesting)
	*interesting = false;
    return false;
}

bool
ITEM::electrify(int points, MOB *shocker, bool *interesting)
{
    if (interesting)
	*interesting = true;

    // Energize rapiers.
    if (getDefinition() == ITEM_RAPIER)
    {
	formatAndReport(shocker, "%U <be> filled with electricity!");

	myDefinition = ITEM_LIGHTNINGRAPIER;
	addCharges(points);
	clearChargesKnown();
	return false;
    }

    if (getDefinition() == ITEM_LIGHTNINGRAPIER)
    {
	formatAndReport(shocker, "%U <drink> deeply from the electric current!");

	addCharges(points*2);
	clearChargesKnown();
	return false;
    }

    // Got here, nothing interesting happened.
    if (interesting)
	*interesting = false;
    return false;
}

bool
ITEM::canBurn() const
{
    bool		doburn = false;
    MATERIAL_NAMES	material;

    material = getMaterial();
    doburn = glb_materialdefs[material].burnable;

    // If we have resistance to fire, we never burn
    // If we have vulnerability, we always burn.
    // If we have both, it is as normal.
    if (hasIntrinsic(INTRINSIC_RESISTFIRE))
    {
	if (!hasIntrinsic(INTRINSIC_VULNFIRE))
	{
	    doburn = false;
	}
    }
    else if (hasIntrinsic(INTRINSIC_VULNFIRE))
    {
	doburn = true;
    }

    // Check for quest flag.  We don't want to burn those!
    if (glb_itemdefs[getDefinition()].isquest)
	doburn = false;

    return doburn;
}

// This was factored out in White Rock BC.
bool
ITEM::douse(MOB *douser, bool *interesting)
{
    if (interesting)
	*interesting = true;

    // If the dipped item is a flaming sword, it goes out.
    if (getDefinition() == ITEM_FLAMESWORD)
    {
	formatAndReport(douser, "%U <be> extinguished.");
	myDefinition = ITEM_LONGSWORD;
    }
    else if (getDefinition() == ITEM_FIREARROW)
    {
	formatAndReport(douser, "%U <be> extinguished.");
	myDefinition = ITEM_ARROW;
    }
    // As do torches.
    else if (getDefinition() == ITEM_TORCH)
    {
	formatAndReport(douser, "%U <be> extinguished.");
	myDefinition = ITEM_CLUB;
	// Clear out the isshield flag.
	myFlag1 = (ITEMFLAG1_NAMES) 
	    (myFlag1 & ~glb_itemdefs[ITEM_TORCH].flag1);
    }
    // Empty bottles fill.  This may seem odd as smashing one bottle may
    // fill ten, but the same happens with dipping.
    else if (getDefinition() == ITEM_BOTTLE)
    {
	formatAndReport(douser, "%U <be> filled.");
	myDefinition = ITEM_WATER;
    }
    else
    {
	formatAndReport(douser, "%U <get> wet.");
	if (interesting)
	    *interesting = false;
    }

    // Water destroys nothing right now.
    return false;
}

bool
ITEM::actionDip(MOB *dipper, ITEM *dippee,
		ITEM *&newpotion, ITEM *&newitem)
{
    bool		consumed = false;
    bool		possible = false;
    BUF			buf;

    UT_ASSERT(dippee != this);
    // This should already be unlinked.
    UT_ASSERT(!getNext());
    UT_ASSERT(!dippee->getNext());

    newpotion = this;
    newitem = dippee;

    // Check for magic type potion...
    switch ((MAGICTYPE_NAMES) getMagicType())
    {
	case MAGICTYPE_POTION:
	{
	    switch ((POTION_NAMES) getMagicClass())
	    {
		case POTION_GREEKFIRE:
		{
		    bool interesting = false;

		    possible = true;
		    consumed = true;

		    buf = MOB::formatToString("%MU <M:dip> %IU into %U.",
				0, this, dipper, dippee);
		    dipper->reportMessage(buf);
		    if (dippee->ignite(dipper, &interesting))
		    {
			// Burned up...
			delete dippee;
			newitem = 0;
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);
			// Id it...
			markIdentified();
			// Delete self
			delete this;

			break;
		    }
		    else if (interesting)
		    {
			// Something happened.
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);
			// Id it...
			markIdentified();
			// Delete self
			delete this;

			break;
		    }
		    else
		    {
			dipper->formatAndReport("Nothing happens.");
		    }
		    break;
		}

		case POTION_HEAL:
		case POTION_BLIND:
		case POTION_SMOKE:
		    buf = MOB::formatToString("%MU <M:dip> %IU into %U.  Nothing happens.",
				0, this, dipper, dippee);
		    dipper->reportMessage(buf);
		    possible = true;
		    consumed = true;
		    break;

		case POTION_MANA:
		    buf = MOB::formatToString("%MU <M:dip> %IU into %U.",
				0, this, dipper, dippee);
		    dipper->reportMessage(buf);

		    if (dippee->getMagicType() == MAGICTYPE_WAND)
		    {
			// Dipping a wand into a mana potion recharges it.
			dipper->formatAndReport("%IU hungrily <I:drink> the liquid!", dippee);

			newpotion = ITEM::create(ITEM_BOTTLE, false, true);

			int		netnewcharges;

			netnewcharges = rand_dice(1, 
						  6 - isCursed()*3, 
						  isBlessed()*3);

			// Charge the wand.
			dippee->addCharges(netnewcharges);
			// New number of charges is not known.
			dippee->clearChargesKnown();

			// Id it...
			markIdentified();
			// Delete self
			delete this;

			possible = true;
			consumed = true;
		    }
		    else if (dippee->getMagicType() == MAGICTYPE_SPELLBOOK)
		    {
			// Dipping a spellbook into a mana potion recharges it.
			dipper->formatAndReport("The pages of %IU swiftly absorb the liquid!", dippee);

			newpotion = ITEM::create(ITEM_BOTTLE, false, true);

			int		netnewcharges;

			netnewcharges = rand_dice(1, 
						  3 - isCursed()*2, 
						  isBlessed());

			// Charge the spellbook.
			dippee->addCharges(netnewcharges);
			// New number of charges is not known.
			dippee->clearChargesKnown();

			// Id it...
			markIdentified();
			// Delete self
			delete this;

			possible = true;
			consumed = true;
		    }
		    else
		    {
			// Fail the dip.
			dipper->reportMessage("Nothing happens.");
			possible = true;
			consumed = true;
		    }
		    break;
		    
		case POTION_CURE:
		    if (dippee->isPoisoned())
		    {
			dippee->makePoisoned(POISON_NONE, 0);

			// If the user knew it was poisoned, they get
			// a message.  Otherwise, no visible effect.
			// Well, the potion is removed that is a bit
			// of a give away...
			if (dippee->isKnownPoison())
			{
			    buf = MOB::formatToString("%MU <M:dip> %IU into %U.",
					0, this, dipper, dippee);
			    dipper->reportMessage(buf);

			    buf = MOB::formatToString("The poison on %U is neutralized by %IU.",
				    0, dippee, 0, this);
			    dipper->reportMessage(buf);
			}
			else
			{
			    buf = MOB::formatToString("%MU <M:dip> %IU into %U.  Nothing happens.",
					0, this, dipper, dippee);
			    dipper->reportMessage(buf);
			}
			    
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);

			// Id it...
			markIdentified();
			// Delete self
			delete this;

			possible = true;
			consumed = true;
		    }
		    else
		    {
			// Fail the dip.
			buf = MOB::formatToString("%MU <M:dip> %IU into %U.  Nothing happens.",
				    0, this, dipper, dippee);
			dipper->reportMessage(buf);
			possible = true;
			consumed = true;
		    }
		    break;

		case POTION_ENLIGHTENMENT:
		    if (dippee->isFullyIdentified())
		    {
			// No id, no effect.
			buf = MOB::formatToString("%MU <M:dip> %IU into %U.  Nothing happens.",
				    0, this, dipper, dippee);
			dipper->reportMessage(buf);
			possible = true;
			consumed = true;
		    }
		    else
		    {
			buf = MOB::formatToString("%MU <M:dip> %IU into %U.",
				    0, this, dipper, dippee);
			dipper->reportMessage(buf);

			// Identify the dippee.
			if (dipper->isAvatar())
			    dippee->markIdentified();
			dipper->formatAndReport("%U <gain> insight into the nature of %IU.", dippee);
			
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);

			// Id it...
			markIdentified();
			// Delete self
			delete this;

			possible = true;
			consumed = true;
		    }
		    break;

		case POTION_POISON:
		{
		    POISON_NAMES		poison;
		    
		    buf = MOB::formatToString("%MU <M:dip> %IU into %U.",
				0, this, dipper, dippee);
		    dipper->reportMessage(buf);  

		    
		    if (dippee->hasIntrinsic(INTRINSIC_RESISTPOISON))
		    {
			// Fail the dip.
			dipper->reportMessage("Nothing happens.");
			possible = true;
			consumed = true;
			break;
		    }

		    buf.sprintf("A thick slime coats %s.",
			    dippee->getAccusative());
		    dipper->reportMessage(buf);  

		    if (isCursed())
			poison = POISON_MILD;
		    else if (isBlessed())
			poison = POISON_STRONG;
		    else
			poison = POISON_NORMAL;

		    dippee->makePoisoned(poison, rand_dice(1, 3, 0));
		    if (dipper == MOB::getAvatar())
			dippee->markPoisonKnown();

		    newpotion = ITEM::create(ITEM_BOTTLE, false, true);
		    // Id it...
		    markIdentified();
		    // Delete self
		    delete this;

		    possible = true;
		    consumed = true;
		    break;
		}

		case POTION_ACID:
		{
		    buf = MOB::formatToString("%MU <M:dip> %IU into %U.",
				0, this, dipper, dippee);
		    dipper->reportMessage(buf);
		    
		    if (dippee->dissolve(dipper))
		    {
			newitem = 0;
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);
			// Id it...
			markIdentified();
			delete dippee;
			// Delete self
			delete this;
		    }
		    else
		    {
			dipper->reportMessage("Nothing happens.");
		    }
			    
		    possible = true;
		    consumed = true;
		    break;
		}

		case NUM_POTIONS:
		{
		    UT_ASSERT(0);
		    buf.sprintf("Unknown potion type %d", getMagicClass());
		    dipper->reportMessage(buf);
		    break;
		}
	    }
	    break;
	}

	    // Other magic types do nothing:
	case MAGICTYPE_NONE:
	case MAGICTYPE_HELM:
	case MAGICTYPE_BOOTS:
	case MAGICTYPE_WAND:
	case MAGICTYPE_SCROLL:
	case MAGICTYPE_AMULET:
	case MAGICTYPE_RING:
	case MAGICTYPE_SPELLBOOK:
	case MAGICTYPE_STAFF:
	case NUM_MAGICTYPES:
	    break;
    }

    // Check for specific items...
    // Only do this if previous checks failed.
    if (!consumed)
    {
	switch ((ITEM_NAMES) myDefinition)
	{
	    case ITEM_WATER:
	    {
		// If the potion is holy, make new guy holy.
		// If potion evil, make new guy evil.
		// If potion plain, just wet new guy.
		if (isCursed())
		{
		    dipper->formatAndReport("%IU <I:glow> black.", dippee);
		    dippee->makeCursed();
		    if (dipper->isAvatar())
			dippee->markCursedKnown();
		    possible = true;
		    consumed = true;
		    newpotion = ITEM::create(ITEM_BOTTLE, false, true);
		    delete this;
		}
		else if (isBlessed())
		{
		    dipper->formatAndReport("%IU <I:glow> blue.", dippee);
		    dippee->makeBlessed();
		    if (dipper->isAvatar())
			dippee->markCursedKnown();
		    possible = true;
		    consumed = true;
		    newpotion = ITEM::create(ITEM_BOTTLE, false, true);
		    delete this;
		}
		else
		{
		    bool		interesting;
		    if (dippee->douse(dipper, &interesting))
		    {
			newitem = 0;
			delete dippee;
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);
			delete this;
		    }
		    else if (interesting)
		    {
			newpotion = ITEM::create(ITEM_BOTTLE, false, true);
			delete this;
		    }

		    possible = true;
		    consumed = true;
		}
	    }
	    default:
		break;
	}
    }

    if (!possible)
    {
	// No one managed to do anything...
	buf = MOB::formatToString("%MU cannot figure out how to dip %IU into %U.",
				0, this, dipper, dippee);
	dipper->reportMessage(buf);
    }

    return consumed;
}

bool
ITEM::actionZap(MOB *zapper, int dx, int dy, int dz)
{
    bool		consumed = false;
    bool		possible = false;
    bool		targetself = false;
    const char		*directionflavour = 0;
    bool		destroythis = false;
    bool		doid = false;
    bool		forceid = false;
    BUF			buf;
    // Zapper position.
    int			zx, zy;

    zx = zapper->getX();
    zy = zapper->getY();
    if (!dx && !dy && !dz)
    {
	targetself = true;
	directionflavour = zapper->getReflexive();
    }
    if (dz)
    {
	if (dz < 0)
	    directionflavour = "the floor";
	else
	    directionflavour = "the ceiling";
    }
    
    // Try the magic type.
    switch ((MAGICTYPE_NAMES)getMagicType())
    {
	case MAGICTYPE_WAND:
	{
	    // Print prequel message..
	    zapper->formatAndReport("%U <zap> %IU%B1%B2.",
		    this,
		    directionflavour ? " at " : "",
		    directionflavour ? directionflavour : "");
	    
	    // Determine if we have enough charges...
	    if (myCharges == 0)
	    {
		// Check for wrest-last-charge
		// 1 in 20 to be nice as it is much harder UI wise
		// than in Nethack.
		// I have since reduced this even farther as the goal
		// is to make it not a tactical decision.
		if (!rand_choice(7))
		{
		    zapper->formatAndReport("%U <wrest> one last charge.");
		    destroythis = true;
		}
		else
		{
		    zapper->reportMessage("Nothing happens.");
		    possible = true;
		    consumed = true;
		    // To be nice, we record the wand is out of charges.
		    markChargesKnown();
		    // EARLY EXIT:
		    break;
		}
	    }


	    // Getting here implies we successfully zapped.
	    if (myCharges)
		myCharges--;

	    if (!destroythis && !isKnownCharges())
	    {
		if (zapper->isAvatar())
		{
		    if (!rand_choice(10))
		    {
			markChargesKnown();
			buf = MOB::formatToString("%U <know> %r %Iu better.  ",
				zapper, 0, 0, this);
			// This is important enough to be broadcast.
			msg_announce(gram_capitalize(buf));
		    }
		}
	    }

	    // Check to see if it is cursed and fizzled on the user.
	    if (isCursed() && rand_chance(10))
	    {
		doid = false;
		// This is a pretty simple wand to do...
		zapper->formatAndReport("%IU <I:fizzle>.", this);
		if (zapper->isAvatar())
		{
		    // We now know it is cursed
		    markCursedKnown();
		}
		possible = true;
		consumed = true;
		break;
	    }

	    // Apply any of our artifact based powers.
	    if (isArtifact())
	    {
		const ARTIFACT	*art;
		MOB		*target;

		target = glbCurLevel->getMob(zx + dx,
					     zy + dy);
		if (target)
		{
		    art = getArtifact();

		    const char	*intrinsics;

		    intrinsics = art->intrinsics;
		    while (intrinsics && *intrinsics)
		    {
			target->setTimedIntrinsic(zapper,
				    (INTRINSIC_NAMES) *intrinsics,
				    rand_dice(3+isBlessed()-isCursed(), 50, 100));
			intrinsics++;
		    }
		}
	    }
	    
	    switch ((WAND_NAMES) getMagicClass())
	    {
		case WAND_NOTHING:
		{
		    doid = false;
		    // This is a pretty simple wand to do...
		    zapper->reportMessage("Nothing happens.");
		    possible = true;
		    consumed = true;
		    break;
		}
		case WAND_CREATEMONSTER:
		{
		    doid = false;
		    possible = true;
		    consumed = true;

		    if (glbCurLevel->getMob(zx + dx,
					    zy + dy))
		    {
			zapper->reportMessage("Nothing happens.");
			break;
		    }

		    MOB		*mob;

		    mob = MOB::createNPC(100);

		    if (!mob->canMove(zx + dx,
				      zy + dy,
				      true))
		    {
			zapper->reportMessage("Nothing happens.");
			delete mob;
			break;
		    }

		    mob->move(zx + dx,
			      zy + dy,
			      true);
		    glbCurLevel->registerMob(mob);

		    mob->formatAndReport("%U <appear> out of thin air!");

		    // We have a chance of getting a tamed creature
		    // based on the BUC of the wand.
		    int		tamechance = 5;
		    int		hostilechance = 0;
		    if (isBlessed())
			tamechance += 15;
		    if (isCursed())
		    {
			tamechance = 1;
			hostilechance = 20;
		    }

		    if (rand_chance(tamechance))
		    {
			mob->makeSlaveOf(zapper);
		    }
		    else if (rand_chance(hostilechance))
		    {
			// The creature turns on the zapper!
			mob->setAITarget(zapper);
		    }

		    // Imbue the created creature with our wands
		    // abilities.
		    if (isArtifact())
		    {
			const ARTIFACT	*art;
			art = getArtifact();

			const char	*intrinsics;

			intrinsics = art->intrinsics;
			while (intrinsics && *intrinsics)
			{
			    mob->setTimedIntrinsic(zapper,
					(INTRINSIC_NAMES) *intrinsics,
					rand_dice(3+isBlessed()-isCursed(), 50, 100));
			    intrinsics++;
			}
		    }

		    zapper->pietyZapWand(this);

		    doid = true;
		    break;
		}

		case WAND_CREATETRAP:
		{
		    possible = true;
		    consumed = true;
		    doid = glbCurLevel->createTrap(zx + dx,
						    zy + dy,
						    zapper);
		    if (doid)
		    {
			zapper->formatAndReport("%U <create> a hidden surprise.");
			zapper->pietyZapWand(this);
		    }
		    else
			zapper->reportMessage("Nothing happens.");
		    break;
		}
		case WAND_LIGHT:
		{
		    MOB		*target;
		    doid = true;
		    possible = true;
		    consumed = true;
		    ourZapper.setMob(zapper);
		    zapper->formatAndReport("A lit field surrounds %U.");
		    glbCurLevel->applyFlag(SQUAREFLAG_LIT,
			    zx + dx,
			    zy + dy,
			    5, false, true);
		    zapper->pietyZapWand(this);

		    // Blind the target, if any.
		    if (!dz)
		    {
			zapper = ourZapper.getMob();

			target = glbCurLevel->getMob(zx+dx, zy+dy);
			if (target)
			{
			    // Wands always apply damage...
			    target->receiveDamage(ATTACK_LIGHTWAND, 
				    zapper,
				    this, 0,
				    ATTACKSTYLE_WAND);
			}
		    }
		    
		    zapper = ourZapper.getMob();
		    break;
		}
		
		case WAND_TELEPORT:
		{
		    MOB		*tele = 0;
		    
		    possible = true;
		    consumed = true;

		    // If it went vertical, nothing happens..
		    if (!dz)
		    {
			tele = glbCurLevel->getMob(zx + dx,
					    zy + dy);
		    }
		    if (tele)
		    {
			// Teleport the monster
			ourZapper.setMob(zapper);
			tele->actionTeleport();
			zapper = ourZapper.getMob();
			if (zapper)
			    zapper->pietyZapWand(this);
			doid = true;
		    }
		    // Tracked down a bug here.
		    // Bug was: "TODO: Teleport items"
		    // :>
		    // Still in the airport lounge enjoying free beer.
		    // Fear this code!
		    // No doubt as a result of aforementioned free beer,
		    // I foolishly dereferenced zapper->getX() to get the
		    // tile location.  Zapper might be tele in which case
		    // it might very well be dead.  In any case, this
		    // would end up teleporting items at your destination
		    // square rather than source square!
		    ITEMSTACK		stack;
		    glbCurLevel->getItemStack(stack, zx + dx,
						     zy + dy);
		    if (stack.entries())
		    {
			ITEM			*item;
			
			doid = true;
			// We don't want to double count this zap.
			if (zapper && !tele)
			    zapper->pietyZapWand(this);

			for (int i = 0; i < stack.entries(); i++)
			{
			    item = stack(i);
			    item->randomTeleport(glbCurLevel, true, zapper);
			}
		    }

		    if (!doid && zapper)
		    {
			zapper->reportMessage("Nothing happens.");
		    }
		    break;
		}
		
		case WAND_POLYMORPH:
		{
		    MOB		*poly = 0;
		    ITEM	*item = 0;
		    
		    possible = true;
		    consumed = true;

		    // If it went vertical, nothing happens..
		    if (!dz)
		    {
			poly = glbCurLevel->getMob(zx + dx,
					    zy + dy);
		    }
		    if (poly)
		    {
			MOBREF		zapmobref;

			zapmobref.setMob(zapper);

			// Polymorph the monster
			// If the monster was us, zapper may now point
			// to the base object rather than the top object.
			// Thus we track the ref.
			poly->actionPolymorph();
			zapper = zapmobref.getMob();
			doid = true;
		    }
		    // Zapping the ground means polying on the ground.
		    // (doid here means did something)
		    if (dz <= 0 && !doid)
		    {
			item = glbCurLevel->getItem(zx + dx,
						zy + dy);
		    }
		    if (item)
		    {
			ITEM		*newitem;

			newitem = item->polymorph(zapper);
			
			if (newitem)
			{
			    if (item->getStackCount() > 1)
				item = item->splitStack(1);
			    else
				glbCurLevel->dropItem(item);

			    glbCurLevel->acquireItem(newitem,
						zx + dx,
						zy + dy,
						zapper);
			    delete item;
			}

			doid = true;
		    }
		    if (!doid && zapper)
		    {
			zapper->reportMessage("Nothing happens.");
		    }
		    else if (zapper)
		    {
			zapper->pietyZapWand(this);
		    }
		    break;
		}
		
		case WAND_INVISIBLE:
		{
		    MOB		*invis = 0;
		    bool	 getpiety = false;
		    
		    possible = true;
		    consumed = true;
		    

		    // If it went vertical, nothing happens..
		    if (!dz)
		    {
			invis = glbCurLevel->getMob(zx + dx,
					    zy + dy);
		    }
		    if (invis)
		    {
			// Invisible the monster
			// We notice something if the monster
			// is not already invisible.
			bool quietchange = false;
			if (!invis->hasIntrinsic(INTRINSIC_INVISIBLE))
			{
			    if (invis != MOB::getAvatar())
			    {
				invis->formatAndReport("%U <disappear> from sight.");
			    }
			    doid = true;

			    // We must determine if we can see the monster
			    // *before* we make it invisible.  If we wait
			    // for normal doid check, the monster may
			    // be invisible which prevents us from
			    // iding the wand.
			    if (zapper == MOB::getAvatar() ||
				(MOB::getAvatar() && 
				 MOB::getAvatar()->canSense(zapper)))
			    {
				forceid = true;
			    }
			}
			else
			{
			    quietchange = true;
			}
			invis->setTimedIntrinsic(zapper, 
				INTRINSIC_INVISIBLE,
				rand_dice(3+isBlessed()-isCursed(), 50, 100),
				quietchange);
			getpiety = true;
		    }
		    SQUARE_NAMES		tile;

		    tile = glbCurLevel->getTileSafe(zx+dx, zy+dy);

		    // Check to see if we can make a door invisible
		    switch (tile)
		    {
			// Open doors need to be closed first to avoid
			// this nerfing the knock spell.
			case SQUARE_DOOR:
			case SQUARE_BLOCKEDDOOR:
			    doid = true;
			    if (zapper == MOB::getAvatar() ||
				glbCurLevel->hasFOV(zx+dx, zy+dy))
			    {
				forceid = true;
			    }
			    glbCurLevel->reportMessage("The door fades into the wall.", zx+dx, zy+dy);
			    glbCurLevel->setTile(zx+dx, zy+dy, SQUARE_SECRETDOOR);
			    getpiety = true;
			    break;
			case SQUARE_FLOORSMOKEVENT:
			case SQUARE_PATHSMOKEVENT:
			case SQUARE_FLOORPOISONVENT:
			case SQUARE_PATHPOISONVENT:
			case SQUARE_FLOORTELEPORTER:
			case SQUARE_PATHTELEPORTER:
			case SQUARE_FLOORHOLE:
			case SQUARE_PATHHOLE:
			case SQUARE_FLOORPIT:
			case SQUARE_PATHPIT:
			case SQUARE_FLOORSPIKEDPIT:
			case SQUARE_PATHSPIKEDPIT:
			{
			    BUF		buf;

			    doid = true;
			    if (zapper == MOB::getAvatar() ||
				glbCurLevel->hasFOV(zx+dx, zy+dy))
			    {
				forceid = true;
			    }
			    
			    buf.sprintf("%s fades into the ground.",
					glb_squaredefs[tile].description);
			    glbCurLevel->reportMessage(buf, zx+dx, zy+dy);
			    glbCurLevel->setTile(zx+dx, zy+dy, (SQUARE_NAMES) glb_squaredefs[tile].invissquare);
			    getpiety = true;
			    break;
			}

			default:
			    // Do nothing
			    break;
		    }
		    if (!doid)
		    {
			zapper->reportMessage("Nothing happens.");
		    }
		    if (getpiety)
			zapper->pietyZapWand(this);
		    break;
		}

		case WAND_SPEED:
		{
		    MOB		*speed = 0;
		    
		    possible = true;
		    consumed = true;

		    // If it went vertical, nothing happens..
		    if (!dz)
		    {
			speed = glbCurLevel->getMob(zx + dx,
					    zy + dy);
		    }
		    if (speed)
		    {
			// Quicken the monster
			// We notice something if the monster
			// is not already quick.
			if (!speed->hasIntrinsic(INTRINSIC_QUICK))
			{
			    doid = true;
			}
			speed->setTimedIntrinsic(zapper, 
				INTRINSIC_QUICK,
				rand_dice(3+isBlessed()-isCursed(), 50, 100));
			zapper->pietyZapWand(this);
		    }
		    if (!doid)
		    {
			zapper->reportMessage("Nothing happens.");
		    }
		    break;
		}

		case WAND_SLOW:
		{
		    MOB		*slow = 0;
		    
		    possible = true;
		    consumed = true;

		    // If it went vertical, nothing happens..
		    if (!dz)
		    {
			slow = glbCurLevel->getMob(zx + dx,
					    zy + dy);
		    }
		    if (slow)
		    {
			// Quicken the monster
			// We notice something if the monster
			// is not already quick.
			if (!slow->hasIntrinsic(INTRINSIC_SLOW))
			{
			    doid = true;
			}
			slow->setTimedIntrinsic(zapper, 
				INTRINSIC_SLOW,
				rand_dice(3+isBlessed()-isCursed(), 50, 100));
			zapper->pietyZapWand(this);
		    }
		    if (!doid)
		    {
			zapper->reportMessage("Nothing happens.");
		    }
		    break;
		}

		case WAND_FIRE:
		case WAND_ICE:
		{
		    possible = true;
		    consumed = true;
		    doid = true;		// This is pretty obvious.

		    ourZapper.setMob(zapper);
		    zapper->pietyZapWand(this);
		    if (targetself || dz)
		    {
			bool		dosquareeffect = false;

			// While underwater, always works.
			if (zapper->hasIntrinsic(INTRINSIC_SUBMERGED))
			    dosquareeffect = true;
			
			if (dz && !dosquareeffect)
			{
			    // Shot downwards? affect square
			    if (dz < 0)
				dosquareeffect = true;
			    zapper->reportMessage("The ray bounces.");
			}
			// Affect the square.
			if (dosquareeffect && 
			    (WAND_NAMES) getMagicClass() == WAND_ICE)
			{
			    glbCurLevel->freezeSquare(zx, zy, zapper);
			}
			if (dosquareeffect && 
			    (WAND_NAMES) getMagicClass() == WAND_FIRE)
			{
			    glbCurLevel->burnSquare(zx, zy, zapper);
			}
			// Apply to self..
			zapCallback(zx, zy);
		    }
		    else
		    {
			glbCurLevel->fireRay(zx, zy,
				dx, dy,
				6,
				MOVE_STD_FLY, true,
				zapCallbackStatic,
				this);

		    }
		    zapper = ourZapper.getMob();
		    break;
		}
		
		case WAND_SLEEP:
		{
		    possible = true;
		    consumed = true;
		    doid = true;		// This is pretty obvious.

		    ourZapper.setMob(zapper);
		    zapper->pietyZapWand(this);
		    if (targetself || dz)
		    {
			if (dz)
			{
			    zapper->reportMessage("The ray bounces.");
			}

			// Apply to self..
			zapCallback(zx, zy);
		    }
		    else
		    {
			glbCurLevel->fireRay(zx, zy,
				dx, dy,
				6,
				MOVE_STD_FLY, true,
				zapCallbackStatic,
				this);

		    }
		    zapper = ourZapper.getMob();
		    break;
		}

		case WAND_DIGGING:
		{
		    possible = true;
		    consumed = true;
		    doid = true;		// Wands of digging autoid.

		    ourZapper.setMob(zapper);
		    zapper->pietyZapWand(this);
		    zapper->actionDig(dx, dy, dz, 6, true);
		    zapper = ourZapper.getMob();

		    break;
		}

		case NUM_WANDS:
		{
		    UT_ASSERT(!"Unhandled wand type!");
		    break;
		}
	    }
	}

	// Other MAGICTYPES that, of course, do nothing...
	case MAGICTYPE_NONE:
	case MAGICTYPE_POTION:
	case MAGICTYPE_SPELLBOOK:
	case MAGICTYPE_SCROLL:
	case MAGICTYPE_RING:
	case MAGICTYPE_HELM:
	case MAGICTYPE_BOOTS:
	case MAGICTYPE_AMULET:
	case MAGICTYPE_STAFF:
	case NUM_MAGICTYPES:
	    break;
    }

    // Try the definition.
    if (!possible)
    {
	switch ((ITEM_NAMES) myDefinition)
	{
	    case ITEM_LIGHTNINGRAPIER:
	    {
		possible = true;
		consumed = true;
		doid = false;		// Already ided.

		ourZapper.setMob(zapper);
		// Rapiers still count as wands in terms of piety.
		zapper->pietyZapWand(this);

		zapper = ourZapper.getMob();

		// Electrocution by lighting rapiers
		// only occurs if fired while submerged,
		// not if merely fired at the ground.
		if (zapper && zapper->hasIntrinsic(INTRINSIC_SUBMERGED))
		{
		    // Drain our charges..
		    zapper->formatAndReport("%R %Iu violently <I:discharge>.", this);
		    myCharges = 0;
		    setDefinition(ITEM_RAPIER);
		    zapper->rebuildAppearance();
		    // Affect the square.
		    glbCurLevel->electrocuteSquare(zx, zy, zapper);

		    // All done, your targetting has no effect
		    break;
		}
		zapper = ourZapper.getMob();

		// Getting here implies we successfully zapped.
		if (myCharges > 10)
		    myCharges -= 10;
		else
		    myCharges = 0;
		if (!myCharges)
		{
		    if (zapper)
			zapper->formatAndReport("%R %Iu <I:power> down.", this);
		    setDefinition(ITEM_RAPIER);
		    if (zapper)
			zapper->rebuildAppearance();
		}

		if (myCharges && !isKnownCharges())
		{
		    if (zapper && zapper->isAvatar())
		    {
			if (!rand_choice(10))
			{
			    markChargesKnown();
			    buf = MOB::formatToString("%U <know> %r %Iu better.  ",
				    zapper, 0, 0, this);
			    // This is important enough to be broadcast.
			    msg_announce(gram_capitalize(buf));
			}
		    }
		}
		if (targetself || dz)
		{
		    if (dz && zapper)
		    {
			zapper->reportMessage("The ray bounces.");
		    }
		    // Apply to self..
		    zapCallback(zx, zy);
		}
		else
		{
		    glbCurLevel->fireRay(zx, zy,
			    dx, dy,
			    6,
			    MOVE_STD_FLY, true,
			    zapCallbackStatic,
			    this);

		}
		zapper = ourZapper.getMob();
		break;
	    }
	    default:
		// Nothing...
		break;
	}
    }

    // Check if zapper is dead.
    if (zapper)
    {
	if (zapper->hasIntrinsic(INTRINSIC_DEAD))
	{
	    zapper = 0;
	}
    }

    if (!possible && zapper)
    {
	// Noone figured out how to zap it.
	zapper->formatAndReport("%U cannot figure out how to zap %IU.",
				this);
    }

    // Check if we should id this...
    // Ideally you could id a wand by a creature killing itself with it,
    // but that makes the canSense call rather difficult.
    if (doid && zapper)
    {
	if (forceid ||
	    zapper == MOB::getAvatar() ||
	    (MOB::getAvatar() && MOB::getAvatar()->canSense(zapper)))
	{
	    // We only id type, not the particulars!
	    markClassKnown();
	}
    }

    // If we are to destroy this, do so...
    // Wands that crumble to dust with the same charge that kills their
    // owner get a saving throw.
    // (By saving throw, I mean that they don't get destroyed :>)
    if (destroythis && zapper)
    {
	ITEM		*drop;
	
	zapper->formatAndReport("%IU <I:crumble> to dust.", this);
	
	drop = zapper->dropItem(getX(), getY());
	UT_ASSERT(drop == this);
	delete this;
    }

    return consumed;
}

bool
ITEM::grenadeCallbackStatic(int x, int y, bool final, void *data)
{
    return ((ITEM *)data)->grenadeCallback(x, y);
}

bool
ITEM::grenadeCallback(int x, int y)
{
    MOB		*victim;
    BUF		 buf;
    
    // Get the mob at this location.
    victim = glbCurLevel->getMob(x, y);

    // Magic potions..
    if (getMagicType() == MAGICTYPE_POTION)
    {
	POTION_NAMES		potion;

	potion = (POTION_NAMES) getMagicClass();

	switch (potion)
	{
	    case POTION_HEAL:
	    {
		if (!victim)
		    break;

		int		heal;
		
		if (isBlessed())
		    heal = rand_dice(1, 10, 10);
		else if (isCursed())
		    heal = rand_dice(1, 10, 0);
		else
		    heal = rand_dice(1, 20, 0);

		if (victim->receiveHeal(heal, ourZapper.getMob()))
		{
		    victim->formatAndReport("%U <look> healthier.");

		    if (victim->isAvatar() || 
			(MOB::getAvatar() && 
			 MOB::getAvatar()->canSense(victim)))
			markIdentified();
		}
		break;
	    }

	    case POTION_MANA:
	    {
		if (!victim)
		    break;

		int		magic;
		
		if (isBlessed())
		    magic = rand_dice(1, 20, 20);
		else if (isCursed())
		    magic = rand_dice(1, 20, 0);
		else
		    magic = rand_dice(1, 40, 0);

		if (victim->receiveMana(magic, ourZapper.getMob()))
		{
		    victim->formatAndReport("%U <look> recharged.");

		    if (victim->isAvatar() || 
			(MOB::getAvatar() && 
			 MOB::getAvatar()->canSense(victim)))
			markIdentified();
		}
		break;
	    }

	    case POTION_ACID:
	    {
		// Check to see if we upconvert smoke into acid.
		if (glbCurLevel->getSmoke(x, y) == SMOKE_SMOKE)
		{
		    glbCurLevel->reportMessage("The smoke is acidified.", x, y);
		    if (MOB::getAvatar() && 
			glbCurLevel->hasLOS(MOB::getAvatar()->getX(),
				    MOB::getAvatar()->getY(),
				    x, y))
		    {
			markIdentified();
		    }
		    glbCurLevel->setSmoke(x, y, SMOKE_ACID, ourZapper.getMob());
		}
		if (!victim)
		    break;

		// Do identify if the avatar can sense this square.
		if (glbCurLevel->hasFOV(x, y))
		    markIdentified();

		victim->receiveDamage(ATTACK_ACIDPOTION, ourZapper.getMob(), this, 0,
					ATTACKSTYLE_MISC);
		break;
	    }

	    case POTION_GREEKFIRE:
	    {
		// Check for burning squares.  We identify if the
		// avatar witnesses this.
		if (glbCurLevel->burnSquare(x, y, ourZapper.getMob()))
		{
		    if (glbCurLevel->hasFOV(x, y))
			markIdentified();
		}
		
		// Burning the square may kill our victim,
		// so we refetch.
		victim = glbCurLevel->getMob(x, y);
		if (!victim)
		    break;

		// Do identify if the avatar can sense this square.
		if (glbCurLevel->hasFOV(x, y))
		    markIdentified();

		victim->receiveDamage(ATTACK_GREEKFIREPOTION, 
				      ourZapper.getMob(), this, 0,
				      ATTACKSTYLE_MISC);
		break;
	    }

	    case POTION_SMOKE:
	    {
		// We can only put smoke where things can fly.
		if (!(glb_squaredefs[glbCurLevel->getTile(x, y)].movetype &
		      MOVE_STD_FLY))
		    break;
		
		// Do identify if the avatar can sense this.
		if (glbCurLevel->hasFOV(x, y))
		    markIdentified();

		// Make this square smokey.
		// If the smoke potion is poisoned, act accordinly
		if (isPoisoned())
		    glbCurLevel->setSmoke(x, y, SMOKE_POISON, ourZapper.getMob());
		else
		    glbCurLevel->setSmoke(x, y, SMOKE_SMOKE, ourZapper.getMob());
		break;
	    }

	    case POTION_BLIND:
	    {
		if (!victim)
		    break;

		int		turns;

		// You get 3d20 turns of blindness.
		if (isBlessed())
		    turns = rand_dice(3, 10, 30);
		else if (isCursed())
		    turns = rand_dice(3, 10, 0);
		else
		    turns = rand_dice(3, 20, 0);

		// Determine if we notice anything...
		if (victim->isAvatar() ||
		    (MOB::getAvatar() && 
		     MOB::getAvatar()->canSense(victim)))
		{
		    markIdentified();
		}

		victim->setTimedIntrinsic(ourZapper.getMob(), 
					    INTRINSIC_BLIND, turns);
		break;
	    }

	    case POTION_POISON:
	    {
		// Check to see if we upconvert smoke into poison.
		if (glbCurLevel->getSmoke(x, y) == SMOKE_SMOKE)
		{
		    glbCurLevel->reportMessage("The smoke is poisoned.", x, y);
		    if (MOB::getAvatar() && 
			glbCurLevel->hasLOS(MOB::getAvatar()->getX(),
				    MOB::getAvatar()->getY(),
				    x, y))
		    {
			markIdentified();
		    }
		    glbCurLevel->setSmoke(x, y, SMOKE_POISON, ourZapper.getMob());
		}

		if (!victim)
		    break;

		int			turns;
		POISON_NAMES	poison;

		turns = rand_dice(4, 5, 0);
		if (isBlessed())
		    poison = POISON_STRONG;
		else if (isCursed())
		    poison = POISON_MILD;
		else
		    poison = POISON_NORMAL;

		victim->setTimedIntrinsic(ourZapper.getMob(), (INTRINSIC_NAMES)
				  glb_poisondefs[poison].intrinsic, 
				  turns);
		// The avatar is exempt as the permament intrinsic
		// will duplicate this.
		if (!victim->isAvatar())
		{
		    victim->formatAndReport("%U <be> poisoned.");
		}
		
		// Determine if we notice anything...
		if (victim->isAvatar() ||
		    (MOB::getAvatar() && 
		     MOB::getAvatar()->canSense(victim)))
		{
		    markIdentified();
		}

		break;
	    }

	    case POTION_CURE:
	    {
		// Check to see if we downconvert poison smoke into smoke.
		if (glbCurLevel->getSmoke(x, y) == SMOKE_POISON)
		{
		    glbCurLevel->reportMessage("The poisoned smoke is neutralized.", x, y);
		    if (MOB::getAvatar() && 
			glbCurLevel->hasLOS(MOB::getAvatar()->getX(),
				    MOB::getAvatar()->getY(),
				    x, y))
		    {
			markIdentified();
		    }

		    glbCurLevel->setSmoke(x, y, SMOKE_SMOKE, ourZapper.getMob());
		}

		if (!victim)
		    break;

		if (victim->receiveCure())
		{
		    victim->formatAndReport(
			    "The poison is expunged from %R veins.");
		    
		    if (victim->isAvatar() || 
			(MOB::getAvatar() && 
			 MOB::getAvatar()->canSense(victim)))
			markIdentified();
		}
		break;
	    }

	    case POTION_ENLIGHTENMENT:
	    {
		if (!victim)
		    break;

		glbShowIntrinsic(victim);
		markIdentified();
		break;
	    }

	    case NUM_POTIONS:
		UT_ASSERT(!"Unknown potion class!");
		break;
	}
    }
    else
    {
	// Regular stuff...
	switch (getDefinition())
	{
	    case ITEM_WATER:
	    {
		glbCurLevel->douseSquare(x, y, isBlessed(), ourZapper.getMob());

		break;
	    }
	    case ITEM_BOTTLE:
	    {
		if (!victim)
		    break;

		// Do normal piercing damage
		victim->receiveDamage(ATTACK_GLASSFRAGMENTS, ourZapper.getMob(), this,
					0, ATTACKSTYLE_THROWN);
		break;
	    }

	    default:
		// Do nothing!
		// Not handled currently.
		UT_ASSERT(!"Unknown grenade!");
		break;
	}
    }

    return true;
}

bool
ITEM::zapCallbackStatic(int x, int y, bool final, void *data)
{
    if (final) return false;
    return ((ITEM *)data)->zapCallback(x, y);
}

bool
ITEM::zapCallback(int x, int y)
{
    MOB			*mob;
    ATTACK_NAMES	 attack = ATTACK_NONE;

    mob = glbCurLevel->getMob(x, y);
    if (mob)
    {
	if (getMagicType() == MAGICTYPE_WAND)
	{
	    switch ((WAND_NAMES) getMagicClass())
	    {
		case WAND_FIRE:
		    attack = ATTACK_FIREWAND;
		    break;
		case WAND_ICE:
		    attack = ATTACK_ICEWAND;
		    break;
		case WAND_DIGGING:
		    if (mob->getDefinition() == MOB_EARTHELEMENTAL)
		    {
			attack = ATTACK_DIGEARTHELEMENTAL;
		    }
		    break;
		case WAND_SLEEP:
		    attack = ATTACK_SLEEPWAND;
		    break;
		default:
		    break;
	    }
	}
	switch (getDefinition())
	{
	    case ITEM_LIGHTNINGRAPIER:
		// note it is very important that this can't charge lightning
		// rapiers!
		attack = ATTACK_ZAPLIGHTNINGRAPIER;
		break;
	    default:
		// No, I am not going to list all the items here to take
		// advantage of enumeration warnings.
		break;
	}
	if (attack != ATTACK_NONE)
	{
	    mob->receiveDamage(attack, ourZapper.getMob(), this,
				0, ATTACKSTYLE_WAND);
	}
    }

    // Special square related stuff...
    if (getMagicType() == MAGICTYPE_WAND && getMagicClass() == WAND_DIGGING)
    {
	SQUARE_NAMES		tile;

	tile = (SQUARE_NAMES) glbCurLevel->getTile(x, y);
	switch (tile)
	{
	    case SQUARE_EMPTY:
	    case SQUARE_WALL:
	    {
		glbCurLevel->setTile(x, y, SQUARE_CORRIDOR);
		break;
	    }

	    case SQUARE_DOOR:
	    case SQUARE_BLOCKEDDOOR:
	    case SQUARE_SECRETDOOR:
	    {
		// Can't tunnel through these!
		return false;
	    }

	    default:
		break;
	}
    }
    
    if (getMagicType() == MAGICTYPE_WAND && getMagicClass() == WAND_ICE)
    {
	// On successful freeze, fizzle out wand.
	if (glbCurLevel->freezeSquare(x, y, ourZapper.getMob()))
	    return false;
    }
    
    if (getMagicType() == MAGICTYPE_WAND && getMagicClass() == WAND_FIRE)
    {
	// Fire wands keep burning :>
	glbCurLevel->burnSquare(x, y, ourZapper.getMob());
    }
    
    return true;
}

bool
ITEM::randomTeleport(MAP *map, bool mapownsitem, MOB *teleporter)
{
    // Check for tele fixed items.
    if (hasIntrinsic(INTRINSIC_TELEFIXED))
    {
	formatAndReport("%U <shudder>.");
	return false;
    }
    bool teleported = false;
    int x, y;
    if (map->findRandomLoc(x, y, MOVE_WALK,
				true,
				false, false,
				false, false,
				false))
    {
	// Teleport the item
	formatAndReport("%U <teleport>.");
	// We lose our mapping information.
	markMapped(false);
	if (mapownsitem)
	    map->dropItem(this);
	// This ensures our item maps are updated
	// and it drops in lava, etc.
	map->acquireItem(this, x, y, teleporter);
	teleported = true;
    }
    return teleported;
}

bool
ITEM::fallInHole(MAP *curLevel, bool mapownsitem, MOB *dropper)
{
    MAP		*nextmap;
    int		 x, y;

    nextmap = curLevel->getMapDown();
    if (!nextmap)
    {
	formatAndReport("%R fall <be> stopped by an invisible barrier.");
	return false;
    }

    if (!nextmap->findRandomLoc(x, y, MOVE_WALK,
				true,
				false, false,
				false, false,
				false))
    {
	formatAndReport("%U <land> on the edge of a hole.");
	return false;
    }

    formatAndReport("%U <fall> into a hole.");

    // We no longer know where it is
    markMapped(false);

    if (mapownsitem)
	curLevel->dropItem(this);
    nextmap->acquireItem(this, x, y, dropper);

    return true;
}

ITEM *
ITEM::load(SRAMSTREAM &is)
{
    ITEM	*me;
    int		 val;

    me = new ITEM();
    
    is.uread(val, 8);
    me->myDefinition = val;
    me->myName.load(is);
    is.uread(val, 8);
    val ^= 1;
    me->myStackCount = val;
    is.uread(val, 8);
    me->myX = val;
    is.uread(val, 8);
    me->myY = val;
    is.read(val, 8);
    me->myEnchantment = val;
    is.uread(val, 8);
    me->myCharges = val;
    is.uread(val, 8);
    me->myPoison = val;
    is.uread(val, 8);
    me->myPoisonCharges = val;
    is.uread(val, 32);
    val ^= glb_itemdefs[me->myDefinition].flag1;
    me->myFlag1 = (ITEMFLAG1_NAMES) val;

    // Read in the corpse mob.
    me->myCorpseMob.load(is);
    if (!me->myCorpseMob.isNull())
    {
	MOB		*mob;

	mob = MOB::load(is);
	// myCorpseMob will already point to mob, but might as well
	// assert.
	UT_ASSERT(mob == me->myCorpseMob.getMob());
    }

    return me;
}

void
ITEM::save(SRAMSTREAM &os)
{
    os.write(myDefinition, 8);
    myName.save(os);
    os.write(myStackCount ^ 1, 8);
    os.write(myX, 8);
    os.write(myY, 8);
    os.write(myEnchantment, 8);
    os.write(myCharges, 8);
    os.write(myPoison, 8);
    os.write(myPoisonCharges, 8);

    os.write(myFlag1 ^ glb_itemdefs[myDefinition].flag1, 32);

    // Save our corpse mob.
    // If it is non-zero, we also will save the mob itself.
    MOB		*mob;
    mob = myCorpseMob.getMob();

    myCorpseMob.save(os);
    if (mob)
	mob->save(os);

    // TODO: Save the name???
}

bool
ITEM::verifyMob() const
{
    MOB		*mob;
    mob = myCorpseMob.getMob();
    if (mob)
	return mob->verifyMob();
    return true;
}

bool
ITEM::verifyCounterGone(INTRINSIC_COUNTER *counter) const
{
    MOB		*mob;
    mob = myCorpseMob.getMob();
    if (mob)
	return mob->verifyCounterGone(counter);
    return true;
}

void
ITEM::loadGlobal(SRAMSTREAM &is)
{
    int		i, val;

    for (i = 0; i < NUM_ITEMS; i++)
    {
	is.uread(val, 8);
	glb_magicitem[i] = val;
    }

    for (i = 0; i < NUM_ITEMS; i++)
    {
	is.uread(val, 8);
	glb_itemid[i] = val ? true : false;
    }

    for (i = 0; i < NUM_ITEMS; i++)
    {
	char	buf[100];
	is.readString(buf, 100);
	if (glb_itemnames[i])
	    free(glb_itemnames[i]);
	if (buf[0])
	    glb_itemnames[i] = strdup(buf);
	else
	    glb_itemnames[i] = 0;
    }
}

void
ITEM::saveGlobal(SRAMSTREAM &is)
{
    int		i, val;

    for (i = 0; i < NUM_ITEMS; i++)
    {
	val = glb_magicitem[i];
	is.write(val, 8);
    }

    for (i = 0; i < NUM_ITEMS; i++)
    {
	val = glb_itemid[i];
	is.write(val, 8);
    }
    for (i = 0; i < NUM_ITEMS; i++)
    {
	is.writeString(glb_itemnames[i], 100);
    }
}
