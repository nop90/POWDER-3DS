/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        piety.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This implements all the piety handling code.
 */

#include "glbdef.h"
#include "assert.h"
#include <stdio.h>
#include "creature.h"
#include "piety.h"
#include "sramstream.h"
#include "item.h"
#include "itemstack.h"
#include "msg.h"
#include "map.h"
#include "encyc_support.h"

//
// This stores our total piety & total action points.
//
s16 glbPiety[NUM_GODS];
s16 glbGodPoints[NUM_GODS];
GOD_NAMES	glbChosenGod = GOD_AGNOSTIC;
GOD_NAMES	glbWhimOfXOM;
u8 glbWhimOfXOMTimer;
u8 glbFlameStrikeTimer;
GOD_NAMES glbFlameGod;

//
// Global Piety Functions
//
GOD_NAMES
piety_randomnonxomgod()
{
    int		god;

    while (1)
    {
	god = rand_choice(NUM_GODS);
	if (god == GOD_AGNOSTIC || god == GOD_CULTIST)
	    continue;
	break;
    }

    return (GOD_NAMES) god;
}

void
piety_init()
{
    GOD_NAMES	god;

    FOREACH_GOD(god)
    {
	glbPiety[god] = 0;
	glbGodPoints[god] = 0;
    }

    // We do not set our god here as it was set in the god selection
    // stage
    // glbChosenGod = GOD_AGNOSTIC;
    glbWhimOfXOM = piety_randomnonxomgod();
    glbWhimOfXOMTimer = rand_range(100, 200);
    glbFlameStrikeTimer = 0;
    glbFlameGod = GOD_AGNOSTIC;
}

GOD_NAMES
piety_chosengod()
{
    return glbChosenGod;
}

int
piety_chosengodspiety()
{
    GOD_NAMES	god = piety_chosengod();

    if (god == GOD_AGNOSTIC)
	return 0;

    return glbPiety[god];
}

void
piety_setgod(GOD_NAMES god)
{
    glbChosenGod = god;
}

void
piety_load(SRAMSTREAM &is)
{
    int		val;
    GOD_NAMES	god;

    FOREACH_GOD(god)
    {
	is.read(val, 16);
	glbPiety[god] = val;
	is.read(val, 16);
	glbGodPoints[god] = val;
    }

    is.read(val, 8);
    glbChosenGod = (GOD_NAMES) val;

    is.read(val, 8);
    glbWhimOfXOM = (GOD_NAMES) val;
    
    is.read(val, 8);
    glbWhimOfXOMTimer = val;

    is.read(val, 8);
    glbFlameStrikeTimer = val;
    is.read(val, 8);
    glbFlameGod = (GOD_NAMES) val;
}

void
piety_save(SRAMSTREAM &os)
{
    GOD_NAMES	god;

    FOREACH_GOD(god)
    {
	os.write(glbPiety[god], 16);
	os.write(glbGodPoints[god], 16);
    }

    os.write(glbChosenGod, 8);
    os.write(glbWhimOfXOM, 8);
    os.write(glbWhimOfXOMTimer, 8);
    os.write(glbFlameStrikeTimer, 8);
    os.write(glbFlameGod, 8);
}

//
// MOB::piety Functions
//
void
MOB::pietyKill(MOB *mob, ATTACKSTYLE_NAMES attackstyle)
{
    int		val;

    // Check to see if we are a slave.  If so, grant the piety
    // to our master as well.
    MOB		*master;
    master = getMaster();
    if (master)
    {
	master->pietyKill(mob, ATTACKSTYLE_MINION);
    }

    val = mob->getExpLevel();

    if (val < getExpLevel() - 5)
	val = 1;
    else if (val < getExpLevel() + 2)
	val = 2;
    else
	val = 3;

    if (attackstyle != ATTACKSTYLE_SPELL)
        pietyGrant(GOD_FIGHTER, val * 2);
    
    if (attackstyle == ATTACKSTYLE_MELEE)
        pietyGrant(GOD_BARB, val);
    else
	pietyGrant(GOD_NECRO, val);

    if (attackstyle == ATTACKSTYLE_SPELL)
	pietyGrant(GOD_WIZARD, val);

    if (attackstyle == ATTACKSTYLE_THROWN)
	pietyGrant(GOD_ROGUE, val);
    
    if (mob->defn().mobtype == MOBTYPE_UNDEAD)
    {
	pietyGrant(GOD_CLERIC, val);
    }
}

void
MOB::pietyEat(ITEM *item, int foodval)
{
    if (item->getCorpseMob())
    {
	// Fallen foe....
	// Only counts if you aren't hungry.
	if (getHungerLevel() > HUNGER_HUNGRY)
	    pietyGrant(GOD_CLERIC, -2);
    }
}

void
MOB::pietyDress(INTRINSIC_NAMES intrinsic)
{
    // Only one in 5 chance of actually responding to this state.
    if (rand_choice(5))
	return;
    
    if (intrinsic == INTRINSIC_DRESSED_WIZARD)
	pietyGrant(GOD_WIZARD, 1);
    if (intrinsic == INTRINSIC_DRESSED_FIGHTER)
	pietyGrant(GOD_FIGHTER, 1);
    if (intrinsic == INTRINSIC_DRESSED_RANGER)
	pietyGrant(GOD_ROGUE, 1);
    if (intrinsic == INTRINSIC_DRESSED_CLERIC)
    {
	// Pax isn't too impressed by all this..
	if (rand_choice(8))
	    return;
	pietyGrant(GOD_CLERIC, 1);
    }
    if (intrinsic == INTRINSIC_DRESSED_NECROMANCER)
    {
	// Necros should not be about piety.
	if (rand_choice(4))
	    return;
	pietyGrant(GOD_NECRO, 1);
    }
    if (intrinsic == INTRINSIC_DRESSED_BARBARIAN)
    {
	// H'ruth isn't into fashion.
	if (rand_choice(8))
	    return;
	pietyGrant(GOD_BARB, 1);
    }
}

void
MOB::pietyZapWand(ITEM *wand)
{
    pietyGrant(GOD_ROGUE, 2);
    pietyGrant(GOD_WIZARD, 1);
    pietyGrant(GOD_BARB, -1);
}

void
MOB::pietySurrounded(int numenemy)
{
    // Barbarian bonus:
    if (numenemy > 1)
    {
	// TODO: This would be a good time to grant BERSERK.
	pietyGrant(GOD_BARB, numenemy * 2);

	// Wizards do not like to be surrounded.
	pietyGrant(GOD_WIZARD, -1);
    }
}

void
MOB::pietyHeal(int amount, bool self, bool enemy, bool vampiric)
{
    // Necros are rewarded for healing enemies, as it is assumed
    // one merely wants them to suffer longer.
    // Otherwise, it isn't something one should be doing.
    if (enemy || vampiric)
	pietyGrant(GOD_NECRO, 1);
    else
	pietyGrant(GOD_NECRO, -5);

    // Clerics are of two minds of enemy healing.  It is a no-op
    // as otherwise it would encourage uncleric like behaviour.
    // We get massive bonuses for healing our pets.
    if (!enemy)
    {
	// vampiric styles of healing are not appreciated.
	if (vampiric)
	    pietyGrant(GOD_CLERIC, -1);
	else
	    pietyGrant(GOD_CLERIC, self ? 1 : 3);
    }
}

void
MOB::pietyCastSpell(SPELL_NAMES spell)
{
    int		val;

    val = glb_spelldefs[spell].mpcost +
	  (glb_spelldefs[spell].hpcost/2) +
	  (glb_spelldefs[spell].xpcost/10);

    // An additional point is given for every 5 vals.
    val /= 5;
    if (val <= 0) val = 1;

    pietyGrant(GOD_WIZARD, val);

    if (glb_spelldefs[spell].type == SPELLTYPE_DEATH)
    {
	pietyGrant(GOD_NECRO, (val + 1) / 2);
	pietyGrant(GOD_CLERIC, -val, true);
    }
    if (glb_spelldefs[spell].type == SPELLTYPE_HEAL)
    {
	pietyGrant(GOD_NECRO, -val, true);
	pietyGrant(GOD_CLERIC, (val + 1) / 2);
    }

    // Forbidden for barbarians!
    pietyGrant(GOD_BARB, -val, true);
}

void
MOB::pietyCreateTrap()
{
    pietyGrant(GOD_ROGUE, 5);
}

void
MOB::pietyFindSecret()
{
    pietyGrant(GOD_ROGUE, 5);
}

void
MOB::pietyAttack(MOB *mob, const ATTACK_DEF *attack, ATTACKSTYLE_NAMES style)
{
    // Determine if it is a friendly attack.
    ATTITUDE_NAMES		attitude;

    attitude = mob->getAttitude(this);

    // Check for attacking friendly or what not.
    if (attitude == ATTITUDE_FRIENDLY)
    {
	// Killing friendly undead is an act of mercy.
	if (mob->defn().mobtype != MOBTYPE_UNDEAD)
	    pietyGrant(GOD_CLERIC, -5);
    }
    if (attitude == ATTITUDE_NEUTRAL)
    {
	// It is not *that* bad to start combat.
	// However, if the target is undead, Pax has no issues.
	if (mob->defn().mobtype != MOBTYPE_UNDEAD)
	    pietyGrant(GOD_CLERIC, -1);
    }

    if ( ( mob->hasIntrinsic(INTRINSIC_PARALYSED) &&
	  !mob->hasIntrinsic(INTRINSIC_FREEDOM)) ||
	 (mob->hasIntrinsic(INTRINSIC_ASLEEP)) )   
    {
	// Strike helpless.
	pietyGrant(GOD_ROGUE, 1);
	pietyGrant(GOD_NECRO, 2);

	// The exception is undead.  You are free to
	// engage them regardless of their status.
	if (mob->defn().mobtype != MOBTYPE_UNDEAD)
	{
	    pietyGrant(GOD_CLERIC, -2);
	}
    }

    if (style == ATTACKSTYLE_THROWN)
    {
	// Ranged attack.
	pietyGrant(GOD_ROGUE, 1);
	pietyGrant(GOD_BARB, -2);
    }

    if (style == ATTACKSTYLE_MELEE)
    {
	// Melee attack
	if (!rand_choice(5))
	{
	    pietyGrant(GOD_WIZARD, -1);
	    pietyGrant(GOD_NECRO, -1);
	}
    }
}

void
MOB::pietyMakeNoise(int noiselevel)
{
    if (noiselevel > 2)
    {
	if (!rand_choice(30))
	    pietyGrant(GOD_ROGUE, -1 * (noiselevel - 2));
    }
}

void
MOB::pietyIdentify(ITEM_NAMES /*itemdef*/)
{
    pietyGrant(GOD_ROGUE, 5);
    pietyGrant(GOD_WIZARD, 3);
}

void
MOB::pietyBreak(ITEM *item)
{
    if (glbWhimOfXOM == GOD_CLERIC)
    {
	// The hint makes it seem like that breaking things abuses
	// ><0|V|  The wiki picked up on this and I'd hate for
	// it to be incorrect so I've retconned this pathway
	pietyGrant(GOD_CULTIST, -2);
    }
}

void
MOB::pietyGrant(GOD_NAMES god, int amount, bool forbidden)
{
    // Only record piety for the avatar.
    if (!isAvatar())
	return;

    // If this god is CULTIST, we need to see what the current
    // whim of ><0|V| is, and act accordingly.
    if (god == glbWhimOfXOM)
    {
	pietyGrant(GOD_CULTIST, amount, forbidden);
    }
    
    // If the action is forbidden, we want to zero out
    // our piety in one go.
    if (forbidden && glbPiety[god] > 0)
    {
	// You will likely invoke the wrath of the god.  If it is
	// your god, you will wipe your xp.
	pietyTransgress(god);
	glbPiety[god] += amount*10;
    }

    // If the piety is negative, and it is your chosen god,
    // it is twice as bad.
    if (god == glbChosenGod && amount < 0)
	amount *= 2;

    // DIminish returns on positive piety gain to prevent people 
    // stacking up 10k piety for constant healing
    if (amount > 0)
    {
	if (glbPiety[god] > 200)
	{
	    // Lose 1/16th chance for every 100 piety above this
	    // up to 15 * 100 + 200 = 1700..
	    int		goal = (glbPiety[god]-200)/100;
	    if (goal >= 15)
		goal = 14;
	    if (rand_choice(16) <= goal)
		amount = 0;

	    // Double jeopardy for every 1000 there on (Caps at 10k)
	    if (glbPiety[god] > 2000)
	    {
		if (rand_choice(16) < (glbPiety[god]-2000)/1000)
		{
		    amount = 0;
		}
	    }
	}
    }

    // Adjust our piety & god points.
    if (glbPiety[god] * amount > 0)
    {
	// Both piety & amount have same sign, increment god points.
	if (amount > 0)
	    glbGodPoints[god] += amount;
	else
	    glbGodPoints[god] -= amount;
    }
    glbPiety[god] += amount;

    // Clamp piety to reasonable limits to avoid overflow/underflow
    if (glbPiety[god] > 10000)
	glbPiety[god] = 10000;
    if (glbPiety[god] < -10000)
	glbPiety[god] = -10000;

    // Reduce god points to not become larger than piety.
    if (abs(glbPiety[god]) < glbGodPoints[god])
	glbGodPoints[god] = abs(glbPiety[god]);

#if 0
    BUF		buf;

    buf.sprintf("%s: %d:%d.  ",
		glb_goddefs[god].name,
		glbPiety[god],
		amount);
    msg_report(buf);
#endif
}

void
MOB::pietyTransgress(GOD_NAMES god)
{
    // The user did something forbidden to this god.
    // The penalty is determined by:
    // 1) If you are currently following this god, wipe experience,
    //    report transgression, and dump all god piety into punishment.
    // 2) If you are not, check to see if the god points are more than
    //    your current god's piety.  If so, punish.  Otherwise it is
    //    assumed that your own god stood in the way.
    //
    //    We wipe out our god points, but don't wipe out our piety. 
    BUF		buf;

    // First, the god is given as many points as he wants to play with!
    glbGodPoints[god] = glbPiety[god];

    if (god == glbChosenGod)
    {
	const char *abusemsg[] =
	{
	    "Your Actions Defile Me!",
	    "Do you value me so little?",
	    "Respect My Commands!"
	};
		
	buf.sprintf("%s: %s  ", glb_goddefs[god].name,
				abusemsg[rand_choice(3)]);
	msg_announce(buf);
	wipeExp();
	pietyPunish(god);
    }
    else
    {
	if (glbChosenGod == GOD_AGNOSTIC)
	{
	    const char *agnosticsavemsg[] =
	    {
		"You sense celestial displeasure.  ",
		"You wonder if there are gods after all?  ",
		"A painful hangnail briefly distracts you.  "
	    };

	    msg_announce(agnosticsavemsg[rand_choice(3)]); 

	    // Your powers of disbelief succeed.
	    glbGodPoints[god] = 0;
	}
	else if (glbGodPoints[god] > glbPiety[glbChosenGod])
	{
	    // Your current god cannot come to your rescue.
	    const char *abusemsg[] =
	    {
		"%s: Do not look to %s to protect you!  ",
		"%s: You should obey Me!  ",
		"%s: You cannot hide from Me!  "
	    };
	    buf.sprintf(abusemsg[rand_choice(3)],
			glb_goddefs[god].name,
			glb_goddefs[glbChosenGod].name);
	    msg_announce(buf);
	    pietyPunish(god);
	}
	else
	{
	    // Let the player know a divine struggle just occurred.
	    const char *godsavemsg[] =
	    {
		"%s: %s shall not meddle with my disciple!  ",
		"%s: Fear not %s while you follow me!  ",
		"%s: %s!  Leave my people alone!  "
	    };
	    buf.sprintf(godsavemsg[rand_choice(3)],
			glb_goddefs[glbChosenGod].name,
			glb_goddefs[god].name);
	    msg_announce(buf);
	    // This will cost you piety with your god!
	    glbPiety[glbChosenGod] -= glbGodPoints[god];
	    // We may have moved our piety to 0.  If we leave
	    // our god points, this could result in us being
	    // punished!
	    // Thus, we ensure god points are less than piety.
	    if (glbGodPoints[glbChosenGod] > abs(glbPiety[glbChosenGod]))
		glbGodPoints[glbChosenGod] = abs(glbPiety[glbChosenGod]);
	    // They all get nullified against your chosen god.
	    glbGodPoints[god] = 0;
	}
    }
    
    glbPiety[god] = glbGodPoints[god];
    glbGodPoints[god] = 0;
}

void
MOB::pietyPunish(GOD_NAMES god)
{
    // Choose a punishment that fits the god and fits the available points.
    PUNISH_NAMES	valid[NUM_PUNISHS];
    int			numvalid = 0;
    int			points;
    PUNISH_NAMES	punish;
    BUF			buf;

    points = glbGodPoints[god];
    
    FOREACH_PUNISH(punish)
    {
	if (glb_punishdefs[punish].points <= points)
	{
	    if (strchr(glb_punishdefs[punish].god, (char) god))
	    {
		// This god is willing to do this punishment.
		valid[numvalid++] = punish;
	    }
	}
    }

    // If there is no valid punishments, quit.
    if (!numvalid)
	return;

    punish = valid[rand_choice(numvalid)];

    // Subtract the points & apply the punishment.
    glbGodPoints[god] -= glb_punishdefs[punish].points;
    
    switch (punish)
    {
	case PUNISH_FLAMESTRIKE:
	{
	    formatAndReport("%U <smell> sulphur.");
	    glbFlameStrikeTimer = rand_range(3, 6);
	    glbFlameGod = god;
	    break;
	}

	case PUNISH_SUMMON:
	{
	    buf.sprintf("%s: Prove your worth!  ",
			 glb_goddefs[god].name);
	    msg_announce(buf);

	    MOB		*mob;
	    int		 dx, dy;

	    for (dy = -1; dy <= 1; dy++)
	    {
		for (dx = -1; dx <= 1; dx++)
		{
		    if (glbCurLevel->getMob(getX()+dx,
					    getY()+dy))
			continue;

		    mob = MOB::createNPC(getExpLevel()+3);
		    if (!mob->canMove(getX() + dx,
				      getY() + dy,
				      true))
		    {
			delete mob;
			continue;
		    }

		    mob->move(getX() + dx,
			      getY() + dy,
			      true);
		    glbCurLevel->registerMob(mob);

		    mob->formatAndReport("%U <appear> out of thin air!");

		    // Ensure we want to kill the target, which is this.
		    mob->setAITarget(this);
		}
	    }

	    break;
	}

	case PUNISH_CURSE_WORN:
	{
	    buf.sprintf("%s: To the pits with you!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    // Curse only equpped items.
	    receiveCurse(true, false);
	    break;
	}
	
	case PUNISH_CURSE_ANY:
	{
	    buf.sprintf("%s: A pox on you!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    // Curse all items with equal chance.
	    receiveCurse(false, true);
	    break;
	}

	case PUNISH_DISENCHANT_WEAPON:
	{
	    buf.sprintf("%s: You are an inferior tool!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    // gods do not invoke shuddering.
	    receiveWeaponEnchant(-1, false);
	    break;
	}

	case PUNISH_DISENCHANT_ARMOUR:
	{
	    buf.sprintf("%s: You deserve no protection!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    // gods do not invoke shuddering.
	    receiveArmourEnchant((ITEMSLOT_NAMES)-1, -1, false);
	    break;
	}

	case PUNISH_POISON:
	{
	    buf.sprintf("%s: Drink the poison you have brewed!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    setTimedIntrinsic(0, INTRINSIC_POISON_STRONG, 20);

	    break;
	}

	case PUNISH_PARALYSE:
	{
	    buf.sprintf("%s: Freeze with Fright at my Wrath!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    setTimedIntrinsic(0, INTRINSIC_PARALYSED, 5);
	    break;
	}

	case PUNISH_SLEEP:
	{
	    buf.sprintf("%s: Dream in Terror!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    setTimedIntrinsic(0, INTRINSIC_ASLEEP, 30);
	    break;
	}

	case PUNISH_POLY:
	{
	    MOB_NAMES		mob;
	    
	    // Determine a suitable pathetic beast.
	    MOB_NAMES		targets[3] =
			{
			    MOB_MOUSE,
			    MOB_BROWNSLUG,
			    MOB_GRIDBUG
			};

	    if (god == GOD_NECRO)
		mob = MOB_SKELETON;
	    else
		mob = targets[rand_choice(3)];

	    buf.sprintf("%s: Learn Humility!  ",
			 glb_goddefs[god].name);
	    msg_announce(buf);

	    // And poly the loser into it!
	    actionPolymorph(false, true, mob);
	    break;
	}

	case PUNISH_MANADRAIN:
	{
	    buf.sprintf("%s: Your Magic Will Not Avail You!  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);

	    // drain magic.
	    // 50 + 5d10 turns.
	    setTimedIntrinsic(0, INTRINSIC_MAGICDRAIN,
			   rand_dice(5, 10, 50));
	    break;
	}

	case PUNISH_POLYWEAPON:
	{
	    ITEM		*weapon;

	    weapon = getEquippedItem(ITEMSLOT_RHAND);
	    if (!weapon)
	    {
		// Should not occur?
		buf.sprintf("%s: I have caught you empty handed!  Ha!  ",
			glb_goddefs[god].name);
		msg_announce(buf);
		break;
	    }

	    BUF weapname = weapon->getName();
	    buf.sprintf("%s: Your over use of %s bores me!  ",
			glb_goddefs[god].name,
			weapname.buffer());
	    msg_announce(buf);

	    // polymorph the weapon...
	    ITEM		*newweapon;

	    newweapon = weapon->polymorph(this);

	    // If the polymorph fails, we don't want to delete
	    // their weapon!
	    if (!newweapon)
	    {
		// We haven't yet dropped the old weapon,
		// so just break for status quo
		break;
	    }
	    
	    // Some day I will make a less error prone way of writing
	    // this.  This is not that day!
	    weapon = dropItem(weapon->getX(), weapon->getY());

	    // If the weapon has a stack, we split & reacquire.  Otherwise
	    // we destroy.
	    if (weapon->getStackCount() > 1)
	    {
		weapon->splitAndDeleteStack();
		// Try and reacquire as a non-weapon.
		if (!acquireItem(weapon))
		{
		    // Failed to put in inventory, drop.
		    formatAndReport("%U <lack> room for %IU.", weapon);
		    glbCurLevel->acquireItem(weapon, getX(), getY(), 0);
		}
	    }
	    else
	    {
		// Toss the item silently
		delete weapon;
	    }

	    // Re-equip the new item. This could fail.
	    // First, we need it in our inventory.
	    if (!acquireItem(newweapon, &newweapon))
	    {
		// Failed to acquire, drop on ground.
		formatAndReport("%U <lack> room for %IU.", newweapon);
		glbCurLevel->acquireItem(newweapon, getX(), getY(), 0);
	    }
	    else
	    {
		// It is now in our inventory.  Try to equip it,
		// including failure messages!
		actionEquip(newweapon->getX(), newweapon->getY(), ITEMSLOT_RHAND);
	    }

	    break;
	}

	case NUM_PUNISHS:
	{
	    // Confusion!
	    buf.sprintf("%s wails in confusion.  ",
			glb_goddefs[god].name);
	    msg_announce(buf);
	    break;
	}
    }
}

bool
MOB::pietyBoonValid(BOON_NAMES boon)
{
    ITEM		*item;
    bool		 haswater = false;
    bool		 hascursed = false;
    bool		 hasweapon = false;
    bool		 hasarmour = false;
    bool		 hasnonid = false;

    // Check for items needed.
    for (item = myInventory; item; item = item->getNext())
    {
	// Check for potential sanctify:
	if ( item->getDefinition() == ITEM_WATER &&
	    !item->isBlessed())
	    haswater = true;

	// Check if the item is an unided artifact or unided
	// magic type.
	if (!item->isIdentified() && item->getMagicType() != MAGICTYPE_NONE)
	    hasnonid = true;
		
	if (item->getArtifact() && !item->isFullyIdentified())
	    hasnonid = true;
	
	// Check for an uncursing.
	if (item->isCursed() && item->getX() == 0)
	    hascursed = true;

	if (item->getX() == 0 &&
	    item->getY() == ITEMSLOT_RHAND)
	    hasweapon = true;

	if (item->getX() == 0 &&
	    (item->getY() == ITEMSLOT_LHAND ||
	     item->getY() == ITEMSLOT_HEAD ||
	     item->getY() == ITEMSLOT_BODY ||
	     item->getY() == ITEMSLOT_FEET))
	    hasarmour = true;
    }
    
    switch (boon)
    {
	case BOON_UNCURSE:
	    return hascursed;

	case BOON_HEAL:
	    if (getHP() < getMaxHP() / 3)
		return true;
	    return false;

	case BOON_LICHFORM:
	    if (getHP() < getMaxHP() / 4)
		return true;
	    return false;

	case BOON_CURE:
	    return aiIsPoisoned();

	case BOON_SANCTIFY:
	    return haswater;

	case BOON_SURROUNDATTACK:
	    // Verify you are surrounded by a foe...
	    if (calculateFoesSurrounding() > 1)
	    {
		return true;
	    }
	    break;

	case BOON_UNSTONE:
	    // If you are turning to stone, and are not actively resisting
	    // turning to stone, then help.
	    return hasIntrinsic(INTRINSIC_STONING) && 
		    !hasIntrinsic(INTRINSIC_RESISTSTONING) &&
		    !hasIntrinsic(INTRINSIC_UNCHANGING);

	case BOON_ENCHANT_WEAPON:
	    return hasweapon;

	case BOON_ENCHANT_ARMOUR:
	    return hasarmour;

	case BOON_GRANT_SPELL:
	    // TODO: Find if we have any holes in our spell schools.
	    return true;
	    
	// Trivially true boons:
	case BOON_GIFT_WEAPON:
	case BOON_GIFT_ARMOUR:
	case BOON_GIFT_WAND:
	case BOON_GIFT_SPELLBOOK:
	case BOON_GIFT_STAFF:
	    return true;

	case BOON_IDENTIFY:
	    return hasnonid;

	case NUM_BOONS:
	    break;
    }

    return false;
}

void
MOB::pietyBoon(GOD_NAMES god)
{
    BOON_NAMES		valid[NUM_BOONS], boon;
    int			numvalid = 0;
    BUF			buf;
    ITEM		*item;
    bool		hasrescue = false;

    // Run through the boons determining if they apply and if they
    // can be paid for.
    FOREACH_BOON(boon)
    {
	// See if we can afford the cost.
	if (glb_boondefs[boon].points > glbGodPoints[god])
	    continue;

	// See if this is a valid god.
	if (!strchr(glb_boondefs[boon].god, (char) god))
	    continue;

	// Check to see if it applies.  Eg, no point curing
	// someone not poisoned.
	if (pietyBoonValid(boon))
	{
	    if (hasrescue && !glb_boondefs[boon].isrescue)
		continue;

	    if (glb_boondefs[boon].isrescue && !hasrescue)
	    {
		hasrescue = true;
		numvalid = 0;
	    }
	    valid[numvalid++] = boon;
	}
    }

    // If no valid boons, quit.
    if (!numvalid)
	return;

    boon = valid[rand_choice(numvalid)];

    // Pay the cost.
    glbGodPoints[god] -= glb_boondefs[boon].points;

    // Do the boon:
    switch (boon)
    {
	case BOON_UNCURSE:
	    buf.sprintf("%s: Allow me to aid you.  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);
	    receiveUncurse(false);
	    break;
	    
	case BOON_HEAL:
	    buf.sprintf("%s: Your flesh shall mend!  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);
	    // Full heal
	    receiveHeal(getMaxHP() - getHP(), 0);
	    break;

	case BOON_LICHFORM:
	    buf.sprintf("%s: Show your devotion!  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);
	    setTimedIntrinsic(0, INTRINSIC_LICHFORM, 2);
	    break;

	case BOON_SURROUNDATTACK:
	{
	    buf.sprintf("%s: You've proven your worth!",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    int		dx, dy;

	    FORALL_4DIR(dx, dy)
	    {
		MOB		*mob;
		
		mob = glbCurLevel->getMob(getX()+dx, getY()+dy);

		if (mob && mob->getAttitude(this) == ATTITUDE_HOSTILE)
		{
		    mob->receiveAttack(ATTACK_SURROUNDSMITE, 0, 0, 0,
					    ATTACKSTYLE_SPELL);
		}
	    }
	    break;
	}

	case BOON_CURE:
	    buf.sprintf("%s: Your affliction pains me!  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);

	    receiveCure();
	    break;
	    
	case BOON_UNSTONE:
	    buf.sprintf("%s: I wish no more statues.  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);

	    clearIntrinsic(INTRINSIC_STONING);
	    setTimedIntrinsic(0, INTRINSIC_RESISTSTONING, 10);
	    break;

	case BOON_SANCTIFY:
	{
	    buf.sprintf("%s: Your water is blessed!  ",
			    glb_goddefs[god].name);
	    msg_announce(buf);

	    for (item = myInventory; item; item = item->getNext())
	    {
		if (item->getDefinition() == ITEM_WATER)
		{
		    // Twice to ensure we end up at blessed status.
		    item->makeBlessed();
		    item->makeBlessed();
		    // It is rather obvious that the water must now be
		    // holy water.
		    item->markCursedKnown();
		}
	    }
	    
	    break;
	}

	case BOON_ENCHANT_WEAPON:
	{
	    buf.sprintf("%s: You are my weapon: Cut Deeply!  ",
		    glb_goddefs[god].name);
	    msg_announce(buf);

	    receiveWeaponEnchant(1, false);
	    break;
	}

	case BOON_ENCHANT_ARMOUR:
	{
	    buf.sprintf("%s: Accept my protection.  ",
		    glb_goddefs[god].name);
	    msg_announce(buf);

	    receiveArmourEnchant((ITEMSLOT_NAMES) -1, 1, false);
	    break;
	}

	case BOON_GRANT_SPELL:
	{
	    buf.sprintf("%s: Knowledge is Power!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    // Random spell..
	    SPELL_NAMES			spell;

	    spell = findSpell(god);

	    if (spell == SPELL_NONE)
	    {
		// You are so cool you already know all
		// the spells you can!  Deity is suitably impressed.
		msg_announce("You already have knowledge.  Now gain power!  ");
		
		// Maximize MP.
		if (getMP() < getMaxMP())
		{
		    formatAndReport("%U <be> energized.");
		    myMP = getMaxMP();
		}

		// Bonus XP.
		formatAndReport("%U <gain> experience.");
		receiveExp(100);
	    }
	    else
		learnSpell(spell);
	    break;
	}

	case BOON_GIFT_WEAPON:
	case BOON_GIFT_ARMOUR:
	case BOON_GIFT_WAND:
	case BOON_GIFT_SPELLBOOK:
	case BOON_GIFT_STAFF:
	{
	    buf.sprintf("%s: Use my gift wisely.  ",
			glb_goddefs[god].name);
	    msg_announce(buf);

	    ITEM		*gift;

	    gift = ITEM::createRandomType((ITEMTYPE_NAMES) 
					    glb_boondefs[boon].gifttype);

	    // Ensure it is blessed and a +3 enchant.
	    gift->makeBlessed();
	    gift->makeBlessed();
	    
	    gift->enchant(3 - gift->getEnchantment());

	    // Drop at feet:
	    buf = formatToString("%IU <I:appear> by %R %B1.",
		    this, 0, 0, gift,
		    getSlotName(ITEMSLOT_FEET) ?
			    getSlotName(ITEMSLOT_FEET) :
			    "shadow");
	    reportMessage(buf);

	    glbCurLevel->acquireItem(gift, getX(), getY(), 0);
	    break;
	}

	case BOON_IDENTIFY:
	{
	    ITEMSTACK		 stack;
	    ITEM		*item;

	    buf.sprintf("%s: Know Your Tools!  ",
			glb_goddefs[god].name);
	    msg_announce(buf);
	    
	    for (item = myInventory; item; item = item->getNext())
	    {
		if (!item->isIdentified() && item->getMagicType() != MAGICTYPE_NONE)
		    stack.append(item);
		else if (item->getArtifact() && !item->isFullyIdentified())
		    stack.append(item);
	    }

	    UT_ASSERT(stack.entries());
	    if (!stack.entries())
		break;
	    
	    item = stack(rand_choice(stack.entries()));

	    if (isAvatar())
		item->markIdentified();
	    
	    formatAndReport("%U <gain> insight into the nature of %IU.", item);
	    break;
	}

	case NUM_BOONS:
	    // Should not occur.
	    break;
    }
}

void
MOB::pietyAnnounceWhimOfXom()
{
    // If you worship ><0|V|, you get a clue as to his new preference.
    if (glbChosenGod == GOD_CULTIST)
    {
	msg_announce(glb_goddefs[glbWhimOfXOM].whimofxom);
    }
}

void
MOB::pietyRungods()
{
    GOD_NAMES		god;

    // Only run the god code on the avatar.
    if (!isAvatar())
	return;

    // Update ><0|V|s whim.
    if (rand_chance(80))
	glbWhimOfXOMTimer--;

    if (!glbWhimOfXOMTimer)
    {
	glbWhimOfXOMTimer = rand_range(100, 200);
	glbWhimOfXOM = piety_randomnonxomgod();

	pietyAnnounceWhimOfXom();
    }

    // If we are worshipping no one god, we get no benefits or punishments.
    if (glbChosenGod == GOD_AGNOSTIC)
    {
	return;
    }

    if (glbFlameStrikeTimer)
    {
	glbFlameStrikeTimer--;

	if (!glbFlameStrikeTimer)
	{
	    BUF		buf;

	    buf.sprintf("%s: Suffer my Wrath!  ",
			glb_goddefs[glbFlameGod].name);
	    msg_announce(buf);

	    // Peform flamestrike.
	    // TODO: Have MOBs for placeholders for the gods.
	    receiveAttack(ATTACK_FLAMESTRIKE, 0, 0, 0,
				    ATTACKSTYLE_SPELL);
	    return;
	}
    }
	
    FOREACH_GOD(god)
    {
	// Fickle gods do not always act.
	// this is broken and should be continue, not return, or we bias
	// against later gods.
	if (!rand_choice(10))
	    return;
	if (glbGodPoints[god] > 100)
	{
	    // Consider granting a boon.
	    // TODO: Track number of levels gained and bias
	    // accordingly.
	    if (god == glbChosenGod || !rand_choice(50))
	    {
		if (glbPiety[god] > 0)
		    pietyBoon((GOD_NAMES) god);
		else
		    pietyPunish((GOD_NAMES) god);

		// Do not test anything else in case we died
		return;
	    }
	}
    }
}

void
MOB::pietyGainLevel()
{
    GOD_NAMES		 godlist[NUM_GODS], god;
    int			 numvalid = 0, i, choice, y;
    int			 aorb = 0;
    MOB			*base;
    SPELL_NAMES		 spell = (SPELL_NAMES) -1;
    BUF			 buf;
    SKILL_NAMES		 skill = SKILL_NONE;
    int			 newpts;

    base = myBaseType.getMob();

    const char *gainmsg[] =
    {
	"You have gained a level!  ",
	"Welcome to the next level.  ",
	"Level Up!  ",
	"You learn from experience.  ",
	"Another kill, Another level.  "
    };
    // This message announce helps avoid accidental button presses.
    msg_announce(gainmsg[rand_choice(5)]);

    // Report current god...
    BUF		curgod;
    curgod.sprintf("You currently worship %s.  ", glb_goddefs[glbChosenGod].name);
    msg_report(curgod);

    FOREACH_GOD(god)
    {
	// Is this a valid god?  Agnostic is always valid.
	if (god == GOD_AGNOSTIC ||
	    glbPiety[god] >= 10)
	{
	    godlist[numvalid++] = god;
	}
    }

    // Ask the user to pick a level.
    char		**menu;

    menu = new char *[numvalid+2];
    // First choice is a sentinel to insure rapid clickers don't get
    // get the wrong choice.
    menu[0] = "Select a god";
    for (i = 0; i < numvalid; i++)
    {
	if (godlist[i] != GOD_AGNOSTIC)
	{
	    // Give some detailed info about the god.
	    // I think providing both numbers is a bit confusing, so
	    // we'll stick with the Piety
	    menu[i+1] = new char [30];
	    sprintf(menu[i+1], "%-11s (%d)",
		    glb_goddefs[godlist[i]].classname,
		    glbPiety[godlist[i]]);
	}
	else
	    menu[i+1] = (char *) glb_goddefs[godlist[i]].classname;
    }
    menu[numvalid+1] = 0;

    while (1)
    {
	choice = gfx_selectmenu(30 - 20, 3, (const char **)menu, aorb);
	for (y = 3; y < 19; y++)
	    gfx_cleartextline(y);

	if (hamfake_forceQuit())
	{
	    // We sadly have to force-pick a god to be able
	    // to save successfully.  Adventurer is likely the safest.
	    god = GOD_AGNOSTIC;
	    break;
	}

	// Valid choices,,,,
	if (choice >= 1 && !aorb)
	{
	    BUF		godchoice;

	    // Fix off by one due to sentinel
	    choice--;

	    // User chose godlist[choice].
	    god = godlist[choice];

	    // Inform the user of their future decision.
	    // Display the info text.
	    encyc_viewentry("GOD", god);

	    godchoice.sprintf("Follow %s?", glb_goddefs[god].name);

	    // Verify user is serious.
	    if (gfx_yesnomenu(godchoice))
		break;
	}
    }

    // Wipe out our menu.
    for (i = 0; i < numvalid; i++)
    {
	if (godlist[i] != GOD_AGNOSTIC)
	{
	    delete [] menu[i+1];
	}
    }
    delete [] menu;

    glbChosenGod = god;
    
    formatAndReport("%U <worship> %B1.",
		glb_goddefs[god].name);

    // Add an exp level.
    myExpLevel++;

    // Add our health & magic.
    myHitDie += glb_goddefs[god].physical;

    // 3-5 points per level with a bias to 4.  Considered having it
    // controled by piety but that just forces people away from
    // experimentation.
    // 2d2+1
    newpts = rand_dice(glb_goddefs[god].physical*2, 2, glb_goddefs[god].physical);
    incrementMaxHP(newpts);

    if (base)
    {
	base->myHitDie += glb_goddefs[god].physical;
    }
    
    myMagicDie += glb_goddefs[god].mental;
    newpts = rand_dice(glb_goddefs[god].mental*2, 2, glb_goddefs[god].mental);
    incrementMaxMP(newpts);

    if (base)
    {
	base->myMagicDie += glb_goddefs[god].mental;
    }

    int		numskill = 0, numspell = 0;
    
    skill = findSkill(god, &numskill);
    spell = findSpell(god, &numspell);

    // You only get one of skill or spell, never both.
    // We bias according to the number of valid choices.
    if (spell != SPELL_NONE && skill != SKILL_NONE)
    {
	if (rand_choice(numspell + numskill) < numskill)
	    spell = SPELL_NONE;
	else
	    skill = SKILL_NONE;
    }

    // Special cases for gods...
    switch (god)
    {
	case GOD_CULTIST:
	{
	    // Gain random hit & magic dice.
	    int		phys, ment;

	    phys = rand_choice(5);
	    ment = rand_choice(5);
	    myHitDie += phys;
	    myMagicDie += ment;
	    if (base)
	    {
		base->myHitDie += phys;
		base->myMagicDie += ment;
	    }

	    phys = rand_choice(phys * 10);
	    ment = rand_choice(ment * 10);

	    incrementMaxHP(phys);
	    incrementMaxMP(ment);
	    break;
	}

	case GOD_WIZARD:
	{
	    break;
	}

	case GOD_CLERIC:
	{
	    break;
	}

	case GOD_NECRO:
	{
	    break;
	}

	case GOD_AGNOSTIC:
	{
	    // Do nothing!
	    break;
	}

	case GOD_FIGHTER:
	{
	    break;
	}

	case GOD_ROGUE:
	{
	    break;
	}

	case GOD_BARB:
	{
	    break;
	}

	case NUM_GODS:
	{
	    break;
	}
    }

    learnSpell(spell);
    learnSkill(skill);

    // Report number of free slots
    buf.sprintf("You have %d free spell and %d free skill slots.",
		    getFreeSpellSlots(),
		    getFreeSkillSlots());
    reportMessage(buf);

    // If you just started following Xom, pretty important.
    pietyAnnounceWhimOfXom();
}

void
MOB::pietyReport()
{
    char		*menu[NUM_GODS + 1];
    int			 dressiness[NUM_GODS];

    int			 i, aorb;
    GOD_NAMES		 god;

    determineClassiness(dressiness);

    FOREACH_GOD(god)
    {
	menu[god] = new char[30];

	sprintf(menu[god], "%10s: %d (%d)",
		glb_goddefs[god].name,
		glbPiety[god],
		glbGodPoints[god]);
	if (dressiness[god])
	{
	    // Pad out a tad
	    strcat(menu[god], " ");
	}
	for (int i = 2; i <= dressiness[god]; i+=2)
	    strcat(menu[god], "*");
	if (dressiness[god] & 1)
	    strcat(menu[god], "+");
    }
    menu[NUM_GODS] = 0;

    {
	BUF		buf;
	buf.sprintf("Now Worshipping: %s", glb_goddefs[glbChosenGod].name);
	gfx_printtext(2, 4, buf.buffer());
    }

    gfx_selectmenu(5, 6, (const char **) menu, aorb, glbChosenGod);

    // And clear it...
    for (i = 3; i < 18; i++)
    {
	gfx_cleartextline(i);
    }

    for (i = 0; i < NUM_GODS; i++)
	delete [] menu[i];
}
