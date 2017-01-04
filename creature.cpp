/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        creature.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This would be better named mob.cpp or mob.h, but the change
 *	to MOB from CREATURE occured a bit late.
 *
 *	This handles all the generic MOB manipulation routines.
 *	Note that action and ai functions can be found in action.cpp
 *	or ai.cpp.
 */

#include "creature.h"

#include "mygba.h"
#include <stdio.h>
#include <ctype.h>
#include "assert.h"
#include "gfxengine.h"
#include "glbdef.h"
#include "map.h"
#include "creature.h"
#include "rand.h"
#include "msg.h"
#include "grammar.h"
#include "item.h"
#include "itemstack.h"
#include "intrinsic.h"
#include "sramstream.h"
#include "victory.h"
#include "control.h"
#include "piety.h"
#include "encyc_support.h"
#include "hiscore.h"

MOBREF	glbAvatar;

int  	MOB::ourFireBallCount = 0;
SPELL_NAMES MOB::ourZapSpell;
ATTACK_NAMES MOB::ourEffectAttack;
bool	MOB::ourAvatarOnFreshSquare = false;
bool	MOB::ourAvatarMoveOld = true;
u16	glbKillCount[NUM_MOBS];
s8	MOB::ourAvatarDx = 0;
s8	MOB::ourAvatarDy = 0;

int	glbMobCount = 0;

// Defined in main.cpp
extern bool glbAutoRunEnabled;

void
mob_init()
{
    // Nothing to do here... yet.
}

//
// MOB::Methods...
//
MOB::MOB()
{
    myNext = 0;
    myInventory = 0;
    myCounters = 0;
    myWornIntrinsic = 0;
    glbMobCount++;
}

MOB::~MOB()
{
    INTRINSIC_COUNTER		*counter, *next;

    // We don't want people pointing to our mob value any more:
    if (!myOwnRef.transferMOB(0))
    {
	UT_ASSERT(!"Failed transfer in destructor");
    }

    // We obviously don't need these anymore.
    for (counter = myCounters; counter; counter = next)
    {
	next = counter->myNext;
	delete counter;
    }
    myCounters = 0;

    delete myWornIntrinsic;

    MOB		*mob;

    mob = myBaseType.getMob();
    if (mob)
    {
	// This is safe as we own it.
	delete mob;
    }

    // Delete all of our inventory.
    // Normal in game mob deletions will not have inventory as we'll
    // drop it first, but when cleaning up after a game we just
    // delete the mob straight
    {
	ITEM		*cur, *next;

	// TODO: This is very dangerous!  We may have a quest
	// item or similar?
	// Ideally we'll assert false if inventory is present
	// except when doing map destruction.
	for (cur = myInventory; cur; cur = next)
	{
	    next = cur->getNext();

	    cur->setNext(0);
	    delete cur;
	}

	myInventory = 0;
    }

    // Mobs should only be destructed after they have been removed
    // from the relevant lists.
    UT_ASSERT(!myNext);

    glbMobCount--;
}

void
MOB::init()
{
    memset(glbKillCount, 0, sizeof(u16) * NUM_MOBS);
    ourAvatarDx = 0;
    ourAvatarDy = 0;
    ourAvatarMoveOld = true;
}

MOB *
MOB::getAvatar()
{
    return glbAvatar.getMob();
}

void
MOB::setAvatar(MOB *mob)
{
    glbAvatar.setMob(mob);
}

void
MOB::saveAvatar(SRAMSTREAM &os)
{
    glbAvatar.save(os);
}
    
void
MOB::loadAvatar(SRAMSTREAM &os)
{
    glbAvatar.load(os);
}
    

void
MOB::buildAscensionKit()
{
    ITEM		*item;

    // This is a recreation of a winning character.
    item = ITEM::createMagicItem(MAGICTYPE_HELM, HELM_WARNING);
    acquireItem(item);
    item = ITEM::createMagicItem(MAGICTYPE_AMULET, AMULET_LIFESAVING);
    acquireItem(item);
    item = ITEM::create(ITEM_FLAMESWORD);
    acquireItem(item);
    item = ITEM::create(ITEM_REFLECTSHIELD);
    acquireItem(item);
    item = ITEM::create(ITEM_PLATEMAIL);
    acquireItem(item);
    item = ITEM::createMagicItem(MAGICTYPE_RING, RING_REGENERATE);
    acquireItem(item);
    item = ITEM::createMagicItem(MAGICTYPE_BOOTS, BOOTS_SPEED);
    acquireItem(item);

    setIntrinsic(INTRINSIC_SKILL_ARMOUR_HELMET);
    setIntrinsic(INTRINSIC_SKILL_ARMOUR_SHIELD);
    setIntrinsic(INTRINSIC_SKILL_ARMOUR_BODY);
    setIntrinsic(INTRINSIC_SKILL_ARMOUR_BOOTS);
    setIntrinsic(INTRINSIC_SKILL_WEAPON_RANGED);
    setIntrinsic(INTRINSIC_SKILL_WEAPON_EDGED);
    setIntrinsic(INTRINSIC_SKILL_WEAPON_MEDIUM);
    setIntrinsic(INTRINSIC_SKILL_WEAPON_PARRY);
    
    setIntrinsic(INTRINSIC_SPELL_MAGICMISSILE);
    setIntrinsic(INTRINSIC_SPELL_SPARK);
    setIntrinsic(INTRINSIC_SPELL_LIGHTNINGBOLT);
    setIntrinsic(INTRINSIC_SPELL_REGENERATE);
    setIntrinsic(INTRINSIC_SPELL_SLOWPOISON);
    setIntrinsic(INTRINSIC_SPELL_HEAL);
    setIntrinsic(INTRINSIC_SPELL_CUREPOISON);
    setIntrinsic(INTRINSIC_SPELL_MAJORHEAL);
    setIntrinsic(INTRINSIC_SPELL_RESURRECT);
    setIntrinsic(INTRINSIC_SPELL_FORCEBOLT);
    setIntrinsic(INTRINSIC_SPELL_ACIDSPLASH);
    setIntrinsic(INTRINSIC_SPELL_DISINTEGRATE);
    setIntrinsic(INTRINSIC_SPELL_DIG);
    setIntrinsic(INTRINSIC_SPELL_TELEPORT);
    setIntrinsic(INTRINSIC_SPELL_BLINK);
    setIntrinsic(INTRINSIC_SPELL_IDENTIFY);
    setIntrinsic(INTRINSIC_SPELL_DETECTCURSE);
    setIntrinsic(INTRINSIC_SPELL_PETRIFY);

    myMP = 147;
    myMaxMP = 147;
    myMagicDie = 38;
    myHitDie = 26;
    myHP = 103;
    myMaxHP = 103;
}

MOB *
MOB::create(MOB_NAMES definition)
{
    MOB	*mob;

    mob = new MOB();
    
    mob->myOwnRef.createAndSetMOB(mob);
    
    mob->myDefinition = definition;
    mob->myOrigDefinition = definition;
    mob->myExpLevel = glb_mobdefs[definition].explevel;
    mob->myExp = 0;
    mob->myHitDie = glb_mobdefs[definition].hitdie * 2;
    mob->myHP = rand_dice(glb_mobdefs[definition].hp);
    mob->myMaxHP = mob->myHP;
    mob->myMagicDie = glb_mobdefs[definition].mpdie * 2;
    mob->myMP = rand_dice(glb_mobdefs[definition].mp);
    mob->myMaxMP = mob->myMP;
    mob->myNoiseLevel = 0;

    {
	GENDERCHANCE_NAMES	genderchance;
	int			male, female, neuter, percent;
	
	genderchance = (GENDERCHANCE_NAMES) glb_mobdefs[definition].gender;

	male = glb_genderchancedefs[genderchance].male;
	female = glb_genderchancedefs[genderchance].female;
	neuter = glb_genderchancedefs[genderchance].neuter;

	UT_ASSERT(male + female + neuter == 100);

	percent = rand_choice(100);

	if (percent < male)
	    mob->myGender = VERB_HE;
	else if (percent < male + female)
	    mob->myGender = VERB_SHE;
	else
	    mob->myGender = VERB_IT;

	// After all this, the avatar uses the global glbGender value
	// to seed their gender.
	if (definition == MOB_AVATAR)
	{
	    if (glbGender)
		mob->myGender = VERB_SHE;
	    else
		mob->myGender = VERB_HE;
	}
    }

    // Everyone starts of relatively well off.
    mob->myFoodLevel = 1500;

    mob->myIntrinsic.loadFromString(glb_mobdefs[definition].intrinsic);

    {
	// Determine handedness.
	ITEMSLOTSET_NAMES	slotset;

	slotset = (ITEMSLOTSET_NAMES) defn(definition).slotset;
	if (glb_itemslotsetdefs[slotset].lhand_name && 
	    glb_itemslotsetdefs[slotset].rhand_name)
	{
	    // 10% chance of a left handed creature.
	    if (rand_chance(10))
		mob->setIntrinsic(INTRINSIC_LEFTHANDED);
	}
    }

    // Set this out of range so we don't munge any old data.
    mob->myX = (u8) -1;
    mob->myY = (u8) -1;

    mob->setDLevel(-1);

    mob->myAIFSM = AI_FSM_JUSTBORN;
    mob->myAIState = 0;

    // Create some treasure...
    ITEM		*item;

    if (glbTutorial && definition == MOB_AVATAR)
    {
	// Special case for the tutorial.  You start with a chainmail.
	item = ITEM::create(ITEM_CHAINMAIL, false, true);
	if (item) mob->acquireItem(item);

	// Because it is a tutorial, we take pity and give the player
	// some bonus strength.
	mob->myMP = 30;
	mob->myMaxMP = 30;
	mob->myMagicDie = 4;
	mob->myHitDie = 4;
        mob->myHP = 30;
	mob->myMaxHP = 30;
    }
    else if (definition == MOB_AVATAR)
    {
	// You are lucky!  You always start with a weapon, an armour,
	// a book, and a random item.
	// Rethink...  You start with a balanced set that
	// is always balanced.
	// 
	// You always wear some clothing as it is otherwise
	// ugly to see your naked self.  (Some may say the charm
	// of POWDER is starting naked, though :>)
	// Your weapon strength is the inverse of the body armour
	//
	// DSR: Don't update the comments much do you?  :)
	//
	// JML: I believe comments form a place to engage in a dialog
	// to better understand not just what the code is, but what it
	// was, so people can gain the sense of history without
	// resorting to svn blame.  Which is a rationalization for
	// my not updating them much.
	// The *really* long dialog is in the createNPC function, IIRC.
	//
	ITEM_NAMES		armour, weapon;
	armour = ITEM_PLATEMAIL;
	weapon = ITEM_WARHAMMER;

	// Did the user pick something specific?  Give him ...
	// ...well, more or less what he asked for ;)
	//
	// The user's pick is judged by the currently worshiped god.
	//
	// The adventurer and cultist can have the standard random
	// selection -- the first is true to original POWDER,
	// and the second, well, Xom is the god of "random"...

	/////////////////////////////////////////////////////////////////////
	//
	// The Fighter
	//
	// Always gets an edged/blunt large or medium weapon.
	// Always gets metal 'fabricated' armor.
	//
	// Never gets the "guile" skill book as his guaranteed skillbook.
	// Never gets the "divination" spell book as his guaranteed spellbook.
	if (piety_chosengod() == GOD_FIGHTER)
	{
	    ITEM_NAMES weplist[] =
	    {
		ITEM_LONGSWORD,
		ITEM_SILVERSWORD,
		ITEM_WARHAMMER,
		ITEM_MACE
	    };
	    ITEM_NAMES armlist[] =
	    {
		ITEM_PLATEMAIL,
		ITEM_SPLINTMAIL,
		ITEM_BANDEDMAIL,
		ITEM_CHAINMAIL
	    };
	    weapon = weplist[rand_choice(sizeof(weplist)/sizeof(ITEM_NAMES))];
	    armour = armlist[rand_choice(sizeof(armlist)/sizeof(ITEM_NAMES))];
	}

	// The Wizard
	//
	// Always gets a 'small' weapon.
	// Always gets a robe.
	//
	// Gets two spell books (note he still starts with
	// one skill and one spell slot, he just gets better choice
	// in his starting spells).
	else if (piety_chosengod() == GOD_WIZARD)
	{
	    ITEM_NAMES weplist[] =
	    {
		ITEM_KNIFE,
		ITEM_DAGGER,
		ITEM_SILVERDAGGER,
		ITEM_CLUB,
		ITEM_SHORTSWORD
	    };
	    weapon = weplist[rand_choice(sizeof(weplist)/sizeof(ITEM_NAMES))];
	    armour = ITEM_ROBE;
	}

	// 
	// The Ranger
	//
	// Always gets a bow and some arrows, OR a 'pointed' weapon.
	// Always gets cloth or leather armor.
	//
	// Never receives the book of H'ruth as his guaranteed skillbook.
	// Never receives the book of healing as his guaranteed spellbook.
	else if (piety_chosengod() == GOD_ROGUE)
	{
	    ITEM_NAMES weplist[] =
	    {
		ITEM_BOW,
		ITEM_DAGGER,
		ITEM_SILVERDAGGER,
		ITEM_SPEAR,
		ITEM_SILVERSPEAR,
		ITEM_RAPIER
	    };
	    ITEM_NAMES armlist[] =
	    {
		ITEM_ROBE,
		ITEM_LEATHERTUNIC,
		ITEM_STUDDEDLEATHER
	    };
	    weapon = weplist[rand_choice(sizeof(weplist)/sizeof(ITEM_NAMES))];
	    armour = armlist[rand_choice(sizeof(armlist)/sizeof(ITEM_NAMES))];
	}

	// The Cleric
	//
	// Always gets a blunt weapon.
	// Only one who can start with the 'special' armors;
	// will never start with leather.
	//
	// Never receives the book of Necromancy or Death as his guaranteed
	// spellbook.
	else if (piety_chosengod() == GOD_CLERIC)
	{
	    ITEM_NAMES weplist[] =
	    {
		ITEM_CLUB,
		ITEM_MACE,
		ITEM_WARHAMMER
	    };
	    ITEM_NAMES armlist[] =
	    {
		ITEM_ROBE,
		ITEM_CHAINMAIL,
		ITEM_MITHRILMAIL,
		ITEM_BANDEDMAIL,
		ITEM_SPLINTMAIL,
		ITEM_PLATEMAIL,
		ITEM_CRYSTALPLATE
	    };
	    weapon = weplist[rand_choice(sizeof(weplist)/sizeof(ITEM_NAMES))];
	    armour = armlist[rand_choice(sizeof(armlist)/sizeof(ITEM_NAMES))];
	}

	// The Necromancer
	//
	// Always starts with a knife and wearing a robe.
	//
	// Always starts with either the book of Death or Necromancy
	// as his guaranteed spellbook, to compensate.
	else if (piety_chosengod() == GOD_NECRO)
	{
	    weapon = ITEM_KNIFE;
	    armour = ITEM_ROBE;
	}

	// The Barbarian
	//
	// Always starts with a bashing weapon or spear.
	// Always starts with leather armor of some form.
	//
	// Begins with two skill books; note that he still receives
	// a single spell and a single skill slot, however.
	else if (piety_chosengod() == GOD_BARB)
	{
	    ITEM_NAMES weplist[] =
	    {
		ITEM_CLUB,
		ITEM_MACE,
		ITEM_WARHAMMER,
		ITEM_SPEAR,
		ITEM_SILVERSPEAR
	    };
	    ITEM_NAMES armlist[] =
	    {
		ITEM_LEATHERTUNIC,
		ITEM_STUDDEDLEATHER
	    };
	    weapon = weplist[rand_choice(sizeof(weplist)/sizeof(ITEM_NAMES))];
	    armour = armlist[rand_choice(sizeof(armlist)/sizeof(ITEM_NAMES))];
	}

	// The Cultist
	//
	// Befitting the random nature, you get a random weapon and
	// random armour with no attempt at balance.  Your special
	// books are likewise entirely random.
	else if (piety_chosengod() == GOD_CULTIST)
	{
	    ITEM	*temp;
	    temp = ITEM::createRandomType(ITEMTYPE_WEAPON);
	    weapon = temp->getDefinition();
	    delete temp;
	    temp = ITEM::createRandomType(ITEMTYPE_ARMOUR);
	    armour = temp->getDefinition();
	    delete temp;
	}
	// The Agnostic
	//
	// The classic weapon set that is good for any problem.  This
	// is also the default for new players.
	else if (piety_chosengod() == GOD_AGNOSTIC)
	{
	    switch (rand_choice(3))
	    {
		case 0:		// Robe
		{
		    ITEM_NAMES	list[] = 
		    {
			ITEM_LONGSWORD,
			ITEM_SPEAR,
			ITEM_MACE,
			ITEM_RAPIER
		    };
		    armour = ITEM_ROBE;
		    weapon = list[rand_choice(sizeof(list)/sizeof(ITEM_NAMES))];
		    break;
		}
		case 1:		// Leather
		{
		    ITEM_NAMES	list[] = 
		    {
			ITEM_SHORTSWORD,
			ITEM_CLUB
		    };
		    armour = ITEM_LEATHERTUNIC;
		    weapon = list[rand_choice(sizeof(list)/sizeof(ITEM_NAMES))];
		    break;
		}
		case 2:		// Studded Leather
		{
		    ITEM_NAMES	list[] = 
		    {
			ITEM_DAGGER,
			ITEM_KNIFE,
			ITEM_BOW
		    };
		    armour = ITEM_STUDDEDLEATHER;
		    weapon = list[rand_choice(sizeof(list)/sizeof(ITEM_NAMES))];
		    break;
		}
	    }
	}

	item = ITEM::create(armour);
	if (item)
	{
	    item->makeVanilla();
	    mob->acquireItem(item);
	    mob->actionEquip(item->getX(), item->getY(),
			    ITEMSLOT_BODY, true);
	}
	item = ITEM::create(weapon);
	if (item)
	{
	    item->makeVanilla();
	    mob->acquireItem(item);
	    mob->actionEquip(item->getX(), item->getY(),
			    (item->getDefinition() == ITEM_BOW) ?
				ITEMSLOT_LHAND :
				ITEMSLOT_RHAND, true);
	}
	// If the item is a bow, add arrows.  If it is arrows,
	// add a bow.
	if (item && item->getDefinition() == ITEM_BOW)
	{
	    item = ITEM::create(ITEM_ARROW, false);
	    if (item)
	    {
		item->makeVanilla();
		item->setStackCount(7);
		item->markQuivered();
		mob->acquireItem(item);
	    }
	}

	// We want to create on spellbook and one skill book.
	{
	    SPELLBOOK_NAMES		spelllist[NUM_SPELLBOOKS];
	    SPELLBOOK_NAMES		skilllist[NUM_SPELLBOOKS];
	    int				numspell, numskill;
	    SPELLBOOK_NAMES		book;

	    numspell = 0;
	    numskill = 0;
	    FOREACH_SPELLBOOK(book)
	    {
		if (*glb_spellbookdefs[book].spells)
		{
		    // Note the funny use of continue
		    // inside swtich to abort adding spells/skills
		    switch (piety_chosengod())
		    {
			case GOD_BARB:
			    continue;
			case GOD_NECRO:
			    if (book != SPELLBOOK_DEATH &&
				book != SPELLBOOK_NECRO)
				continue;
			    break;
			case GOD_CLERIC:
			    if (book == SPELLBOOK_DEATH ||
				book == SPELLBOOK_NECRO)
				continue;
			    break;
			case GOD_FIGHTER:
			    if (book == SPELLBOOK_DIVINATION)
				continue;
			    break;
			case GOD_ROGUE:
			    if (book == SPELLBOOK_HEAL)
				continue;
			    break;
			case GOD_CULTIST:
			    // Both books are of any type.
			    skilllist[numskill++] = book;
			    break;
			case NUM_GODS:
			case GOD_AGNOSTIC:
			    // Nothing special
			    break;
			case GOD_WIZARD:
			    // Get only spell books so add to skill list.
			    skilllist[numskill++] = book;
			    break;
		    }
		    // It has spells
		    spelllist[numspell++] = book;
		}
		else
		{
		    // Note the funny use of continue
		    // inside swtich to abort adding spells/skills
		    switch (piety_chosengod())
		    {
			case GOD_BARB:
			    // Barbarians get two skill books
			    spelllist[numspell++] = book;
			    break;
			case GOD_NECRO:
			case GOD_CLERIC:
			    break;
			case GOD_FIGHTER:
			    // The fighter never gets guile
			    if (book == SPELLBOOK_ROGUE)
				continue;
			    break;
			case GOD_ROGUE:
			    // The ranger never gets H'ruth
			    if (book == SPELLBOOK_BARB)
				continue;
			    break;
			case GOD_CULTIST:
			    // Both books are of any type.
			    spelllist[numspell++] = book;
			    break;
			case NUM_GODS:
			case GOD_AGNOSTIC:
			    // Nothing special
			    break;
			case GOD_WIZARD:
			    // No skill books for wizards
			    continue;
		    }
		    skilllist[numskill++] = book;
		}
	    }

	    // Pick one random of each...
	    book = spelllist[rand_choice(numspell)];
	    item = ITEM::createMagicItem(MAGICTYPE_SPELLBOOK, book, false);
	    if (item) mob->acquireItem(item);

	    book = skilllist[rand_choice(numskill)];
	    item = ITEM::createMagicItem(MAGICTYPE_SPELLBOOK, book, false);
	    if (item) mob->acquireItem(item);
	}

	// The bonus item!
	item = ITEM::createRandom();
	if (item) mob->acquireItem(item);
    }
    else
    {
	// Standard treasure is a random item...
	// We, however, don't give it to every creature all the time.
	if (rand_chance(10))
	{
	    item = ITEM::createRandom();
	    mob->acquireItem(item);
	}
    }

    // Add all the standard loot the critter has.
    const char		*loot;

    loot = glb_mobdefs[definition].loot;
    while (*loot)
    {
	item = ITEM::create((ITEM_NAMES) *loot);
	mob->acquireItem(item);
	loot++;
    }
    
    // Likewise, any loot types...
    loot = glb_mobdefs[definition].loottype;
    while (*loot)
    {
	item = ITEM::createRandomType((ITEMTYPE_NAMES) *loot);
	mob->acquireItem(item);
	loot++;
    }

//    if (definition == MOB_AVATAR)
//	mob->setIntrinsic(INTRINSIC_SPELL_SUMMON_IMP);
    // Debug insta assign for item testing...
#if 0
    for (int i = 0; i < 15; i++)
    {
	item = ITEM::create(ITEM_WATER);
	mob->acquireItem(item);
    }
#endif
#if 0
    if (definition == MOB_AVATAR)
    {
	mob->setIntrinsic(INTRINSIC_LICHFORM);
	item = ITEM::create(ITEM_BLACKHEART);
	mob->acquireItem(item);
	item = ITEM::create(ITEM_YRUNE);
	mob->acquireItem(item);
	item = ITEM::create(ITEM_ARROW);
	mob->acquireItem(item);
	item = ITEM::create(ITEM_BOW);
	mob->acquireItem(item);
	// Test cleric dress.
	item = ITEM::create(ITEM_SANDALS);
	mob->acquireItem(item);
	item = ITEM::create(ITEM_RUBYNECKLACE);
	mob->acquireItem(item);
	item = ITEM::create(ITEM_CHAINMAIL);
	mob->acquireItem(item);
	// Verify mini icons.
	item = ITEM::create(ITEM_SCROLL_FOOBAR);
	mob->acquireItem(item);
	item = ITEM::create(ITEM_PURPLEWAND);
	mob->acquireItem(item);
    }
#endif

    // Simple hash.
    if (definition == MOB_AVATAR)
    {
#ifndef iPOWDER
	if (rand_hashstring(glbAvatarName) == 3939879139u)
	{
	    glbWizard = true;
	}
	else
	{
	    glbWizard = false;
	}
#endif
    }

#if 0
    // Report the hash to make it easy to change the passwords.
    if (definition == MOB_AVATAR)
    {
	BUF		buf;
	
	buf.sprintf("%x  ", rand_hashstring(glbAvatarName));
	msg_report(buf);
    }
#endif

#if 0
    // You can now wish for all this.
    if (definition == MOB_AVATAR && glbWizard)
    {
	mob->setIntrinsic(INTRINSIC_SPELL_RESURRECT);
	mob->setIntrinsic(INTRINSIC_SPELL_WIZARDSEYE);
	mob->setIntrinsic(INTRINSIC_SPELL_POSSESS);
	mob->setIntrinsic(INTRINSIC_SPELL_BLINK);
	mob->setIntrinsic(INTRINSIC_SPELL_FETCH);
	mob->setIntrinsic(INTRINSIC_SPELL_BINDSOUL);
	mob->setIntrinsic(INTRINSIC_SPELL_ENTOMB);
	mob->setIntrinsic(INTRINSIC_SPELL_PETRIFY);
	mob->setIntrinsic(INTRINSIC_SPELL_DETECTCURSE);
	mob->setIntrinsic(INTRINSIC_SPELL_IDENTIFY);
	mob->setIntrinsic(INTRINSIC_SPELL_MAJORHEAL);
	mob->setIntrinsic(INTRINSIC_SPELL_FORCEBOLT);
	mob->setIntrinsic(INTRINSIC_SPELL_ACIDSPLASH);
	mob->setIntrinsic(INTRINSIC_SPELL_CORROSIVEEXPLOSION);
	mob->setIntrinsic(INTRINSIC_SPELL_CREATEPIT);
	mob->setIntrinsic(INTRINSIC_SPELL_TRACK);
	mob->setIntrinsic(INTRINSIC_SPELL_DIAGNOSE);
	mob->setIntrinsic(INTRINSIC_SPELL_ROLLINGBOULDER);
	mob->setIntrinsic(INTRINSIC_SPELL_SUMMON_DEMON);
	mob->setIntrinsic(INTRINSIC_SPELL_LIVINGFROST);
	mob->setIntrinsic(INTRINSIC_SPELL_BLIZZARD);
	mob->setIntrinsic(INTRINSIC_SPELL_PRESERVE);
	mob->setIntrinsic(INTRINSIC_SPELL_DIRECTWIND);
	mob->setIntrinsic(INTRINSIC_SPELL_STICKYFLAMES);
	mob->setIntrinsic(INTRINSIC_SKILL_WEAPON_RIPOSTE);
	mob->setIntrinsic(INTRINSIC_SKILL_WEAPON_TRUEAIM);
	mob->setIntrinsic(INTRINSIC_SKILL_CHARGE);
	mob->setIntrinsic(INTRINSIC_SKILL_LEAPATTACK);
	mob->setIntrinsic(INTRINSIC_SKILL_TWOWEAPON);
	mob->setIntrinsic(INTRINSIC_SKILL_WEAPON_DISARM);
	mob->setIntrinsic(INTRINSIC_SPELL_DISINTEGRATE);
	mob->setIntrinsic(INTRINSIC_SPELL_RAISE_UNDEAD);
	mob->setIntrinsic(INTRINSIC_SPELL_RECLAIM_SOUL);
	mob->setIntrinsic(INTRINSIC_SPELL_DARK_RITUAL);
	mob->setIntrinsic(INTRINSIC_SPELL_FORCEWALL);
	mob->setIntrinsic(INTRINSIC_SPELL_SUMMON_FAMILIAR);
	mob->setIntrinsic(INTRINSIC_SPELL_TRANSFER_KNOWLEDGE);

	mob->setIntrinsic(INTRINSIC_REFLECTION);
	mob->setIntrinsic(INTRINSIC_JUMP);
	mob->myMP = 200;
//	mob->myMagicDie = 100;
	mob->myHitDie = 8;
	mob->myMaxMP = 200;
        mob->myHP = 100;
	mob->myMaxHP = 100;
    }
#endif

#if 0
    for (int i = 0; definition == MOB_AVATAR && i < 90; i++)
    //for (int i = 0; i < 90; i++)
    {
	// item = ITEM::createRandom();
	// We can also create a bunch of items of a given type, useful
	// for testing...
	item = ITEM::createRandomType(ITEMTYPE_WAND);
	mob->acquireItem(item);
	item = ITEM::createRandomType(ITEMTYPE_WEAPON);
	mob->acquireItem(item);
    }
    // mob->myMagicDie = 38;
    // mob->myHitDie = 38;
#endif
    if (glbStressTest && !glbMapStats && definition == MOB_AVATAR)
    {
	if (rand_chance(80))
	{
	    mob->buildAscensionKit();
	}
	else
	{
	    int		i, n;
	    SPELL_NAMES	spell;
	    SKILL_NAMES	skill;

	    // A much more random character...
	    n = 20;
	    for (i = 0; i < 20; i++)
	    {
		item = ITEM::createRandomType(ITEMTYPE_ANY);
		mob->acquireItem(item);
	    }

	    n = rand_range(1, 20);
	    for (i = 0; i < n; i++)
	    {
		spell = (SPELL_NAMES) rand_range(1, NUM_SPELLS-1);
		mob->setIntrinsic((INTRINSIC_NAMES)glb_spelldefs[spell].intrinsic);
	    }
	    n = rand_range(1, 20);
	    for (i = 0; i < n; i++)
	    {
		skill = (SKILL_NAMES) rand_range(1, NUM_SKILLS-1);
		mob->setIntrinsic((INTRINSIC_NAMES)glb_skilldefs[skill].intrinsic);
	    }
	    mob->myMP = rand_range(30, 150);
	    mob->myMaxMP = mob->myMP;
	    mob->myMagicDie = mob->myMP / 7;
	    mob->myHP = rand_range(30, 150);
	    mob->myMaxHP = mob->myHP;
	    mob->myHitDie = mob->myHP / 7 + 1;
	}
    }
    
    // Make everyone do a poly dance.
    // mob->setIntrinsic(INTRINSIC_POLYMORPH);

    return mob;
}

void
MOB::changeDefinition(MOB_NAMES definition, bool reset)
{
    myDefinition = definition;
    myOrigDefinition = definition;

    if (reset)
    {
	myExpLevel = glb_mobdefs[definition].explevel;
	myExp = 0;
	myHitDie = glb_mobdefs[definition].hitdie * 2;
	myHP = rand_dice(glb_mobdefs[definition].hp);
	myMaxHP = myHP;
	myMagicDie = glb_mobdefs[definition].mpdie * 2;
	myMP = rand_dice(glb_mobdefs[definition].mp);
	myMaxMP = myMP;
	myNoiseLevel = 0;

	myIntrinsic.clearAll();
	myIntrinsic.loadFromString(glb_mobdefs[definition].intrinsic);
    }
}

MOB_NAMES
MOB::chooseNPC(int threatlevel)
{
    int		percentages[NUM_MOBS];
    int		total = 0;
    int		explevel, guess;
    bool	fuck_the_player = false;
    MOB_NAMES	def;

    if (piety_chosengod() == GOD_CULTIST)
    {
	if (rand_choice(100) == 66)
	{
	    msg_report("><0|V| laughs.");
	    fuck_the_player = true;
	}
    }
    else
    {
	if (rand_choice(5000) == 666)
	    fuck_the_player = true;
    }

    if (threatlevel < 0)
	threatlevel = 0;

    threatlevel = threatlevel / 2;
    
    FOREACH_MOB(def)
    {
	if (def == MOB_NONE)
	    percentages[def] = 0;
	else if (def == MOB_AVATAR)
	    percentages[def] = 0;
	else
	{
	    // The "ExpLevel" is the rough guess of the strength of the mob.
	    explevel = glb_mobdefs[def].explevel;
	    
	    // We want to create mobs matching our adjusted threatlevel.
	    // We do this by biassing the creation.  Anything within two 
	    // levels gets a value of 65536.  For every level outside
	    // that, it is dropped in half, to a minimum of 1.  So
	    // you could get all powerful creatures in level 1.
	    // This gives a level difference of 18 to mean there isn't
	    // a chance in one direction of successful combat.

	    // I don't like the above schema very much, a level 1 character
	    // has a 1/32 bias against a dragon of any type.  As there
	    // is a peak of dragons currnetly, (32 possible monsters, 3
	    // are non-green dragons, chance of non-green dragon is 
	    // one in 341 chance of a dragon.  
	    // I'd like something 8 levels away to be 1/1000 chance.
	    
	    // New system:
	    // Things your level have chance 1024.
	    // Every level above your level halves chance, for 1 chance
	    // at 10 over (7 over on dragons is 1/128, or 8)
	    // Below you falls off slower, and halves every two levels.
	    // Creatures an odd number below are 75%.

	    // Screw these schemas.  They are all crap.
	    // The first rule is KEEP IT SIMPLE STUPID.
	    // I obviously am not sufficiently stupid.
	    // The new perfect rule is this:
	    // 1) All mobs less than the threat level have equal chance
	    //    of spawning.
	    // 2) One higher has 50%.  Two higher 10%.
	    // 3) More than two higher?  0%.
	    // If the fuck_the_player flag is set, all mobs are equally likely.
	    if (explevel <= threatlevel)
		percentages[def] = 10;
	    else if (explevel <= threatlevel+1)
		percentages[def] = 5;
	    else if (explevel <= threatlevel+2)
		percentages[def] = 1;
	    else
		percentages[def] = 0;

	    if (fuck_the_player)
		percentages[def] = 1;
#if 0
	    // You know... I think I have the following set entirely backwards.
	    // That likely explains why it sucked so much :>
	    if (threatlevel == explevel)
		percentages[def] = 1024;
	    else if (threatlevel < explevel)
	    {
		int		odd;
		
		diff = explevel - threatlevel - 1;
		if (diff < 0)
		{
		    percentages[def] = 1024;
		}
		else if (diff >= 20)
		{
		    percentages[def] = 1;
		}
		else
		{
		    odd = diff & 1;
		    diff = (diff + 1) >> 1;
		    percentages[def] = 1 << (10 - diff);
		    if (odd)
			percentages[def] += percentages[def] >> 1;
		}
	    }
	    else
	    {
		diff = threatlevel - explevel;
		if (diff >= 10)
		    percentages[def] = 1;
		else
		    percentages[def] = 1 << (10 - diff);
	    }
#endif
#if 0
	    if (abs(threatlevel - explevel) <= 2)
		percentages[def] = 65536;
	    else
	    {
		diff = abs(threatlevel - explevel) - 2;
		if (diff > 16)
		    percentages[def] = 1;
		else
		    percentages[def] = 1 << (16 - diff);
	    }
#endif

	    // The percentage is then modulated by the rarity.  100 is
	    // common, smaller numbers less common.
	    // This can reduce the percentage to 0.
	    percentages[def] *= glb_mobdefs[def].rarity;
	}
#if 0
	BUF		buf;
	buf.sprintf("%s - %d: %d.  ",
		glb_mobdefs[def].name, def, percentages[def]);
	msg_report(buf);
#endif

	total += percentages[def];
    }

    guess = rand_choice(total);

#if 0
    {
	BUF		buf;
	buf.sprintf("%d of %d, ", guess, total);
	msg_report(buf);
    }
#endif

    total = 0;
    // NB: We could store the sum in percentages rather than the individual
    // value, then do a binary search here.  (or even phonebook search!)
    FOREACH_MOB(def)
    {
	total += percentages[def];
	if (total > guess)
	    break;
    }
#if 0
    {
	BUF		buf;
	buf.sprintf("%d.  ", def);
	msg_report(buf);
    }
#endif

    if (def == NUM_MOBS)
    {
	// Ran through wihtout a match, quite a bad case...
	UT_ASSERT(!"Didn't find a mob!");
	def = (MOB_NAMES) (def-1);
    }

    // Note we CANNOT hard code to a single MOB as then
    // self-poly prevention triggers an infinite loop!
#if 0
    if (rand_choice(2))
        def =  MOB_GELATINOUSCUBE;
    else
	def =  MOB_GOLDBEETLE;
#endif
    
    return def;
}

MOB *
MOB::createNPC(int threatlevel)
{
    MOB_NAMES	def;

    def = chooseNPC(threatlevel);

    return create(def);
}

void
MOB::makeUnique()
{
    // Build the unique name.
    const char *syllable[] =
    {
	"gor",
	"blad",
	"ret",
	"waz",
	"por",
	"ylem",
	"kral",
	"qual",
	"t'rl",
	"zam",
	"jon",
	"eri",
	"opi",
	"hjur",
	0
    };
    int		i, n;
    BUF		buf;
    
    setIntrinsic(INTRINSIC_UNIQUE);

    for (n = 0; syllable[n]; n++);
    
    buf.strcpy(syllable[rand_choice(n)]);
    if (rand_chance(90))
	buf.strcat(syllable[rand_choice(n)]);

    christen(buf);

    // Buf the mob.
    myMaxHP = myMaxHP * 2 + 20;
    myHP = myMaxHP;
    if (myMP)
    {
	myMaxMP = myMaxMP * 2 + 40;
	myMP = myMaxMP;
    }

    myHitDie = myHitDie * 2;
    myMagicDie = myMagicDie * 2;
    myExpLevel = myExpLevel * 2;

    // We get extra bonus loot!
    i = 0;

    while (i < 5)
    {
	ITEM		*loot;
	loot = ITEM::createRandom();
	acquireItem(loot);
	i++;
	if (rand_chance(30))
	    break;
    }

    // We get bonus intrinsics.
    INTRINSIC_NAMES		intrinsics[] =
    {
	INTRINSIC_QUICK,
	INTRINSIC_UNCHANGING,
	INTRINSIC_RESISTSTONING,
	INTRINSIC_RESISTFIRE,
	INTRINSIC_RESISTCOLD,
	INTRINSIC_RESISTSHOCK,
	INTRINSIC_RESISTACID,
	INTRINSIC_RESISTPOISON,
	INTRINSIC_RESISTPHYSICAL,
	INTRINSIC_TELEPORTCONTROL,
	INTRINSIC_REGENERATION,
	INTRINSIC_REFLECTION,
	INTRINSIC_SEARCH,	// In someways, this is bad :>
	INTRINSIC_INVISIBLE,
	INTRINSIC_SEEINVISIBLE,
	INTRINSIC_TELEPATHY,
	INTRINSIC_NOBREATH,
	INTRINSIC_LIFESAVE,	// This really sucks on a creature.
				// Got axed by a red dragon unique
				// with this.
	INTRINSIC_RESISTSLEEP,
	INTRINSIC_FREEDOM,
	INTRINSIC_WATERWALK,
	INTRINSIC_JUMP,
	INTRINSIC_WARNING,
	INTRINSIC_LICHFORM,	// I look forward to this triggering!
				// This has proven the most fun thing for
				// people to discover!
	INTRINSIC_DIG,		// Likely nerfed as the ai doesn't wander 
				// into walls
	INTRINSIC_NONE
    };

    for (n = 0; intrinsics[n] != INTRINSIC_NONE; n++);

    setIntrinsic(intrinsics[rand_choice(n)]);
}

MOVE_NAMES
MOB::getMoveType() const
{
    MOVE_NAMES		movetype;
    
    movetype = (MOVE_NAMES) glb_mobdefs[myDefinition].movetype;
    
    // The avatar is a special case: They are allowed to
    // jump in water even though they can't swim.
    if (isAvatar())
    {
	movetype = (MOVE_NAMES) (movetype | MOVE_STD_SWIM);
    }
    else if (hasIntrinsic(INTRINSIC_WATERWALK))
    {
	// If we have water walking and are not the avatar, we can swim
	movetype = (MOVE_NAMES) (movetype | MOVE_STD_SWIM);
    }

    return movetype;
}

bool
MOB::canMove(int nx, int ny, bool checkitem, bool allowdoor, bool checksafety) const
{
    int		tile;
    
    if (nx < 0 || ny < 0 || nx >= MAP_WIDTH || ny >= MAP_HEIGHT)
	return false;

    if (getSize() >= SIZE_GARGANTUAN)
	if (nx >= MAP_WIDTH-1 || ny >= MAP_HEIGHT-1)
	    return false;

    // Check for items.
    if (checkitem)
    {
	ITEM		*item;

	item = glbCurLevel->getItem(nx, ny);
	if (item && !item->isPassable())
	    return false;
    }
    
    // Get the relevant tile...
    tile = glbCurLevel->getTile(nx, ny);
    if ( (glb_squaredefs[tile].movetype & getMoveType()) ||
	 (allowdoor && tile == SQUARE_DOOR) ||
	 (allowdoor && isAvatar() && tile == SQUARE_BLOCKEDDOOR))
    {
	// Easy as pie...
	if (getSize() >= SIZE_GARGANTUAN)
	{
	    if ( (glb_squaredefs[glbCurLevel->getTile(nx+1,ny)].movetype & getMoveType()) &&
	         (glb_squaredefs[glbCurLevel->getTile(nx+1,ny+1)].movetype & getMoveType()) &&
	         (glb_squaredefs[glbCurLevel->getTile(nx,ny+1)].movetype & getMoveType()) )
	    {
		// fall through
	    }
	    else
		return false;
	}
    }
    else
	return false;

    if (checksafety)
    {
	// Prohibit moving to tiles that will damage us.
	switch (tile)
	{
	    case SQUARE_LAVA:
		if (!hasIntrinsic(INTRINSIC_RESISTFIRE) ||
		     hasIntrinsic(INTRINSIC_VULNFIRE))
		    return false;
		break;
	    case SQUARE_ACID:
		// Flying critters are safe from acid,
		if (!(getMoveType() & MOVE_FLY))
		{
		    // Verify we actually care about acid damage
		    if (!hasIntrinsic(INTRINSIC_RESISTACID) ||
			 hasIntrinsic(INTRINSIC_VULNACID))
		    {
			return false;
		    }
		}
		break;
	    case SQUARE_STARS1:
	    case SQUARE_STARS2:
	    case SQUARE_STARS3:
	    case SQUARE_STARS4:
	    case SQUARE_STARS5:
		if (!hasIntrinsic(INTRINSIC_NOBREATH))
		    return false;
		break;
	}
    }
    return true;
}

bool
MOB::canMoveDelta(int dx, int dy, bool checkitem, bool allowdoor, bool checksafety) const
{
    if (!dx || !dy)
	return canMove(getX()+dx, getY()+dy, checkitem, allowdoor, checksafety);

    // Verify either ortho directions satisfy regular movement.
    if (!canMove(getX()+dx, getY(), false, false, false) &&
        !canMove(getX(), getY()+dy, false, false, false))
    {
	return false;
    }

    // Now go back to regular canMove
    return canMove(getX()+dx, getY()+dy, checkitem, allowdoor, checksafety);
}

bool
MOB::canSense(const MOB *mob, bool checklos, bool onlysight) const
{
    // We try to do these tests from the fastest to slowest.

    // This fails from ::refresh for no readily apparent reason!
    // UT_ASSERT(mob != 0);

    int			dist, distx, disty;

    // A quick sanity test on the mob.
    if (!mob)
	return false;

    bool		thisdoublesize = getSize() >= SIZE_GARGANTUAN;
    bool		mobdoublesize = mob->getSize() >= SIZE_GARGANTUAN;

    // Obviously, different levels are illegal.  Either falling
    // through a trap door, or perhaps -1 because it has not
    // yet been created.
    if (mob->getDLevel() != getDLevel())
	return false;

    // Mobs start with -1 position, which should always be out of sight.
    if (mob->getX() == -1 || mob->getX() == 255)
	return false;

    distx = abs(getX() - mob->getX());
    disty = abs(getY() - mob->getY());
    dist = distx + disty;

    // See if the target has position revealed, in which case
    // everyone gets to see them.
    if (!onlysight && mob->hasIntrinsic(INTRINSIC_POSITIONREVEALED))
    {
	return true;
    }

    // See if we can hear our target.
    if (!onlysight && !hasIntrinsic(INTRINSIC_DEAF))
    {
	// You hear the monster.
	// You are not allowed to hear yourself!
	// Hearing is a tricky business, you don't always hear things.
	if (dist && dist <= mob->getNoiseLevel(this))
	    return true;
    }

    if (!onlysight && mob->isSlave(this))
    {
	if (hasIntrinsic(INTRINSIC_TELEPATHY))
	    return true;
    }

    if (!onlysight && hasIntrinsic(INTRINSIC_WARNING))
    {
	if (dist < 5)
	    return true;
    }

    // First, if this is blind, we can see iff we have telepathy and
    // the target has a mind...
    if (hasIntrinsic(INTRINSIC_BLIND))
    {
	if (onlysight)
	    return false;
	
	if (mob->getSmarts() != 0 &&
	    hasIntrinsic(INTRINSIC_TELEPATHY))
	{
	    return true;
	}

	// Being blind doesn't stop you from seeing yourself.
	if (mob == this)
	   return true;

	return false;
    }

    // From here on are only sight checks.

    // Now, if the target is invisible, make sure we can see invisible
    // or we can't possibly sense.
    if (mob->hasIntrinsic(INTRINSIC_INVISIBLE))
    {
	if (!hasIntrinsic(INTRINSIC_SEEINVISIBLE))
	    return false;
    }

    // We could potentially see the mob in the usual fashion.
    // We now check to see if the mob is lit...
    // We can always be aware of mobs beside us even in the dark.
    if (mob != this &&
	!mobdoublesize && 
	(MAX(distx, disty) > 1) && 
	!glbCurLevel->isLit(mob->getX(), mob->getY()))
	return false;

    // Giant mobs have looser is lit requirements.
    if (mob != this &&
	 mobdoublesize &&
	 (MAX(distx, disty) > 1) &&
	!glbCurLevel->isLit(mob->getX(), mob->getY()) &&
	!glbCurLevel->isLit(mob->getX()+1, mob->getY()) &&
	!glbCurLevel->isLit(mob->getX(), mob->getY()+1) &&
	!glbCurLevel->isLit(mob->getX()+1, mob->getY()+1) )
	return false;

    // If either of the mobs is in a pit, we can only see each other
    // if one square away.
    if (mob->hasIntrinsic(INTRINSIC_INPIT) || hasIntrinsic(INTRINSIC_INPIT))
    {
	if (MAX(distx, disty) > 1)
	{
	    return false;
	}
    }

    // If target is in tree, but we are not, only visible at one
    // square
    if (mob->hasIntrinsic(INTRINSIC_INTREE) && !hasIntrinsic(INTRINSIC_INTREE))
    {
	if (MAX(distx, disty) > 1)
	{
	    return false;
	}
    }

    // If one of the mobs is submerged and the other is not, there is
    // no way to see.
    if (mob->hasIntrinsic(INTRINSIC_SUBMERGED) ^ hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	return false;
    }

    if (!checklos)
	return true;

    // Finally, ensure we have LOS.
    UT_ASSERT(mob->getX() >= 0 && mob->getX() < MAP_WIDTH);
    UT_ASSERT(mob->getY() >= 0 && mob->getY() < MAP_HEIGHT);
    if (glbCurLevel->hasLOS(getX(), getY(), mob->getX(), mob->getY()))
    {
	return true;
    }
    // If we are huge, we gotta check rest of the mob..
    if (thisdoublesize &&
	(glbCurLevel->hasLOS(getX()+1, getY(), mob->getX(), mob->getY()) ||    
	 glbCurLevel->hasLOS(getX(), getY()+1, mob->getX(), mob->getY()) ||    
	 glbCurLevel->hasLOS(getX()+1, getY()+1, mob->getX(), mob->getY())) )
	return true;
    // If target is huge, gotta check the rest of the mob.
    if (mobdoublesize &&
	(glbCurLevel->hasLOS(getX(), getY(), mob->getX()+1, mob->getY()) ||    
	 glbCurLevel->hasLOS(getX(), getY(), mob->getX(), mob->getY()+1) ||    
	 glbCurLevel->hasLOS(getX(), getY(), mob->getX()+1, mob->getY()+1)) )
	return true;
    // No LOS, no sight...
    return false;
}

SENSE_NAMES
MOB::getSenseType(const MOB *mob) const
{
    // This is a bit expensive as we have to address
    // the senses from the most informative to the least.

    // This fails for no reason from refresh...
    //UT_ASSERT(mob != 0);
    
    // A quick sanity test on the mob.
    if (!mob)
	return SENSE_NONE;

    // Obviously, different levels are illegal.  Either falling
    // through a trap door, or perhaps -1 because it has not
    // yet been created.
    if (mob->getDLevel() != getDLevel())
	return SENSE_NONE;

    // Mobs start with -1 position, which should always be out of sight.
    if (mob->getX() == -1 || mob->getX() == 255)
	return SENSE_NONE;

    // Check for boring sensing.
    if (canSense(mob, true, true))
	return SENSE_SIGHT;

    // First, if this is blind, we can see iff we have telepathy and
    // the target has a mind...
    if (hasIntrinsic(INTRINSIC_BLIND))
    {
	if (mob->getSmarts() != 0 &&
	    hasIntrinsic(INTRINSIC_TELEPATHY))
	{
	    return SENSE_ESP;
	}

	// Being blind doesn't stop you from seeing yourself.
	if (mob == this)
	   return SENSE_ESP;
    }

    // If the target has position revealed, we get to sense
    // the target via ESP.
    if (mob->hasIntrinsic(INTRINSIC_POSITIONREVEALED))
    {
	return SENSE_ESP;
    }

    // If the target is our slave and we have telepathy we get to sense
    // the target via ESP.
    if (mob->isSlave(this))
    {
	if (hasIntrinsic(INTRINSIC_TELEPATHY))
	    return SENSE_ESP;
    }

    int			dist;

    dist = abs(getX() - mob->getX()) + abs(getY() - mob->getY());
    
    // See if we can hear our target.
    if (!hasIntrinsic(INTRINSIC_DEAF))
    {
	// You hear the monster.
	// You are not allowed to hear yourself!
	// Hearing is a tricky business, you don't always hear things.
	if (dist && dist <= mob->getNoiseLevel(this))
	    return SENSE_HEAR;
    }

    // See if warning kicks in.
    if (hasIntrinsic(INTRINSIC_WARNING))
    {
	if (dist < 5)
	    return SENSE_WARN;
    }

    return SENSE_NONE;
}

void
MOB::makeNoise(int noiselevel)
{
    // You don't always gain the full noiselevel.
    // The general theory is that:
    // 1) Your noiselevel should be at least the maximum
    //    individual noiselevel
    // 2) Additional noise sources have some effect.
    // 
    // We want a system where a + a = a + 1.
    // The chance of going up is thus 1 in 1 << |a - b|

    pietyMakeNoise(noiselevel);

    // Ensure myNoiseLevel is the maximum of the two.
    if (myNoiseLevel < noiselevel)
    {
	int		tmp;
	tmp = myNoiseLevel;
	myNoiseLevel = noiselevel;
	noiselevel = tmp;
    }

    if (noiselevel)
    {
	if (!rand_choice( 1 << (myNoiseLevel - noiselevel) ))
	    myNoiseLevel++;

	// Clamp the max noise level.    
	// No matter how unlikely, overflow errors are embarrassing.
	if (myNoiseLevel > 250)
	    myNoiseLevel = 250;
    }
}

void
MOB::makeEquipNoise(ITEMSLOT_NAMES slot)
{
    ITEM		*item;

    item = getEquippedItem(slot);
    if (item)
    {
	makeNoise(item->getNoiseLevel());
    }
}

int
MOB::getNoiseLevel(const MOB *observer) const
{
    if (!observer)
	return myNoiseLevel;

    if (!myNoiseLevel)
	return 0;
    
    // Find a hash.
    u32			hash;

    MOBREF		thisref, obref;

    thisref.setMob(this);
    obref.setMob(observer);

    hash = thisref.getId();
    hash = (hash << 16) | (hash >> 16);
    hash ^= obref.getId();

    hash ^= speed_gettime() * 2654435761U;	// gold
    
    // Now use hash to find a random number.  We do this by wang hashing
    // it and then modulating by the noise level + 1.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3);
    hash ^=  (hash >> 6);
    hash += ~(hash << 11);
    hash ^=  (hash >> 16);
    
    // Modulus.
    hash %= myNoiseLevel + 1;

    return hash;
}

int
MOB::getTile() const
{
    return glb_mobdefs[myDefinition].tile;
}

int
MOB::getTileLL() const
{
    return glb_mobdefs[myDefinition].tilell;
}

int
MOB::getTileUR() const
{
    return glb_mobdefs[myDefinition].tileur;
}

int
MOB::getTileLR() const
{
    return glb_mobdefs[myDefinition].tilelr;
}

void
MOB::incrementMaxHP(int amount)
{
    MOB		*base;

    // If we our polyed, our base form also gets the bonus/malus.
    base = myBaseType.getMob();
    if (base)
	base->incrementMaxHP(amount);
    
    if (amount < 0)
    {
	// do not allow max hp to go negative.
	if (myMaxHP + amount < 0)
	    myMaxHP = 1;
	else
	    myMaxHP += amount;

	// Reduce our HP if necessary.
	if (myHP > myMaxHP)
	    myHP = myMaxHP;
    }
    else
    {
	// Avoid embarrassing overflows.
	if (myMaxHP + amount > 30000)
	{
	    amount = 30000 - myMaxHP;
	}
	// Both our HP and maxHP get boosted.
	myMaxHP += amount;
	myHP += amount;
    }
}

void
MOB::incrementMaxMP(int amount)
{
    MOB		*base;

    // If we our polyed, our base form also gets the bonus/malus.
    base = myBaseType.getMob();
    if (base)
	base->incrementMaxMP(amount);
    
    if (amount < 0)
    {
	// do not allow max hp to go negative.
	if (myMaxMP - amount < 0)
	    myMaxMP = 1;
	else
	    myMaxMP -= amount;

	// Reduce our MP if necessary.
	if (myMP > myMaxMP)
	    myMP = myMaxMP;
    }
    else
    {
	// Avoid embarrassing overflows.
	if (myMaxMP + amount > 30000)
	{
	    amount = 30000 - myMaxMP;
	}
	// Both our MP and maxMP get boosted.
	myMaxMP += amount;
	myMP += amount;
    }
}

bool
MOB::move(int nx, int ny, bool interlevel)
{
    UT_ASSERT(nx >= 0 && nx < MAP_WIDTH);
    UT_ASSERT(ny >= 0 && ny < MAP_HEIGHT);
    if (interlevel || (myX != nx) || (myY != ny))
    {
	// Any successful move will clear off our in pit flag.
	clearIntrinsic(INTRINSIC_INPIT);

	// If the new square isn't a forest, clear the tree flag
	if (glbCurLevel && 
	    (getDLevel() == glbCurLevel->getDepth()))
	{
	    if (!glb_squaredefs[glbCurLevel->getTileSafe(nx, ny)].isforest)
	    {
		if (hasIntrinsic(INTRINSIC_INTREE))
		{
		    formatAndReport("%U <fall> to the ground.");
		    clearIntrinsic(INTRINSIC_INTREE);
		}
	    }
	}
	else	// Don't track this for other levels.
	    clearIntrinsic(INTRINSIC_INTREE);

	// Any successful move of the avatar sets the global avatar
	// fresh move flag.
	if (isAvatar())
	    setAvatarOnFreshSquare(true);
    }
    
    if (interlevel)
    {
	// And our submerged...
	clearIntrinsic(INTRINSIC_SUBMERGED);
    }

    // Update the map to reflect the mob's movement:
    if (glbCurLevel && 
	(getDLevel() == glbCurLevel->getDepth()))
    {
	glbCurLevel->moveMob(this, myX, myY, nx, ny);
    }

    myX = nx;
    myY = ny;

    // We now check to see if, as a result of our move,
    // we encountered any nasty surprises.
    // We only do this if we are not teleporting between levels (as
    // then we don't know if glbCurLevel will be the right place
    // to find our tile!)
    if (!interlevel)
    {
	// We always implicitly search the square we stand on.  This
	// will also cause us to do the trap detection, and thus
	// be able to fall into the newly found trap :>
	// This may be weird as less alert people are less likely
	// to trigger traps, but we can retcon it by noting less alert
	// implies less curious, thus less likely to say: "What does
	// this switch do?"
	// However, we balance it by forcing the trap to work if the
	// user found it the hard way :>
	// Note that searchSquare will call dropMobs with forcetrap
	// if a new trap is found.  Thus new traps will have a double
	// jeopordy, though the second case is likely irrelevant as if
	// you survive forcetrap, you are immune.
	// That being said, you _will_ get spam from the second drop.
	// Thus, if the search found something, we don't trigger dropMobs.
	if (searchSquare(nx, ny, false))
	    return true;

	return glbCurLevel->dropMobs(nx, ny, false, this);
    }

    // The mob lived!
    return true;
}

ATTITUDE_NAMES
MOB::getAttitude(const MOB *appraisee) const
{
    // Everyone likes themselves :>
    if (appraisee == this)
	return ATTITUDE_FRIENDLY;

    MOB		*master;
    // If the appraisee is possessed by ourselves, it is friendly.
    if (appraisee->hasIntrinsic(INTRINSIC_POSSESSED))
    {
	master = appraisee->getInflictorOfIntrinsic(INTRINSIC_POSSESSED);
	if (master == this)
	    return ATTITUDE_FRIENDLY;
    }

    master = getMaster();
    if (master)
    {
	// Check for FSM state.
	if (myAIFSM == AI_FSM_ATTACK)
	{
	    if (appraisee == getAITarget())
	    {
		// Check for a common master.  We don't want
		// infighting!
		if (hasCommonMaster(appraisee))
		{
		    // It would make sense to clear the ai target
		    // but that is non const.
		    return ATTITUDE_FRIENDLY;
		}
		else
		    return ATTITUDE_HOSTILE;
	    }
	}
	// Yesh mashter...
	return master->getAttitude(appraisee);
    }

    // If I am the avatar, I have the opinion of the other guy.
    // This way we are friendly to friendlies, and hate hatreds.
    if (isAvatar())
	return appraisee->getAttitude(this);

    // If they are our target to kill, clearly we don't like them.
    if (appraisee == getAITarget())
	return ATTITUDE_HOSTILE;

    // Now, base it off our ai...
    int		ai = glb_mobdefs[myDefinition].ai;

    // Special case greedy gold beetles.
    if (getDefinition() == MOB_GOLDBEETLE)
    {
	if (appraisee->isWearing(MATERIAL_GOLD))
	    return ATTITUDE_HOSTILE;
    }

    if (getDefinition() == MOB_VAMPIREBAT)
    {
	// Not a good source of food?  Ignore.
	if (appraisee->isBloodless())
	{
	    return ATTITUDE_NEUTRAL;
	}
    }
    
    if (glb_aidefs[ai].attackavatar && appraisee->isAvatar())
	return ATTITUDE_HOSTILE;
    
    if (!glb_aidefs[ai].attacktype)
    {
	// Determine if other is of our mobtype.  If so, friendly.
	// If not, go to speciest.
	// NOTE: UNUSUAL mob types do not ally as this is a catch
	// all for one offs!
	if (glb_mobdefs[appraisee->myDefinition].mobtype != MOBTYPE_UNUSUAL)
	{
	    if (glb_mobdefs[appraisee->myDefinition].mobtype ==
		glb_mobdefs[myDefinition].mobtype)
	    {
		return ATTITUDE_FRIENDLY;
	    }
	}
    }
    if (!glb_aidefs[ai].attackspecies)
    {
	// Determine if other is our species.  If so, friendly.
	// Otherwise, neutral.
	if (appraisee->myDefinition == myDefinition)
	    return ATTITUDE_FRIENDLY;
    }
    // It is an alien, return based on that...
    if (!glb_aidefs[ai].attackalien)
	return ATTITUDE_FRIENDLY;

    return ATTITUDE_NEUTRAL;
}

int
MOB::calculateFoesSurrounding() const
{
    // Check surrounding squares for enemies:
    int		numenemy = 0;
    int		dx, dy;

    FORALL_4DIR(dx, dy)
    {
	MOB		*mob;
	
	mob = glbCurLevel->getMob(getX()+dx, getY()+dy);

	if (mob && mob->getAttitude(this) == ATTITUDE_HOSTILE
		&& !mob->hasIntrinsic(INTRINSIC_ASLEEP)
		&& !mob->hasIntrinsic(INTRINSIC_PARALYSED))
	{
	    numenemy++;
	}
    }

    return numenemy;
}

bool
MOB::doHeartbeat()
{
    // Remove intrinsics...
    bool			 lived = true;
    bool			 unchanging;
    INTRINSIC_COUNTER		*counter, *next, *last;
#ifdef STRESS_TEST
    INTRINSIC_COUNTER		*ctest;
#endif
    SQUARE_NAMES		 tile;
    ITEM			*item;
    POISON_NAMES		 poison;
    MOBREF			 myself;

    // Determine if we are in a proper phase.
    if (!speed_isheartbeatphase())
	return true;

    // Determine what sort of square we are on
    tile = glbCurLevel->getTile(getX(), getY());

    // Manually test if we are about to lose possesed status
    // as it is too complicated to do later.
    if (hasIntrinsic(INTRINSIC_POSSESSED))
    {
	counter = getCounter(INTRINSIC_POSSESSED);
	if (counter && counter->myTurns <= 1)
	{
	    // About to expire, so trigger by hand
	    actionReleasePossession();
	}
    }

    // Decay all knowledge of ourselves & clear up any null pointers
    aiDecayKnowledgeBase();

    myself.setMob(this);

    // Reset all of our accumulated noise.
    myNoiseLevel = 0;
    // Get the insta-noise from equipment, etc.
    // Footwear, shields, and weapons only make noise on use.
    for (item = myInventory; item; item = item->getNext())
    {
	if (item->getX() == 0)
	{
	    switch ((ITEMSLOT_NAMES) item->getY())
	    {
		case ITEMSLOT_HEAD:
		case ITEMSLOT_AMULET:
		case ITEMSLOT_LRING:
		case ITEMSLOT_RRING:
		case ITEMSLOT_BODY:
		    makeNoise(item->getNoiseLevel());
		    break;
		case ITEMSLOT_LHAND:
		case ITEMSLOT_RHAND:
		case ITEMSLOT_FEET:
		case NUM_ITEMSLOTS:
		    break;
	    }
	}
    }
    // Check for noisy intrinsic.
    if (hasIntrinsic(INTRINSIC_NOISY))
	makeNoise(3);
    // Add our creature base noise.
    makeNoise(glb_mobdefs[getDefinition()].noise);

    // If we have a missing finger, we should drop a ring.
    if (hasIntrinsic(INTRINSIC_MISSINGFINGER))
	dropOneRing();

    // Check to see if the avatar will wake us.
    if (hasIntrinsic(INTRINSIC_ASLEEP))
    {
	int		noisethreshold = 10;
	if (hasIntrinsic(INTRINSIC_TIRED))
	{
	    noisethreshold = 20;
	    // A small chance of clearing the tired intrinsic early!
	    if (!rand_choice(20))
	    {
		clearIntrinsic(INTRINSIC_TIRED);
	    }
	}
	if (isAvatar())
	{
	    // If we are the avatar, we check all other mobs to
	    // see if any of them will wake us.
	    // Technically, all mobs should use this code path,
	    // but we do have to worry about efficiency somewhere.
	    MOB		*mob;
	    for (mob = glbCurLevel->getMobHead(); mob; mob = mob->getNext())
	    {
		// Ignore our own noise!
		if (mob == this)
		    continue;

		int		noise, dist;

		noise = getNoiseLevel(mob);
		dist = abs(getX() - mob->getX()) 
		     + abs(getY() - mob->getY());

		noise -= dist;
		if (noise > 0)
		{
		    if (rand_choice(noisethreshold) < noise)
		    {
			clearIntrinsic(INTRINSIC_ASLEEP);
			// Can stop the search.
			break;
		    }
		}
	    }
	}
	else if (getAvatar())
	{
	    int		noise, dist;

	    noise = getAvatar()->getNoiseLevel(this);
	    dist = abs(getAvatar()->getX() - getX()) 
		 + abs(getAvatar()->getY() - getY());

	    noise -= dist;
	    if (noise > 0)
	    {
		if (rand_choice(noisethreshold) < noise)
		    clearIntrinsic(INTRINSIC_ASLEEP);
	    }
	}
    }

    unchanging = hasIntrinsic(INTRINSIC_UNCHANGING);

    // If we have stoning resist, we auto-clear the stoning intrinsic.
    if (hasIntrinsic(INTRINSIC_RESISTSTONING))
	clearIntrinsic(INTRINSIC_STONING);
    
    // If we have sleep resist, we autoclear sleeping.
    if (hasIntrinsic(INTRINSIC_RESISTSLEEP))
	clearIntrinsic(INTRINSIC_ASLEEP);

    // We store what the final poison damage is so we apply it *after*
    // processing all the counters.  Any damage dealt has the potential
    // of killing but life saving, which clearDeathIntrinsics, which thus
    // destroys the counter linked list!
    // Note this only allows us to apply one final poison in a single
    // heartbeat, but our intention is that one only suffers from
    // the most powerful poison, so this isn't an error.
    ATTACK_NAMES	finalpoisondamage = ATTACK_NONE;
    MOBREF		finalpoisoninflictor;
    INTRINSIC_NAMES	finalpoisonintrinsic = INTRINSIC_NONE;
    bool		finalpoisonreport = false;
    bool		losttame = false;

    last = 0;
    for (counter = myCounters; counter; counter = next)
    {
	next = counter->myNext;

	// Special case amnesia and tame:  If the source is dead, cap at five
	// turns.
	if (glb_intrinsicdefs[counter->myIntrinsic].losewheninflictordies)
	{
	    if (!counter->myInflictor.getMob() ||
		(counter->myInflictor.getMob()->hasIntrinsic(INTRINSIC_DEAD)))
	    {
		// We are about to dec the counter, hence 6 rather
		// than 5.
		if (counter->myTurns > 6)
		    counter->myTurns = 6;
	    }
	}

	// Special case tame.  
	// Do not decrement if it is over 10k as we want this sort
	// of tameness to be permament.
	// But what if your pet resurrects you?  They are in a five
	// turn count down, so will then go wild shortly thereafter.
	// We thus rephrase this to mean that we lock tame to 10k so
	// long as the master is alive, so in that currently impossible
	// resurrection scenario the pet will immediately go full-tame.
	if (counter->myIntrinsic == INTRINSIC_TAME)
	{
	    if (counter->myInflictor.getMob() &&
		!counter->myInflictor.getMob()->hasIntrinsic(INTRINSIC_DEAD))
	    {
		counter->myTurns = 10000;
	    }
	}

	counter->myTurns--;
	if (counter->myTurns <= 0)
	{
	    // Intrinsic has expired (and no permament set since, as
	    // that will destroy the counter)
	    if (last)
		last->myNext = next;
	    else
		myCounters = next;

	    // Check to see if it is a poison, and if so, if there
	    // is a final damage message....
	    // NOTE: We ignore poison resistance here!  This has been
	    // defined, for now, as a feature, not a bug.
	    if (glb_intrinsicdefs[counter->myIntrinsic].ispoison)
	    {
		FOREACH_POISON(poison)
		{
		    if (counter->myIntrinsic == glb_poisondefs[poison].intrinsic)
		    {
			// Apply the relevant damage...
			if (glb_poisondefs[poison].finaldamage != ATTACK_NONE)
			{
			    finalpoisondamage = (ATTACK_NAMES) 
					glb_poisondefs[poison].finaldamage;
			    finalpoisoninflictor = counter->myInflictor;
			    finalpoisonintrinsic = counter->myIntrinsic;
			}
			break;
		    }
		}
	    }

	    // Check to see if it is petrification, in which case
	    // the mob gets stoned.
	    if (counter->myIntrinsic == INTRINSIC_STONING)
	    {
		// We cached the unchanging state prior to starting this
		// loop.  If unchanging expires the same turn as stoning
		// takes effect, we give the player the benefit of the doubt
		// as otherwise juggling Preserve with Stoning is too
		// unpredictable.
		if (unchanging)
		{
		    formatAndReport("%U <feel> momentarily stony.");
		}
		else if (actionPetrify())
		{
		    // Don't do losetxt as it was lost in a most
		    // unfortunate way.
		    myIntrinsic.clearIntrinsic(counter->myIntrinsic);
		    delete counter;
#ifdef STRESS_TEST
		    for (ctest = myCounters; ctest; ctest = ctest->myNext)
		    {
			UT_ASSERT(ctest != counter);
		    }
#endif
		    return false;
		}
	    }

	    // Check to see if it is summoned, in which case the
	    // mob disappears in a puff of smoke.
	    if (counter->myIntrinsic == INTRINSIC_SUMMONED)
	    {
		// Exception are trees that turn into forest squares
		if (getDefinition() == MOB_LIVINGTREE)
		    formatAndReport("%U <return> to an inanimate state.");
		else
		    formatAndReport("%U <disappear> in a puff of smoke.");

		// Release any possession
		if (hasIntrinsic(INTRINSIC_POSSESSED))
		    actionReleasePossession(true);

		if (getDefinition() == MOB_LIVINGTREE)
		{
		    glbCurLevel->growForest(getX(), getY(), this);
		    if (hasIntrinsic(INTRINSIC_AFLAME))
		    {
			// Burning trees turn into burning forests
			if (glbCurLevel->getTile(getX(), getY()) == SQUARE_FOREST)
			    glbCurLevel->setTile(getX(), getY(), SQUARE_FORESTFIRE);
		    }
		}
		else
		    glbCurLevel->setSmoke(getX(), getY(), SMOKE_SMOKE, this);

		glbCurLevel->unregisterMob(this);

		// Drop the inventory.
		ITEM		*cur;

		// Drop our inventory on this square...
		while ((cur = myInventory))
		{
		    // We manually drop the item for efficiency.
		    myInventory = cur->getNext();
		    cur->setNext(0);
		    glbCurLevel->acquireItem(cur, getX(), getY(), 0);
		}

		// Destroy the counter that was unlinked.
		delete counter;
#ifdef STRESS_TEST
		for (ctest = myCounters; ctest; ctest = ctest->myNext)
		{
		    UT_ASSERT(ctest != counter);
		}
#endif

		delete this;
		return false;
	    }

	    // Mark if the creature just went wild
	    if (counter->myIntrinsic == INTRINSIC_TAME)
	    {
		losttame = true;
	    }

	    if (myIntrinsic.clearIntrinsic(counter->myIntrinsic))
	    {
		if (glb_intrinsicdefs[counter->myIntrinsic].affectappearance)
		    rebuildAppearance();
		// Was previously set...
		if (isAvatar() ||
		    (getAvatar() && 
		     glb_intrinsicdefs[counter->myIntrinsic].visiblechange &&
		     getAvatar()->canSense(this)))
		{
		    if (counter->myIntrinsic == finalpoisonintrinsic)
		    {
			// Don't report right away as this gives 
			// you are cured before the final damage
			finalpoisonreport = true;
		    }
		    else
			formatAndReport(
			    glb_intrinsicdefs[counter->myIntrinsic].losetxt);
		}
	    }
	    
	    delete counter;
#ifdef STRESS_TEST
	    for (ctest = myCounters; ctest; ctest = ctest->myNext)
	    {
		UT_ASSERT(ctest != counter);
	    }
#endif
	}
	else
	{
	    // Intrinsic is still active...
	    last = counter;
	}
    }
    
    // Apply the final poison damage.
    if (finalpoisondamage != ATTACK_NONE)
    {
	formatAndReport("%U <be> wracked by the last of the poison.");
				
	lived = receiveDamage(finalpoisondamage, 
				finalpoisoninflictor.getMob(), 
				0, 0,
				ATTACKSTYLE_POISON);

	if (!lived)
	{
	    // We died thanks to the final poison damage
	    return false;
	}

	// Only if they live do we get cured.
	if (finalpoisonreport)
	    formatAndReport(glb_intrinsicdefs[finalpoisonintrinsic].losetxt);
    }

    // If we went wild this turn, we also grow momentarily confused.
    if (losttame)
	setTimedIntrinsic(0, INTRINSIC_CONFUSED, 4);

    //
    // Consume food.
    //
    starve(1);

    // Bonus food consumption for specific intrinsics.
    // This only takes effect when the intrinsic comes from wearing an
    // item.
    if (hasIntrinsic(INTRINSIC_INVISIBLE))
    {
	if (getSourceOfIntrinsic(INTRINSIC_INVISIBLE))
	    starve(1);
    }
    if (hasIntrinsic(INTRINSIC_REGENERATION))
    {
	if (getSourceOfIntrinsic(INTRINSIC_REGENERATION))
	    starve(1);
    }
    
    HUNGER_NAMES		hunger;

    hunger = getHungerLevel();

    // If you can endure hunger, you suffer no malus for starving.
    if (hunger < HUNGER_NORMAL)
    {
	if (hasSkill(SKILL_ENDUREHUNGER))
	    hunger = HUNGER_NORMAL;
    }
    
    // Regain health...
    bool allowregen = !hasIntrinsic(INTRINSIC_NOREGEN);
    if (myHP < getMaxHP() && hunger > HUNGER_STARVING)
    {
	if (hasIntrinsic(INTRINSIC_REGENERATION))
	{
	    // If noregen is present, this only has effect if the
	    // regeneration is temporary.  The idea is we want the
	    // regeneration spell to work, but we don't want rings of
	    // regeneration to negate noregen.
	    if (allowregen ||
		(intrinsicFromWhat(INTRINSIC_REGENERATION) & INTRINSIC_FROM_TEMPORARY))
	    {
		// We want to regen every turn if full even if
		// ring of regen is on.  Otherwise a ring becomes less
		// useful when full.
		if (speed_matchheartbeat(2) || (hunger > HUNGER_NORMAL))
		{
		    myHP++;
		    starve(1);
		}
	    }
	}
	else if (allowregen)
	{
	    // Every 8th turn, at random...
	    if (hunger > HUNGER_HUNGRY)
	    {
		if (speed_matchheartbeat(8))
		{
		    starve(1);
		    myHP++;
		}
		else if (hunger > HUNGER_NORMAL)
		{
		    myHP++;
		    starve(1);
		}
	    }
	}

	if (myHP < getMaxHP() 
		&& allowregen 
		&& hasIntrinsic(INTRINSIC_DRESSED_FIGHTER))
	{
	    // Fighters get half regen!  Yowzers!
	    if (speed_matchheartbeat(2))
	    {
		myHP++;
		starve(1);
	    }
	}

	// If we are tame and our owner is dressed like a cleric
	// we get regen...
	// Cleric induced regen is active even for constructs.
	if (myHP < getMaxHP())
	{
	    MOB		*master = getMaster();

	    if (master)
	    {
		if (master->hasIntrinsic(INTRINSIC_DRESSED_CLERIC))
		{
		    myHP++;
		    starve(1);
		}
	    }
	}
    }

    // If we have mind drain intrinsic, we loses magic points.
    if (myMP && hasIntrinsic(INTRINSIC_MAGICDRAIN))
    {
	myMP--;
    }

    // Regain magic...
    if (myMP < getMaxMP() && hunger > HUNGER_STARVING)
    {
	// Regain every turn.
	if (hunger > HUNGER_HUNGRY)
	{
	    if (speed_matchheartbeat(2))
	    {
		myMP++;
		starve(1);
	    }
	}
	else
	{
	    // Hungry players gain at half rate.
	    if (speed_matchheartbeat(4))
	    {
		myMP++;
		starve(1);
	    }
	}
	
	// Wizards get bonus magic!  Yowzers!
	if (myMP < getMaxMP() && hasIntrinsic(INTRINSIC_DRESSED_WIZARD))
	{
	    myMP++;
	    starve(1);
	}
    }

    // Check to see if we teleport...
    if (hasIntrinsic(INTRINSIC_TELEPORT))
    {
	if (!rand_choice(100))
	{
	    // This may kill the player.
	    actionTeleport();

	    // Check if we died...
	    if (!myself.getMob() ||
		myself.getMob() != this ||    
		myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
	    {
		return false;
	    }
	}
    }

    // If we have searching, auto search.
    if (hasIntrinsic(INTRINSIC_SEARCH))
    {
	actionSearch();

	// Searching may reveal a pit that kills us.
	// Check if we died...
	if (!myself.getMob() ||
	    myself.getMob() != this ||    
	    myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
	{
	    return false;
	}
    }

    // Handle worn items being affected by liquids
    if (hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	submergeItemEffects(tile);
    }

    // Handle breath related attacks.  These are strangling, drowning,
    // and smoke inhalation.  You can only suffer one attack, as
    // strangling prevents drowning, and drowners can't inhale smoke.
    // Vaccuum issues are handled by lava effects.
    if (!hasIntrinsic(INTRINSIC_NOBREATH))
    {
	if (hasIntrinsic(INTRINSIC_STRANGLE))
	{
	    // Lose 1d10 hits a turn..
	    ITEM		*strangler;
	    MOB			*stranglemob = this;

	    counter = getCounter(INTRINSIC_STRANGLE);
	    if (counter)
		stranglemob = counter->myInflictor.getMob();

	    strangler = getSourceOfIntrinsic(INTRINSIC_STRANGLE);
	    if (strangler)
		stranglemob = 0;

	    lived = receiveDamage(ATTACK_STRANGLED, stranglemob, strangler, 0,
				  ATTACKSTYLE_MISC);
	}
	else if (hasIntrinsic(INTRINSIC_SUBMERGED) ||
		// If we are stuck in a wall?
		// ignore doors as they are thin
		 (!canMove(getX(), getY(), false, true)))
	{
	    // Lose damage to drowning...
	    // We want to use a different message if suffocating
	    // rather than drowning, so check what sort of square
	    // we are submerged in..
	    lived = receiveDamage((ATTACK_NAMES) glb_squaredefs[tile].submergeattack,
				    glbCurLevel->getGuiltyMob(getX(), getY()), 
				    0, 0,
				  ATTACKSTYLE_MISC);
	}
	else
	{
	    SMOKE_NAMES		 smoke;
	    MOB			*smoker;
	    
	    smoke = glbCurLevel->getSmoke(getX(), getY(), &smoker);
	    if (smoke != SMOKE_NONE)
	    {
		// Inflict the smoke damage.
		lived = receiveDamage(
			    (ATTACK_NAMES)glb_smokedefs[smoke].attack,
			    smoker, 0, 0,
			    ATTACKSTYLE_MISC);
	    }
	}
    }

    // Deal with bleeding..
    if (lived && hasIntrinsic(INTRINSIC_BLEED))
    {
	// This likely is impossible, but be careful.
	if (isBloodless())
	    clearIntrinsic(INTRINSIC_BLEED);
	else
	{
	    counter = getCounter(INTRINSIC_BLEED);
	    lived = receiveDamage(ATTACK_BLEED, 
		    (counter ? counter->myInflictor.getMob() : 0), 0, 0,
		    ATTACKSTYLE_MISC);
	}
    }

    // Now, deal with poison...
    if (lived)
    {
	bool		 resistpoison = hasIntrinsic(INTRINSIC_RESISTPOISON);
	MOB		*inflictor;

	FOREACH_POISON(poison)
	{
	    if (!lived)
		break;
		    
	    if (hasIntrinsic((INTRINSIC_NAMES) glb_poisondefs[poison].intrinsic))
	    {
		// Apply the poison damage if the chance passes...
		if (speed_matchheartbeat(glb_poisondefs[poison].modulus))
		{
		    counter = getCounter((INTRINSIC_NAMES) glb_poisondefs[poison].intrinsic);
		    inflictor = (counter ? counter->myInflictor.getMob() : 0);
		    
		    // Report poison resist or apply poison damage.
		    if (resistpoison)
		    {
			if (inflictor)
			    inflictor->aiNoteThatTargetHasIntrinsic(this,
					    INTRINSIC_RESISTPOISON);
		    }
		    else
		    {
			lived = receiveDamage(
			    (ATTACK_NAMES)glb_poisondefs[poison].damage, 
			    inflictor, 0, 0,
			    ATTACKSTYLE_POISON);
		    }
		}
	    }
	}
    }

    // Deal with being blown by the wind.
    if (lived && glb_mobdefs[getDefinition()].blownbywind)
    {
	int		wdx, wdy;

	glbCurLevel->getWindDirection(wdx, wdy);

	if (wdx || wdy)
	{
	    if (canMoveDelta(wdx, wdy, true))
	    {
		// Move!
		actionWalk(wdx, wdy);

		// It is unlikely because this is a flying creature,
		// but moving can kill us.
		// Note that if the creature changes levels we may get
		// some weird behaviour here, but I think it is safe
		// At least that's what the gin & gingerale I'm drinking
		// is telling me.  :>
		if (!myself.getMob() ||
		    myself.getMob() != this ||    
		    myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
		{
		    lived = false;
		}
	    }
	}
    }

    // Disperse smoke.
    if (lived && getDefinition() == MOB_AIRELEMENTAL)
    {
	glbCurLevel->setSmoke(getX(), getY(), SMOKE_NONE, 0);
    }

    // Burn squares due to our own heat.
    if (lived && getDefinition() == MOB_FIREELEMENTAL)
    {
	glbCurLevel->burnSquare(getX(), getY(), this);

	// If we are standing on water we will reveal a pit that we
	// fall into.  The resulting pit may kill us.
	if (!myself.getMob() ||
	    myself.getMob() != this ||    
	    myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
	{
	    lived = false;
	}
    }

    // Freeze squares due to our own chill.
    if (lived && getDefinition() == MOB_LIVINGFROST)
    {
	glbCurLevel->freezeSquare(getX(), getY(), this);

	// I don't think freezing a square can kill you, but
	// I don't want to find out the hard way.
	if (!myself.getMob() ||
	    myself.getMob() != this ||    
	    myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
	{
	    lived = false;
	}
    }

    // Deal with standing on/in bad stuff.
    if (lived)
    {
	switch (tile)
	{
	    case SQUARE_LAVA:
		if (!hasIntrinsic(INTRINSIC_RESISTFIRE) ||
		     hasIntrinsic(INTRINSIC_VULNFIRE))
		{
		    lived = receiveDamage(ATTACK_LAVABURN,
					    0, 0, 0,
					    ATTACKSTYLE_MISC);
		}
		if (lived && !hasIntrinsic(INTRINSIC_AFLAME))
		{
		    int		delay;

		    delay = rand_dice(1, 4, 2);
		    // Set aflame...
		    setTimedIntrinsic(this, INTRINSIC_AFLAME, delay);
		}
		break;

	    case SQUARE_FORESTFIRE:
		if (!hasIntrinsic(INTRINSIC_RESISTFIRE) ||
		     hasIntrinsic(INTRINSIC_VULNFIRE))
		{
		    lived = receiveDamage(ATTACK_FORESTFIRE,
					    0, 0, 0,
					    ATTACKSTYLE_MISC);
		}
		break;


	    case SQUARE_ACID:
		// We only burn if we are submerged or are not
		// flying
		if (hasIntrinsic(INTRINSIC_SUBMERGED) ||
		    !(getMoveType() & MOVE_FLY))	
		{
		    lived = receiveDamage(ATTACK_ACIDPOOLBURN,
					    0, 0, 0,
					    ATTACKSTYLE_MISC);
		}
		break;

	    case SQUARE_WATER:
		if (hasIntrinsic(INTRINSIC_SUBMERGED) &&
		    hasIntrinsic(INTRINSIC_AFLAME))
		{
		    formatAndReport("The water puts out %R flames.");
		    clearIntrinsic(INTRINSIC_AFLAME);
		}
		break;
	    case SQUARE_STARS1:
	    case SQUARE_STARS2:
	    case SQUARE_STARS3:
	    case SQUARE_STARS4:
	    case SQUARE_STARS5:
		if (!hasIntrinsic(INTRINSIC_NOBREATH) &&
		    !hasIntrinsic(INTRINSIC_STRANGLE))
		{
		    // We have made sure you didn't already pay
		    // for this cost elsewhere.
		    lived = receiveDamage(ATTACK_ASPHYXIATE,
				    0, 0, 0,
				    ATTACKSTYLE_MISC);
		}
		break;
	}
    }

    // Now, deal with being set afire.
    if (lived && (!hasIntrinsic(INTRINSIC_RESISTFIRE) || hasIntrinsic(INTRINSIC_VULNFIRE)) )
    {
	if (hasIntrinsic(INTRINSIC_AFLAME))
	{
	    counter = getCounter(INTRINSIC_AFLAME);
	    lived = receiveDamage(ATTACK_AFLAME, 
				(counter ? counter->myInflictor.getMob() : 0), 0, 0,
				ATTACKSTYLE_MISC);
	}
    }

    // Apply silver damage for anything we are wielding
    if (lived && hasIntrinsic(INTRINSIC_VULNSILVER))
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    if (item->getX() == 0)
	    {
		if (item->getMaterial() == MATERIAL_SILVER)
		{
		    lived = receiveDamage(ATTACK_SILVERITEM, 0,
					    item, 0, 
					    ATTACKSTYLE_MISC);

		    if (!lived)
			break;
		    // Note that even if we live, we may delete
		    // items, ie, items of life saving,
		    // and thus may end up with a corrupt inventory 
		    // list.
		    // Our hacky solution is to detect life saving
		    // by looking for us having max hp..
		    if (getHP() == getMaxHP())
			break;
		}
	    }
	}
    }

    // Apply gold damage for anything we are wielding
    if (lived && hasIntrinsic(INTRINSIC_GOLDALLERGY))
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    if (item->getX() == 0)
	    {
		if (item->getMaterial() == MATERIAL_GOLD)
		{
		    lived = receiveDamage(ATTACK_GOLDITEM, 0,
					    item, 0, 
					    ATTACKSTYLE_MISC);

		    if (!lived)
			break;
		    // Note that even if we live, we may delete
		    // items, ie, items of life saving,
		    // and thus may end up with a corrupt inventory 
		    // list.
		    // Our hacky solution is to detect life saving
		    // by looking for us having max hp..
		    if (getHP() == getMaxHP())
			break;
		}
	    }
	}
    }

    // Run heartbeat on all our items...
    if (lived)
    {
	ITEM		*item, *next;

	for (item = myInventory; item; item = next)
	{
	    next = item->getNext();

	    if (!item->doHeartbeat(glbCurLevel, this))
	    {
		ITEM		*drop;
		drop = dropItem(item->getX(), item->getY());
		UT_ASSERT(drop == item);

		delete drop;
	    }
	}
    }

    // This must be last, as it may change our MOB on us!
    if (lived && hasIntrinsic(INTRINSIC_POLYMORPH) && !unchanging)
    {
	if (!rand_choice(300))
	{
	    actionPolymorph();

	    // Note we do not want anyone calling doAI.
	    // We hand wave this by saying it is the shock
	    // of polymorphing. 
	    lived = false;
	}
    }
    
    if (lived && isPolymorphed() && !unchanging)
    {
	if (!rand_choice(1000))
	{
	    actionUnPolymorph();

	    // Note we do not want anyone calling doAI.
	    // We hand wave this by saying it is the shock
	    // of polymorphing. 
	    lived = false;
	}
    }

    // Check for any per-turn piety conditions.
    // If piety ever goes beyond avatars, should check for all
    // MOBs.  However, some of these tests are expensive, so
    // we keep it limitted for now.
    if (lived && isAvatar())
    {
	if (hasIntrinsic(INTRINSIC_DRESSED_WIZARD))
	    pietyDress(INTRINSIC_DRESSED_WIZARD);
	if (hasIntrinsic(INTRINSIC_DRESSED_RANGER))
	    pietyDress(INTRINSIC_DRESSED_RANGER);
	if (hasIntrinsic(INTRINSIC_DRESSED_FIGHTER))
	    pietyDress(INTRINSIC_DRESSED_FIGHTER);
	if (hasIntrinsic(INTRINSIC_DRESSED_CLERIC))
	    pietyDress(INTRINSIC_DRESSED_CLERIC);
	if (hasIntrinsic(INTRINSIC_DRESSED_BARBARIAN))
	    pietyDress(INTRINSIC_DRESSED_BARBARIAN);
	if (hasIntrinsic(INTRINSIC_DRESSED_NECROMANCER))
	    pietyDress(INTRINSIC_DRESSED_NECROMANCER);
	
	// Check surrounding squares for enemies:
	int		numenemy = 0;

	numenemy = calculateFoesSurrounding();

	pietySurrounded(numenemy);
    }

    return lived;
}

void
MOB::findRandomValidMove(int &dx, int &dy, MOB *owner)
{
    int		possible_moves[8];
    const static int	sdx[8] = { -1, 1,  0, 0, -1, 1, -1,  1 };
    const static int	sdy[8] = {  0, 0, -1, 1,  1, 1, -1, -1 };
    int		i, j, max;
    bool	allowdoor = false;
    bool	onlysafe = true;

    allowdoor = aiWillOpenDoors();
    if (isAvatar())
    {
	allowdoor = true;
	onlysafe = false;
    }

    max = 4;
    if (myDefinition == MOB_GRIDBUG)
	max = 8;
    
    j = 0;
    for (i = 0; i < max; i++)
    {
	if (canMoveDelta(sdx[i], sdy[i], true, allowdoor/*, onlysafe*/))
	{
	    // Determine if there is a mob there and if it is
	    // our no swap mob.  Displacing our owner is bad form.
	    if (!owner || 
		(glbCurLevel->getMob(getX()+sdx[i], getY()+sdy[i]) != owner))
	    {
		possible_moves[j++] = i;
	    }
	}
    }

    if (!j)
    {
	dx = dy = 0;
	return;
    }
    i = possible_moves[rand_choice(j)];
    dx = sdx[i];
    dy = sdy[i];
}

void
MOB::applyConfusion(int &dx, int &dy)
{
    int		dz = 0;

    applyConfusion(dx, dy, dz);
}

void
MOB::applyConfusionNoSelf(int &dx, int &dy)
{
    int		dz = 0;
    bool	notatself = false;

    if (dx || dy)
	notatself = true;

    applyConfusion(dx, dy, dz);

    if (!dx && !dy && notatself)
    {
	// Whoops, redirected at self.  Must cancel.
	rand_eightwaydirection(dx, dy);
    }
}

void
MOB::applyConfusion(int &dx, int &dy, int &dz)
{
    // Determine if the critter is confused.
    if (!hasIntrinsic(INTRINSIC_CONFUSED))
	return;

    // Random chance we get to do what we wanted to do.
    if (rand_chance(80))
	return;

    // If dz is set, randomize it.
    if (dz)
    {
	dz = rand_sign();
    }

    // If dx & dy are within the given range, pick a random direction.
    // Note this can allow diagonal movement, for example.
    if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1)
    {
	dx = rand_range(-1, 1);
	dy = rand_range(-1, 1);
    }
    else
    {
	// We are doing an arbitrary ranged command (like teleport)
	// We thus fall somewhere random.
	dx = rand_range(-dx*2, dx*2);
	dy = rand_range(-dy*2, dy*2);
    }
}

bool
MOB::validMovePhase(PHASE_NAMES phase)
{
    bool		skipthismove = false;

    // Check our phase to see if we are allowed to move.
    switch (speed_getphase())
    {
	case PHASE_FAST:
	    // If we don't have fast, no go.
	    if (!hasIntrinsic(INTRINSIC_FAST))
		skipthismove = true;
	    break;

	case PHASE_QUICK:
	    // If we don't have quick, no go.
	    if (!hasIntrinsic(INTRINSIC_QUICK))
		skipthismove = true;
	    break;
	    
	case PHASE_NORMAL:
	    // Always allowed this phase.
	    break;
	    
	case PHASE_SLOW:
	    // So long as we DON'T have slow, we can move.
	    if (hasIntrinsic(INTRINSIC_SLOW))
		skipthismove = true;
	    break;
	    
	case NUM_PHASES:
	    UT_ASSERT(!"Invalid phase!");
	    break;
    }

    return !skipthismove;
}

bool
MOB::doMovePrequel()
{
    if (!validMovePhase(speed_getphase()))
	return false;

    // Check for paralysis:
    if (hasIntrinsic(INTRINSIC_PARALYSED))
    {
	if (!hasIntrinsic(INTRINSIC_FREEDOM))
	{
	    // Sit here and do nothing...
	    // We don't want to spam the avatar as it is annoying.
	    // IN any case, it is in the status bar!
	    if (!isAvatar())
	    {
		formatAndReport("%U <be> paralysed.");
	    }
	    return false;
	}
	else
	{
	    // People with freedom have paralysis instantly cured.
	    clearIntrinsic(INTRINSIC_PARALYSED);
	}
    }

    // Check to see if we are sleeping.
    if (hasIntrinsic(INTRINSIC_ASLEEP))
    {
	if (!hasIntrinsic(INTRINSIC_RESISTSLEEP))
	{
	    // TODO: Check for any wakeup notifiers!

	    // Snore.  Do not snore if one doesn't breath.
	    if (!hasIntrinsic(INTRINSIC_NOBREATH) && !rand_choice(10))
	    {
		formatAndReport("%U <snore>.");
	    }
	    return false;
	}
	else
	{
	    // Sleepers with resist sleep instantly awake.
	    clearIntrinsic(INTRINSIC_ASLEEP);
	}
    }

    // Check if starving and auto forage 2% of time
    if (isAvatar() && getHungerLevel() <= HUNGER_STARVING &&
	!hasSkill(SKILL_ENDUREHUNGER) &&
	rand_chance(2))
    {
	const char *msg[] = 
	{
	    "%U <scrape> lichen from the wall and <eat> it.",
	    "%U <find> a soft-looking rock to eat.",
	    "%U <find> and <gobble> up an unidentified piece of offal.",
	    "%U <stop> and <listen> to the growling of %r stomach.",
	    "%U <scoop> up a fat grub and <crunch> on it happily.",
	    0
	};

	formatAndReport(rand_string(msg));

	// We lose this turn
	return false;
    }

    return true;
}

bool
MOB::meltRocksCBStatic(int x, int y, bool final, void *data)
{
    MOBREF		*mobref = (MOBREF *) data;

    if (final) return false;

    SQUARE_NAMES	old = glbCurLevel->getTile(x, y);
    
    if (!glb_squaredefs[old].invulnerable)
    {
	// If the square is a ladder, we don't want to melt rocks.
	glbCurLevel->setTile(x, y, SQUARE_LAVA);
	glbCurLevel->setFlag(x, y, SQUAREFLAG_LIT);
	glbCurLevel->dropItems(x, y, mobref->getMob());
    }
    return true;
}


bool
MOB::areaAttackCBStatic(int x, int y, bool final, void *data)
{
    MOB			*mob;
    MOBREF		*srcmobref;
    bool		 hit = false;

    srcmobref = (MOBREF *) data;

    if (final) return false;
    
    ATTACK_NAMES	 attack = ourEffectAttack;

    mob = glbCurLevel->getMob(x, y);
    if (mob)
    {
	mob->receiveAttack(attack, srcmobref->getMob(), 
			    0, 0, ATTACKSTYLE_SPELL, &hit);
    }

    // If it was cold or fire, do the right thing.
    if (glb_attackdefs[attack].element == ELEMENT_FIRE)
    {
	glbCurLevel->burnSquare(x, y, srcmobref->getMob());
    }

    if (glb_attackdefs[attack].element == ELEMENT_COLD)
    {
	glbCurLevel->freezeSquare(x, y, srcmobref->getMob());
    }

    // Might be tempted to do just shock, but what of less
    // powerful shocks, ala spark?  Do I care?
    // Answer is to report the amount of damage done so we
    // can get a fair estimate.
    if (glb_attackdefs[attack].element == ELEMENT_SHOCK)
    {
	glbCurLevel->electrifySquare(x, y, 
			rand_dice(glb_attackdefs[attack].damage), 
			srcmobref->getMob());
    }
    

    if (hit && glb_attackdefs[attack].stopathit)
    {
	return false;
    }

    // Continue the breath attack...
    return true;
}

bool
MOB::searchSquare(int x, int y, bool isaction)
{
    SQUARE_NAMES	tile;
    BUF			buf;
    int			findchance;

    tile = glbCurLevel->getTile(x, y);

    findchance = glb_squaredefs[tile].findchance;

    // Half chance to find in the dark.
    // Note that searching by walking on the square doesn't get
    // the penalty - people walking in the dark are as likely to set
    // off a trap door as those walking in the light.  (If anything,
    // we may want to reverse this!)
    if (isaction && !glbCurLevel->isLit(x, y))
    {
	// Decrease find chance.
	findchance = (findchance + 1) / 2;
    }

    // First, determine if this was a secret square.  If so,
    // the secret is up!
    if (glb_squaredefs[tile].hiddensquare != SQUARE_EMPTY &&
	(rand_chance(findchance) ||
	 hasIntrinsic(INTRINSIC_SKILL_SEARCH)))
    {
	tile = (SQUARE_NAMES) glb_squaredefs[tile].hiddensquare;

	// Grant piety.
	pietyFindSecret();
	
	buf = formatToString("%U <find> %B1.", this, 0, 0, 0,
			glb_squaredefs[tile].description, 0);
	reportMessage(buf);
	glbCurLevel->setTile(x, y, tile);
	glbCurLevel->dropItems(x, y, this);

	// Warning: This may kill this!
	glbCurLevel->dropMobs(x, y, true, this, true);

	return true;		// Found something.
    }
    return false;		// Nothing to find.
}

bool
MOB::canCastSpell(SPELL_NAMES spell)
{
    if (!hasIntrinsic((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic))
	return false;

    if (hasIntrinsic(INTRINSIC_AMNESIA))
	return false;
    
    if (myMP < glb_spelldefs[spell].mpcost)
	return false;
    if (myHP <= glb_spelldefs[spell].hpcost)
	return false;
    if (myExp < glb_spelldefs[spell].xpcost)
	return false;

    return true;
}

static void
testability(int &ability, int &total, int threshold, int value)
{
    if (threshold)
    {
	// Actually test against this requirement.
	total++;
	if (value >= threshold)
	    ability += 256;
	else
	{
	    // Always round down so exactly equal only occurs when
	    // everything is fulfilled.
	    ability += (value*256) / threshold;
	}
    }
}

int
MOB::spellCastability(SPELL_NAMES spell)
{
    if (!hasIntrinsic((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic))
	return 0;

    if (hasIntrinsic(INTRINSIC_AMNESIA))
	return 0;

    int		total = 0;
    int		ability = 0;

    testability(ability, total, glb_spelldefs[spell].mpcost, myMP);
    testability(ability, total, glb_spelldefs[spell].hpcost, myHP);
    testability(ability, total, glb_spelldefs[spell].xpcost, myExp);

    if (total == 0)
    {
	// A spell with no requirements is always castable!
	return 256;
    }

    return ability / total;
}

HUNGER_NAMES
MOB::getHungerLevel() const
{
    HUNGER_NAMES		hunger;

    FOREACH_HUNGER(hunger)
    {
	if (myFoodLevel < glb_hungerdefs[hunger].foodlevel)
	    break;
    }
    hunger = (HUNGER_NAMES) (hunger-1);

    // If we can't eat anything, we can't starve. 
    if (!glb_mobdefs[myDefinition].edible ||
	!*glb_mobdefs[myDefinition].edible)
	return HUNGER_NORMAL;
    
    return hunger;
}

void
MOB::starve(int foodvalue)
{
    UT_ASSERT(foodvalue >= 0);

    // If we can't eat anything, we can't starve. 
    if (!glb_mobdefs[myDefinition].edible ||
	!*glb_mobdefs[myDefinition].edible)
	return;
    
    if (foodvalue < myFoodLevel)
	myFoodLevel -= foodvalue;
    else
	myFoodLevel = 0;
}

void
MOB::feed(int foodvalue)
{
    UT_ASSERT(foodvalue >= 0);
    // Maximum food value of 10,000.  As you can't eat
    // past full (2500) this should be pretty safe.
    if (foodvalue + myFoodLevel > 10000)
    {
	myFoodLevel = 10000;
    }
    else
    {
	myFoodLevel += foodvalue;
    }
}

void
MOB::systemshock()
{
    // Apply system shock: The base types max hit points are damaged
    // as a result of the pain.
    int		newmax;

    newmax = getMaxHP();
    // Lose 10%.
    newmax -= (newmax+9) / 10;
    if (newmax < 1)
	newmax = 1;
    
    // C++'s private definitions are meaningless if I can do crap
    // like this :>
    myMaxHP = newmax;
    if (getHP() > getMaxHP())
	setMaximalHP();

    // Let people be warned of the effect!
    formatAndReport("%U <feel> a little dead inside.");
}

int
MOB::getWeaponSkillLevel(const ITEM *weapon, ATTACKSTYLE_NAMES attackstyle) const
{
    // Non-avatar are exempt from this.
    // They get maximum skill.
    if (!isAvatar())
	return 2;

    // Determine if we have the attack skill.
    SKILL_NAMES		 attackskill;
    const ATTACK_DEF	*attack;
    int			 skilllevel = 0;
    bool		 misused = false;

    if (weapon)
    {
	attack = 0;
	if (attackstyle == ATTACKSTYLE_MELEE)
	{
	    attack = weapon->getAttack();
	}
	else if (attackstyle == ATTACKSTYLE_THROWN)
	{
	    attack = weapon->getThrownAttack();
	}
	if (attack == &glb_attackdefs[ATTACK_MISUSED] ||
	    attack == &glb_attackdefs[ATTACK_MISUSED_BUTWEAPON] ||
	    attack == &glb_attackdefs[ATTACK_MISTHROWN])
	    misused = true;

	attackskill = weapon->getAttackSkill();
	if (misused)
	    attackskill = SKILL_WEAPON_IMPROVISE;
	if (hasSkill(attackskill))
	    skilllevel++;

	if (!misused && hasSkill(weapon->getSpecialSkill()))
	    skilllevel++;
    }

    // Switch on the attack style.
    switch (attackstyle)
    {
	case ATTACKSTYLE_MELEE:
	    if (weapon)
	    {
		switch (weapon->getSize())
		{
		    case SIZE_TINY:
		    case SIZE_SMALL:
			if (hasSkill(SKILL_WEAPON_SMALL))
			    skilllevel++;
			break;
		    case SIZE_MEDIUM:
			if (hasSkill(SKILL_WEAPON_MEDIUM))
			    skilllevel++;
			break;
		    case SIZE_LARGE:
			if (hasSkill(SKILL_WEAPON_LARGE))
			    skilllevel++;
			break;
		    default:
			// No adjustment, as no skills for these weapons.
			break;
		}
	    }
	    break;

	case ATTACKSTYLE_THROWN:
	    if (hasSkill(SKILL_WEAPON_RANGED))
		skilllevel++;
	    break;

	default:
	    // It is assumed we are skilled with magic attacks, etc.
	    skilllevel++;
	    break;
    }

    return skilllevel;
}

int
MOB::getArmourSkillLevel(const ITEM *armour) const
{
    // Determine what skill we have.
    // Creatures always get maximum skill
    if (!isAvatar())
	return 2;

    int			skilllevel = 0;

    if (armour)
    {
	// Switch base on type of armour
	if (armour->isHelmet())
	    skilllevel += hasSkill(SKILL_ARMOUR_HELMET);
	if (armour->isJacket())
	    skilllevel += hasSkill(SKILL_ARMOUR_BODY);
	if (armour->isShield())
	    skilllevel += hasSkill(SKILL_ARMOUR_SHIELD);
	if (armour->isBoots())
	    skilllevel += hasSkill(SKILL_ARMOUR_BOOTS);

	// Switch based on material of armour.
	switch (armour->getMaterial())
	{
	    case MATERIAL_CLOTH:
	    case MATERIAL_PAPER:
		skilllevel += hasSkill(SKILL_ARMOUR_CLOTH);
		break;

	    case MATERIAL_LEATHER:
	    case MATERIAL_WOOD:
		skilllevel += hasSkill(SKILL_ARMOUR_LEATHER);
		break;

	    case MATERIAL_IRON:
		skilllevel += hasSkill(SKILL_ARMOUR_IRON);
		break;

	    case MATERIAL_GOLD:
	    case MATERIAL_GLASS:
	    case MATERIAL_MITHRIL:
	    case MATERIAL_SILVER:
	    case MATERIAL_STONE:
	    case MATERIAL_FLESH:
		skilllevel += hasSkill(SKILL_ARMOUR_EXOTIC);
		break;
		
	    case MATERIAL_ETHEREAL:
	    case MATERIAL_WATER:
	    case MATERIAL_NONE:
	    case NUM_MATERIALS:
		// No skills let you use this.
		break;
	}
    }

    return skilllevel;
}

void
MOB::noteAttacker(MOB *src)
{
    if (!src)
	return;

    // Don't hate self.  (This is technically covered by
    // hasCommonMaster, but it is better safe than sorry - nothing
    // is more embarrasing than suicidal mobs.)
    if (src == this)
	return;

    // This is likely friendly fire.
    if (hasCommonMaster(src))
    {
	// TODO: Weaken our bonds!
	return;
    }
	
    setAITarget(src);

    if (glb_aidefs[getAI()].markattackpos)
    {
	myAIState &= ~AI_LAST_HIT_LOC;
	myAIState |= src->getX() + src->getY() * MAP_WIDTH;
    }
}

bool
MOB::receiveAttack(ATTACK_NAMES attack, MOB *src, ITEM *weapon, 
		    ITEM *launcher,
		    ATTACKSTYLE_NAMES style, bool *didhit,
		    int multiplierbonus)
{
    return receiveAttack(&glb_attackdefs[attack],
			src, weapon, launcher, style, didhit,
			multiplierbonus);
}

bool
MOB::receiveAttack(const ATTACK_DEF *attack, MOB *src, ITEM *weapon,
		    ITEM *launcher,
		    ATTACKSTYLE_NAMES style, bool *didhit,
		    int multiplierbonus)
{
    // Determine if it hit...
    int		hitroll;		// Raw d20.
    int		multiplier = 1 + multiplierbonus;
    int		modifier;
    int		acroll;
    int		attackroll;
    bool	lived;
    int		damage_reduction = 0;
    int		skilllevel;
    bool	forcehit = false;
    bool	forcemiss = false;
    bool	abletomove;

    if (isAvatar())
    {
	// An attempt to hit us will also break the autorun even
	// if it fails.
	if (glbAutoRunEnabled)
	    formatAndReport("%U <stop> over concerns for your safety.");
	glbAutoRunEnabled = false;
    }

    // Before we get angered, we check with piety.  This way
    // we can detect attacks on friendlies.
    if (src && src->isAvatar() && src != this)
    {
	src->pietyAttack(this, attack, style);
    }	    

    // Someone attacked us!  We will make it our target if it exists!
    noteAttacker(src);

    // Assign all the noises.
    // Defender always noises their shield.
    // Attacker always noises their weapon.
    // If no weapon, we use the attacker's base noise level (A bit of
    // hack...  Perhaps should use attack's noise level?)
    makeEquipNoise(ITEMSLOT_LHAND);
    if (src)
    {
	if (weapon)
	    src->makeNoise(weapon->getNoiseLevel());
	else
	    src->makeNoise(glb_mobdefs[src->getDefinition()].noise);
    }

    //
    // Summary of To Hit Calculation
    //
    // We start with a hitroll = 1..20
    //     1 is crit miss, 20 crit threat.
    // We have an attack bonus (Phys or Magic level), which is 1..20
    // depending on how advanced the character is.
    // This modifier is rolled against depending on skilllevel
    // Weapon enchantment is added (0..7), this is rolled against
    // skilllevel.
    // Target Invisible [0,-4]
    // Target natural foe [0, 4]
    // Ranged attack from trees, [0,4]
    // -10 is added to allow AC 0 to represent 50% hit chance.
    // Attack Roll is thus:
    //  (1..20) + (1..20]) + (0..7) + [0,-4] + [0,4] + [0,4] - 10
    // (() marks a random range, [] a fixed range dependent on some factor.)

    // AC is calculated
    // AC is the rolled natural ac (0..20)  (Turtleoids go to 40!)
    // The worn AC of the specific body part 
    //   This is the items base AC [0..10]
    //   Worn AC is adjusted by enchantment [0..5]
    //   And again by skilllevel [0..2]
    // AC adds the rolled worn AC (0..17)
    // The two are then combined using 1/3rd semantic:
    // AC = max(nat, worn) + min(nat, worn) / 3.
    // Usually, we have one system or the other, so have
    // AC = (0..20)

    // We then compare the two:
    // if (attack_roll - 10 >= ac_roll) then hit occurs.
    // This is:
    // (1..20) + (0..[0..20]) + [0..7] - 10 >= (0..20)

    // First, we determine those cases where the attacker is guaranteed
    // to hit.
    forcehit = attack->alwayshit;
    if (src && weapon && (style == ATTACKSTYLE_THROWN))
    {
	// Check for true aim ability.
	if (weapon->defn().specialskill == SKILL_WEAPON_TRUEAIM)
	{
	    if (src->hasSkill(SKILL_WEAPON_TRUEAIM))
		forcehit = true;
	}
    }

    // Next, if they are guaranteed to miss.  Missing always takes
    // precedence.
    if (src && src->hasIntrinsic(INTRINSIC_OFFBALANCE) &&
	    (style == ATTACKSTYLE_THROWN || style == ATTACKSTYLE_MELEE))
    {
	// Off balance creatures will always miss
	forcemiss = true;
    }
    
    hitroll = rand_range(1, 20);

    // Check for guaranteed misses or criticals....
    if (forcemiss || (hitroll == 1 && !forcehit))
    {
	// Critical miss.
	if (src)
	    src->formatAndReport("%U completely <miss>!");
	else
	    reportMessage("It completely misses!");
	return true;
    }

    if (src)
    {
	modifier = src->getAttackBonus(attack, style);
	// Determine skill level with this weapon and this style.
	skilllevel = src->getWeaponSkillLevel(weapon, style);
    }
    else
    {
	modifier = 0;
	skilllevel = 2;	// Non-sourced match monsters for behaviour.
    }

    // The base modifier gets a skilllevel bonus so level 1 people
    // get a substantial boost for learning their weapons.
    modifier += skilllevel;

    // If there is an attacking weapon, its enchantment becomes
    // a modifier.
    // We roll these against skill level as well.
    if (weapon)
	modifier += weapon->getEnchantment();
    if (weapon && launcher)
	modifier += launcher->getEnchantment();

    // We roll against skill level once for total modifier
    // rather than once per bonus, this will minimize the effectiveness
    // of the bonus - ie, +1 enchant is just as useful as +1 physical
    // level.
    modifier = rand_roll(modifier, skilllevel - 1);

    // If we are invisible, and the source exists, and it cannot see
    // invisible, we get a negative modifier of -4.
    if (src)
    {
	if (!src->canSense(this))    
	{
	    modifier -= 4;
	}

	// If we are the natural foe of the attacker, +4
	if (src->defn().naturalfoe == defn().mobtype)
	{
	    modifier += 4;
	}

	// If we are attacked by someone in a tree at range,
	// opposite effect.
	if (style == ATTACKSTYLE_THROWN &&
	    src->hasIntrinsic(INTRINSIC_INTREE) &&
	    !hasIntrinsic(INTRINSIC_INTREE))
	{
	    modifier += 4;
	}
    }
    
    attackroll = hitroll + modifier - 10;

    acroll = rollAC(src);
    
    if (hitroll == 20)
    {
	int		threatroll;
	
	// We now have a critical threat.  We check to see if we hit
	// a second time.
	threatroll = rand_range(1, 20); 
	attackroll = threatroll + modifier - 10;
	if (threatroll == 20 ||
	    (threatroll != 1 && 
	            (attackroll >= acroll)) )
	{
	    multiplier++;
	}
    }
    else
    {
	if (attackroll < acroll && !forcehit)
	{
	    // A miss...
	    if (src)
	    {
		// Cannot use M:miss trick as then reflexive is handled
		// backwards.  Maybe reflexive should analyse verb
		// to find direction?
		BUF	buf = formatToString("%U <miss> %MU.", src, 0,
						this, 0);
		reportMessage(buf);
	    }
	    else if (weapon)
		formatAndReport("%IU <I:miss> %U.", weapon);
	    else
		formatAndReport("It misses %U.");
	    return true;
	}
    }

    // Figure out damage reduction.
    // We know, other than crits, that attackroll >= acroll.
    // However, a high acroll should reduce end damage somewhat.
    // Nethack style damage reduction.  Unlike nethack, this
    // primarily helps the monsters :>
    if (acroll > 10)
	damage_reduction = acroll - 10;
    
    // We may have a hit.  Check for other special skills.
    // Obviously all these skills require that we can actually move :>
    // if we can dodge, we get a flat 10% dodge chance.
    abletomove = false;
    if (!hasIntrinsic(INTRINSIC_ASLEEP) &&
	(!hasIntrinsic(INTRINSIC_PARALYSED) || hasIntrinsic(INTRINSIC_FREEDOM)))
    {
	abletomove = true;
	// Check for dodging: Flat 10%.
	if (!forcehit && hasSkill(SKILL_DODGE) && skillProc(SKILL_DODGE))
	{
	    const char *dodgelist[] = 
	    { 
		"dodge", 
		"sidestep", 
		"evade",
		0
	    };
	    BUF		weapname;
	    BUF		buf;

	    if (weapon)
		weapname = weapon->getName(false);
	    else
		weapname.reference("attack");
	    
	    if (src)
		buf = formatToString("%U <%B1> %MR %B2.",
			this, 0, src, 0,
			rand_string(dodgelist), weapname.buffer());
	    else
		buf = formatToString("%U <%B1> its %B2.",
			this, 0, 0, 0,
			rand_string(dodgelist), weapname.buffer());
	    reportMessage(buf);
	    return true;
	}

	// Check for moving target: 20% if moved last turn.
	if (!forcehit && 
	    (style == ATTACKSTYLE_THROWN || 
	     style == ATTACKSTYLE_SPELL || 
	     style == ATTACKSTYLE_WAND) && 
	    hasMovedLastTurn() && 
	    hasSkill(SKILL_MOVINGTARGET) && 
	    skillProc(SKILL_MOVINGTARGET))
	{
	    // Phew!  Wonder if anyone will do this?
	    // Yep, I've seen this in practice :>
	    const char *dodgelist[] = 
	    { 
		"avoid", 
		"duck under", 
		"roll clear of",
		0
	    };
	    BUF		weapname;
	    BUF		buf;

	    if (weapon)
		weapname = weapon->getName(false);
	    else
		weapname.reference("attack");
	    
	    if (src)
		buf = formatToString("%U <%B1> %MR %B2.",
			this, 0, src, 0,
			rand_string(dodgelist), weapname.buffer());
	    else
		buf = formatToString("%U <%B1> its %B2.",
			this, 0, 0, 0,
			rand_string(dodgelist), weapname.buffer());
	    reportMessage(buf);
	    return true;
	}

	// Check for parry.
	if (!forcehit && hasSkill(SKILL_WEAPON_PARRY))
	{
	    // Check if we actually have valid weapons.
	    ITEM		*myweapon;
	    int			 slot;

	    for (slot = ITEMSLOT_RHAND; slot <= ITEMSLOT_LHAND; slot++)
	    {
		myweapon = getEquippedItem((ITEMSLOT_NAMES)slot);
		if (!myweapon)
		    continue;
		if (myweapon->getSpecialSkill() != SKILL_WEAPON_PARRY)
		    continue;

		// This is a parriable weapon..
		// Check if this is off hand.
		if (slot == ITEMSLOT_LHAND)
		{
		    // One must have two weapon skill and pass
		    // the two weapon test.
		    if (!rand_chance(getSecondWeaponChance()))
			continue;
		}

		// Flat 10% parry chance.
		if (skillProc(SKILL_WEAPON_PARRY))
		{
		    const char *parrylist[] = 
		    { 
			"block", 
			"parry", 
			"deflect",
			0
		    };
		    BUF		weapname, myweapname;
		    BUF		buf;

		    myweapname = myweapon->getName(false);

		    if (weapon)
			weapname = weapon->getName(false);
		    else
			weapname.reference("attack");
		    
		    if (src)
			buf = formatToString("%U <%B1> %MR %B2 with %r %B3.",
				this, 0, src, 0,
				rand_string(parrylist), weapname.buffer(), 
				myweapname.buffer());
		    else
			buf = formatToString("%U <%B1> its %B2 with %r %B3.",
				this, 0, 0, 0,
				rand_string(parrylist), weapname.buffer(), 
				myweapname.buffer());
		    reportMessage(buf);
		    return true;
		}
	    }
	}
    }

    if (didhit)
	*didhit = true;

    lived = receiveDamage(attack, src, weapon, launcher, style, multiplier,
			  damage_reduction);

    if (lived && weapon)
    {
	weapon->applyPoison(src, this);
    }	    

    // We only trigger reflex attacks on melee attacks.
    if (lived && src && (style == ATTACKSTYLE_MELEE))
    {
	ATTACK_NAMES		reflex_attack;

	reflex_attack = (ATTACK_NAMES) defn().reflex_attack;
	// Successful attack, get the reflex...
	if (reflex_attack != ATTACK_NONE)
	{
	    if (rand_chance(defn().reflex_chance))
	    {
		// We do NOT go to receiveAttack here, as we will
		// get infinite loop.  Further, the damage is not
		// based on to-hit, but rather a flat chance.
		// Thus, we go straight to receive damage.
		src->receiveDamage(reflex_attack, this, 0, 0, ATTACKSTYLE_MELEE);
	    }
	}
	else if (abletomove)
	{
	    // Still a chance that we'll decide to riposte the attack.
	    if (hasSkill(SKILL_WEAPON_RIPOSTE))
	    {
		// Check if we actually have valid weapons.
		ITEM		*myweapon;
		int			 slot;

		for (slot = ITEMSLOT_RHAND; slot <= ITEMSLOT_LHAND; slot++)
		{
		    myweapon = getEquippedItem((ITEMSLOT_NAMES)slot);
		    if (!myweapon)
			continue;
		    if (myweapon->getSpecialSkill() != SKILL_WEAPON_RIPOSTE)
			continue;

		    // This is a parriable weapon..
		    // Check if this is off hand.
		    if (slot == ITEMSLOT_LHAND)
		    {
			// One must have two weapon skill and pass
			// the two weapon test.
			if (!rand_chance(getSecondWeaponChance()))
			    continue;
		    }

		    // Flat 10% riposte chance.  If you succeed at riposte,
		    // you always do damage without triggering the attackers
		    // reflex.  This avoids infinite loops and ensures that
		    // riposte isn't a bad thing.
		    // Well, I guess worshipers of Tlosh may be upset at
		    // losing piety for a melee kill.
		    if (skillProc(SKILL_WEAPON_RIPOSTE))
		    {
			const char *ripostelist[] = 
			{ 
			    "riposte", 
			    "counterstrike", 
			    "counterattack",
			    0
			};
			BUF		weapname, myweapname;

			if (weapon)
			    weapname = weapon->getName(false);
			else
			    weapname.reference("attack");

			myweapname = myweapon->getName(false);
			
			BUF		buf;
			if (src)
			    buf = formatToString("%U <%B1> %MR %B2 with %r %B3.",
				    this, 0, src, 0,
				    rand_string(ripostelist), weapname.buffer(),
				    myweapname.buffer());
			else
			    buf = formatToString("%U <%B1> its %B2 with %r %B3.",
				    this, 0, 0, 0,
				    rand_string(ripostelist), weapname.buffer(),
				    myweapname.buffer());
			reportMessage(buf);

			// Actually apply riposte effect.
			src->receiveDamage(myweapon->getAttack(), this, 0, 0, ATTACKSTYLE_MELEE);
			
			return lived;
		    }
		}
	    }
	}
    }
    return lived;
}

bool
MOB::receiveDamage(ATTACK_NAMES attack, MOB *src, ITEM *weapon, 
		    ITEM *launcher,
		    ATTACKSTYLE_NAMES style,
		    int initialmultiplier,
		    int initialdamagereduction)
{
    return receiveDamage(&glb_attackdefs[attack], src, weapon, launcher,
			    style, initialmultiplier,
			    initialdamagereduction);
}

bool
MOB::receiveDamage(const ATTACK_DEF *attack, MOB *src, ITEM *weapon, 
		    ITEM *launcher,
		    ATTACKSTYLE_NAMES style,
		    int initialmultiplier,
		    int initialdamagereduction)
{
    const char		*verb;
    ELEMENT_NAMES	 element;
    int			 multiplier = initialmultiplier;
    ATTACK_NAMES	 attackname;
    int			 damage_reduction = initialdamagereduction;
    bool		 silversafe = false;
    bool		 goldsafe = false;

    if (isAvatar())
    {
	// Being damaged will always break the auto run.
	if (glbAutoRunEnabled)
	    formatAndReport("%U <stop> over concerns for your well-being.");
	glbAutoRunEnabled = false;
    }

    // TODO: Add innate damage reduction
    // Roll against damage reduction.
    // We want a possibility of 0!
    if (damage_reduction)
	damage_reduction = rand_roll(damage_reduction + 1, -1) - 1;
    
    // Someone attacked us!  We will make it our target if it exists!
    // This covers cases where people damage is "less" intentionally,
    // ie: zapping wands, reading scrolls, etc.
    noteAttacker(src);

    verb = attack->verb;

    // We may have a critical already...
    if (multiplier > 1)
	verb = "crush";

    element = (ELEMENT_NAMES) attack->element;

    // Check elemental resistance or vulnerability.
    switch (element)
    {
	case ELEMENT_FIRE:
	    if (hasIntrinsic(INTRINSIC_RESISTFIRE))
		multiplier = 0;
	    if (hasIntrinsic(INTRINSIC_VULNFIRE))
		multiplier++;
	    break;
	case ELEMENT_COLD:
	    if (hasIntrinsic(INTRINSIC_RESISTCOLD))
		multiplier = 0;
	    if (hasIntrinsic(INTRINSIC_VULNCOLD))
		multiplier++;
	    break;
	case ELEMENT_ACID:
	    if (hasIntrinsic(INTRINSIC_RESISTACID))
		multiplier = 0;
	    if (hasIntrinsic(INTRINSIC_VULNACID))
		multiplier++;

	    // In any case, we lose our stoning when we take this
	    // sort of damage.
	    clearIntrinsic(INTRINSIC_STONING);
	    break;
	case ELEMENT_SHOCK:
	    if (hasIntrinsic(INTRINSIC_RESISTSHOCK))
		multiplier = 0;
	    if (hasIntrinsic(INTRINSIC_VULNSHOCK))
		multiplier++;
	    break;
	case ELEMENT_LIGHT:
	    // Blind people obviously don't care about light...
	    if (hasIntrinsic(INTRINSIC_BLIND))
		multiplier = 0;
	    // Sleeping people are blind.
	    if (hasIntrinsic(INTRINSIC_ASLEEP))
		multiplier = 0;
	    break;
	case ELEMENT_PHYSICAL:
	    if (hasIntrinsic(INTRINSIC_RESISTPHYSICAL))
		multiplier = 0;
	    if (hasIntrinsic(INTRINSIC_VULNPHYSICAL))
		multiplier++;
	    break;
	case ELEMENT_DEATH:
	    // The undead get free resistance
	    // Or should this be death resistance?
	    if (glb_mobdefs[myDefinition].mobtype == MOBTYPE_UNDEAD ||
		glb_mobdefs[myDefinition].mobtype == MOBTYPE_DAEMON)
		multiplier = 0;
	    if (hasIntrinsic(INTRINSIC_DRESSED_NECROMANCER))
		multiplier = 0;		// You look too cool to die.
	    break;
	case ELEMENT_POISON:
	case ELEMENT_REFLECTIVITY:
	case NUM_ELEMENTS:
	    // Unknown element..
	    UT_ASSERT(!"Unhandled ELEMENT!");
	    break;
    }

    // Check if it has a vulnerability to the material...
    if (hasIntrinsic(INTRINSIC_VULNSILVER))
    {
	if (weapon && weapon->getMaterial() == MATERIAL_SILVER)
	{
	    multiplier++;
	    verb = "sear";
	    silversafe = true;
	}
	// Creatures made of silver that engage in melee do silver
	// damage.
	if (!weapon && src && (style == ATTACKSTYLE_MELEE) &&
	    (src->getMaterial() == MATERIAL_SILVER))
	{
	    multiplier++;
	    verb = "sear";
	}
    }
    if (hasIntrinsic(INTRINSIC_GOLDALLERGY))
    {
	if (weapon && weapon->getMaterial() == MATERIAL_GOLD)
	{
	    multiplier++;
	    verb = "sear";
	    goldsafe = true;
	}
	// Creatures made of gold that engage in melee do gold
	// damage.
	if (!weapon && src && (style == ATTACKSTYLE_MELEE) &&
	    (src->getMaterial() == MATERIAL_GOLD))
	{
	    multiplier++;
	    verb = "sear";
	}
    }

    // Check if we are to damage silver weapons
    if (!silversafe && weapon && weapon->getMaterial() == MATERIAL_SILVER
	    && (style == ATTACKSTYLE_MELEE || style == ATTACKSTYLE_THROWN)
	    && !weapon->isArtifact())
    {
	if (rand_chance(1))
	{
	    if (src)
		src->formatAndReport("%R %Iu <I:dull>.", weapon);
	    else
		formatAndReport("%Iu <I:dull>.", weapon);
	    weapon->enchant(-1);
	}
    }

    // Check if we are to damage gold weapons
    if (!goldsafe && weapon && weapon->getMaterial() == MATERIAL_GOLD
	    && (style == ATTACKSTYLE_MELEE || style == ATTACKSTYLE_THROWN)
	    && !weapon->isArtifact())
    {
	// Gold is a lot more malleable
	if (rand_chance(10))
	{
	    if (src)
		src->formatAndReport("%R %Iu <I:dull>.", weapon);
	    else
		formatAndReport("%Iu <I:dull>.", weapon);
	    weapon->enchant(-1);
	}
    }

    // Earth elementals take extra damage from anyone who can
    // dig
    // Provided, of course, it is a melee attack.
    if (getDefinition() == MOB_EARTHELEMENTAL &&
	((weapon && weapon->hasIntrinsic(INTRINSIC_DIG)) ||
	 (!weapon && src && src->hasIntrinsic(INTRINSIC_DIG) && (style == ATTACKSTYLE_MELEE))))
    {
	multiplier++;
	verb = "disintegrate";
    }

    // Check to see if damage was resisted.  If so, we want the
    // attacker to make a note of it.  Notice that vuln to silver may
    // cause one to think that someone can't resist fire - this is intended.
    // (Silver wands of fire are great against he who cannot be spelled
    // properly)
    if (!multiplier && src)
    {
	src->aiNoteThatTargetCanResist(this, element);
    }

    // Apply damage...
    int		damage;

    damage = rand_dice(attack->damage, multiplier);

    // If it came from a weapon, the weapon's enchantment is added
    // in.  Note this DOES include the multiplier!
    // We only add the bonus to the base attack to avoid double-dipping
    // with flaming swords and what not.
    // We also roll against the total enchantment to effectively
    // cap barbarian bonuses.
    int		enchantbonus = 0;
    
    if (attack->sameattack == ATTACK_NONE)
    {
	if (weapon)
	    enchantbonus += weapon->getEnchantment() * multiplier;
	if (weapon && launcher)
	    enchantbonus += launcher->getEnchantment() * multiplier;

	// If there is a multiplier, we boost the re-rolls.
	enchantbonus = rand_roll(enchantbonus, multiplier-1);
    }
    damage += enchantbonus;

    // Apply damage reduction.  This is not allowed to
    // set the damage to 0.  (Or should it?)
    if (damage > 0)
    {
	damage -= damage_reduction;
	if (damage <= 0)
	    damage = 1;
    }
    else
    {
	// Damage is not greater than zero.  It could, however,
	// be less than zero.  We don't want damage attacks to
	// heal people!
	damage = 0;
    }

    if (damage == 0)
    {
	BUF		buf;

	// The attack did no damage.  Likely resistance, should
	// give feedback so loser doesn't keep acting.
	if (src)
	    formatAndReport("%U <ignore> %MR attack.", src);
	else if (weapon)
	    formatAndReport("%U <ignore> %IR attack.", weapon);
	else
	    formatAndReport("%U <ignore> its attack.");
    }
    else
    {
	BUF	buf;


	if (attack->eat > 0 && src)
	{
	    if (!isBloodless())
	    {
		src->feed(attack->eat);
	    }
	}
	
	// Regular attack.
	if (src)
	    buf = formatToString("%U <%B1> %MU",
			src, 0, this, 0,
			verb, 0);
	else if (weapon)
	    buf = formatToString("%U <%B1> %MU",
			0, weapon, this, 0,
			verb, 0);
	else
	{
	    buf = formatToString("%p <%B1> %MU",
			0, 0, this, 0,
			verb, 0);
	}
	if (damage > 10)
	    buf.append('!');
	else
	    buf.append('.');
	reportMessage(buf);
    }

    // Test for explosion...
    if (attack->explode_chance &&
	rand_chance(attack->explode_chance))
    {
	// TODO: This is improper conjugation!
	formatAndReport("%B1 explodes!", attack->explode_name);

	// Create the relevant smoke.
	if (attack->explode_smoke != SMOKE_NONE)
	{
	    int		dx, dy;
	    SMOKE_NAMES smoke = (SMOKE_NAMES) 
				    attack->explode_smoke;

	    for (dy = -1; dy <= 1; dy++)
		for (dx = -1; dx <= 1; dx++)
		    if (glbCurLevel->canMove(getX()+dx, getY()+dy, 
					    MOVE_STD_FLY))
			glbCurLevel->setSmoke(getX()+dx, getY()+dy,
					    smoke, src);
	}

	// Inflict the proper attacks...
	// Do not reply apply to this, or we may kill ourselves.
	if (src && attack->explode_attack != ATTACK_NONE)
	{
	    ATTACK_NAMES		oldeffect;
	    MOBREF			myself;
	    MOBREF			srcref;

	    myself.setMob(this);
	    srcref.setMob(src);

	    oldeffect = ourEffectAttack;
	    ourEffectAttack = (ATTACK_NAMES)
				    attack->explode_attack;

	    glbCurLevel->fireBall(getX(), getY(), 1, true,
				    areaAttackCBStatic, &srcref);

	    ourEffectAttack = oldeffect;

	    // Check to see if we are still alive!  If not, we want
	    // to return early.
	    if (!myself.getMob() || 
		myself.getMob() != this ||    
		myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
	    {
		return false;
	    }
	}
    }
    
    // TODO: Should we do this even if no damage?
    if (damage)
    {
	// Chance to wake up as a result of being hit.  Check before
	// inflictable in case this is a sleep attack.
	// Only get woken up 1 in 10 for 1 damage, additional 10%
	// for each more damage, to guarantee wake up with 10 damage.
	if (hasIntrinsic(INTRINSIC_ASLEEP))
	{
	    int noisethreshold = 10;
	    if (hasIntrinsic(INTRINSIC_TIRED))
		noisethreshold = 20;
	    if (rand_choice(noisethreshold) < damage)
	    {
		// Notification done automatically.
		clearIntrinsic(INTRINSIC_ASLEEP);
	    }
	}
	
	// Check for inflictable intrinsic...
	if (attack->inflict_intrinsic != INTRINSIC_NONE)
	{
	    if (rand_chance(attack->inflict_chance))
	    {
		if (!hasIntrinsic((INTRINSIC_NAMES) attack->inflict_resistance) 
		    && (!src || !src->hasIntrinsic((INTRINSIC_NAMES) attack->inflict_negate)))
		{
		    setTimedIntrinsic(src, (INTRINSIC_NAMES) attack->inflict_intrinsic,
				      rand_dice(attack->inflict_duration));
		}
	    }
	}

	// If the attack uses the light element, it will cause 
	// blindness.
	if (element == ELEMENT_LIGHT)
	{
	    // We cap this at 6 to prevent stupidly long blindness
	    // (and also the nerfing of light weapons)
	    setTimedIntrinsic(src, INTRINSIC_BLIND, (damage < 6) ? damage : 6);
	}

	// Decrement the weapon's charges to reflect the hit.
	if (weapon && src)
	{
	    // Only use charges in melee combat, not in ranged combat
	    // such as by zapping the rapier.
	    if (weapon->getDefinition() == ITEM_LIGHTNINGRAPIER &&
		style == ATTACKSTYLE_MELEE)
	    {
		weapon->useCharge();
		if (!weapon->getCharges())
		{
		    src->formatAndReport("%R %Iu <I:power> down.", weapon);
		    weapon->setDefinition(ITEM_RAPIER);
		    src->rebuildAppearance();
		}
		else
		{
		    // Quicken!
		    src->setTimedIntrinsic(src, INTRINSIC_QUICK, 2);
		}
	    }
	}
    }

    myHP -= damage;
    if (myHP <= 0)
    {
	bool		madecorpse = false;

	// Determine if the creature actually dies.  They may have life
	// saving...
	if (attemptLifeSaving("%U <die>!"))
	{
	    // Proceed to the next attack.  (Which will hopefuly
	    // finish the job in a suitably ironic manner :>)
	    goto nextattack;
	}

	// Determine if they can recover through polymorph...
	if (myBaseType.getMob())
	{
	    // However, if they are unchanging, the unpolymorph
	    // will fail, so they should die anyways.
	    if (hasIntrinsic(INTRINSIC_UNCHANGING))
	    {
		formatAndReport("%U <convulse> violently.");
	    }
	    else
	    {
		// Unpolymorph with system shock.
		actionUnPolymorph(false, true);

		// this is now deleted, so return false!
		return false;
	    }
	}

	// Getting here implies that the creature will truly die.

	const char *deathlist;
	const char *deathmsg;

	deathlist = glb_deathmsgdefs[
			glb_mobdefs[getDefinition()].deathmsg].msglist;
	
	deathmsg = glb_deathmsg_msgdefs[ (u8)
			deathlist[rand_choice(strlen(deathlist))]].msg;

	formatAndReport(deathmsg);

	// If a possessed creature dies, release the possession
	// with system shock
	if (hasIntrinsic(INTRINSIC_POSSESSED))
	{
	    actionReleasePossession(true);
	}
	
	// Create an item from this dude and dump on the level...
	madecorpse = rand_chance(defn().corpsechance);

	// If the dead creature is a familiar we always leave a corpse
	// to give you the chance to resurrect your pet.
	// We only do this if there is a non-zero corpse chance
	// to avoid corpse generation of impossible monsters
	if (hasIntrinsic(INTRINSIC_FAMILIAR) && defn().corpsechance)
	{
	    // Writing this while travelling 890km/h in a westerly
	    // direction.  Thankfully the new 777s have a normal power
	    // output so I should not have to run out of power on
	    // this 13 hour flight.  The p1610 has good battery power,
	    // but not that good!
	    madecorpse = true;
	}

	// See if the attacker has the clean kill ability and this was
	// a melee kill.  In that case, we increase the odds of a corpse.
	if (!madecorpse && defn().corpsechance &&
	    style == ATTACKSTYLE_MELEE && src && src->hasSkill(SKILL_CLEANKILL))
	{
	    // Re-roll our chance.
	    madecorpse = rand_chance(defn().corpsechance);
	}

	// Summoned critters never leave corpses.
	// Summoned creatures that become familiars likewise leave not
	// a corpse.
	if (hasIntrinsic(INTRINSIC_SUMMONED))
	    madecorpse = false;

	// Tell the world we were slain.
	triggerAsDeath(attack, src, weapon, madecorpse);

	// Find the worst poison that we have so we can apply it
	// to the corpse.
	POISON_NAMES		worstpoison = POISON_NONE;
	
	{
	    int		i;
	    
	    for (i = POISON_NONE; i < NUM_POISONS; i++)
	    {
		if (glb_poisondefs[i].intrinsic != INTRINSIC_NONE)
		{
		    if (hasIntrinsic((INTRINSIC_NAMES) glb_poisondefs[i].intrinsic))
		    {
			worstpoison = (POISON_NAMES) i;
		    }
		}
	    }
	}

	// Clear their death intrinsics so if they are ressed, they
	// don't keep burning...
	// We want to do this AFTER the victory screen, or the
	// death intrinsics won't be listed.
	clearDeathIntrinsics(true);
	
	glbCurLevel->unregisterMob(this);

	ITEM		*cur;
	bool		 isbelowgrade;
	
	// We wish to track if the resulting items should be put
	// underground.
	isbelowgrade = hasIntrinsic(INTRINSIC_INPIT) ||
		       hasIntrinsic(INTRINSIC_SUBMERGED);

	// Drop our inventory on this square...
	while ((cur = myInventory))
	{
	    // We manually drop the item for efficiency.
	    myInventory = cur->getNext();
	    cur->setNext(0);

	    cur->markBelowGrade(isbelowgrade);
	    
	    // Because we want the grade of the item to be used
	    // we set ignoregrade.  This seems entirely backwards
	    // but I'm sure I had a good reason for calling it that.
	    glbCurLevel->acquireItem(cur, getX(), getY(), 0, 0, true);
	}

	// Grant experience to the slayer.
	// ExpLevel is considered differently from hit die as it reflects
	// the full challenge of the critter.
	// DO NOT reward experience for suiciding!
	// (I once gained a level by dying :>)
	// We do not get experience for offing allies
	if (src && src != this)
	{
	    if (!hasCommonMaster(src))
		src->receiveDeathExp(myExpLevel);

	    // Grant piety for killing:
	    src->pietyKill(this, style);
	}

	// If we created a corpse, the corpse tracks us and thus
	// will delete this.  Otherwise, we better clean up.
	if (madecorpse)
	{
	    // We also want to set the dead intrinsic so those
	    // following old mobrefs will correctly determine our state.
	    setIntrinsic(INTRINSIC_DEAD);

	    // Note that acquiring a corpse on a square may delete the
	    // mob, ie: if it falls into a pool of acid.
	    // That is why we had to set the dead intrinsic first.
	    cur = ITEM::createCorpse(this);
	    cur->markBelowGrade(isbelowgrade);

	    // Make the object poisoned.
	    if (worstpoison != POISON_NONE)
		cur->makePoisoned(worstpoison, 1);

	    // Mark ignore grade so we end up where we died.
	    glbCurLevel->acquireItem(cur, getX(), getY(), 0, 0, true);
	}
	else
	{
	    delete this;
	}
	
	// In any case, the mob is removed from the map list so
	// is deleted in the eyes of the caller.
	return false;
    }

nextattack:
    attackname = (ATTACK_NAMES) attack->sameattack;
    if (attackname != ATTACK_NONE)
	return receiveDamage(attackname, src, weapon, launcher, 
			     style, 
			     initialmultiplier, initialdamagereduction);

    return true;
}

void
MOB::clearDeathIntrinsics(bool silent)
{
    // Remove all intrinsics that have clearondeath set...
    INTRINSIC_NAMES		intrinsic;

    for (intrinsic = INTRINSIC_NONE; intrinsic < NUM_INTRINSICS;
	 intrinsic = (INTRINSIC_NAMES) (intrinsic+1))
    {
	if (glb_intrinsicdefs[intrinsic].clearondeath)
	{
	    clearIntrinsic(intrinsic, silent);
	}
    }
}

void
MOB::receiveDeathExp(int explevel)
{
    float		basic;
    float		newbiebonus;
    
    // The rough attempt at balance is this:
    // 1) We can kill people of our own exp level easily
    // 2) You must kill 10 + myExpLevel to get a new level
    // 3) You get a new level every 1000 exp.

    basic = 1000.0f / (10 + myExpLevel);
    while (explevel > myExpLevel)
    {
	basic *= 1.10f;
	explevel--;
    }
    while (explevel < myExpLevel)
    {
	basic *= 0.8f;
	explevel++;
    }

    newbiebonus = 1.0f;
#if 0
    // This doubles the rate of experience at early levels.
    // It has been removed as monster generation is now safer.
    if (myExpLevel < 10)
    {
	newbiebonus = 2.0f - (myExpLevel / 10.0f);
    }
#endif

    basic *= newbiebonus;
    
    // Round up...
    receiveExp((int)basic + 1);
}

SPELL_NAMES
MOB::findSpell(GOD_NAMES god, int *pnumspell) const
{
    // Pick a random skill
    SPELL_NAMES		spells[NUM_SPELLS];
    SPELL_NAMES		spell;
    int			numspell = 0;

    FOREACH_SPELL(spell)
    {
	if (spell == SPELL_NONE)
	    continue;
	
	if (hasSpell(spell))
	    continue;

	if (god == GOD_AGNOSTIC)
	    continue;

	// See if this is a valid god.
	if (!strchr(glb_spelldefs[spell].god, (char) god))
	    continue;

	// Make sure we have prereq.
	if (!canLearnSpell(spell))
	    continue;
	
	// Valid addition.
	spells[numspell++] = spell;
    }

    if (numspell)
	spell = spells[rand_choice(numspell)];
    else
	spell = SPELL_NONE;

    if (pnumspell)
	*pnumspell = numspell;

    return spell;
}


SKILL_NAMES
MOB::findSkill(GOD_NAMES god, int *pnumskill) const
{
    // Pick a random skill
    SKILL_NAMES		skills[NUM_SKILLS];
    SKILL_NAMES		skill;
    int			numskill = 0;

    FOREACH_SKILL(skill)
    {
	if (skill == SKILL_NONE)
	    continue;
	
	if (hasSkill(skill))
	    continue;

	if (god == GOD_AGNOSTIC)
	    continue;

	// See if this is a valid god.
	if (!strchr(glb_skilldefs[skill].god, (char) god))
	    continue;

	// Make sure we have prereq.
	if (!canLearnSkill(skill))
	    continue;
	
	// Valid addition.
	skills[numskill++] = skill;
    }

    if (numskill)
	skill = skills[rand_choice(numskill)];
    else
	skill = SKILL_NONE;

    if (pnumskill)
	*pnumskill = numskill;

    return skill;
}

void
MOB::learnSpell(SPELL_NAMES spell, int duration)
{
    int		i;

    // Check for bad spell..
    if (spell == (SPELL_NAMES) -1)
	return;

    if (spell == SPELL_NONE)
	return;
    
    // Find the right intrinsic...

    i = glb_spelldefs[spell].intrinsic;

    UT_ASSERT(i != NUM_INTRINSICS);
    if (i == NUM_INTRINSICS)
    {
	return;
    }

    if (duration < 0)
    {
	setIntrinsic((INTRINSIC_NAMES) i);
    }
    else
    {
	setTimedIntrinsic(this, (INTRINSIC_NAMES) i, duration);
    }

    // Apply to base self, if present.
    MOB		*base;

    base = myBaseType.getMob();
    if (base)
	base->learnSpell(spell, duration);
}

void
MOB::learnSkill(SKILL_NAMES skill, int duration)
{
    int		i;

    // Check for bad skill..
    if (skill == SKILL_NONE)
	return;
    
    // Find the right intrinsic...

    i = glb_skilldefs[skill].intrinsic;

    UT_ASSERT(i != NUM_INTRINSICS);
    if (i == NUM_INTRINSICS)
    {
	return;
    }

    if (duration < 0)
    {
	setIntrinsic((INTRINSIC_NAMES) i);
    }
    else
    {
	setTimedIntrinsic(this, (INTRINSIC_NAMES) i, duration);
    }

    // Apply to base self, if present.
    MOB		*base;

    base = myBaseType.getMob();
    if (base)
	base->learnSkill(skill, duration);
}
void
MOB::gainLevel()
{
    // If this is a creature, gain a physical level.  If it causes
    // us to evolve, evolve.
    if (glbStressTest || !isAvatar())
    {
	MOB		*base;
	BUF		 buf;

	base = myBaseType.getMob();

	int		 newhp, newmp = 0;
	int		 newhd, newmd = 0;		

	newhd = 2;
	newhp = rand_dice(newhd, 4, newhd);
	
	// If we have magic, grant us another half magic level.
	if (myMaxMP)
	{
	    newmd = 1;
	    newmp = rand_dice(newmd, 4, newmd);
	}
	    
	myHitDie += newhd;
	myHP += newhp;
	myMaxHP += newhp;

	myMagicDie += newmd;
	myMP += newmp;
	myMaxMP += newmp;
	
	myExpLevel++;

	if (base)
	{
	    base->myHitDie += newhd;
	    base->myHP += newhp;
	    base->myMaxHP += newhp;
	    
	    base->myMagicDie += newmd;
	    base->myMP += newmp;
	    base->myMaxMP += newmp;

	    base->myExpLevel++;
	}
	
	formatAndReport("%U <be> stronger!");

	// Only do evolution if we are in our base form.
	if (!base)
	{
	    // Check to see if we are now greater than our evolve target.
	    if (defn().evolvetarget != MOB_NONE)
	    {
		if (myExpLevel >= defn().evolvelevel)
		{
		    buf.sprintf("%%U <grow> into %s%s!", 
			    gram_getarticle(glb_mobdefs[defn().evolvetarget].name),
			    glb_mobdefs[defn().evolvetarget].name);
		    formatAndReport(buf.buffer());

		    // Transform our base type.
		    myDefinition = defn().evolvetarget;

		    // Roll new mp & hp for ourselves with the new type
		    // and adjust us upwards.
		    // This way we ensure we are at least as powerful as
		    // we should be regardless of the actual level gap.
		    newhp = rand_dice(defn().hp);
		    newmp = rand_dice(defn().mp);
		    if (newhp > getMaxHP())
			incrementMaxHP(newhp - getMaxHP());
		    if (newmp > getMaxMP())
			incrementMaxMP(newmp - getMaxMP());

		    // Acquire all the new nifty intrinsics.
		    mergeIntrinsicsFromString(defn().intrinsic);
		}
	    }

	    // If we are a familiar in our base form, we can
	    // learn, for now, any new spell.  Most will be a waste
	    // of XP, but, admittedly, may be needed as a prereq.
	    if (hasIntrinsic(INTRINSIC_FAMILIAR))
	    {
		SPELL_NAMES		spell;

		spell = findSpell(GOD_WIZARD);
		learnSpell(spell);
		if (spell != SPELL_NONE)
		{
		    buf.sprintf("%%U <learn> %s!", 
			    glb_spelldefs[spell].name);
		    formatAndReport(buf.buffer());
		}
	    }
	}

	return;
    }

    // Coming here means that this is the avatar.
    // We wish the user to select any one of the legal professions:
    // This is all handled by the piety code, thus the jump to
    // pietyGainLevel().
    pietyGainLevel();
}

void
MOB::receiveExp(int exp)
{
    int		 newexp;
    MOB		*base;
    MOB		*master;

    // The owner of this mob gets his cut.
    master = getMaster();
    if (master)
    {
	// Just in case we have an infinite loop here, we verify
	// we have non-zero exp to propagate.
	if (exp > 2)
	    master->receiveExp(exp / 2 + 1);
    }
    
    // We do this bit of craziness to allow us to guarantee
    // that a u16 will fit the experience.
    newexp = myExp;
    newexp += exp;
    while (newexp >= 1000)
    {
	newexp -= 1000;
	gainLevel();
    }
    myExp = newexp;

    base = myBaseType.getMob();
    if (base)
	base->myExp = myExp;
}

void
MOB::wipeExp()
{
    MOB		*base;

    myExp = 0;
    base = myBaseType.getMob();
    if (base)
	base->myExp = myExp;
}

void
MOB::receiveArmourEnchant(ITEMSLOT_NAMES slot, int bonus, bool shudder)
{
    int		 	 numvalid;
    ITEM		*armour, *drop;
    ITEMSLOT_NAMES	 slots[4];
    const char		*flavour;
    BUF			 buf;
    
    // First, determine an armour slot to enchant...
    if (slot == (ITEMSLOT_NAMES) -1)
    {
	numvalid = 0;
	if (getEquippedItem(ITEMSLOT_HEAD)) 
	    slots[numvalid++] = ITEMSLOT_HEAD;

	// Only enchange shields in left hand.
	armour = getEquippedItem(ITEMSLOT_LHAND);
	if (armour && !armour->isShield())
	    armour = 0;
	if (armour)
	    slots[numvalid++] = ITEMSLOT_LHAND;

	if (getEquippedItem(ITEMSLOT_BODY)) 
	    slots[numvalid++] = ITEMSLOT_BODY;
	if (getEquippedItem(ITEMSLOT_FEET)) 
	    slots[numvalid++] = ITEMSLOT_FEET;

	if (numvalid)
	    slot = slots[rand_choice(numvalid)];
	else
	    slot = ITEMSLOT_BODY;
    }

    armour = getEquippedItem(slot);
    if (armour)
    {
	switch (bonus)
	{
	    case -1:
		flavour = "dull brown";
		break;
	    case 0:
		// Likely a problem.
		flavour = "not at all";
		break;
	    case 1:
		flavour = "silver";
		break;
	    case 2:
		flavour = "bright silver";
		break;
	    default:
		// One of the many nods to Crawl.
		flavour = "aubergine";
		break;
	}
	
	formatAndReport("%R %Iu <I:glow> %B1.", armour, flavour);
	// If this is you, you now know the enchantment:
	if (isAvatar())
	    armour->markEnchantKnown();
	armour->enchant(bonus);
	// Warn if enchant is now greater than 3.
	if (shudder && (armour->getEnchantment() > 3))
	{
	    buf = formatToString("%U <shudder>!", 0, armour, 0, 0);
	    reportMessage(buf);
	}
	if (shudder && (armour->getEnchantment() > 5))
	{
	    // Determine if it is destroyed!
	    // NOTE: We had better hope the armour is not enchanting itself :>
	    if (rand_choice(2))
	    {
		buf = formatToString("%U <dissolve> into dust.", 0, armour, 0, 0);
		reportMessage(buf);

		drop = dropItem(armour->getX(), armour->getY());
		UT_ASSERT(drop == armour);
		delete drop;

		rebuildAppearance();
	    }
	}
    }
    else
    {
	formatAndReport("%R skin hardens.");
    }
}

void
MOB::receiveWeaponEnchant(int bonus, bool shudder)
{
    ITEM	*weapon, *drop;
    const char	*flavour;
    BUF		 buf;

    weapon = getEquippedItem(ITEMSLOT_RHAND);

    if (!weapon)
    {
	// TODO: You may not have hands!
	if (bonus > 0)
	    formatAndReport("%R hands move quicker.");
	else if (bonus == 0)
	    formatAndReport("%R hands move the same speed.");
	else
	    formatAndReport("%R hands move slower.");
    }
    else
    {
	if (!weapon->isWeapon())
	{
	    // Not a real weapon...
	    formatAndReport("%R %Iu <I:glow> briefly.", weapon);
	}
	else
	{
	    switch (bonus)
	    {
		case -1:
		    flavour = "dull brown";
		    break;
		case 0:
		    // Likely a problem.
		    flavour = "not at all";
		    break;
		case 1:
		    flavour = "silver";
		    break;
		case 2:
		    flavour = "bright silver";
		    break;
		default:
		    // One of the many nods to Crawl.
		    flavour = "aubergine";
		    break;
	    }

	    formatAndReport("%R %Iu <I:glow> %B1.", weapon, flavour);
	    weapon->enchant(bonus);
	    // If this is you, you now know the enchantment:
	    if (isAvatar())
		weapon->markEnchantKnown();
	    
	    // Warn if +5 or higher...
	    if (shudder && (weapon->getEnchantment() > 5))
	    {
		buf = formatToString("%U <shudder>!", 0, weapon, 0, 0);
		reportMessage(buf);
	    }
	    // Check if it dissolves!
	    if (shudder && (weapon->getEnchantment() > 7))
	    {
		// NOTE: We had better hope the weapon is not
		// enchanting itself!
		if (rand_choice(2))
		{
		    buf = formatToString("%U <dissolve> into dust!", 0, weapon, 0, 0);
		    reportMessage(buf);

		    drop = dropItem(weapon->getX(), weapon->getY());
		    UT_ASSERT(drop == weapon);
		    delete drop;

		    rebuildAppearance();
		}
	    }
	}
    }
}

void
MOB::receiveCurse(bool onlyequip, bool forceany)
{
    ITEM		*curselist[MOBINV_WIDTH * MOBINV_HEIGHT];
    int			 numcurse = 0;
    ITEM		*item;
    BUF			 buf;
    
    for (item = myInventory; item; item = item->getNext())
    {
	if (!item->isCursed())
	{
	    if (item->getX() == 0)
	    {
		// This is an equipped item.
		if (!onlyequip && !forceany)
		    numcurse = 0;
		onlyequip = true;
		curselist[numcurse++] = item;
	    }
	    else if (!onlyequip || forceany)
	    {
		curselist[numcurse++] = item;
	    }
	}
    }

    // If you have no items that are uncursed, we get a black cloud
    // on a body part.
    if (!numcurse)
    {
	ITEMSLOT_NAMES	 slot;
	slot = (ITEMSLOT_NAMES) rand_choice(NUM_ITEMSLOTS);

	// It could be that the current creature
	// doesn't have that body part!  Then
	// we give a different message.
	const char *bodypart = getSlotName(slot);

	if (!bodypart)
	{
	    formatAndReport("A black cloud forms nowhere near %U.");
	}
	else
	{
	    // Nothing equipped there.
	    buf = formatToString("A black cloud surrounds %r %B1.",
		    this, 0, 0, 0,
		    bodypart);
	    reportMessage(buf);
	}

	return;
    }

    // Choose an item to curse at random.
    item = curselist[rand_choice(numcurse)];

    buf = formatToString("%U <glow> black.",
		0, item, 0, 0);
    reportMessage(buf);
    item->makeCursed();

    // The black glow is a bit of a give away
    if (isAvatar())
	item->markCursedKnown();
}

void
MOB::receiveUncurse(bool doall)
{
    bool		 onlyequip = false;
    ITEM		*uncurselist[MOBINV_WIDTH * MOBINV_HEIGHT];
    int			 numuncurse = 0, i;
    ITEM		*item;
    BUF			 buf;
    
    for (item = myInventory; item; item = item->getNext())
    {
	if (item->isCursed())
	{
	    if (item->getX() == 0)
	    {
		// This is an equipped item.
		if (!onlyequip && !doall)
		    numuncurse = 0;
		onlyequip = true;
		uncurselist[numuncurse++] = item;
	    }
	    else if (!onlyequip || doall)
	    {
		uncurselist[numuncurse++] = item;
	    }
	}
    }

    // If you have nothing cursed, get helping message.
    if (!numuncurse)
    {
	formatAndReport("%U <feel> as if someone were helping %A.");
	return;
    }

    // Choose an item to uncurse at random, unless doall is set
    // in which case we uncurse it all.
    for (i = 0; i < numuncurse; i++)
    {
	if (doall)
	    item = uncurselist[i];
	else
	    item = uncurselist[rand_choice(numuncurse)];

	formatAndReport("%IU <I:glow> blue.", item);
	item->makeBlessed();

	// The blue glow is a bit of a give away
	if (isAvatar())
	    item->markCursedKnown();

	if (!doall)
	    break;
    }
}

bool
MOB::receiveHeal(int hp, MOB *src, bool vampiric)
{
    bool	didanything = false;
    if (myHP < getMaxHP())
    {
	didanything = true;

	// Only top up our health.
	if (myHP + hp > getMaxHP())
	    hp = getMaxHP() - myHP;

	if (src)
	    src->pietyHeal(hp, src == this, 
			   getAttitude(src) == ATTITUDE_HOSTILE,
			   vampiric);
	
	myHP += hp;
    }
    // Cure bleeding.  This is done even if you were at max hp.
    if (hasIntrinsic(INTRINSIC_BLEED))
    {
	clearIntrinsic(INTRINSIC_BLEED);
	didanything = true;
    }
    return didanything;
}

bool
MOB::receiveMana(int mp, MOB *src)
{
    bool	didanything = false;
    if (myMP < getMaxMP())
    {
	didanything = true;

	// Only top up our health.
	if (myMP + mp > getMaxMP())
	    mp = getMaxMP() - myMP;

	myMP += mp;
    }
    // Cure magic drain.  This is done even if you were at max mp.
    if (hasIntrinsic(INTRINSIC_MAGICDRAIN))
    {
	clearIntrinsic(INTRINSIC_MAGICDRAIN);

	// it could be we got the intrinsic from our armour.  Thus,
	// we only count as it doing something if there was a net change.
	if (!hasIntrinsic(INTRINSIC_MAGICDRAIN))
	    didanything = true;
    }
    return didanything;
}

bool
MOB::receiveCure()
{
    int		i;
    bool	didanything = false;

    for (i = 0; i < NUM_POISONS; i++)
    {
	if (hasIntrinsic((INTRINSIC_NAMES) glb_poisondefs[i].intrinsic))
	{
	    clearIntrinsic((INTRINSIC_NAMES) glb_poisondefs[i].intrinsic);
	    didanything = true;
	}
    }

    return didanything;
}

bool
MOB::receiveSlowPoison()
{
    // Half all our poison counters...
    INTRINSIC_COUNTER		*counter;
    int				 i;
    bool			 didanything = false;

    for (i = 0; i < NUM_POISONS; i++)
    {
	counter = getCounter((INTRINSIC_NAMES) glb_poisondefs[i].intrinsic);
	if (counter)
	{
	    counter->myTurns = (counter->myTurns + 1) / 2;
	    didanything = true;
	}
    }

    return didanything;
}

void
MOB::reportMessage(const char *str) const
{
    // We sometimes have certain things null, like gaintxt.
    if (!str)
	return;
    
    // Check if we can see the source of the message.  If not, we
    // can't very well report it!
#if 1
    if (isAvatar())
    {
	UT_ASSERT(getX() >= 0 && getX() < MAP_WIDTH);
	UT_ASSERT(getY() >= 0 && getY() < MAP_HEIGHT);
    }
    if (MOB::getAvatar() &&
	!glbCurLevel->hasLOS(MOB::getAvatar()->getX(), MOB::getAvatar()->getY(),
		      getX(), getY()))
    {
	// Eat the message.
	return;
    }
#endif
    
    msg_report(gram_capitalize(str));
    // We always trail with some spaces, this will not cause line wrap,
    // but will ensure successive messages are all froody.
    msg_append("  ");
}

void
MOB::formatAndReport(const char *str)
{
    formatAndReportExplicit(str, 0, 0, 0);
}

void
MOB::formatAndReport(const char *str, const char *b1, const char *b2, const char *b3)
{
    formatAndReportExplicit(str, 0, 0, b1, b2, b3);
}

void
MOB::formatAndReport(const char *str, const MOB *mob, const char *b1, const char *b2, const char *b3)
{
    formatAndReportExplicit(str, mob, 0, b1, b2, b3);
}

void
MOB::formatAndReport(const char *str, const ITEM *item, const char *b1, const char *b2, const char *b3)
{
    formatAndReportExplicit(str, 0, item, b1, b2, b3);
}

void
MOB::formatAndReportExplicit(const char *str, const MOB *mob, const ITEM *item, const char *b1, const char *b2, const char *b3)
{
    BUF			buf;

    // It is valid to call with no string - for example the gaintxt
    // message of intrinsics.
    if (!str)
	return;
    
    buf = MOB::formatToString(str, this, 0, mob, item, b1, b2, b3);
    reportMessage(buf);
}

BUF
MOB::formatToString(const char *str, const MOB *self, const ITEM *selfitem, const MOB *mob, const ITEM *item, const char *b1, const char *b2, const char *b3)
{
    BUF		buf1, buf2, buf3;
    buf1.reference(b1);
    buf2.reference(b2);
    buf3.reference(b3);
    return formatToString(str, self, selfitem, mob, item, buf1, buf2, buf3);
}

BUF
MOB::formatToString(const char *str, const MOB *self, const ITEM *selfitem, const MOB *mob, const ITEM *item, BUF b1, BUF b2, BUF b3)
{
    BUF			 verbbuf;
    int			 targettype;
    BUF			 buf;
    BUF			*buflist[3];
    bool		 forcesingle = false;

    buflist[0] = &b1;
    buflist[1] = &b2;
    buflist[2] = &b3;

    buf.reference("");

    // It is acceptable to have a null string.  For example, when
    // being called with gaintxt.
    if (!str)
    {
	return buf;
    }

    // Only one of self and selfitem should be true!
    // HOwever, both can be false in which case we are "something".
    UT_ASSERT(!self || !selfitem);

    buf.allocate(100);
    
    while (*str)
    {
	targettype = 0;		// yourself
	if (*str == '%')
	{
	    str++;
	    if (*str == 'M')
	    {
		// The mob
		targettype = 1;
		UT_ASSERT(mob != 0);
		if (!mob)
		    targettype = 0;
		str++;
	    }
	    else if (*str == 'I')
	    {
		// the item
		targettype = 2;
		str++;
		if (*str == '1')
		{
		    str++;
		    forcesingle = true;
		}

		UT_ASSERT(item != 0);
		if (!item)
		    targettype = 0;
	    }
	    if (*str == 'U')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(self->getName(true, true, false));
		    else if (selfitem)
			buf.strcat(selfitem->getName(true, false, false, false, forcesingle));
		    else
			buf.strcat("something");
		}
		else if (targettype == 1)
		{
		    // If you target yourself, use "yourself".
		    if (mob == self)
			buf.strcat(mob->getReflexive());
		    else
			buf.strcat(mob->getName(true, true, false));
		}
		else if (targettype == 2)
		    buf.strcat(item->getName(true, false, false, false, forcesingle));
	    }
	    else if (*str == 'B')
	    {
		// Get the buffer number.
		int		bufnum = str[1] - '1';
		if (bufnum < 0)
		    bufnum = 0;
		if (bufnum > 2)
		    bufnum = 2;
		
		buf.strcat(*buflist[bufnum]);
		str+=2;
	    }
	    else if (*str == 'i')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(self->getName(true, true, false));
		    else if (selfitem)
			buf.strcat(selfitem->getName(true, false, false, false, forcesingle));
		    else
			buf.strcat("something");
		}
		else if (targettype == 1)
		{
		    // If you target yourself, use "yourself".
		    if (mob == self)
			buf.strcat(mob->getReflexive());
		    else
			buf.strcat(mob->getName(true, true, false));
		}
		else if (targettype == 2)
		    buf.strcat(item->getName(true, false, false, true, forcesingle));
	    }
	    else if (*str == 'u')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(self->getName(false, false, false));
		    else if (selfitem)
			buf.strcat(selfitem->getName(false, false, false, false, forcesingle));
		    else
			buf.strcat("something");
		}
		else if (targettype == 1)
		{
		    // If you target yourself, use "yourself".
		    if (mob == self)
			buf.strcat(mob->getReflexive());
		    else
			buf.strcat(mob->getName(false, false, false));
		}
		else if (targettype == 2)
		    buf.strcat(item->getName(false, false, false, false, forcesingle));
	    }
	    else if (*str == 'R')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(gram_makepossessive(self->getName(true, true, false)));
		    else if (selfitem)
			buf.strcat(gram_makepossessive(selfitem->getName(true, false, false, false, forcesingle)));
		    else
			buf.strcat("something's");
		}
		else if (targettype == 1)
		    buf.strcat(gram_makepossessive(mob->getName(true, true, false)));
		else if (targettype == 2)
		    buf.strcat(gram_makepossessive(item->getName(true, false, false, false, forcesingle)));
	    }
	    else if (*str == 'r')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(self->getPossessive());
		    else if (selfitem)
			buf.strcat(selfitem->getPossessive());
		    else
			buf.strcat("its");
		}
		else if (targettype == 1)
		    buf.strcat(mob->getPossessive());
		else if (targettype == 2)
		    buf.strcat(item->getPossessive());
	    }
	    else if (*str == 'A')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(self->getAccusative());
		    else if (selfitem)
			buf.strcat(selfitem->getAccusative());
		    else
			buf.strcat("something");
		}
		else if (targettype == 1)
		    buf.strcat(mob->getAccusative());
		else if (targettype == 2)
		    buf.strcat(item->getAccusative());
	    }
	    else if (*str == 'p')
	    {
		str++;
		if (targettype == 0)
		{
		    if (self)
			buf.strcat(self->getPronoun());
		    else if (selfitem)
			buf.strcat(selfitem->getPronoun());
		    else
			buf.strcat("it");
		}
		else if (targettype == 1)
		    buf.strcat(mob->getPronoun());
		else if (targettype == 2)
		    buf.strcat(item->getPronoun());
	    }
	    else if (*str)
	    {
		buf.append(*str++);
	    }
	}
	else if (*str == '<')
	{
	    // Double angle means a normal angle.
	    if (str[1] == '<')
	    {
		buf.append(*str++);
		str++;
		continue;
	    }

	    targettype = 0;

	    // Check for an item or mob specifier.
	    if (str[1] != '\0' && str[1] != '>' && str[2] == ':')
	    {
		if (str[1] == 'M')
		{
		    str += 2;
		    targettype = 1;
		    UT_ASSERT(mob != 0);
		    if (!mob)
			targettype = 0;
		}
		else if (str[1] == 'I')
		{
		    str += 2;
		    targettype = 2;
		    UT_ASSERT(item != 0);
		    if (!item)
			targettype = 0;
		}
	    }

	    // Search for back angle.
	    verbbuf.clear();
	    str++;
	    if (*str == '%' && str[1] == 'B')
	    {
		// Read in the specified buffer number
		int	bufnum = str[2] - '1';
		if (bufnum < 0) bufnum = 0;
		if (bufnum > 2) bufnum = 2;

		verbbuf = *buflist[bufnum];
		    
		// Eat up to next close
		while (*str && *str != '>')
		    str++;
	    }
	    else
	    {
		while (*str && *str != '>')
		{
		    verbbuf.append(*str++);
		}
	    }
	    if (*str)
		str++;
	    // Conjugate & write out the verb.
	    switch (targettype)
	    {
		case 0:
		    if (self)
			buf.strcat(self->conjugate(verbbuf.buffer()));
		    else if (selfitem)
			buf.strcat(selfitem->conjugate(verbbuf.buffer()));
		    else
			buf.strcat(gram_conjugate(verbbuf, VERB_IT));
		    break;
		case 1:
		    buf.strcat(mob->conjugate(verbbuf.buffer()));
		    break;
		case 2:
		    buf.strcat(item->conjugate(verbbuf.buffer()));
		    break;
	    }
	}
	else
	{
	    buf.append(*str++);
	}
    }

    return buf;
}

// Grammar style stuff...
BUF
MOB::conjugate(const char *verb, bool past) const
{
    return gram_conjugate(verb, getPerson(), past);
}

VERB_PERSON
MOB::getPerson() const
{
    if (isAvatar())
	return VERB_YOU;

    return (VERB_PERSON) myGender;
}

VERB_PERSON
MOB::getGender() const
{
    // We can't just go to getPerson as that returns VERB_YOU which
    // isn't a gender.
    return (VERB_PERSON) myGender;
}

const char *
MOB::getPronoun() const
{
    return gram_getpronoun(getPerson());
}

const char *
MOB::getPossessive() const
{
    return gram_getpossessive(getPerson());
}

const char *
MOB::getAccusative() const
{
    return gram_getaccusative(getPerson());
}

const char *
MOB::getOwnership() const
{
    return gram_getownership(getPerson());
}

const char *
MOB::getReflexive() const
{
    return gram_getreflexive(getPerson());
}

BUF
MOB::getName(bool usearticle, bool usedefinite, 
	     bool forcevisible, bool neverpronoun) const
{
    BUF		 basename;
    BUF		 buf;
    
    // The avatar, regardless of his current incarnation, is you.
    if (isAvatar())
    {
	buf.reference("you");
	return buf;
    }

    basename.reference(glb_mobdefs[myDefinition].name);

    // If we are a zombie or skeleton, include our basetype
    if (myDefinition != myOrigDefinition)
    {
	// Check that our new definition is the sort of thing that
	// gets double counted.  Ie, a cave troll turning into a normal
	// troll should not be double printed, while one becoming a ghast
	// should.
	if (defn().usenameasprefix)
	    basename.sprintf("%s %s", glb_mobdefs[myOrigDefinition].name, glb_mobdefs[myDefinition].name);
    }

    // Determine if we can see this critter.  If we can't,
    // it is an it.  We can always see ourselves in this sense.
    if (!forcevisible && !isAvatar() && getAvatar())
    {
	SENSE_NAMES	 sense;
	sense = getAvatar()->getSenseType(this);

	switch (sense)
	{
	    case NUM_SENSES:
		UT_ASSERT(0);
		// FALL THROUGH
	    case SENSE_SIGHT:
		// This is the traditional control path.
		break;
	    case SENSE_NONE:
		// We cannot sense the creature at all.
		// Makes you wonder why we are requesting the name :>
		buf.reference("something");
		return buf;
	    case SENSE_HEAR:
		// Normal control path.  It is assumed the sounds
		// are sufficient to disambiguate.
		break;
	    case SENSE_ESP:
		// Again, normal control path.
		break;
	    case SENSE_WARN:
	    {
		// We only have a sense of the relative power.
		int		leveldiff;

		leveldiff = getExpLevel() - getAvatar()->getExpLevel();
		if (leveldiff < -2)
		    basename.reference("wimpy creature");
		else if (leveldiff <= -1)
		    basename.reference("weak creature");
		else if (leveldiff == 0)
		    basename.reference("creature");
		else if (leveldiff <= 2)
		    basename.reference("strong creature");
		else if (leveldiff <= 5)
		    basename.reference("scary creature");
		else
		    basename.reference("very scary creature");

		// Note: We proceed to apply the normal article
		// rules to this entry!	
		break;
	    }
	}
    }

    // Specifically named critters always have the internal
    // pronoun, and no external one.
    // Unseen creatures do not get this path!
    if (myName.getName())
    {
	buf.sprintf("%s the %s",
		     myName.getName(), basename.buffer());
	return buf;
    }

    // Default to just the plain basename
    buf = basename;
    if (!usearticle)
    {
	return buf;
    }

    // Some people never use articles.  Specifically those which
    // are pronouns.
    // This path should not be reached.
    if (gram_ispronoun(basename))
	return buf;

    // Or those that are proper names, ie, start with upper case.
    if (isupper(basename.buffer()[0]))
	return buf;

    if (usedefinite)
    {
        buf.sprintf("the %s", basename.buffer());
    }
    else
    {
	buf.sprintf("%s%s", gram_getarticle(basename), basename.buffer());
    }
    
    return buf;
}

BUF
MOB::getDescription() const
{
    const char		*health;
    const char		*con;
    BUF			 buf;
    SENSE_NAMES		 sense;

    // Determine how the avatar can sense it.
    sense = SENSE_SIGHT;
    if (getAvatar())
    {
	sense = getAvatar()->getSenseType(this);
    }

    if (getHP() > getMaxHP())
    {
	health = "in superior condition";
    }
    else if (getHP() == getMaxHP())
    {
	health = "in perfect shape";
    }
    else if (getHP() > getMaxHP() * 0.6)
    {
	health = "in good shape";
    }
    else if (getHP() > getMaxHP() * 0.3)
    {
	health = "in poor shape";
    }
    else if (getHP() > 1)
    {
	health = "near death";
    }
    else
	health = "at death's gate";

    if (getAvatar())
    {
	int leveldiff;
	
	leveldiff = getExpLevel() - getAvatar()->getExpLevel();
	if (leveldiff < -2)
	    con = "a windshield kill";
	else if (leveldiff <= -1)
	    con = "an easy fight";
	else if (leveldiff == 0)
	    con = "an even fight";
	else if (leveldiff <= 2)
	    con = "a hard fight";
	else if (leveldiff <= 5)
	    con = "an impossible fight";
	else
	    con = "your funeral";

	// We always get the verbose description of ourselves.
	if (isAvatar())
	{
	    sense = SENSE_SIGHT;
	}

	BUF		name;
	switch (sense)
	{
	    case SENSE_SIGHT:
		buf = formatToString("%U <be> %B1.  %p <look> like %B2.  ",
				this, 0, 0, 0,
				health, con);
		buf = gram_capitalize(buf);
		break;
	    case SENSE_HEAR:
		name = getName(true, false);
		buf.sprintf("Sounds like %s.  ",
			    name.buffer());
		break;
	    case SENSE_ESP:
		name = getName(true, false);
		buf.sprintf("Thinks like %s.  ",
			    name.buffer());
		break;
	    case SENSE_WARN:
		buf.sprintf("It feels like %s.  ",
			    con);
		break;
	    case NUM_SENSES:
	    case SENSE_NONE:
		buf.sprintf("Seems suspicious....  ");
		break;
	}
    }
    else
    {
	buf = formatToString("%U <be> %B1.  ",
		this, 0, 0, 0,
		health);
    }

    return buf;
}

void
MOB::viewDescription() const
{
    BUF		buf;

    gfx_pager_addtext(getDescription());
    gfx_pager_newline();

    // If we are unique, add the proper unique description.
    if (hasIntrinsic(INTRINSIC_UNIQUE))
    {
	// TODO: Randomly generate something that hints at
	// the given abilities.
	buf = formatToString("The rampages of %U have become so well known as to earn %A a name!  ", this, 0, 0, 0);
	gfx_pager_addtext(buf);
	gfx_pager_newline();
    }

    // Add specific intrinsics that we can see at a glance.
    // These are any intrinsics with the visiblechange flag.
    INTRINSIC_NAMES		intrinsic;
    bool			hasany = false;

    for (intrinsic = INTRINSIC_NONE; intrinsic < NUM_INTRINSICS;
	    intrinsic = (INTRINSIC_NAMES) (intrinsic + 1))
    {
	// Skip spell and skill as they get listed separately.
	if (glb_intrinsicdefs[intrinsic].isspell ||
	    glb_intrinsicdefs[intrinsic].isskill)
	    continue;

	// We know if we are amnesiac.
	if (((intrinsic == INTRINSIC_AMNESIA && isAvatar()) || 
		glb_intrinsicdefs[intrinsic].visiblechange) &&
	    hasIntrinsic(intrinsic))
	{
	    if (!hasany)
	    {
		buf = formatToString("%p <be> ",
			this, 0, 0, 0);
		buf = gram_capitalize(buf);
		gfx_pager_addtext(buf);
		hasany = true;
	    }
	    else
		gfx_pager_addtext(", ");

	    if (intrinsic == INTRINSIC_TAME)
	    {
		// It is confusing to call enemy creatures "tame" when
		// they are in the employ of a foe.  Thus we special
		// case them
		if (isSlave(getAvatar()))
		    gfx_pager_addtext(glb_intrinsicdefs[intrinsic].name);
		else
		    gfx_pager_addtext("a follower");
	    }
	    else
	    {
		gfx_pager_addtext(glb_intrinsicdefs[intrinsic].name);
	    }
	}
    }

    // Reveal the general noise level as we see it...
    if (getAvatar())
    {
	int		noiselevel;

	noiselevel = getNoiseLevel(getAvatar());

	const char *noisetext;
	switch (noiselevel)
	{
	    case 0:
		noisetext = "silent";
		break;
	    case 1:
		noisetext = "quiet";
		break;
	    case 2:
		noisetext = "noisy";
		break;
	    case 3:
		noisetext = "loud";
		break;
	    default:
	    case 4:
		noisetext = "very loud";
		break;
	}
	if (!hasany)
	{
	    buf = formatToString("%p <be> ",
		    this, 0, 0, 0);
	    buf = gram_capitalize(buf);
	    gfx_pager_addtext(buf);
	    hasany = true;
	}
	else
	    gfx_pager_addtext(", ");

	gfx_pager_addtext(noisetext);
    }

    // Reveal the friendliness level.  We don't do this if we
    // are tame or neutral.
    if (!isAvatar() && getAvatar())
    {
	const char		*iff = 0;

	switch (getAttitude(getAvatar()))
	{
	    case ATTITUDE_HOSTILE:
		iff = "hostile";
		break;
		
	    case ATTITUDE_FRIENDLY:
		iff = "friendly";
		break;

	    case NUM_ATTITUDES:
	    case ATTITUDE_NEUTRAL:
		// Report nothing in these cases.
		break;
	}

	if (iff)
	{
	    if (!hasany)
	    {
		buf = formatToString("%p <be> ",
			this, 0, 0, 0);
		buf = gram_capitalize(buf);
		gfx_pager_addtext(buf);
		hasany = true;
	    }
	    else
		gfx_pager_addtext(", ");

	    gfx_pager_addtext(iff);
	}
    }

    if (hasany)
    {
	gfx_pager_addtext(".  ");
	gfx_pager_newline();
    }


    // If this is the avatar, list their spells and skills.
    if (isAvatar())
    {
	hasany = false;
	SPELL_NAMES		spell;
	FOREACH_SPELL(spell)
	{
	    if (hasSpell(spell, true, true))
	    {
		if (!hasany)
		{
		    gfx_pager_newline();
		    gfx_pager_addtext("Known spells: ");
		    hasany = true;
		}
		else
		    gfx_pager_addtext(", ");
		
		gfx_pager_addtext(glb_spelldefs[spell].name);	    
	    }
	}
	if (hasany)
	{
	    gfx_pager_addtext(".  ");
	    gfx_pager_newline();
	}
	
	hasany = false;
	SKILL_NAMES		skill;
	FOREACH_SKILL(skill)
	{
	    if (hasSkill(skill, true, true))
	    {
		if (!hasany)
		{
		    gfx_pager_newline();
		    gfx_pager_addtext("Learned skills: ");
		    hasany = true;
		}
		else
		    gfx_pager_addtext(", ");
		
		gfx_pager_addtext(glb_skilldefs[skill].name);	    
	    }
	}
	if (hasany)
	{
	    gfx_pager_addtext(".  ");
	    gfx_pager_newline();
	}
    }

    // Describe what we can see of its equipment.
    ITEMSLOT_NAMES	slot;
    bool		hasitems = false;

    FOREACH_ITEMSLOT(slot)
    {
	ITEM		*item;

	item = getEquippedItem(slot);

	if (item)
	{
	    // You can't have anything equipped in empty slots.
	    UT_ASSERT(getSlotName(slot) != 0);
	    if (getSlotName(slot))
	    {
		if (!hasitems)
		{
		    gfx_pager_separator();
		    gfx_pager_addtext("Equipped Items:");
		    gfx_pager_newline();
		    hasitems = true;
		}

		BUF		itemname = item->getName(false, false, true);

		buf.sprintf("- %s %s %s.",
			itemname.buffer(),
			glb_itemslotdefs[slot].preposition,
			getSlotName(slot));
		buf = gram_capitalize(buf);

		gfx_pager_addtext(buf);
		gfx_pager_newline();
	    }
	}
    }

    // Add encyclopedia entry.
    if (encyc_hasentry("MOB", getDefinition()))
    {
	gfx_pager_separator();
	encyc_pageentry("MOB", getDefinition());
    }

    gfx_pager_display();
}

void
MOB::pageCharacterDump(bool ondeath, bool didwin, int truetime) 
{
    BUF		buf;

    if (truetime == -1)
	truetime = gfx_getframecount();

    if (isAvatar())
    {
	buf.sprintf("POWDER %03d", hiscore_getversion());
	gfx_pager_addtext(buf);
	gfx_pager_newline();

	// Output our current platform.  No real easy way to
	// do this yet :>
#ifdef USING_SDL
#ifdef WIN32
	gfx_pager_addtext("Windows Version");
#elif defined(MACOSX)
	gfx_pager_addtext("Mac OSX Version");
#elif defined(SYS_PSP)
	gfx_pager_addtext("PSP Version");
#elif defined(iPOWDER)
	gfx_pager_addtext("iPOWDER Version");
#elif defined(LINUX)
	gfx_pager_addtext("Linux Version");
#else
	gfx_pager_addtext("Unknown SDL Version");
#endif
#else
#ifdef USING_DS
	gfx_pager_addtext("NDS Version");
#else
	gfx_pager_addtext("GBA Version");
#endif
#endif
	gfx_pager_newline();
	if (!hiscore_isnewgame() && hiscore_savecount() > 2)
	{
	    buf.sprintf("You have save scummed %d times.", 
			    hiscore_savecount()-2);
	    gfx_pager_addtext(buf);
	    gfx_pager_newline();
	}
    }
    
    // Name...
    if (glbWizard)
    {
	// Don't reveal wizard code.
	buf.sprintf("Wizard mode is activated.");
	gfx_pager_addtext(buf);
    }
    else
    {
	gfx_pager_addtext("Name: ");
	if (isAvatar())
	    gfx_pager_addtext(glbAvatarName);
	else
	    gfx_pager_addtext(getName(false));
    }
    gfx_pager_newline();

    gfx_pager_separator();
    
    buf.sprintf("Physical: %d/%d (max %d)",
	    getHP(), getHitDie(), getMaxHP());
    gfx_pager_addtext(buf); gfx_pager_newline();

    buf.sprintf("Mental: %d/%d (max %d)",
	    getMP(), getMagicDie(), getMaxMP());
    gfx_pager_addtext(buf); gfx_pager_newline();

    buf.sprintf("AC: %d", getAC());
    gfx_pager_addtext(buf); gfx_pager_newline();

    buf.sprintf("X: %d", getExp());
    gfx_pager_addtext(buf); gfx_pager_newline();

    if (isAvatar())
    {
	// We take the global level as authoratative.  If we are stoned
	// we are removed from the level map so have a dlevel of -1.
	if (glbCurLevel)
	    buf.sprintf("Depth: %d", glbCurLevel->getDepth());
	else
	    buf.sprintf("Depth: %d", getDLevel());
	gfx_pager_addtext(buf); gfx_pager_newline();

	buf.sprintf("%d moves over ", speed_gettime());
	gfx_pager_addtext(buf); 

	// Report wall clock time taken.
	buf = victory_formattime(truetime);
	gfx_pager_addtext(buf); gfx_pager_newline();

	if (ondeath)
	{
	    // Give the final score.
	    buf.sprintf("Score: %d", calcScore(didwin));
	    gfx_pager_addtext(buf); gfx_pager_newline();
	}
    }

    // Add all intrinsics.
    // These are any intrinsics with the visiblechange flag.
    INTRINSIC_NAMES		intrinsic;
    bool			hasany = false;
    char 			dash[3] = { '-', ' ', 0 };

    FOREACH_INTRINSIC(intrinsic)
    {
	// Don't show unnecessary stuff.
	if (!ondeath && !glb_intrinsicdefs[intrinsic].visiblechange)
	    continue;

	// Skip spell and skill intrinsics as they'd be duplicate
	if (glb_intrinsicdefs[intrinsic].isspell ||
	    glb_intrinsicdefs[intrinsic].isskill)
	    continue;

	if (hasIntrinsic(intrinsic))
	{
	    if (!hasany)
	    {
		hasany = true;
		gfx_pager_separator();
		gfx_pager_addtext("Intrinsics:");
		gfx_pager_newline();
	    }
	    dash[0] = *intrinsicFromWhatLetter(intrinsic);
	    gfx_pager_addtext(dash);
	    gfx_pager_addtext(gram_capitalize(glb_intrinsicdefs[intrinsic].name));
	    gfx_pager_newline();
	}
    }

    hasany = false;
    SPELL_NAMES		spell;
    FOREACH_SPELL(spell)
    {
	intrinsic = (INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic;
	if (hasSpell(spell, true, true))
	{
	    if (!hasany)
	    {
		gfx_pager_separator();
		gfx_pager_addtext("Spells:");
		gfx_pager_newline();
		hasany = true;
	    }
	    
	    dash[0] = *intrinsicFromWhatLetter(intrinsic);
	    gfx_pager_addtext(dash);
	    gfx_pager_addtext(glb_spelldefs[spell].name);	    
	    gfx_pager_newline();
	}
    }
    
    hasany = false;
    SKILL_NAMES skill;
    FOREACH_SKILL(skill)
    {
	intrinsic = (INTRINSIC_NAMES) glb_skilldefs[skill].intrinsic;
	if (hasSkill(skill, true, true))
	{
	    if (!hasany)
	    {
		gfx_pager_separator();
		gfx_pager_addtext("Skills:");
		gfx_pager_newline();
		hasany = true;
	    }
	    
	    dash[0] = *intrinsicFromWhatLetter(intrinsic);
	    gfx_pager_addtext(dash);
	    gfx_pager_addtext(glb_skilldefs[skill].name);	    
	    gfx_pager_newline();
	}
    }
    
    // Describe what we can see of its equipment.
    int			slot;
    bool		hasitems = false;
    ITEM		*item;

    for (slot = 0; slot < NUM_ITEMSLOTS; slot++)
    {
	item = getEquippedItem((ITEMSLOT_NAMES) slot);

	if (item)
	{
	    // You can't have anything equipped in empty slots.
	    UT_ASSERT(getSlotName((ITEMSLOT_NAMES) slot) != 0);
	    if (getSlotName((ITEMSLOT_NAMES) slot))
	    {
		if (!hasitems)
		{
		    gfx_pager_separator();
		    gfx_pager_addtext("Equipped Items:");
		    gfx_pager_newline();
		    hasitems = true;
		}
		if (ondeath)
		    item->markIdentified(true);

		BUF		itemname = item->getName(false, false, true);
		buf.sprintf("- %s %s %s.",
			itemname.buffer(),
			glb_itemslotdefs[slot].preposition,
			getSlotName((ITEMSLOT_NAMES) slot));
		buf = gram_capitalize(buf);

		gfx_pager_addtext(buf);
		gfx_pager_newline();
	    }
	}
    }

    sortInventoryBySlot();

    // List all artifacts
    hasitems = false;
    for (item = myInventory; item; item = item->getNext())
    {
	if (!item->isArtifact())
	    continue;

	if (!hasitems)
	{
	    gfx_pager_separator();
	    gfx_pager_addtext("Artifacts:");
	    gfx_pager_newline();
	    hasitems = true;
	}
	else
	    gfx_pager_newline();
	if (ondeath)
	    item->markIdentified(true);

	item->pageDescription(true);
    }

    // List all items
    hasitems = false;
    for (item = myInventory; item; item = item->getNext())
    {
	if (!hasitems)
	{
	    gfx_pager_separator();
	    gfx_pager_addtext("Inventory Items:");
	    gfx_pager_newline();
	    hasitems = true;
	}
	if (ondeath)
	    item->markIdentified(true);

	buf.strcpy("- ");
	buf.strcat(item->getName(false, false, true));
	buf.strcat(".");
	buf = gram_capitalize(buf);

	gfx_pager_addtext(buf);
	gfx_pager_newline();
    }
}

void
MOB::characterDump(bool ondeath, bool didwin, bool savefile, int truetime) 
{
    BUF			buf;

    if (savefile)
    {
	gfx_pager_setwidth(72);		// Nice USENET width.

	pageCharacterDump(ondeath, didwin, truetime);

	// Save the character dump
	buf.sprintf("%s.txt", glbAvatarName);
	buf.makeFileSafe();

	gfx_pager_savetofile(buf);

	gfx_pager_setwidth(30);		// Back to GBA mode
    }

    pageCharacterDump(ondeath, didwin, truetime);

    gfx_pager_display();
}

bool
MOB::hasMovedLastTurn() const
{
    if (!isAvatar())
	return false;

    if (ourAvatarDx || ourAvatarDy)
	return true;

    return false;
}

void
MOB::getLastMoveDirection(int &dx, int &dy) const
{
    if (!isAvatar())
    {
	dx = 0;
	dy = 0;
	return;
    }
    dx = ourAvatarDx;
    dy = ourAvatarDy;
}

void
MOB::resetLastMoveDirection() const
{
    if (!isAvatar())
    {
	return;
    }
    if (ourAvatarMoveOld)
    {
	ourAvatarDx = 0;
	ourAvatarDy = 0;
    }
}

void
MOB::flagLastMoveAsOld() const
{
    ourAvatarMoveOld = true;
}

void
MOB::christen(const char *name)
{
    if (name)
	myName.setName(gram_capitalize(name));
    else
	myName.setName(0);
}

int
MOB::smartsCheck(int smarts)
{
    int		test;
    
    // The general theory is that people can adjust their actual
    // level by +/- 2 whenever they do a test.
    // Success is greater or equal.
    if (getSmarts() < smarts-2)
	return -1;		// Force failure.
    if (getSmarts() >= smarts+2)
	return 2;		// Force success.
    
    // Probability curve:
    // -2 : 1/9
    // -1 : 2/9
    //  0 : 3/9
    //  1 : 2/9
    //  2 : 1/9
    test = getSmarts() + rand_range(-1, 1) + rand_range(-1, 1);

    if (test >= smarts)
	return 1;

    return 0;
}

int
MOB::strengthCheck(int str)
{
    int		test;
    
    // The general theory is that people can adjust their actual
    // level by +/- 2 whenever they do a test.
    // Success is greater or equal.
    if (getStrength() < str-2)
	return -1;		// Force failure.
    if (getStrength() >= str+2)
	return 2;		// Force success.
    
    test = getStrength() + rand_range(-1, 1) + rand_range(-1, 1);

    if (test >= str)
	return 1;
    return 0;
}

int
MOB::getAC() const
{
    int		 natural_ac, worn_ac;
    int  	 slot;
    ITEM	*item;

    natural_ac = glb_mobdefs[myDefinition].baseac;
    
    worn_ac = 0;

    // Each slot has a rough multiplier weight we use.

    for (slot = 0; slot < NUM_ITEMSLOTS; slot++)
    {
	item = getEquippedItem((ITEMSLOT_NAMES)slot);
	if (item)
	{
	    int		skilllevel, itemac;
	    
	    // If the slot is the left hand, we only count it if it
	    // has the shield flag.  You can't put platemail in your
	    // left hand and get a super shield :>
	    // We do want to support artifacts that get a armour boost,
	    // however.
	    if (slot == ITEMSLOT_LHAND)
	    {
		if (!item->isShield() && item->isArmor())
		    continue;
	    }
	    // We want to give an effective estimate of the ac,
	    // so this should match the expected value of the
	    // rollAC call.
	    skilllevel = getArmourSkillLevel(item);
	    itemac = item->getAC();
	    itemac += skilllevel;

	    // Shields use skillchance for modifier, everything else
	    // can use the official table
	    if (slot == ITEMSLOT_LHAND)
	    {
		int		parrychance;
		if (skilllevel <= 0)
		    parrychance = 25;
		else if (skilllevel == 1)
		    parrychance = 50;
		else
		    parrychance = 75;
		
		worn_ac += itemac * parrychance;
	    }
	    else
	    {
		// Apply rolling effect.
		itemac = rand_rollMean(itemac, skilllevel-1, 256);
		itemac *= glb_itemslotdefs[slot].areaweight;
		itemac += 128;
		itemac >>= 8;

		worn_ac += itemac;
	    }
	}
    }

    // Normalize the worn_ac.  We do so against 50, not 100, because
    // this grants torso armour its apparent full AC, leading to
    // less user confusion (we can hope :>)
    // We have to normalize again now that we properly reflect
    // the fact we roll skill chance into the equation
    worn_ac = (worn_ac + 24) / 25;

    // The worn_ac that corresponds to natural_ac only applies for
    // 33%.
    // This is easiest calculated by the minimum of the two by three
    // and adding it up.
    if (worn_ac < natural_ac)
    {
	worn_ac = (worn_ac + 2) / 3;
    }
    else
    {
	natural_ac = (natural_ac + 2) / 3;
    }
    
    return natural_ac + worn_ac;
}

int
MOB::calcScore(bool haswon) const
{
    int		score;
    bool	hasheart;
    const MOB	*mob = this;

    // We don't want to use score of polyed version.
    if (mob->getBaseType())
	mob = mob->getBaseType();

    // First experience level gives no points.
    score = (mob->getExpLevel() - 1) * 1000;
    score += mob->getExp();

    // If we have the heart, get a bonus
    hasheart = mob->hasItem(ITEM_BLACKHEART);

    if (hasheart)
    {
	score += 5000;
    }

    // Get a bonus for every level explored.
    MAP		*map, *lastmap;
    int		 maxdepth, depth;

    depth = glbCurLevel->getDepth();
    lastmap = glbCurLevel;
    for (map = glbCurLevel; map; map = map->getMapDown(false))
    {
	lastmap = map;
    }
    
    maxdepth = lastmap->getDepth();

    // Give no points for the first level.
    score += (maxdepth - 1) * 500;

    if (hasheart && depth < 25)
    {
	// Gain points for every level you got back up.
	score += (25 - depth) * 500;
    }

    if (haswon)
    {
	// We penalize the longer you have been in the dungeon.
	// Your goal is to save the world, you shouldn't be rewarded
	// for dilly-dallying killing kobolds on your way back up.
	// A tiered system is used providing progressively greater
	// per-turn punishments.  This is balanced by the XP score
	// reward, so the break even point is wherever you can't
	// make as many XP per turn as your current reward level.
	score += 100000;

	int		turns = speed_gettime();
	score -= turns;
	if (turns > 20000)
	    score -= (turns-20000);
	if (turns > 60000)
	    score -= (turns-60000);
	if (turns > 120000)
	    score -= (turns-120000);
	if (turns > 240000)
	    score -= 9*(turns-240000);	// Please turn off your bot.  kthx.
    
	// Have pity on the poor fool.
	if (score < 5)
	    score = 5;
    }

    // Total score at this point:
    // Examplar wins:
    // Level:                        38  39  47
    // Turns:                        54k 20k 33k
    // All scores in ks.
    // 500 * 25 * 2 for exploration: 25  25  25
    // 1000 * XP:                    38  39  47
    // Heart bonus:                   5   5   5
    // Win bonus:                    10  10  10
    // 100 - turn                    46  80  67
    //                              114 148 144
    // 100 - turn exponential       -74  70  28
    //                               -6 137 105
    // 100 - two tier turn 	     12  80  54 
    //				     80 147 131
    // 10k flat                      78  77  87
    //

    // Finally, normalize the score.
    score += 5;
    score /= 10;

    if (score > 32678)
    {
	int		bonus;

	bonus = score - 32768;
	bonus /= 100;
	score = 32768 + bonus;

	if (score > 65535)
	    score = 65535;
    }

    return score;
}

int
MOB::rollAC(MOB *attacker) const
{
    int		 	 natural_ac;
    int		 	 worn_ac;
    ITEM		*item;
    ITEM		*shield;
    int		 	 skilllevel = 0;
    ITEMSLOT_NAMES	 slot;

    // The natural ac applies to every part of the creature.
    natural_ac = glb_mobdefs[myDefinition].baseac;
    
    // Determine which slot is hit.
    int			*attackprob;
    int			 attackloc_roll;
    // Probability tables.  These are for Head, Neck, Body, and Feet, scaled
    // to 100.
    // Our old AC contribution of boots was max 3, vs max 7 for body
    // armour.  This suggests a ratio of 1:3:1., or 20:60:20
    int			 attack_by_small[4] = {  5,  1, 55, 39 };
    int			 attack_by_same[4]  = { 20,  5, 55, 20 };
    int			 attack_by_large[4] = { 35,  5, 55,  5 };

    if (!attacker)
	attackprob = attack_by_same;
    else
    {
	if (attacker->getSize() > getSize())
	    attackprob = attack_by_large;
	else if (attacker->getSize() == getSize())
	    attackprob = attack_by_same;
	else
	    attackprob = attack_by_small;
    }

    // Find the slot
    attackloc_roll = rand_choice(100);
    if (attackloc_roll < attackprob[0])
	slot = ITEMSLOT_HEAD;
    else if (attackloc_roll < (attackprob[0] + attackprob[1]))
	slot = ITEMSLOT_AMULET;
    else if (attackloc_roll < (attackprob[0] + attackprob[1] + attackprob[2]))
	slot = ITEMSLOT_BODY;
    else
	slot = ITEMSLOT_FEET;

    // If we don't have the slot, we obviously won't be hit there
    // We then assign the body.
    // I certainly hope we have a body :>
    if (!hasSlot(slot))
	slot = ITEMSLOT_BODY;

    // Get the armour for the body.
    item = getEquippedItem(slot);

    if (item)
    {
	worn_ac = item->getAC();
	skilllevel = getArmourSkillLevel(item);
    }
    else
    {
	worn_ac = 0;
	skilllevel = 0;
    }

    // Calculate our effective worn_ac.
    // We get a bonus based on our skilllevel.  This allows
    // weak armours (which don't benefit from re-rolling) to still
    // benefit from skills.
    worn_ac += skilllevel;

    // Reroll depending on skill level.  -1 on reroll means to 
    // pick lowest.
    worn_ac = rand_roll(worn_ac, skilllevel - 1);

    // Add in the shield.
    shield = getEquippedItem(ITEMSLOT_LHAND);
    if (shield && (shield->isShield() || !shield->isArmor()))
    {
	int		parrychance;
	
	skilllevel = getArmourSkillLevel(shield);
	
	if (skilllevel <= 0)
	    parrychance = 25;
	else if (skilllevel == 1)
	    parrychance = 50;
	else
	    parrychance = 75;

	if (rand_chance(parrychance))
	{
	    worn_ac += rand_roll(shield->getAC() + skilllevel, skilllevel-1);
	}
    }

    // Roll the natural_ac.  No longer any skills for this.
    natural_ac = rand_roll(natural_ac);

    // Combine the two.
    // The minimum is divided by 3 and then they are added together.
    if (natural_ac > worn_ac)
    {
	worn_ac = (worn_ac + 2) / 3;
	worn_ac += natural_ac;
    }
    else
    {
	natural_ac = (natural_ac + 2) / 3;
	worn_ac += natural_ac;
    }

    // Actual AC for the purposes of this computation.
    return worn_ac;
}

int
MOB::getAttackBonus(const ATTACK_DEF *attack, ATTACKSTYLE_NAMES style)
{
    int		bonus;
    
    // Your exact attack bonus depends on the style.
    bonus = 0;

    switch (style)
    {
	case ATTACKSTYLE_MELEE:
	case ATTACKSTYLE_THROWN:
	    bonus += getHitDie();
	    break;
	case ATTACKSTYLE_SPELL:
	    bonus += getMagicDie();
	    break;
	case ATTACKSTYLE_WAND:
	    bonus += getHitDie() + getMagicDie();
	    break;
	case ATTACKSTYLE_MISC:
	    bonus = 0;
	    break;
	case ATTACKSTYLE_POISON:
	    bonus = 0;
	    break;
	case NUM_ATTACKSTYLES:
	case ATTACKSTYLE_MINION:
	    break;
    }

    bonus += attack->bonustohit;

    return bonus;
}

int
MOB::getSecondWeaponChance() const
{
    ITEM	*primary, *secondary;
    int		 totalskill;
    int		 chance;
    const ATTACK_DEF	*attack;
     
    // Try for trivial reject.
    if (!hasSkill(SKILL_TWOWEAPON))
	return 0;

    primary = getEquippedItem(ITEMSLOT_RHAND);
    secondary = getEquippedItem(ITEMSLOT_LHAND);

    // You need a second weapon to proc.
    if (!secondary)
	return 0;

    // Find how skilled we are with this pair.
    totalskill = getWeaponSkillLevel(primary, ATTACKSTYLE_MELEE);
    totalskill += getWeaponSkillLevel(secondary, ATTACKSTYLE_MELEE);

    // If secondary is bigger than myself, not possible.
    if (getSize() < secondary->getSize())
	return 0;
    
    // If we don't have a proper weapon attack, bail.
    // We don't check the ATTACK_NAME as we want to proc on weird
    // artifacts.
    attack = secondary->getAttack();
    if (attack == &glb_attackdefs[ATTACK_MISUSED] ||
	attack == &glb_attackdefs[ATTACK_MISUSED_BUTWEAPON] ||
	attack == &glb_attackdefs[ATTACK_MISTHROWN])
    {
	return 0;
    }

    // Add one star of effectiveness if ambidextrous
    if (hasSkill(SKILL_AMBIDEXTROUS))
	totalskill++;

    // Build the chance table...
    chance = 10;
    if (totalskill < 3)
	chance += totalskill * 8;
    else
	chance += 22 + (totalskill-3)*6;

    // If the secondary is not smaller than the primary, only have 2/3
    // chance...
    // Somewhat ironicly, I type this with only my left hand as S---
    // uses the right to go to sleep.
    // Ambidextrous cancels this.
    if (primary && (primary->getSize() <= secondary->getSize())
	&& !hasSkill(SKILL_AMBIDEXTROUS))
    {
	chance = (2*chance + 2) / 3;
    }

    return chance;
}

bool
MOB::acquireItem(ITEM *item, int sx, int sy, ITEM **newitem)
{
    ITEM		*merge;

    if (newitem)
	*newitem = item;

    // Check if slot is mergeable.
    merge = getItem(sx, sy);

    if (merge)
    {
	if (merge->canMerge(item))
	{
	    // Do a merge.
	    merge->mergeStack(item);
	    delete item;
	    if (newitem)
		*newitem = merge;
	    return true;
	}
	else
	{
	    // Failure to acquire.  Tried to load into a slot
	    // already occupied with a non-mergeable item.
	    return false;
	}
    }

    // Check to see if we have that slot!
    if (!sx)
    {
	if (!hasSlot((ITEMSLOT_NAMES) sy))
	    return false;
    }

    // Add the item to our list..  It should alread be removed
    // from its own list.
    UT_ASSERT(!item->getNext());
    item->setNext(myInventory);
    myInventory = item;
    item->setPos(sx, sy);

    item->setInventory(true);

    // If this was acquired in our equipment slot, rebuild our intrinsics.
    if (!sx || item->isCarryIntrinsic())
    {
	rebuildWornIntrinsic();
	rebuildAppearance();
    }

    // Mark that we will need to reinspect our inventory
    // to see if we should do anything.
    myAIState |= AI_DIRTY_INVENTORY;
    if (canDigest(item))
	myAIState |= AI_HAS_EDIBLE;
    
    return true;
}

bool
MOB::acquireItem(ITEM *item, ITEM **newitem)
{
    int		 sx, sy;
    bool	 foundmerge = false;
    ITEM	*merge;

    // Test to see if we can merge anywhere.
    for (merge = myInventory; merge; merge = merge->getNext())
    {
	// Do *not* merge with stuff already equipped.  Otherwise
	// you can easily wear 6 rings.  (It should be tough for
	// someone to do that :>)
	if (!merge->getX())
	    continue;
	    
	if (merge->canMerge(item))
	{
	    sx = merge->getX();
	    sy = merge->getY();
	    foundmerge = true;
	}
    }

    if (!foundmerge)
    {
	if (!findItemSlot(sx, sy))
	    return false;
    }

    acquireItem(item, sx, sy, newitem);
    return true;
}

bool
MOB::findItemSlot(int &sx, int &sy) const
{
    bool		seenempty = false;
    bool		seenfull = false;
    int			x, y;

    // Default values to ensure someone who forgets the return code
    // won't cause *too* many problems.
    sx = 1;
    sy = 0;

    // Rather than finding the first available slot, we find the slot
    // after the last item slot.  New items are thus added to the end
    // even if there are holes in the inventory.
    // This is a REALLY slow method of doing this!
    for (x = MOBINV_WIDTH-1; x > 0; x--)
    {
	for (y = MOBINV_HEIGHT-1; y >= 0; y--)
	{
	    if (!getItem(x, y))
	    {
		// Empty slot, a valid location.
		seenempty = true;
		sx = x;
		sy = y;
		if (seenfull)
		{
		    // Already seen a full slot so this is the last
		    // available hole so we use it.
		    return true;
		}
	    }
	    else
	    {
		seenfull = true;
		// If we have already seen an empty slot, sx and sy
		// will be initialized to it so we can return accordingly.
		if (seenempty)
		    return true;
		// Otherwise, keep looking for a free slot.
	    }
	}
    }

    // If our inventory is entirely empty, we will get here but
    // be able to succeed
    return seenempty;
}

ITEM *
MOB::getItem(int sx, int sy) const
{
    ITEM		*cur;

    for (cur = myInventory; cur; cur = cur->getNext())
    {
	if (cur->getX() == sx && cur->getY() == sy)
	    return cur;
    }

    return 0;
}

ITEM *
MOB::randomItem() const
{
    ITEM		*cur, *result = 0;
    int			 numitems = 0;

    for (cur = myInventory; cur; cur = cur->getNext())
    {
	if (!rand_choice(++numitems))
	    result = cur;
    }
    return result;
}

ITEM *
MOB::getSourceOfIntrinsic(INTRINSIC_NAMES intrinsic) const
{
    ITEM	*item;

    for (item = myInventory; item; item = item->getNext())
    {
	// This should match rebuildWornIntrinsic!
	if (item->getX() == 0 || item->isCarryIntrinsic())
	{
	    if (item->hasIntrinsic(intrinsic))
		return item;
	}
    }
    return 0;
}

MOB *
MOB::getInflictorOfIntrinsic(INTRINSIC_NAMES intrinsic) const
{
    INTRINSIC_COUNTER		*counter;

    counter = getCounter(intrinsic);

    if (!counter)
	return 0;

    return counter->myInflictor.getMob();
}

ITEM *
MOB::getEquippedItem(ITEMSLOT_NAMES slot) const
{
    return getItem(0, slot);
}

bool
MOB::hasAnyEquippedItems() const
{
    ITEM	*item;

    for (item = myInventory; item; item = item->getNext())
    {
	if (item->getX() == 0)
	    return true;
    }
    return false;
}

bool
MOB::hasSlot(ITEMSLOT_NAMES slot) const
{
    if (getSlotName(slot))
	return true;
    else
	return false;
}

const char *
MOB::getSlotName(ITEMSLOT_NAMES slot) const
{
    ITEMSLOTSET_NAMES	slotset;
    bool		lefthanded = false;

    slotset = (ITEMSLOTSET_NAMES) glb_mobdefs[myDefinition].slotset;

    // Only care about handedness for a few things.
    if (slot == ITEMSLOT_LHAND || slot == ITEMSLOT_RHAND)
    {
	// If the creature only has one "hand", such as a slug,
	// we consider it right handed.
	if (glb_itemslotsetdefs[slotset].lhand_name && 
	    glb_itemslotsetdefs[slotset].rhand_name)
	{
	    lefthanded = hasIntrinsic(INTRINSIC_LEFTHANDED);
	}
	if (lefthanded)
	{
	    // Swap slots...
	    if (slot == ITEMSLOT_LHAND)
		slot = ITEMSLOT_RHAND;
	    else
		slot = ITEMSLOT_LHAND;
	}
    }

    switch (slot)
    {
	case ITEMSLOT_HEAD:
	    return glb_itemslotsetdefs[slotset].head_name;
	case ITEMSLOT_AMULET:
	    return glb_itemslotsetdefs[slotset].neck_name;
	case ITEMSLOT_RHAND:
	    return glb_itemslotsetdefs[slotset].rhand_name;
	case ITEMSLOT_LHAND:
	    return glb_itemslotsetdefs[slotset].lhand_name;
	case ITEMSLOT_BODY:
	    return glb_itemslotsetdefs[slotset].body_name;
	case ITEMSLOT_RRING:
	    return glb_itemslotsetdefs[slotset].rring_name;
	case ITEMSLOT_LRING:
	    return glb_itemslotsetdefs[slotset].lring_name;
	case ITEMSLOT_FEET:
	    return glb_itemslotsetdefs[slotset].feet_name;
	case NUM_ITEMSLOTS:
	    UT_ASSERT(!"Invalid slot name!");
	    return 0;
	    break;
    }

    return 0;
}

ITEM *
MOB::dropItem(int sx, int sy)
{
    ITEM		*cur, *prev = 0;

    for (cur = myInventory; cur; cur = cur->getNext())
    {
	if (cur->getX() == sx && cur->getY() == sy)
	{
	    // Take this guy out of our list.
	    if (prev)
		prev->setNext(cur->getNext());
	    else
		myInventory = cur->getNext();
	    
	    cur->setNext(0);
	    cur->setInventory(false);

	    // If this was droppped from our equipment slot, 
	    // rebuild our intrinsics.
	    if (!cur->getX() || cur->isCarryIntrinsic())
	    {
		rebuildWornIntrinsic();
		rebuildAppearance();
	    }

	    return cur;
	}
	
	prev = cur;
    }

    return 0;
}

void
MOB::destroyInventory()
{
    ITEM		*cur, *next;

    // TODO: This is very dangerous!  We may have a quest
    // item or similar?
    for (cur = myInventory; cur; cur = next)
    {
	next = cur->getNext();

	cur->setNext(0);
	delete cur;
    }

    myInventory = 0;
    rebuildWornIntrinsic();
}

void
MOB::reverseInventory()
{
    ITEM	*newlist, *cur, *next;

    newlist = 0;
    for (cur = myInventory; cur; cur = next)
    {
	next = cur->getNext();
	cur->setNext(newlist);
	newlist = cur;
    }
    myInventory = newlist;
}

void
MOB::dropOneRing()
{
    if (getEquippedItem(ITEMSLOT_RRING) &&
	getEquippedItem(ITEMSLOT_LRING))
    {
	// Random finger falls off...
	ITEM		*drop, *olditem;
	BUF		 buf;
	const char	*slotname;
	ITEMSLOT_NAMES	 slot;

	slot = rand_choice(2) ? ITEMSLOT_LRING : ITEMSLOT_RRING;

	slotname = getSlotName(slot);
	// Since you have something equipped there, it better have a name!
	UT_ASSERT(slotname != 0);
	if (!slotname)
	    slotname = "finger";
	
	olditem = getEquippedItem(slot);

	drop = dropItem(olditem->getX(), olditem->getY());
	UT_ASSERT(drop == olditem);

	glbCurLevel->acquireItem(drop, getX(), getY(), this);

	buf.sprintf("%%IU <I:fall> off %%r %s.", slotname);
	formatAndReport(buf.buffer(), drop);
	
	// We dropped a ring, so must rebuild.
	rebuildAppearance();
	rebuildWornIntrinsic();
    }
}

bool
MOB::hasItem(ITEM_NAMES def) const
{
    ITEM		*item;

    for (item = myInventory; item; item = item->getNext())
    {
	if (item->getDefinition() == def)
	    return true;
    }
    return false;
}

bool
MOB::isLight() const
{
    int		 i;
    ITEM	*item;

    if (glb_mobdefs[myDefinition].lightradius)
	return true;

    if (hasIntrinsic(INTRINSIC_AFLAME))
	return true;

    for (i = 0; i < NUM_ITEMSLOTS; i++)
    {
	item = getEquippedItem((ITEMSLOT_NAMES) i);
	if (item && item->isLight())
	    return true;
    }

    return false;
}

bool
MOB::isBoneless() const
{
    return glb_mobdefs[myDefinition].isboneless;
}

bool
MOB::isBloodless() const
{
    return defn().isbloodless;
}

bool
MOB::canResurrectFromCorpse() const
{
    return defn().canresurrectfromcorpse;
}

MOB *
MOB::getMaster() const
{
    if (!hasIntrinsic(INTRINSIC_TAME))
	return 0;

    return getInflictorOfIntrinsic(INTRINSIC_TAME);
}

bool
MOB::isSlave(const MOB *master) const
{
    const MOB	*slave, *slavesmaster;

    if (!master)
	return false;

    slave = this;
    while (slave->hasIntrinsic(INTRINSIC_TAME))
    {
	slavesmaster = slave->getInflictorOfIntrinsic(INTRINSIC_TAME);

	if (!slavesmaster)
	{
	    // We are no longer tame as our master has died.
	    // However, this is const...
	    // slave->clearIntrinsic(INTRINSIC_TAME);
	    return false;
	}

	slave = slavesmaster;
	
	if (slave == master)
	    return true;
    }
    return false;
}

void
MOB::makeSlaveOf(const MOB *master)
{
    if (master == this)
    {
	// I am already my own master :>
	return;
    }
    setTimedIntrinsic((MOB *) master, INTRINSIC_TAME, 10000);
    // Reset the ai target in case it is a master's friend.
    clearAITarget();
}

bool
MOB::hasCommonMaster(const MOB *other) const
{
    const MOB	*master1, *master2;

    if (!other)
	return false;

    master1 = this;
    while (master1 && master1->hasIntrinsic(INTRINSIC_TAME))
    {
	master1 = master1->getInflictorOfIntrinsic(INTRINSIC_TAME);
    }
    master2 = other;
    while (master2 && master2->hasIntrinsic(INTRINSIC_TAME))
    {
	master2 = master2->getInflictorOfIntrinsic(INTRINSIC_TAME);
    }

    if (master1 && (master1 == master2))
	return true;
    return false;
}

int
MOB::getLightRadius() const
{
    int		 radius = 0;
    int		 i;
    ITEM	*item;
    
    radius = glb_mobdefs[myDefinition].lightradius;
    
    if (hasIntrinsic(INTRINSIC_AFLAME))
    {
	if (radius < 2)
	    radius = 2;
    }

    for (i = 0; i < NUM_ITEMSLOTS; i++)
    {
	item = getEquippedItem((ITEMSLOT_NAMES) i);
	if (item && item->isLight() && item->getLightRadius() > radius)
	    radius = item->getLightRadius();
    }

    return radius;
}

bool
MOB::canMoveDiabolically() const
{
    if (getDefinition() == MOB_GRIDBUG)
	return true;

    if (hasIntrinsic(INTRINSIC_DRESSED_BARBARIAN))
	return true;

    return false;
}

bool
MOB::canDigest(ITEM *item) const
{
    // If item is acid resistant, no one can digest.
    if (item->hasIntrinsic(INTRINSIC_RESISTACID))
    {
	if (!item->hasIntrinsic(INTRINSIC_VULNACID))
	    return false;
    }
    else if (item->hasIntrinsic(INTRINSIC_VULNACID))
    {
	return true;
    }

    MATERIAL_NAMES		 mat;
    const char			*edible;

    mat = item->getMaterial();
    edible = glb_mobdefs[myDefinition].edible;
    if (!edible)
	return false;
    while (*edible)
    {
	if (*edible == mat)
	    return true;
	edible++;
    }

    return false;
}

bool
MOB::safeToDigest(ITEM *item) const
{
    if (!item)
	return false;

    bool	ignorepoison;
    bool	ignorestone;
    bool	ignoregold;
    bool	ignoresilver;

    ignorepoison = hasIntrinsic(INTRINSIC_RESISTPOISON) || hasSkill(SKILL_BUTCHERY);
    ignorestone = hasIntrinsic(INTRINSIC_UNCHANGING) || hasIntrinsic(INTRINSIC_RESISTSTONING);
    ignoregold = !isWearing(MATERIAL_GOLD);  
    ignoresilver = !isWearing(MATERIAL_SILVER);  

    // Don't eat intrinsically poisoned stuff
    // Stuff that got poisoned we may still foolishly eat,
    // unless we are a familiar.  We want to be able to poison others
    // but we don't want familiars that learn poison bolt to suicide.

    // We do let familiars eat poly corpses.  This is considered amusing
    // and rare enough for users to avoid if they dislike.

    // We do let everyone eat acidic, yes it is damaging, but so is
    // being hungry and I don't want to risk the destoning code path
    // failing because someone didn't want to take a hit.

    if (ignorepoison)
    {
	if (hasIntrinsic(INTRINSIC_FAMILIAR))
	{
	    if (item->isPoisoned())
		return false;
	}
    }

    MOB		*corpse;
    corpse = item->getCorpseMob();
    if (corpse)
    {
	INTRINSIC_NAMES		intrinsic;

	FOREACH_INTRINSIC(intrinsic)
	{
	    if (INTRINSIC::hasIntrinsic(
			corpse->defn().eatgrant,
			intrinsic))
	    {
		// See how bad this intrinsic is
		if (glb_intrinsicdefs[intrinsic].ispoison)
		{
		    if (!ignorepoison)
			return false;
		}

		if (intrinsic == INTRINSIC_STONING)
		{
		    if (!ignorestone)
			return false;
		}

		if (intrinsic == INTRINSIC_VULNSILVER)
		{
		    if (!ignoresilver)
			return false;
		}

		if (intrinsic == INTRINSIC_GOLDALLERGY)
		{
		    if (!ignoregold)
			return false;
		}
	    }
	}
    }

    return true;
}

bool
MOB::canFly() const
{
    if (getMoveType() & MOVE_FLY)
	return true;

    return false;
}

bool
MOB::isWearing(MATERIAL_NAMES material) const
{
    ITEM	*item;

    for (item = myInventory; item; item = item->getNext())
    {
	// Ignore inventory items
	if (item->getX())
	    continue;

	if (item->getMaterial() == material)
	    return true;
    }
    return false;
}

static int
glb_itemsortcomparebyslot(const void *v1, const void *v2)
{
    const ITEM	*i1, *i2;

    i1 = *((const ITEM **) v1);
    i2 = *((const ITEM **) v2);

    // First sort by column
    if (i1->getX() < i2->getX())
	return -1;
    if (i1->getX() > i2->getX())
	return 1;

    // Then by row
    if (i1->getY() < i2->getY())
	return -1;
    if (i1->getY() > i2->getY())
	return 1;

    // Really should never happen.
    return 0;
}

void 
MOB::sortInventoryBySlot()
{
    // Sort all the items...
    ITEM		*item;
    int			 numitems, i;

    // Count how many items we have so we can make a flat array.
    numitems = 0;
    for (item = myInventory; item; item = item->getNext())
	numitems++;

    // This is a trivially true path.
    if (!numitems)
	return;

    ITEM		**flatarray;

    // Create a flat array of all the items.
    flatarray = new ITEM *[numitems];
    
    i = 0;
    for (item = myInventory; item; item = item->getNext())
	flatarray[i++] = item;

    // Now, we sort the item pointers according to our rules:
    qsort(flatarray, numitems, sizeof(ITEM *), glb_itemsortcomparebyslot);

    // Now create our item list to match the flat array...
    myInventory = 0;
    while (i--)
    {
	flatarray[i]->setNext(myInventory);
	myInventory = flatarray[i];
    }

    delete [] flatarray;
}

void
MOB::alertMobGone(MOB *mob)
{
    // A no-op as our internal pointers are all marshalled.
    // We could use this to determine when someone has left the leve.
    // Perhaps we should do stair follow if we can see it?  Dive down
    // the hole?  Etc?
}

void
MOB::clearReferences()
{
    // This is used to reset mobs when they change levels.
    // As all references should be marshalled with MOBREF, I don't know
    // what this should do.
}

void
MOB::rebuildAppearance()
{
    ITEM		*item;
    bool		 lefthanded;
    bool		 ismale;

    // Ignore non avatars as we may then equip before we initialize.
    if (!isAvatar())
	return;
    
    // Don't rebuild if our tile isn't the avatar tile.  This
    // can happen if we possess someone else - we don't want the
    // old avatar to update!
    if (getDefinition() != MOB_AVATAR)
	return;

    lefthanded = hasIntrinsic(INTRINSIC_LEFTHANDED);

    // Default to female, "He"s become male.  This leaves "it"s with the
    // feminine tilest util we make a neuter tileset :>
    // Interesting bug: getGender was returning "You", so avatars were
    // always female ;>
    ismale = false;
    if (getGender() == VERB_HE)
	ismale = true;

    // We have a very specific comp order...
    if (hasIntrinsic(INTRINSIC_STONING))
	gfx_copytiledata(TILE_AVATAR, MINI_NAKEDSTONE, ismale);
    else if (aiIsPoisoned())
	gfx_copytiledata(TILE_AVATAR, MINI_NAKEDPOISON, ismale);
    else
	gfx_copytiledata(TILE_AVATAR, MINI_NAKED, ismale);
    item = getEquippedItem(ITEMSLOT_HEAD);
    if (item && item->getMiniTile() != MINI_NONE)
	gfx_compositetile(ITEMSLOT_HEAD, item->getMiniTile(), ismale);
    item = getEquippedItem(ITEMSLOT_FEET);
    if (item && item->getMiniTile() != MINI_NONE)
	gfx_compositetile(ITEMSLOT_FEET, item->getMiniTile(), ismale);
    item = getEquippedItem(ITEMSLOT_BODY);
    if (item && item->getMiniTile() != MINI_NONE)
	gfx_compositetile(ITEMSLOT_BODY, item->getMiniTile(), ismale);
    item = getEquippedItem(ITEMSLOT_RHAND);
    if (item && item->getMiniTile() != MINI_NONE)
    {
	// This seems backwards.  That is just because I drew them
	// all backwards :>
	if (lefthanded)
	    gfx_compositetile(ITEMSLOT_RHAND, item->getMiniTile(), ismale);
	else
	    gfx_compositetile(ITEMSLOT_LHAND, item->getMiniTile(), ismale);
    }
    item = getEquippedItem(ITEMSLOT_LHAND);
    if (item && item->getMiniTile() != MINI_NONE)
    {
	if (lefthanded)
	    gfx_compositetile(ITEMSLOT_LHAND, item->getMiniTile(), ismale);
	else
	    gfx_compositetile(ITEMSLOT_RHAND, item->getMiniTile(), ismale);
    }
    item = getEquippedItem(ITEMSLOT_RRING);
    if (item && item->getMiniTile() != MINI_NONE)
	gfx_compositetile(ITEMSLOT_RRING, item->getMiniTile(), ismale);
    item = getEquippedItem(ITEMSLOT_LRING);
    if (item && item->getMiniTile() != MINI_NONE)
	gfx_compositetile(ITEMSLOT_LRING, item->getMiniTile(), ismale);
    item = getEquippedItem(ITEMSLOT_AMULET);
    if (item && item->getMiniTile() != MINI_NONE)
	gfx_compositetile(ITEMSLOT_AMULET, item->getMiniTile(), ismale);
}

void
MOB::rebuildWornIntrinsic()
{
    ITEM		*item;
    
    if (myWornIntrinsic)
	myWornIntrinsic->clearAll();
    
    for (item = myInventory; item; item = item->getNext())
    {
	// Check to see if it is equipped...
	if (item->getX() == 0 || item->isCarryIntrinsic())
	{
	    // Equipped, build the table:
	    item->buildIntrinsicTable(myWornIntrinsic);
	}
    }

    determineClassiness();
}

void
MOB::determineClassiness(int *srcdressiness)
{
    // Determine if the player is well dressed.
    bool 	wizard = true, fighter = true, ranger = true;
    bool	cleric = true, barb = true, necro = true;
    ITEM_NAMES	itemname;
    ITEM	*item;
    int		lcl_dress[NUM_GODS];
    int		*dressiness;

    if (srcdressiness)
	dressiness = srcdressiness;
    else
	dressiness = lcl_dress;

    memset(dressiness, 0, sizeof(int)*NUM_GODS);
    
    item = getEquippedItem(ITEMSLOT_HEAD);
    if (item)
    {
	itemname = item->getDefinition();
	if (itemname != ITEM_HAT)
	    wizard = false;
	else
	    dressiness[GOD_WIZARD]++;

	if (itemname != ITEM_LEATHERCAP)
	    ranger = false;
	else
	    dressiness[GOD_ROGUE]++;

	if (itemname != ITEM_HELM)
	    fighter = false;
	else
	    dressiness[GOD_FIGHTER]++;

	if (itemname != ITEM_FEATHERHELM)
	    barb = false;
	else
	    dressiness[GOD_BARB]++;

	if (item->getMaterial() != MATERIAL_GOLD)
	    necro = false;
	else
	    dressiness[GOD_NECRO]++;
    }
    else
    {
	wizard = false;
	fighter = false;
	ranger = false;
	necro = false;
	barb = false;
    }

    // Check for Tower shield for fighters.
    if (fighter || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_LHAND);
	if (item)
	{
	    itemname = item->getDefinition();
	    if (itemname != ITEM_TOWERSHIELD)
		fighter = false;
	    else
		dressiness[GOD_FIGHTER]++;
	}
	else
	{
	    fighter = false;
	}
    }

    // Check for spears for barbarians.
    if (barb || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_RHAND);
	if (item)
	{
	    itemname = item->getDefinition();
	    if (itemname != ITEM_SPEAR &&
		itemname != ITEM_SILVERSPEAR)
	    {
		barb = false;
	    }
	    else
		dressiness[GOD_BARB]++;
	}
	else
	    barb = false;
    }

    // Check for either the bow or torch for rangers.
    if (ranger || srcdressiness)
    {
	bool		rangergood = false;

	item = getEquippedItem(ITEMSLOT_RHAND);
	if (item && (item->getDefinition() == ITEM_RAPIER))
	{
	    ITEM		*lhand;
	    // Check for swashbuckling!
	    dressiness[GOD_ROGUE]++;
	    lhand = getEquippedItem(ITEMSLOT_LHAND);
	    if (lhand && (lhand->getDefinition() == ITEM_BUCKLER))
	    {
		rangergood = true;
		dressiness[GOD_ROGUE]++;
	    }
	}
	if (!rangergood && (!item || (item->getDefinition() != ITEM_BOW)))
	{
	    // Check for torch.  Aragorn, after all, wielded
	    // his torch in battle :>
	    item = getEquippedItem(ITEMSLOT_LHAND);
	    if (!item || 
		((item->getDefinition() != ITEM_TORCH) &&
		 (item->getDefinition() != ITEM_BOW)
		))
		ranger = false;
	    else
		dressiness[GOD_ROGUE]++;
	}
	else
	{
	    if (!rangergood && (item->getDefinition() != ITEM_BOW))
		dressiness[GOD_ROGUE]++;
	}
    }

    // Check for a blunt weapon for clerics
    // I don't think this should be necessary...  Spellbooks suffice, IMO.
#if 0
    if (cleric)
    {
	item = getEquippedItem(ITEMSLOT_RHAND);
	if (!item || item->getAttackSkill() != SKILL_WEAPON_BLUNT)
	    cleric = false;
    }
#endif

    // Check for book wield for clerics.
    if (cleric || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_LHAND);
	if (!item || item->getItemType() != ITEMTYPE_SPELLBOOK)
	    cleric = false;
	else
	    dressiness[GOD_CLERIC]++;
    }

    // Check for wand or staff for necromancers
    if (necro || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_RHAND);
	if (!item ||
	    (item->getItemType() != ITEMTYPE_STAFF &&
	     item->getItemType() != ITEMTYPE_WAND))
	{
	    item = getEquippedItem(ITEMSLOT_LHAND);
	    if (!item ||
		(item->getItemType() != ITEMTYPE_STAFF &&
		 item->getItemType() != ITEMTYPE_WAND))
	    {
		necro = false;
	    }
	    else
		dressiness[GOD_NECRO]++;
	}
	else
	    dressiness[GOD_NECRO]++;
    }

    if (wizard || ranger || fighter || cleric || necro || barb || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_BODY);
	if (item)
	{
	    itemname = item->getDefinition();
	    if (itemname != ITEM_ROBE)
		wizard = false;
	    else
		dressiness[GOD_WIZARD]++;
	    if (itemname != ITEM_STUDDEDLEATHER &&
		itemname != ITEM_LEATHERTUNIC)
		ranger = false;
	    else
		dressiness[GOD_ROGUE]++;
	    if (itemname != ITEM_PLATEMAIL)
		fighter = false;
	    else
		dressiness[GOD_FIGHTER]++;
	    if (itemname != ITEM_CRYSTALPLATE)
		necro = false;
	    else
		dressiness[GOD_NECRO]++;
	    if (itemname != ITEM_CHAINMAIL &&
		itemname != ITEM_MITHRILMAIL)
		cleric = false;
	    else
		dressiness[GOD_CLERIC]++;
	    if (itemname != ITEM_STUDDEDLEATHER &&
		itemname != ITEM_LEATHERTUNIC &&
		itemname != ITEM_CHAINMAIL &&
		itemname != ITEM_MITHRILMAIL)
		barb = false;
	    else
		dressiness[GOD_BARB]++;
	}
	else
	{
	    wizard = false;
	    ranger = false;
	    fighter = false;
	    cleric = false;
	    necro = false;
	    barb = false;
	}
    }

    if (wizard || ranger || fighter || cleric || barb || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_FEET);
	if (item)
	{
	    itemname = item->getDefinition();
	    if (itemname != ITEM_POINTEDSLIPPERS)
		wizard = false;
	    else
		dressiness[GOD_WIZARD]++;
	    if (itemname != ITEM_HIKINGBOOTS)
		ranger = false;
	    else
		dressiness[GOD_ROGUE]++;
	    if (itemname != ITEM_IRONSHOES)
		fighter = false;
	    else
		dressiness[GOD_FIGHTER]++;
	    if (itemname != ITEM_SANDALS)
		cleric = false;
	    else
		dressiness[GOD_CLERIC]++;
	    if (itemname != ITEM_RIDINGBOOTS)
		barb = false;
	    else
		dressiness[GOD_BARB]++;
	}
	else
	{
	    wizard = false;
	    ranger = false;
	    fighter = false;
	    cleric = false;
	    barb = false;
	}
    }

    // Wizards require both ring slots to be full.
    // Necros require it full with gold rings.
    if (wizard || necro || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_LRING);
	if (!item)
	{
	    wizard = false;
	    necro = false;
	}
	else if (item->getMaterial() != MATERIAL_GOLD)
	{
	    necro = false;
	    dressiness[GOD_WIZARD]++;
	}
	else
	{
	    dressiness[GOD_WIZARD]++;
	    dressiness[GOD_NECRO]++;
	}
	
	item = getEquippedItem(ITEMSLOT_RRING);
	if (!item)
	{
	    wizard = false;
	    necro = false;
	}
	else if (item->getMaterial() != MATERIAL_GOLD)
	{
	    necro = false;
	    dressiness[GOD_WIZARD]++;
	}
	else
	{
	    dressiness[GOD_WIZARD]++;
	    dressiness[GOD_NECRO]++;
	}
    }

    // Clerics require the proper amulet to be held.
    // THis should be made a specific amulet as soon as I have one
    // that looks more like a holy symbol.
    // Necros are similar, but have specific amulets in mind.
    // Barbarians just want something protecting their neck.
    if (cleric || necro || barb || srcdressiness)
    {
	item = getEquippedItem(ITEMSLOT_AMULET);
	if (!item)
	{
	    cleric = false;
	    necro = false;
	    barb = false;
	}
	else if (item->getMaterial() != MATERIAL_GOLD)
	{
	    necro = false;
	    dressiness[GOD_BARB]++;
	    dressiness[GOD_CLERIC]++;
	}
	else
	{
	    dressiness[GOD_BARB]++;
	    dressiness[GOD_CLERIC]++;
	    dressiness[GOD_NECRO]++;
	}
    }

    if (wizard)
	setIntrinsic(INTRINSIC_DRESSED_WIZARD);
    else
	clearIntrinsic(INTRINSIC_DRESSED_WIZARD);

    if (ranger)
	setIntrinsic(INTRINSIC_DRESSED_RANGER);
    else
	clearIntrinsic(INTRINSIC_DRESSED_RANGER);

    if (fighter)
	setIntrinsic(INTRINSIC_DRESSED_FIGHTER);
    else
	clearIntrinsic(INTRINSIC_DRESSED_FIGHTER);

    if (cleric)
	setIntrinsic(INTRINSIC_DRESSED_CLERIC);
    else
	clearIntrinsic(INTRINSIC_DRESSED_CLERIC);

    if (necro)
	setIntrinsic(INTRINSIC_DRESSED_NECROMANCER);
    else
	clearIntrinsic(INTRINSIC_DRESSED_NECROMANCER);

    if (barb)
	setIntrinsic(INTRINSIC_DRESSED_BARBARIAN);
    else
	clearIntrinsic(INTRINSIC_DRESSED_BARBARIAN);
}

bool
MOB::hasIntrinsic(INTRINSIC_NAMES intrinsic, bool allowitem) const
{
    if (myIntrinsic.hasIntrinsic(intrinsic))
	return true;

    if (allowitem && 
	myWornIntrinsic && myWornIntrinsic->hasIntrinsic(intrinsic))
	return true;

    return false;
}

int
MOB::intrinsicFromWhat(INTRINSIC_NAMES intrinsic) const
{
    int		src = 0;

    if (myWornIntrinsic && myWornIntrinsic->hasIntrinsic(intrinsic))
	src |= INTRINSIC_FROM_ITEM;

    if (myIntrinsic.hasIntrinsic(intrinsic))
    {
	// Determine if permament or not.
	if (getCounter(intrinsic))
	    src |= INTRINSIC_FROM_TEMPORARY;
	else
	    src |= INTRINSIC_FROM_PERMAMENT;
    }

    return src;
}

const char *
MOB::intrinsicFromWhatLetter(INTRINSIC_NAMES intrinsic) const
{
    int		src = intrinsicFromWhat(intrinsic);

    // This is the sort of exciting thing I get to write while
    // waiting for the GO train.  Again, it is 20 minutes late!
    if (src == 0)
	return "";
    if (src == INTRINSIC_FROM_ITEM)
	return "x";
    if (src & INTRINSIC_FROM_ITEM)
	return "*";
    // Built in only.
    return "+";
}

bool
MOB::hasSkill(SKILL_NAMES skill, bool allowitem, bool ignoreamnesia) const
{
    if (skill == SKILL_NONE)
	return false;

    if (!ignoreamnesia && hasIntrinsic(INTRINSIC_AMNESIA))
	return false;

    return hasIntrinsic((INTRINSIC_NAMES) glb_skilldefs[skill].intrinsic, allowitem);
}

bool	 
MOB::skillProc(SKILL_NAMES skill) const
{
    if (skill == SKILL_NONE)
	return false;

    return rand_chance(glb_skilldefs[skill].proc);
}

int
MOB::getUsedSkillSlots() const
{
    bool	testedskill[NUM_INTRINSICS];
    int		i, numslots;

    memset(testedskill, 0, sizeof(bool) * NUM_INTRINSICS);

    numslots = 0;
    for (i = 0; i < NUM_SKILLS; i++)
    {
	if (testedskill[glb_skilldefs[i].intrinsic])
	    continue;
	if (hasSkill((SKILL_NAMES) i))
	{
	    // If this was granted by an item, ignore it.
	    if (getSourceOfIntrinsic((INTRINSIC_NAMES) glb_skilldefs[i].intrinsic))
		continue;
	    numslots++;
	    testedskill[glb_skilldefs[i].intrinsic] = true;
	}
    }

    return numslots;
}

int
MOB::getFreeSkillSlots() const
{
    int		freeslots;
    
    freeslots = getHitDie() - getUsedSkillSlots();

    // We've artificially lifted our skill slots by the given hit die,
    // subtract out our raw form to get how many we have earned.
    freeslots += defn().baseskills - defn().hitdie;
    
    return freeslots;
}

bool
MOB::canLearnSkill(SKILL_NAMES skill, bool explain) const
{
    bool		canlearn = true;
    BUF			buf;
    
    if (skill == SKILL_NONE)
    {
	if (explain)
	    reportMessage("That is not a skill!");
	return false;
    }

    if (hasIntrinsic(INTRINSIC_AMNESIA))
    {
	if (explain)
	    reportMessage("Your condition seems to be anterograde as well as retrograde.");
	return false;
    }

    if (hasSkill(skill))
    {
	if (explain)
	{
	    buf.sprintf("You already know %s.",
			    glb_skilldefs[skill].name);
	    reportMessage(buf);
	}
	return false;
    }

    // Determine if we have any free skills...
    int		freeslots;

    freeslots = getFreeSkillSlots();
    if (freeslots <= 0)
    {
	if (explain)
	{
	    reportMessage("You have no free skill slots.");
	}
	return false;
    }
    
    const char 		*prereq;
    prereq = glb_skilldefs[skill].prereq;
    while (*prereq)
    {
	if (!hasSkill((SKILL_NAMES) *prereq))
	{
	    canlearn = false;
	    if (explain)
	    {
		buf.sprintf("You must first learn %s.",
			    glb_skilldefs[(u8) *prereq].name);
		reportMessage(buf);
	    }
	}
	prereq++;
    }

    return canlearn;
}

bool
MOB::hasSpell(SPELL_NAMES spell, bool allowitem, bool ignoreamnesia) const
{
    if (!ignoreamnesia && hasIntrinsic(INTRINSIC_AMNESIA))
	return false;

    return hasIntrinsic((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic, allowitem);
}

int
MOB::getUsedSpellSlots() const
{
    bool	testedspell[NUM_INTRINSICS];
    int		i, numslots;

    memset(testedspell, 0, sizeof(bool) * NUM_INTRINSICS);

    numslots = 0;
    for (i = 0; i < NUM_SPELLS; i++)
    {
	if (testedspell[glb_spelldefs[i].intrinsic])
	    continue;
	if (hasSpell((SPELL_NAMES) i))
	{
	    // If this was granted by an item, ignore it.
	    // (Ie, staff granted spells)
	    if (getSourceOfIntrinsic((INTRINSIC_NAMES) glb_spelldefs[i].intrinsic))
		continue;
	    numslots++;
	    testedspell[glb_spelldefs[i].intrinsic] = true;
	}
    }

    return numslots;
}

int
MOB::getFreeSpellSlots() const
{
    int		freeslots;
    
    freeslots = getMagicDie() - getUsedSpellSlots();
    
    freeslots += defn().basespells - defn().mpdie;

    return freeslots;
}

bool
MOB::canLearnSpell(SPELL_NAMES spell, bool explain) const
{
    bool		canlearn = true;
    BUF			buf;
    
    if (spell == SPELL_NONE)
    {
	if (explain)
	    reportMessage("That is not a spell!");
	return false;
    }
    
    if (hasIntrinsic(INTRINSIC_AMNESIA))
    {
	if (explain)
	    reportMessage("Your condition seems to be anterograde as well as retrograde.");
	return false;
    }

    if (hasSpell(spell))
    {
	if (explain)
	{
	    buf.sprintf("You already know %s.",
			    glb_spelldefs[spell].name);
	    reportMessage(buf);
	}
	return false;
    }

    // Determine if we have any free skills...
    int		freeslots;

    // Get a bonus magic spell slot cause we count from zero.
    freeslots = getFreeSpellSlots();
    if (freeslots <= 0)
    {
	if (explain)
	{
	    reportMessage("You have no free spell slots.");
	}
	return false;
    }

    const char 		*prereq;
    prereq = glb_spelldefs[spell].prereq;
    while (*prereq)
    {
	if (!hasSpell((SPELL_NAMES) *prereq))
	{
	    canlearn = false;
	    if (explain)
	    {
		buf.sprintf("You must first learn %s.",
			    glb_spelldefs[(u8) *prereq].name);
		reportMessage(buf);
	    }
	}
	prereq++;
    }

    return canlearn;
}

void
MOB::setIntrinsic(INTRINSIC_NAMES intrinsic)
{
    // If we have a counter, we get rid of it.  Otherwise this
    // intrinsic will vanish when the counter is up :>
    removeCounter(intrinsic);

    // Set our intrinsic list...
    if (!myIntrinsic.setIntrinsic(intrinsic))
    {
	if (glb_intrinsicdefs[intrinsic].affectappearance)
	    rebuildAppearance();
	// Verify it is not masked be worn clothing
	if (glb_intrinsicdefs[intrinsic].maskchangeifclothed &&
	    myWornIntrinsic &&
	    myWornIntrinsic->hasIntrinsic(intrinsic))
	{
	    return;
	}

	// A net new intrinsic.  We broad cast it, possible to the world,
	// depending on visiblechange.
	if (isAvatar() || 
	    (getAvatar() && glb_intrinsicdefs[intrinsic].visiblechange &&
	     getAvatar()->canSense(this)))
	{
	    formatAndReport(glb_intrinsicdefs[intrinsic].gaintxt);
	}
    }
}

void
MOB::mergeIntrinsicsFromString(const char *rawstr)
{
    if (!rawstr)
	return;

    while (*rawstr)
    {
	setIntrinsic( (INTRINSIC_NAMES) ((u8)*rawstr) );
	rawstr++;
    }
}

void
MOB::clearIntrinsic(INTRINSIC_NAMES intrinsic, bool silent)
{
    // Remove the counter so future counter sets don't get
    // confused.
    removeCounter(intrinsic);

    if (myIntrinsic.clearIntrinsic(intrinsic))
    {
	if (glb_intrinsicdefs[intrinsic].affectappearance)
	    rebuildAppearance();

	if (silent)
	    return;

	// Verify it is not masked be worn clothing
	if (glb_intrinsicdefs[intrinsic].maskchangeifclothed &&
	    myWornIntrinsic &&
	    myWornIntrinsic->hasIntrinsic(intrinsic))
	{
	    return;
	}
	    
	// Was previously set...
	if (isAvatar() ||
	    (getAvatar() && glb_intrinsicdefs[intrinsic].visiblechange &&
	     getAvatar()->canSense(this)))
	{
	    formatAndReport(glb_intrinsicdefs[intrinsic].losetxt);
	}
    }
}

POISON_NAMES
getPoisonFromIntrinsic(INTRINSIC_NAMES poison)
{
    if (!glb_intrinsicdefs[poison].ispoison)
	return POISON_NONE;

    // Look up current poison.
    POISON_NAMES	pname;
    
    FOREACH_POISON(pname)
    {
	if (glb_poisondefs[pname].intrinsic == poison)
	    return pname;
    }

    UT_ASSERT(!"Poison intrinsic with no poison.");
    return POISON_NONE;
}

INTRINSIC_NAMES
getNextMorePowerfulPoisonIntrinsic(INTRINSIC_NAMES poison)
{
    if (!glb_intrinsicdefs[poison].ispoison)
	return INTRINSIC_NONE;
    
    // Look up current poison.
    POISON_NAMES	pname;
    
    FOREACH_POISON(pname)
    {
	if (glb_poisondefs[pname].intrinsic == poison)
	    break;
    }

    pname = (POISON_NAMES) (pname + 1);
    if (pname >= NUM_POISONS)
	return INTRINSIC_NONE;

    return (INTRINSIC_NAMES) glb_poisondefs[pname].intrinsic;
}

void
MOB::setTimedIntrinsic(MOB *inflictor, INTRINSIC_NAMES intrinsic, int turns, bool quiet)
{
    INTRINSIC_COUNTER	*counter;

    bool		 ispoison;
    ispoison = glb_intrinsicdefs[intrinsic].ispoison;

    // Check for any intrinisc resistance.  These will prevent us
    // from ever getting the intrinsic in the first place.
    if (glb_intrinsicdefs[intrinsic].resistance != INTRINSIC_NONE)
    {
	if (hasIntrinsic((INTRINSIC_NAMES)glb_intrinsicdefs[intrinsic].resistance))
	{
	    // If there is an inflictor, we can tell them that
	    // we resisted.
	    if (inflictor)
	    {
		inflictor->aiNoteThatTargetHasIntrinsic(this,
		    (INTRINSIC_NAMES)glb_intrinsicdefs[intrinsic].resistance);
	    }
	    return;
	}
    }

    counter = getCounter(intrinsic);

    if (counter)
    {
	if (ispoison)
	{
	    // When you stack poisons, you always get the next more
	    // powerful poison.
	    INTRINSIC_COUNTER		*nextcounter;
	    INTRINSIC_NAMES		 nextpoison;

	    nextpoison = getNextMorePowerfulPoisonIntrinsic(intrinsic);
	    if (nextpoison == INTRINSIC_NONE)
	    {
		// Already most powerful poison.
		counter->myTurns += turns;
		counter->myInflictor.setMob(inflictor);
	    }
	    else
	    {
		// See if we already are poisoned with it (we should not be)
		nextcounter = getCounter(nextpoison);
		if (nextcounter)
		{
		    // We are masked by the more powerful poison, so remove
		    // ourselves.
		    // This should never happen.
		    clearIntrinsic(intrinsic, true);
		    return;
		}
		
		// No next more powerful poison exists.
		// Upgrade and create it.
		int	newcount;
		newcount = (counter->myTurns + turns + 1) / 2;
		clearIntrinsic(intrinsic, true);
		setTimedIntrinsic(inflictor, nextpoison, newcount);
		return;
	    }
	}
	else
	{
	    // Increment our counter.
	    counter->myTurns += turns;
	    counter->myInflictor.setMob(inflictor);
	}
    }
    else
    {
	// If we are a poison, we want to make sure we are not
	// masked by a more powerful poison.
	if (ispoison)
	{
	    INTRINSIC_NAMES		 nextpoison;

	    for (nextpoison = getNextMorePowerfulPoisonIntrinsic(intrinsic);
		 nextpoison != INTRINSIC_NONE;
		 nextpoison = getNextMorePowerfulPoisonIntrinsic(nextpoison))
	    {
		if (hasIntrinsic(nextpoison))
		{
		    return;
		}
	    }

	    // Check to see if there is a weaker poison that we will
	    // mask.  If there is, we want to clear it.
	    POISON_NAMES	newpoison = getPoisonFromIntrinsic(intrinsic);
	    POISON_NAMES	lesserpoison;
	    INTRINSIC_NAMES	lesserintrinsic;
	    INTRINSIC_COUNTER	*lessercounter;

	    FOREACH_POISON(lesserpoison)
	    {
		if (lesserpoison == newpoison)
		    break;
		if (lesserpoison != POISON_NONE)
		{
		    lesserintrinsic = (INTRINSIC_NAMES) glb_poisondefs[lesserpoison].intrinsic;
		    lessercounter = getCounter(lesserintrinsic);
		    if (lessercounter)
			turns += (lessercounter->myTurns + 1) >> (int)(newpoison - lesserpoison);
		    clearIntrinsic(lesserintrinsic, true);
		}
	    }
		    
	}
	
	// Determine if we already have the intrinsic
	// permamently.  If we do, we don't want to set
	// the counter as we will then LOSE it when it runs
	// out!
	if (!myIntrinsic.hasIntrinsic(intrinsic))
	    counter = addCounter(inflictor, intrinsic, turns);
	else
	    counter = 0;
    }

    if (counter && glb_intrinsicdefs[intrinsic].maxcount)
    {
	if (counter->myTurns > glb_intrinsicdefs[intrinsic].maxcount)
	    counter->myTurns = glb_intrinsicdefs[intrinsic].maxcount;
    }

    // Always set our main block..
    if (!myIntrinsic.setIntrinsic(intrinsic))
    {
	if (glb_intrinsicdefs[intrinsic].affectappearance)
	    rebuildAppearance();
	// Was previously unset...
	if (isAvatar() ||
	    (getAvatar() && glb_intrinsicdefs[intrinsic].visiblechange &&
	     getAvatar()->canSense(this)))
	{
	    if (!quiet)
		formatAndReport(glb_intrinsicdefs[intrinsic].gaintxt);
	}
    }
}

INTRINSIC_COUNTER *
MOB::getCounter(INTRINSIC_NAMES intrinsic) const
{
    INTRINSIC_COUNTER	*counter;

    for (counter = myCounters; counter; counter = counter->myNext)
    {
	if (counter->myIntrinsic == intrinsic)
	    return counter;
    }
    // None found...
    return 0;
}

void
MOB::removeCounter(INTRINSIC_NAMES intrinsic)
{
    INTRINSIC_COUNTER	*counter, *last;

    last = 0;
    for (counter = myCounters; counter; counter = counter->myNext)
    {
	if (counter->myIntrinsic == intrinsic)
	    break;
	last = counter;
    }

    if (!counter)
	return;		// Trivial remove
    if (last)
	last->myNext = counter->myNext;
    else
	myCounters = counter->myNext;
    delete counter;

    // Debug code to verify the counter was unlinked.
#ifdef STRESS_TEST
    for (last = myCounters; last; last = last->myNext)
    {
	UT_ASSERT(last != counter);
    }
#endif
}

INTRINSIC_COUNTER *
MOB::addCounter(MOB *inflictor, INTRINSIC_NAMES intrinsic, int turns)
{
    INTRINSIC_COUNTER	*counter;

    counter = new INTRINSIC_COUNTER;
    counter->myNext = myCounters;
    counter->myIntrinsic = intrinsic;
    counter->myTurns = turns;
    counter->myInflictor.setMob(inflictor);
    myCounters = counter;

    return counter;
}

void
MOB::loadGlobal(SRAMSTREAM &is)
{
    int		mob;
    int		val;

    for (mob = 0; mob < NUM_MOBS; mob++)
    {
	is.uread(val, 16);
	glbKillCount[mob] = val;
    }

    is.read(val, 8);
    ourAvatarDx = val;
    is.read(val, 8);
    ourAvatarDy = val;

    ai_load(is);
}

void
MOB::saveGlobal(SRAMSTREAM &os)
{
    int		mob;
    int		val;

    for (mob = 0; mob < NUM_MOBS; mob++)
    {
	val = glbKillCount[mob];
	os.write(val, 16);
    }

    os.write(ourAvatarDx, 8);
    os.write(ourAvatarDy, 8);

    ai_save(os);
}

MOB *
MOB::load(SRAMSTREAM &is)
{
    int		 val;
    MOB		*me;

    me = new MOB();
    
    is.uread(val, 8);
    me->myDefinition = val;

    is.uread(val, 8);
    me->myOrigDefinition = val;

    // Reset this to zero so uninitialized memory doesn't make
    // everyone loud.
    me->myNoiseLevel = 0;

    // Load our self-reference.
    me->myOwnRef.load(is);
    if (!me->myOwnRef.transferMOB(me))
    {
	UT_ASSERT(!"Transfer fail in Load");
    }

    me->myName.load(is);

    is.uread(val, 8);
    me->myX = val;
    is.uread(val, 8);
    me->myY = val;
    is.uread(val, 8);
    val ^= glb_mobdefs[me->myDefinition].explevel;
    me->myExpLevel = val;
    is.uread(val, 8);
    me->myDLevel = val;
    is.uread(val, 8);
    me->myAIFSM = val;
    is.uread(val, 16);
    me->myAIState = val;
    is.read(val, 16);
    me->myHP = val;
    is.read(val, 8);
    val ^= (glb_mobdefs[me->myDefinition].hitdie * 2);
    me->myHitDie = val;
    is.read(val, 16);
    me->myMaxHP = val;
    me->myHP ^= me->myMaxHP;
    is.read(val, 16);
    me->myMP = val;
    is.read(val, 8);
    val ^= (glb_mobdefs[me->myDefinition].mpdie * 2);
    me->myMagicDie = val;
    is.read(val, 16);
    me->myMaxMP = val;
    me->myMP ^= me->getMaxMP();
    is.read(val, 16);
    me->myExp = val;
    is.uread(val, 16);
    me->myFoodLevel = val;
    is.uread(val, 8);
    me->myGender = val;

    me->myAITarget.load(is);

    // Read in intrinsics...
    me->myIntrinsic.load(is);

    // Load counters...
    while (1)
    {
	int			 i;
	INTRINSIC_COUNTER	*counter;
	
	is.uread(val, 8);
	if (val)
	    break;
	
	is.uread(i, 8);
	is.uread(val, 32);
	// As it was set in our main block which loaded silently,
	// we shouldn't get the spurious Intrinsic messages here.
	
	// We do not want to call setTimedIntrinsic as that checks
	// against the main block.  Instead, as we know we have the
	// intrinsic already set in the main block, and we must add
	// the counter, we just call add.
	
	counter = me->addCounter(0, (INTRINSIC_NAMES) i, val);
	UT_ASSERT(counter != 0);
	if (counter)
	    counter->myInflictor.load(is);
    }

    // Read in items.
    ITEM		*item;
    while (1)
    {
	is.uread(val, 8);
	if (val)
	    break;
	item = ITEM::load(is);

	me->acquireItem(item, item->getX(), item->getY());
    }

    me->reverseInventory();

    // Load our poly base, if present:
    me->myBaseType.load(is);

    if (!me->myBaseType.isNull())
    {
	MOB		*mob;

	mob = MOB::load(is);
	// myBaseType will already point to mob, but might as well
	// assert.
	UT_ASSERT(mob == me->myBaseType.getMob());
    }

    return me;
}

void
MOB::save(SRAMSTREAM &os)
{
    os.write(myDefinition, 8);
    os.write(myOrigDefinition, 8);
    myOwnRef.save(os);
    myName.save(os);
    os.write(myX, 8);
    os.write(myY, 8);
    os.write(myExpLevel ^ glb_mobdefs[myDefinition].explevel, 8);
    os.write(myDLevel, 8);
    os.write(myAIFSM, 8);
    os.write(myAIState, 16);
    os.write(myHP ^ myMaxHP, 16);
    os.write(myHitDie ^ (glb_mobdefs[myDefinition].hitdie * 2), 8);
    os.write(myMaxHP, 16);
    os.write(myMP ^ (getMaxMP()), 16);
    os.write(myMagicDie ^ (glb_mobdefs[myDefinition].mpdie * 2), 8);
    os.write(myMaxMP, 16);
    os.write(myExp, 16);
    os.write(myFoodLevel, 16);
    os.write(myGender, 8);
    
    myAITarget.save(os);

    // Intrinsics:
    myIntrinsic.save(os);

    // Counters...
    INTRINSIC_COUNTER		*counter;

    for (counter = myCounters; counter; counter = counter->myNext)
    {
	// Flag for a counter...
	os.write(0, 8);
	os.write(counter->myIntrinsic, 8);
	os.write(counter->myTurns, 32);
	counter->myInflictor.save(os);
    }
    // End of intrinsic counters...
    os.write(1, 8);

    // Inventory:
    ITEM		*item;
    for (item = myInventory; item; item = item->getNext())
    {
	os.write(0, 8);
	item->save(os);
    }
    os.write(1, 8);

    // Save our poly base, if present:
    MOB		*mob;

    mob = myBaseType.getMob();
    myBaseType.save(os);
    if (mob)
	mob->save(os);
}

bool
MOB::verifyMob() const
{
    bool		valid = true;
    ITEM		*item;

    MOB			*self;

    self = myOwnRef.getMob();
    if (!self)
    {
	// Null self references...
	msg_report("0");
	valid = false;
    }
    else if (self != this)
    {
	// Invalid link.
	msg_report("!");
	valid = false;
    }

    // Verify all of our inventory.
    for (item = myInventory; item; item = item->getNext())
    {
	valid &= item->verifyMob();
    }

    if (getBaseType())
    {
	valid &= getBaseType()->verifyMob();
    }

    return valid;
}

bool
MOB::verifyCounterGone(INTRINSIC_COUNTER *counter) const
{
    bool		valid = true;
    ITEM		*item;
    INTRINSIC_COUNTER	*test;

    for (test = myCounters; test; test = test->myNext)
    {
	if (test == counter)
	{
	    UT_ASSERT(!"Counter still exists!");
	    valid = false;
	}
    }

    // Verify all of our inventory.
    for (item = myInventory; item; item = item->getNext())
    {
	valid &= item->verifyCounterGone(counter);
    }

    if (getBaseType())
    {
	valid &= getBaseType()->verifyCounterGone(counter);
    }
    return valid;
}

bool
MOB::hasBreathAttack() const
{
    if (getBreathAttack() != ATTACK_NONE)
	return true;

    return false;
}

ATTACK_NAMES
MOB::getBreathAttack() const
{
    return (ATTACK_NAMES) glb_mobdefs[myDefinition].breathattack;
}

int
MOB::getBreathRange() const
{
    ATTACK_NAMES		attack;
    
    attack = getBreathAttack();

    return (glb_attackdefs[attack].range);
}

bool
MOB::isBreathAttackCharged() const
{
    if (hasIntrinsic(INTRINSIC_TIRED))
	return false;

    return true;
}

const char *
MOB::getBreathSubstance() const
{
    return glb_mobdefs[myDefinition].breathsubstance;
}

bool	 
MOB::isTalkative() const
{
    if (getDefinition() == MOB_ELDER)
	return true;
    return false;
}

void
MOB::submergeItemEffects(SQUARE_NAMES tile)
{
    ITEMSLOT_NAMES	 slot;
    ITEM		*item;
    bool		 destroy = false;

    slot = (ITEMSLOT_NAMES) rand_choice(NUM_ITEMSLOTS);
    item = getEquippedItem(slot);
    if (item)
    {
	// Drown the item according to the liquid..
	glbCurLevel->fillBottle(item, getX(), getY());

	// Again find myself relazing on the shinkansen.  Truly
	// a more relaxed way to travel.  Spent most of the trip
	// reviewing for an upcoming meeting, so did not get my
	// usual POWDER development.
	// The new trains seem to have power outlets at every seat, however,
	// so no need to restrict yourself to micro laptops like I am.
	// But, cables?  Who wants to deal with them?
	switch (tile)
	{
	    case SQUARE_WATER:
		destroy = item->douse();
		break;
	    case SQUARE_LAVA:
		destroy = item->ignite();
		break;
	    case SQUARE_ACID:
		destroy = item->dissolve();
		break;
	    default:
		break;
	}
	if (destroy)
	{
	    item = dropItem(item->getX(), item->getY());
	    delete item;
	}

	// We may have just messed with our status..
	rebuildAppearance();
	rebuildWornIntrinsic();
    }
}

bool
MOB::zapCallbackStatic(int x, int y, bool final, void *data)
{
    MOB			*mob;
    MOB			*thismob;
    MOBREF		*mobref;
    bool		 keepgoing = true, lived = true;
    bool		 actuallyhit = false;
    bool		 forcehit = false;
    bool		 allowfinal = false;
    
    ATTACK_NAMES	 attack;

    mobref = (MOBREF *) data;
    thismob = mobref->getMob();

    // We need this to determine attack, Ie: digging eath elementals!
    mob = glbCurLevel->getMob(x, y);

    attack = ATTACK_NONE;
    switch (ourZapSpell)
    {
	case SPELL_DIG:
	    if (mob && mob->getDefinition() == MOB_EARTHELEMENTAL)
	    {
		attack = ATTACK_DIGEARTHELEMENTAL;
	    }
	    break;
	case SPELL_FORCEBOLT:
	    attack = ATTACK_SPELL_FORCEBOLT;
	    break;

	case SPELL_FORCEWALL:
	    attack = ATTACK_SPELL_FORCEWALL;
	    break;

	case SPELL_FROSTBOLT:
	    attack = ATTACK_SPELL_FROSTBOLT;
	    break;

	case SPELL_LIGHTNINGBOLT:
	    attack = ATTACK_SPELL_LIGHTNING;
	    break;

	case SPELL_MAGICMISSILE:   
	    attack = ATTACK_SPELL_MAGICMISSILE;
	    break;

	case SPELL_FIREBALL:
	    attack = ATTACK_SPELL_FIREBALL;
	    allowfinal = true;
	    break;

	case SPELL_CHAINLIGHTNING:
	    attack = ATTACK_SPELL_LIGHTNING;
	    break;

	case SPELL_POISONBOLT:
	    attack = ATTACK_SPELL_POISONBOLT;
	    break;

	case SPELL_CLOUDKILL:
	    attack = ATTACK_SPELL_CLOUDKILL;
	    allowfinal = true;
	    break;

	case SPELL_FINGEROFDEATH:
	    attack = ATTACK_SPELL_FINGEROFDEATH;
	    if (thismob && thismob->hasIntrinsic(INTRINSIC_DRESSED_NECROMANCER))
	    {
		forcehit = true;
	    }
	    break;

	default:
	    attack = ATTACK_NONE;
	    break;
    }

    // Do nothing on final attacks for these modes.
    if (!allowfinal && final)
	return false;

    // Final attakcs always trigger this path.
    // Beware, mob may be null!
    if (mob || final)
    {
	if (attack != ATTACK_NONE)
	{
	    // Check to see if we die...
	    lived = true;
	    actuallyhit = true;
	    if (forcehit && mob && !final)
	    {
		lived = mob->receiveDamage(attack, thismob, 
					0, 0, ATTACKSTYLE_SPELL);
		actuallyhit = true;
	    }
	    else if (mob && !final)
	    {
		lived = mob->receiveAttack(attack, thismob, 
					0, 0, ATTACKSTYLE_SPELL,
					&actuallyhit);
	    }

	    if (!lived && mob == thismob)
		return false;

	    if (actuallyhit)
	    {
		// Magic missile can hit one foe per fireball count.
		if (ourZapSpell == SPELL_MAGICMISSILE)
		{
		    if (ourFireBallCount > 0)
			ourFireBallCount--;
		    else
			keepgoing = false;
		}

		if (ourZapSpell == SPELL_POISONBOLT)
		    keepgoing = false;

		if (ourZapSpell == SPELL_CLOUDKILL)
		{
		    if (final)
		    {
			// Generate our smoke
			// TODO: This is improper conjugation!
			thismob->formatAndReport("%B1 explodes!", glb_attackdefs[attack].explode_name);

			// Create the relevant smoke.
			if (glb_attackdefs[attack].explode_smoke != SMOKE_NONE)
			{
			    int		dx, dy;
			    SMOKE_NAMES smoke = (SMOKE_NAMES) 
						    glb_attackdefs[attack].explode_smoke;

			    for (dy = -1; dy <= 1; dy++)
				for (dx = -1; dx <= 1; dx++)
				    if (glbCurLevel->canMove(x+dx, y+dy, 
							    MOVE_STD_FLY))
					glbCurLevel->setSmoke(x+dx, y+dy,
							    smoke, thismob);
			}
		    }
		    keepgoing = false;
		}

		if (ourZapSpell == SPELL_FIREBALL &&
		    (rand_choice(ourFireBallCount) < 2))
		{
		    keepgoing = false;

		    ourFireBallCount++;
		    
		    // Time for an inferno!
		    {
			BUF		buf;
			buf.sprintf("%s fireball explodes!",
			    (thismob ? thismob->getPossessive() : "the"));
			glbCurLevel->reportMessage(buf, x, y);
		    }

		    ourEffectAttack = ATTACK_SPELL_FIREBALLBALL;
		    glbCurLevel->fireBall(x, y, 1, true,
					    areaAttackCBStatic,
					    mobref);
		}

		// Check for chain lightning.  Note that chain lightning
		// will keep going.
		if (ourZapSpell == SPELL_CHAINLIGHTNING &&
		    (rand_choice(ourFireBallCount) < 5))
		{
		    ourFireBallCount++;

		    {
			BUF		buf;
			buf.sprintf("%s lightning forks!",
				(thismob ? thismob->getPossessive() : "the"));
			glbCurLevel->reportMessage(buf, x, y);
		    }

		    int		dx, dy;

		    rand_direction(dx, dy);

		    glbCurLevel->fireRay(x, y,
					 dx, dy,
					 6,
					 MOVE_STD_FLY, true,
					 zapCallbackStatic,
					 mobref);
		}
	    }
	}
    }

    // Local effects:
    if (ourZapSpell == SPELL_FROSTBOLT)
    {
	if (glbCurLevel->freezeSquare(x, y, thismob))
	    return false;
    }

    if (ourZapSpell == SPELL_FIREBALL)
    {
	glbCurLevel->burnSquare(x, y, thismob);
	// Yes, continue even if melted!
    }

    if (ourZapSpell == SPELL_DIG)
    {
	if (!glbCurLevel->digSquare(x, y, thismob))
	    return false;
    }

    // This dubious decision to omit the spark spell from this effect
    // was made whilst flying in economy over the pacific.  Much
    // thanks to my p1610 which is more than up to the task of working
    // in such confined areas, and thanks to AC for outfitting their 777s
    // with seatback power so I have no fear of even this eleven
    // hour flight depleting my battery.
    // I still think I prefer first class, however.
    // The dubious decision, it turns out, was of no merit as spark doesn't
    // use this code path and was added when I provided support for
    // blue dragons charging rapiers.  Please thus ignore that above
    // comment which no longer applies.  Well, the part about first
    // class certainly applies.
    if (ourZapSpell == SPELL_LIGHTNINGBOLT || ourZapSpell == SPELL_CHAINLIGHTNING)
    {
	// Charge anything on this square...
	glbCurLevel->electrifySquare(x, y, 
			     rand_dice(glb_attackdefs[attack].damage),
			     thismob);
    }

    if (ourZapSpell == SPELL_FORCEBOLT || ourZapSpell == SPELL_FORCEWALL)
    {
	// Force bolts will:
	// Shatter doors
	// Shatter secret doors
	// Shatter boulders.
	// Shatters trees.
	// Shatter viewports
	SQUARE_NAMES		tile;
	BUF			buf;

	tile = glbCurLevel->getTile(x, y);
	switch (tile)
	{
	    case SQUARE_DOOR:
	    case SQUARE_BLOCKEDDOOR:
	    case SQUARE_SECRETDOOR:
	    {
		buf.sprintf("The force bolt shatters the %s.",
			    (tile == SQUARE_SECRETDOOR ? "secret door" : "door"));
		glbCurLevel->reportMessage(buf, x, y);

		glbCurLevel->setTile(x, y, SQUARE_CORRIDOR);

		// Ends the bolt
		return false;
	    }
	    case SQUARE_VIEWPORT:
	    {
		glbCurLevel->reportMessage("The force bolt shatters the glass wall.", x, y);

		glbCurLevel->setTile(x, y, SQUARE_BROKENVIEWPORT);

		// Ends the bolt
		return false;
	    }

	    case SQUARE_FOREST:
	    case SQUARE_FORESTFIRE:
	    {
		buf.sprintf("The force bolt shatters the %s.",
			    (tile == SQUARE_FOREST ? "tree" : "burning tree"));
		glbCurLevel->reportMessage(buf, x, y);
		glbCurLevel->setTile(x, y, SQUARE_CORRIDOR);
		// People should fall out of trees.
		glbCurLevel->dropMobs(x, y, true, mobref->getMob(), true);

		int		dx, dy;
		FORALL_8DIR(dx, dy)
		{
		    ITEM	*arrow;

		    if (tile == SQUARE_FORESTFIRE)
			arrow = ITEM::create(ITEM_FIREARROW);
		    else
			arrow = ITEM::create(ITEM_ARROW);
		    // We only want one arrow at a time.
		    arrow->setStackCount(1);
		    // Throw the arrow...
		    glbCurLevel->throwItem(x, y, dx, dy,
				arrow, 0, 0, mobref->getMob(),
				arrow->getThrownAttack(),
				false);
		}
		
		// Consumes the bolt.
		return false;
	    }

	    default:
		break;
	}

	ITEM		*item;

	item = glbCurLevel->getItem(x, y);
	if (item && item->getDefinition() == ITEM_BOULDER)
	{
	    ITEM	*rocks;

	    item->formatAndReport("The force bolt shatters %U.");
	    // Destroy the item.
	    glbCurLevel->dropItem(item);
	    delete item;

	    // Create some rocks.
	    rocks = ITEM::create(ITEM_ROCK, false, false);
	    rocks->setStackCount(rand_range(3,8));
	    glbCurLevel->acquireItem(rocks, x, y, thismob);
	    
	    return false;
	}

	if (item && item->getDefinition() == ITEM_STATUE)
	{
	    MOB		*victim;
	    ITEM	*rocks;
	    
	    item->formatAndReport("The force bolt shatters %U.");

	    // Create some rocks.
	    rocks = ITEM::create(ITEM_ROCK, false, false);
	    rocks->setStackCount(rand_range(2,4));
	    glbCurLevel->acquireItem(rocks, x, y, thismob);
	    
	    victim = item->getStatueMob();

	    if (victim)
	    {
		// The victim finally dies.
		victim->triggerAsDeath(ATTACK_NONE, thismob, 0, false);

		// You get the experience for killing this dude!
		if (!thismob->hasCommonMaster(victim))
		    thismob->receiveDeathExp(victim->getExpLevel());

		// Good old piety.
		thismob->pietyKill(victim, ATTACKSTYLE_SPELL);

		// Drop the victim's stuff on the ground.
		ITEM		*vit, *next, *drop;

		for (vit = victim->myInventory; vit; vit = next)
		{
		    next = vit->getNext();

		    drop = victim->dropItem(vit->getX(), vit->getY());
		    UT_ASSERT(drop == vit);
		    
		    glbCurLevel->acquireItem(vit, x, y, thismob);
		}

		// Note victim will be deleted when the item is.
	    }

	    // Delete the item (and any mob inside of it)
	    glbCurLevel->dropItem(item);
	    delete item;
	}
    }

    // Continue the zap...
    return keepgoing;
}

bool
MOB::attemptLifeSaving(const char *deathstring)
{
    if (hasIntrinsic(INTRINSIC_LIFESAVE))
    {
	if (getMobType() == MOBTYPE_UNDEAD)
	{
	    // Life saving doesn't work on the dead.  They have
	    // no life to save...
	    formatAndReport("%U <realize> too late that being undead means having no life to save.");
	    return false;
	}
	
	// This is either a built in intrinsic (in which case
	// we now lose it) or its from an item (in which case
	// it will glow and disappear)
	if (myIntrinsic.hasIntrinsic(INTRINSIC_LIFESAVE))
	{
	    // It is a built in intrinsic.
	    formatAndReport(deathstring);
	    clearIntrinsic(INTRINSIC_LIFESAVE);
	    formatAndReport("But %U <return> to life!");
	    myHP = myMaxHP;

	    clearDeathIntrinsics(false);

	    return true;
	}
	
	ITEM		*lifesaver, *drop;

	lifesaver = getSourceOfIntrinsic(INTRINSIC_LIFESAVE);
	
	if (lifesaver)
	{
	    // The life saver is what saved our life!
	    formatAndReport(deathstring);

	    formatAndReport("%R %Iu <I:glow> brightly and crumbles to dust!",
			    lifesaver);

	    // If the heart grants life save it could get rather
	    // embarassing.  Since we can't destroy the heart, we need
	    // to not rescue the carrier.
	    if (lifesaver->defn().isquest)
	    {
		formatAndReport("The dust reassembles itself into %Iu!", 
				lifesaver);
		formatAndReport("%U <do> not return to life.");
		return false;
	    }
	    formatAndReport("%U <return> to life!");
	    
	    // If it was our amulet, or we saw the creature regenerate,
	    // we will get the id of the item.
	    if (isAvatar() ||
		(MOB::getAvatar() && MOB::getAvatar()->canSense(this)))
	    {
		// We only id type, not the particulars!
		lifesaver->markClassKnown();
	    }
    
	    drop = dropItem(lifesaver->getX(), lifesaver->getY());
	    UT_ASSERT(drop == lifesaver);
	    delete lifesaver;
	    setMaximalHP();
	    clearDeathIntrinsics(false);

	    return true;
	}
	// This might occur if rebuildWornIntrinsic
	// doesn't match getSourceOfIntrinsic
	UT_ASSERT(!"UNKNOWN SOURCE OF LIFE SAVING");
	// Cute message for this impossible condition.
	formatAndReport("%U <attempt> to cheat death - but <fail>!");
    }
    else if (hasIntrinsic(INTRINSIC_LICHFORM))
    {
	if (getMobType() == MOBTYPE_UNDEAD)
	{
	    // The undead can't make use of the imbued evil.
	    formatAndReport("%U <feel> %r concentrated evil dissipate uselessly.");
	    return false;
	}

	// Transformation forbidden if unchanging.
	if (hasIntrinsic(INTRINSIC_UNCHANGING))
	{
	    formatAndReport("%U <shudder> as the evil dissipates.");
	    return false;
	}

	// This is either a built in intrinsic (in which case
	// we now lose it) or its from an item (in which case
	// it will glow and disappear)
	if (myIntrinsic.hasIntrinsic(INTRINSIC_LICHFORM))
	{
	    // It is a built in intrinsic.
	    formatAndReport(deathstring);
	    clearIntrinsic(INTRINSIC_LICHFORM);
	    // "The r in intervene is in memory of Zamkral the giant spider,
	    //  who reincarnated himself as a lich when he was killed on
	    //  floor 7 of the dungeon." - Oohara Yuuma
	    formatAndReport("But evil forces intervene!  %U <transform> into a lich!");

	    // Set your id to Lich.
	    myDefinition = MOB_LICH;
	    
	    // Reacquire full HP
	    setMaximalHP();

	    // Gain all of the Lich intrinsics...
	    mergeIntrinsicsFromString(glb_mobdefs[MOB_LICH].intrinsic);

	    clearDeathIntrinsics(false);

	    // We become more powerful!
	    gainLevel();

	    return true;
	}
	
	// For now, we disallow items that grant lichform.  This is supposed
	// to require a proper suicide, after all.
	
	UT_ASSERT(!"UNKNOWN SOURCE OF LICH FORM");
	// Cute message for this impossible condition.
	formatAndReport("%U <attempt> to transform into a lich - but <fail>!");
    }

    return false;
}

void
MOB::triggerAsDeath(ATTACK_NAMES attack, MOB *src, ITEM *weapon, bool becameitem)
{
    triggerAsDeath(&glb_attackdefs[attack], src, weapon, becameitem);
}

void
MOB::triggerAsDeath(const ATTACK_DEF *attack, MOB *src, ITEM *weapon,
		    bool becameitem)
{
    // If a familiar dies, the master is punished.
    // This only occurs when the creature fully decays, so if it is
    // just transformed into an item. skip.
    if (hasIntrinsic(INTRINSIC_FAMILIAR) &&
	!becameitem)
    {
	MOB		*master;
	master = getMaster();
	if (master)
	{
	    master->systemshock();

	    master->formatAndReport("%U <feel> a great loss.");
	}
    }
    
    // Increment the killcount.
    // We do not want to do this if a it was a turn to stone.
    // Except....  We do could kill/res loops, so we should count
    // stone/unstone loops here.
    // No credit to you for that!
    // Also no credit for killing yourself.
    if (!isAvatar())
	if (glbKillCount[getDefinition()] < 65530)
	    glbKillCount[getDefinition()]++;
    
    if (isAvatar())
    {
	// Losing screen.
	if (!glbStressTest)
	{
	    glbVictoryScreen(false, attack, src, weapon);
	}

	// Mark avatar dead.
	setAvatar(0);
    }

    // Check if it is Baezlebub!  Hurrah, good on you, warrior of
    // light!
    if (myDefinition == MOB_BAEZLBUB)
    {
	msg_report("Baezl'bub has died!  Bring his black heart to "
		    "the surface world!  ");
	// Now you do not get victory until retiring to the surface
	// world.
	//glbVictoryScreen(true, attack, src, weapon);
    }
}
