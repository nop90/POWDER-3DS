/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        action.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This file is responsible for implementing all the ui-independent
 *	actions that a MOB can perform.  These actions should encompass
 *	any action which the Player can perform.  Their arguments should
 *	fully qualify the players choice (Ie: which item, what direction,
 *	etc).
 *
 *	The theory is that if the actions are properly abstracted, it
 *	will be equally easy to perform them from the AI routines as
 *	from the User Interface routines.
 *
 *	Note that most actions are implicitly stacked.  actionClimb
 *	will resolve itself to actionClimbUp or actionClimbDown depending
 *	on the context.
 */

#include "mygba.h"

#include <stdio.h>
#include <ctype.h>
#include "assert.h"
#include "artifact.h"
#include "creature.h"
#include "glbdef.h"
#include "rand.h"
#include "map.h"
#include "gfxengine.h"
#include "msg.h"
#include "grammar.h"
#include "item.h"
#include "itemstack.h"
#include "intrinsic.h"
#include "control.h"
#include "victory.h"
#include "piety.h"
#include "encyc_support.h"
#include "stylus.h"

void writeGlobalActionBar(bool useemptyslot);
void hideSideActionBar();

extern bool glbSuppressAutoClimb;
extern void attemptunlock();

//
// ACTIONS
//

bool
MOB::actionBump(int dx, int dy)
{
    int		nx, ny;

    nx = getX() + dx;
    ny = getY() + dy;

    // Check if a monster is here.
    // Is it hostile?  Kill!
    // Is it friendly?  Shove.
    // Is it neutral?  Kill!
    MOB			*mob;
    ITEM		*item;
    mob = glbCurLevel->getMob(nx, ny);
    if (mob == this)
	mob = 0;

    // This is very fucked up.
    // It should be rewritten by someone not drunk.
    // This has now been rewritten by someone not drunk.  It doesn't
    // necessarily work any better.
    if (!mob && getSize() >= SIZE_GARGANTUAN)
    {
	if (dx)
	{
	    if (dx < 0)
		mob = glbCurLevel->getMob(nx, ny+1);
	    else
	    {
		mob = glbCurLevel->getMob(nx+1, ny);
		if (!mob)
		    mob = glbCurLevel->getMob(nx+1, ny+1);
	    }
	}
	else
	{
	    if (dy < 0)
		mob = glbCurLevel->getMob(nx+1, ny);
	    else
	    {
		mob = glbCurLevel->getMob(nx, ny+1);
		if (!mob)
		    mob = glbCurLevel->getMob(nx+1, ny+1);
	    }
	}
	if (mob == this)
	    mob = 0;
	if (mob)
	{
	    dx = mob->getX() - getX();
	    dy = mob->getY() - getY();
	}
    }
    if (mob)
    {
	if (getAttitude(mob) == ATTITUDE_HOSTILE)
	{
	    // Attack!
	    return actionAttack(dx, dy);
	}
	else if (getAttitude(mob) == ATTITUDE_NEUTRAL)
	{
	    // Attack!  THis is bloodthirsty game, after all.
	    // Only the avatar is bloodthirsty.  Other people
	    // are nice and kind.
	    if (isAvatar())
		return actionAttack(dx, dy);
	    else
		return false;		// No bump, no swap.
	}
	else if (getAttitude(mob) == ATTITUDE_FRIENDLY)
	{
	    // Check if the friendship is reciprocated.  If so,
	    // we can swap position.
	    if (mob->getAttitude(this) == ATTITUDE_FRIENDLY)
	    {
		// swap.
		// Okay, this is extradorinarily frustrating!
		// Dancing grid bugs: No more!
		// Only the avatar can swap by defualt (if he is
		// polyied into a critter which is swappable, etc)
		if (isAvatar())
		{
		    // Check if we are to talk
		    if (mob->isTalkative())
			return actionTalk(dx, dy);
		    else
			return actionSwap(dx, dy);
		}
		else
		    return false;
	    }
	    else
	    {
		// Nothing to do.
		return false;
	    }
	}
    }

    // Check if creature can physically move in that direction...
    if (canMoveDelta(dx, dy, false))
    {
	// We can physically move that way.  However, this is no
	// guarantee of freedom.
	
	// Check for item here.
	item = glbCurLevel->getItem(nx, ny);
	if (item && !item->isPassable())
	{
	    // Can we push it?  
	    // Boulder is in the way, try and push.
	    return actionPush(dx, dy);
	}
	
	// An empty, valid square.
	// Will it hurt us?  Are we smart enough to know that?

	// All clear!  Move with no embellishments.
	return actionWalk(dx, dy);
    }
    else
    {
	// Find out why.  Perhaps the creature has an alternative action?
	if (nx < 0 || nx >= MAP_WIDTH ||
	    ny < 0 || ny >= MAP_HEIGHT)
	{
	    // Simple enough reason, abort!
	    // Add a funny message depending on direction
	    if (isAvatar())
	    {
		if (dx)
		{
		    formatAndReport("%U <hear> the sound of someone mashing buttons.");
		}
		else
		{
		    formatAndReport("Do not go there!  You will break the backlight!");
		}
	    }
	    return false;
	}

	SQUARE_NAMES		tile;

	tile = glbCurLevel->getTile(nx, ny);
	switch (tile)
	{
	    case SQUARE_BLOCKEDDOOR:
		// Only avatar can open these
		if (!isAvatar())
		    break;
		// FALL THROUGH

	    case SQUARE_DOOR:
	    case SQUARE_MAGICDOOR:
		// If we are smart enough, open it.
		if (getSmarts() >= SMARTS_FOOL)
		{
		    return actionOpen(dx, dy);
		}
		break;

	    default:
		break;
	}

	// Check for the ability to dig...
	if (hasIntrinsic(INTRINSIC_DIG))
	{
	    return actionDig(dx, dy, 0, 1, false);
	}

	// Report nothing, just fail.
	return false;
    }
}

bool
MOB::actionTalk(int dx, int dy)
{
    applyConfusion(dx, dy);

    MOB		*mob;
    mob = glbCurLevel->getMob(getX()+dx, getY()+dy);

    if (!mob)
    {
	formatAndReport("%U <talk> into empty air.");
	return false;
    }
    if (mob == this)
    {
	formatAndReport("%U <talk> to %A.");
	return false;
    }
    if (!mob->isTalkative())
    {
	formatAndReport("%U <talk> to %MA.", mob);
	formatAndReport("%MU does not answer.", mob);
	return false;
    }

    // Well, for now it means the village elder!
    if (!isAvatar())
	return false;

    // No talking to the elder!
    if (glbStressTest)
	return false;

    encyc_viewentry("TALK", TALK_ELDER);
    if (gfx_yesnomenu("Leave for the feast?"))
    {
	glbVictoryScreen(true, 0, this, 0);
	glbHasAlreadyWon = true;

	// Remove the avatar and elder to the feast
	glbCurLevel->unregisterMob(this);
	glbCurLevel->unregisterMob(mob);

	setAvatar(0);

	delete mob;
	delete this;

	// required as we're dead
	return false;
    }
    return false;
}

bool
MOB::actionBreathe(int dx, int dy)
{
    ATTACK_NAMES		attack;
    BUF	buf;

    applyConfusion(dx, dy);

    attack = getBreathAttack();
    
    if (attack == ATTACK_NONE)
    {
	if (hasIntrinsic(INTRINSIC_NOBREATH))
	    formatAndReport("%U <do> not need to breathe.");
	else if (hasIntrinsic(INTRINSIC_STRANGLE))
	    formatAndReport("%U <be> unable to breathe.");
	else if (hasIntrinsic(INTRINSIC_SUBMERGED))
	    formatAndReport("%U <choke>.");
	else
	    formatAndReport("%U <breathe> heavily.");
	return true;
    }

    if (!dx && !dy)
    {
	// Breathing on self.
	formatAndReport("%U <check> %r breath.");
	return true;
    }

    // Check if we are out of breath...
    if (hasIntrinsic(INTRINSIC_TIRED))
    {
	formatAndReport("%U <belch>");
	return true;
    }

    buf = formatToString("%U <breathe> %B1.",
		    this, 0, 0, 0,
		    getBreathSubstance(), 0);
    reportMessage(buf);

    // Breathing fire may kill ourselves, so we must set the
    // intrinsic prior to sending the ray.
    // After this, we want to time out for the breath delay time...
    setTimedIntrinsic(this, INTRINSIC_TIRED, 
		      rand_dice(glb_mobdefs[myDefinition].breathdelay));
    
    // Now, run the attack with the breath...
    MOBREF		myself;

    myself.setMob(this);

    ourEffectAttack = getBreathAttack();
    glbCurLevel->fireRay(getX(), getY(), 
		         dx, dy,
			 getBreathRange(),
			 MOVE_STD_FLY, 
			 glb_attackdefs[ourEffectAttack].reflect,
			 areaAttackCBStatic,
			 &myself);
    return true;
}

bool
MOB::actionJump(int dx, int dy)
{
    int		nx, ny, mx, my;
    int		tile;
    bool	leapattack = false;
    bool	singlestep = false;

    if (!hasIntrinsic(INTRINSIC_JUMP))
    {
	formatAndReport("%U cannot jump very far.");
	return false;
    }

    // If we are standing on liquid, we can only jump if we have water walk
    // or we don't have enough traction.
    tile = glbCurLevel->getTile(getX(), getY());
    if (!(glb_squaredefs[tile].movetype & MOVE_WALK))
    {
	if (!hasIntrinsic(INTRINSIC_WATERWALK) ||
	    !(glb_squaredefs[tile].movetype & MOVE_SWIM))
	{
	    // The ground isn't solid enough!
	    formatAndReport("%U <lack> traction to jump.");
	    return false;
	}
    }
    else if (hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	// We are buried alive.
	formatAndReport("%U <lack> room to jump.");
	return false;
    }

    if (glb_squaredefs[tile].isstars && !defn().spacewalk)
    {
	// Can't jump in space
	formatAndReport("%U <fail> to brace yourself against vacuum.");
	return false;
    }

    if (hasIntrinsic(INTRINSIC_INPIT))
    {
	// Jumping out of a pit only moves you a single square.
	singlestep = true;
    }

    applyConfusion(dx, dy);

    // Verify the square they will jump over is passable.
    nx = getX() + dx;
    ny = getY() + dy;
    mx = nx;
    my = ny;

    tile = glbCurLevel->getTile(nx, ny);
    if (!(glb_squaredefs[tile].movetype & MOVE_STD_FLY))
    {
	// You try and jump through a wall or something.
	formatAndReport("%U <prepare> to jump, then %U <think> better of it.");
	return false;
    }

    if (!singlestep)
    {
	nx = nx + dx;
	ny = ny + dy;
    }

    // Note we want to not jump to unmapped squares.  However, that is
    // avatar only.  The only time isLit does not suffice is if we
    // are jumping blind.  Hence we prohibit blind jumping.
    if (!canMove(nx, ny, true) ||
	!glbCurLevel->isLit(nx, ny) ||
	hasIntrinsic(INTRINSIC_BLIND))
    {
	formatAndReport("%U <balk> at the leap!");
	return false;
    }

    // Prohibit you jumping onto a creature if a creature is in the way.
    if (glbCurLevel->getMob(nx, ny) && (glbCurLevel->getMob(nx, ny) != this))
    {
	// Check if we have the leap attack skill.
	if (!hasSkill(SKILL_LEAPATTACK) ||
	    getSize() >= SIZE_GARGANTUAN ||	
	    glbCurLevel->getMob(nx, ny)->getSize() >= SIZE_GARGANTUAN)
	{
	    formatAndReport("%U <balk> at the leap!");
	    return false;
	}

	// Do a leap attack!
	leapattack = true;
    }

    // Report that we are leaping.
    formatAndReport("%U <leap>.");

    // Clearly jumping should noise your feet.
    makeEquipNoise(ITEMSLOT_FEET);


    if (leapattack)
    {
	// Push the creature at our destination out of the way.
	// First knock back the intermediate creature into our own square.
	if (glbCurLevel->getMob(mx, my))
	{
	    // Allow us to overwrite our own position as we'll
	    // be moving shortly anyways.
	    glbCurLevel->knockbackMob(mx, my, -dx, -dy, true, true);
	}
	// Next, knockback our target creature into the middle square.
	glbCurLevel->knockbackMob(nx, ny, -dx, -dy, true);
    }

    // Move!  We may die here, hence the test.
    if (!move(nx, ny))
	return true;

    // Attack the vacated creature if still alive.
    if (leapattack && glbCurLevel->getMob(mx, my))
    {
	// Note that because we attack backwards I think you can
	// trigger a charge by running away and then leaping back...
	// Most curious... I wonder if anyone will notice, and if they
	// do, consider it a bug?
	actionAttack(-dx, -dy, 1);
    }

    // Report what we stepped on...
    // We recheck isAvatar() here as it will go false if this is deleted.
    if (isAvatar())
    {
	glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
    }
    
    return true;
}
    
bool
MOB::actionWalk(int dx, int dy, bool spacewalk)
{
    int		nx, ny;
    ITEM	*item;
    MOB		*mob;
    
    applyConfusion(dx, dy);

    nx = getX() + dx;
    ny = getY() + dy;

    if (!canMoveDelta(dx, dy, true))
	return false;
    
    // If there is a monster, can't move.
    mob = glbCurLevel->getMob(nx, ny);
    if (mob && (mob != this))
    {
	// Check to see if we can swap
	if (getAttitude(mob) == ATTITUDE_FRIENDLY)
	{
	    // Check if the friendship is reciprocated.  If so,
	    // we can swap position.
	    if (mob->getAttitude(this) == ATTITUDE_FRIENDLY)
	    {
		// swap.
		// Okay, this is extradorinarily frustrating!
		// Dancing grid bugs: No more!
		// Only the avatar can swap by defualt (if he is
		// polyied into a critter which is swappable, etc)
		if (isAvatar())
		    return actionSwap(dx, dy);
	    }
	}
	return false;
    }

    // If there is an item, and it is not steppable, no move.
    item = glbCurLevel->getItem(nx, ny);
    if (item && !item->isPassable())
	return false;

    // If we are on a star tile, we can only move if we can space
    // walk
    if (!spacewalk && !defn().spacewalk)
    {
	SQUARE_NAMES		oldtile;

	oldtile = glbCurLevel->getTile(getX(), getY());
	if (glb_squaredefs[oldtile].isstars)
	{
	    formatAndReport("%U <flail> against unyielding vacuum!");
	    return true;
	}
    }

    // If we are in a pit, we first must climb out of it.
    if (hasIntrinsic(INTRINSIC_INPIT))
    {
	// The user must first climb out of the pit.  Thus, we try
	// to climb:
	return actionClimbUp();
    }

    if (hasIntrinsic(INTRINSIC_INTREE))
    {
	SQUARE_NAMES		newtile;
	// The user must first climb out of the tree.  Thus, we try
	// to climb:
	// We are allowed to travel from tree to tree, however.
	newtile = glbCurLevel->getTile(nx, ny);
	if (newtile != SQUARE_FOREST &&
	    newtile != SQUARE_FORESTFIRE)
	    return actionClimbDown();
    }

    // If we are submerged, and our destination isn't something
    // you can be submerged in, we must first climb out.
    if (hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	SQUARE_NAMES		newtile;

	newtile = glbCurLevel->getTile(nx, ny);
	if (newtile != SQUARE_WATER &&
	    newtile != SQUARE_LAVA &&
	    newtile != SQUARE_ACID)
	{
	    return actionClimbUp();
	}
    }

    // Because we move, make any noise that stems from our
    // footwear.
    makeEquipNoise(ITEMSLOT_FEET);

    // Move.
    if (!move(nx, ny))
	return true;

    // Report what we stepped on...
    if (isAvatar())
    {
	// Store the delta.
	ourAvatarDx = dx;
	ourAvatarDy = dy;
	ourAvatarMoveOld = false;
	glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
    }
    
    return true;
}

bool
MOB::actionPush(int dx, int dy)
{
    int		 nx, ny, px, py;
    MOB		*mob;
    ITEM	*item, *blockitem;
    BUF		 buf;

    applyConfusion(dx, dy);

    nx = getX() + dx;
    ny = getY() + dy;

    mob = glbCurLevel->getMob(nx, ny);
    if (mob)
    {
	// TODO: Aggro the victim?
	// TODO: Strength/size check, and allow pushing into water?
	formatAndReport("%U <push> %MU.", mob);
	return true;
    }

    item = glbCurLevel->getItem(nx, ny);

    if (item)
    {
	// Check to see if there is room.
	px = nx + dx;
	py = ny + dy;
	if (px < 0 || px >= MAP_WIDTH ||
	    py < 0 || py >= MAP_HEIGHT ||
	    !glbCurLevel->canMove(px, py, MOVE_STD_FLY)
	    )
	{
	    // Can't move as no room for the item.
	    formatAndReport("%U <strain>, but %IU <I:do> not budge.", item);
	    return true;
	}

	mob = glbCurLevel->getMob(px, py);
	if (mob)
	{
	    formatAndReportExplicit("%IU <I:be> blocked by %MU.", mob, item);
	    return true;
	}

	// Determine if it is blocked by another item.
	blockitem = glbCurLevel->getItem(px, py);
	if (blockitem && !blockitem->isPassable())
	{
	    buf = MOB::formatToString("%U <be> blocked by %IU.", 0, item, 0, blockitem);
	    reportMessage(buf);
	    return true;
	}
	
	// We report prior to moving as moving may cause it to be deleted,
	// for example if it falls into a hole.
	formatAndReport("%U <push> %IU.", item);

	// Move the item.
	glbCurLevel->dropItem(item);
	glbCurLevel->acquireItem(item, px, py, this);

	return true;
    }

    // Reduced to pushing map features.
    // TODO: Pushing doors, walls, etc.
    formatAndReport("%U <push> thin air.");
    return false;
}

bool
MOB::actionAttack(int dx, int dy, int multiplierbonus)
{
    int		 nx, ny;
    MOB		*mob, *thismob;
    ITEM	*weapon;
    BUF		 buf;
    int		 ox, oy, pdx, pdy;
    bool	 didhit = false;
    bool	 firstweapon = true;
    bool	 improvised = true;
    bool	 ischarge = false;

    applyConfusionNoSelf(dx, dy);

    ox = getX();
    oy = getY();
    nx = getX() + dx;
    ny = getY() + dy;

    mob = glbCurLevel->getMob(nx, ny);

    if (!mob)
    {
	formatAndReport("%U <swing> wildly at the air!");
	return true;
    }

    getLastMoveDirection(pdx, pdy);
    // See if this is a charge
    if (pdx == dx && pdy == dy && (dx || dy))
    {
	if (hasSkill(SKILL_CHARGE))
	    ischarge = true;	
    }

    if (ischarge)
    {
	const char *chargemsg[] = 
	{
	    "%U <charge>!",
	    "%U <yell> a warcry!",
	    "%U <brace> your weapon!",
	    "%U <rush> into battle!",
	    0
	};
	formatAndReport(rand_string(chargemsg));

	multiplierbonus++;
    }

    // Apply damage to mob.
    const ATTACK_DEF		*attack;

    // Determining attack is simple.  If we have anything in our right
    // hand, we use that.  Otherwise, we use our native attack
    // from our definition file.
    weapon = getEquippedItem(ITEMSLOT_RHAND);
    if (weapon)
    {
	attack = weapon->getAttack();

	// We have a chance of iding a weapon any time it is used.
	if (isAvatar())
	{
	    if (!weapon->isKnownEnchant())
	    {
		int		skill, chance;

		skill = getWeaponSkillLevel(weapon, ATTACKSTYLE_MELEE);
		chance = 5;
		if (skill >= 1)
		    chance += 5;
		if (skill >= 2)
		    chance += 10;
		    
		// About 20 hits to figure it out with 0 skill, 10 for
		// one level skill, and 5 for 2 levels skill.
		if (rand_chance(chance))
		{
		    weapon->markEnchantKnown();
		    buf = MOB::formatToString("%U <know> %r %Iu better.  ", this, 0, 0, weapon);
		    // This is important enough to be broadcast.
		    msg_announce(gram_capitalize(buf));
		}
	    }
	}
    }
    else
	attack = &glb_attackdefs[glb_mobdefs[myDefinition].attack];
    
    thismob = this;
    
    while (attack)
    {
	mob->receiveAttack(attack, thismob, weapon, 0, 
			    ATTACKSTYLE_MELEE, &didhit, multiplierbonus);

	// Refetch the mob as it may have died, teleported, or what
	// have you...
	if (mob != glbCurLevel->getMob(nx, ny))
	    break;

	// Refetch ourselves, as we may have died in the attack.
	if (thismob != glbCurLevel->getMob(ox, oy))
	    break;

	if (attack->nextattack != ATTACK_NONE)
	    attack = &glb_attackdefs[attack->nextattack];
	else
	    attack = 0;

	// If we have run out of attacks with this weapon, see if
	// we should break it!
	if (!attack && weapon && didhit && 
	    (thismob == glbCurLevel->getMob(ox, oy)))
	{
	    bool		didbreak;
	    
	    didbreak = weapon->doBreak(10, this, this, glbCurLevel, nx, ny);
	    
	    // Don't want to use special abilities of broken weapons :>
	    if (didbreak)
		weapon = 0;
	}

	// If we have run out of attacks with a weapon, see if we
	// should invoke its special skill.
	if (!attack && weapon && didhit &&
	    (thismob == glbCurLevel->getMob(ox, oy)) &&
	    (mob == glbCurLevel->getMob(nx, ny)))
	{
	    // 10% chance of power move.
	    if (skillProc(weapon->getSpecialSkill()) && hasSkill(weapon->getSpecialSkill()))
	    {
		// Perform special attack.
		switch (weapon->getSpecialSkill())
		{
		    case SKILL_WEAPON_BLEEDINGWOUND:
			if (!mob->isBloodless())
			{
			    formatAndReport("%U <stab> deeply!");
			    mob->setTimedIntrinsic(thismob, INTRINSIC_BLEED, rand_range(4,6));
			}
			break;

		    case SKILL_WEAPON_DISARM:
		    {
			weapon = mob->getEquippedItem(ITEMSLOT_RHAND);
			if (weapon)
			{
			    const char *slotname = mob->getSlotName(ITEMSLOT_RHAND);
			    formatAndReportExplicit(
					"%U <knock> %IU from %Mr %B1!",
					mob, weapon, 
					(slotname ? slotname : "grasp"));
			    weapon = mob->dropItem(weapon->getX(), weapon->getY());
			    UT_ASSERT(weapon != 0);
			    if (weapon)
			    {
				glbCurLevel->acquireItem(weapon, mob->getX(), mob->getY(), this);

			    }
			}
			else
			{
			    // Weaponless creaturs are thrown
			    // off balance.
			    formatAndReport("%U <trip> %MU.", mob);
			    mob->setTimedIntrinsic(thismob, INTRINSIC_OFFBALANCE, 3);
			}
			break;
		    }
			
		    case SKILL_WEAPON_KNOCKOUT:
			// Only can knock unconscious conscious critters :>
			if (mob->getSmarts() > SMARTS_NONE &&
			    !mob->hasIntrinsic(INTRINSIC_RESISTSLEEP) &&
			    !mob->hasIntrinsic(INTRINSIC_ASLEEP))
			{
			    formatAndReport("%U <knock> %MU unconscious!", mob);
			    mob->setTimedIntrinsic(thismob, INTRINSIC_ASLEEP, rand_range(6,10));
			}
			break;

		    case SKILL_WEAPON_STUN:
			// We confuse the poor critter.
			if (mob->getSmarts() > SMARTS_NONE)
			{
			    formatAndReport("%U <stun> %MU!", mob);
			    mob->setTimedIntrinsic(thismob, INTRINSIC_CONFUSED, rand_range(3,5));
			}
			break;

		    case SKILL_WEAPON_KNOCKBACK:
			// Knock the target back
			// Infinite mass is set as this is an attack.
			glbCurLevel->knockbackMob(nx, ny, dx, dy, true);
			break;

		    case SKILL_WEAPON_IMPALE:
		    {
			// We drive our weapon straight through the
			// creature, possibly impaling whoever is standing
			// behind!
			MOB		*nextvictim = 0;
			int		 nnx, nny;

			nnx = nx + SIGN(dx);
			nny = ny + SIGN(dy);

			if (nnx >= 0 && nnx < MAP_WIDTH &&
			    nny >= 0 && nny < MAP_HEIGHT)
			{
			    nextvictim = glbCurLevel->getMob(nnx, nny);
			}

			if (nextvictim)
			{
			    formatAndReportExplicit("%U <drive> %r %Iu straight through %MU!", mob, weapon);

			    // Do a second attack..
			    actionAttack(nnx-ox, nny-oy);
			}
			break;
		    }

		    default:
			break;
		}
	    }
	}

	if (!attack && firstweapon &&
	    (thismob == glbCurLevel->getMob(ox, oy)))
	{
	    // Try switching to our second weapon.
	    firstweapon = false;
	    didhit = false;

	    // Get the second weapon
	    weapon = getEquippedItem(ITEMSLOT_LHAND);
	    if (weapon)
	    {
		// Set up our new attack.
		attack = weapon->getAttack();
	    }

	    // See if we pass the chance.
	    if (!rand_chance(getSecondWeaponChance()))
		attack = 0;

	    // We will now go on to the next attack..
	}

	// Check for the chance to proc on some artifact equipment.
	if (!attack && !firstweapon && improvised &&
	    (thismob == glbCurLevel->getMob(ox, oy)))
	{
	    int		procchance = 5;
	    // See if we are eligible to proc.
	    if (hasSkill(SKILL_WEAPON_IMPROVISE))
	    {
		procchance = 20;
	    }
	    improvised = false;

	    ITEMSLOT_NAMES		slot;
	    ITEMSLOT_NAMES		validchoice[NUM_ITEMSLOTS];
	    int				numchoice = 0;
	    ITEM			*item;

	    // The right way would be FOREACH_ITEMSLOT followed
	    // by getEquippedItem, but we happen to know we have
	    // O(n) search inside getEquippedItem, so might as well
	    // make this 8 times faster.
	    for (item = myInventory; item; item = item->getNext())
	    {
		// Ignore not equipped
		if (item->getX())
		    continue;
		
		// Cast over slot number
		slot = (ITEMSLOT_NAMES) item->getY();
	    
		switch (slot)
		{
		    // The shield is only valid if it is a
		    // shield.  Artifact shields can get double
		    // weapon proc by both this code path
		    // and the two handed code path.
		    case ITEMSLOT_LHAND:
		    {
			if (!item->isShield())
			    break;
			// FALL THROUGH
		    }
		    case ITEMSLOT_HEAD:
		    case ITEMSLOT_BODY:
		    case ITEMSLOT_RRING:
		    case ITEMSLOT_LRING:
		    case ITEMSLOT_FEET:
		    {
			const ATTACK_DEF	*attack;

			// Check that the attack is something
			// weird.
			attack = item->getAttack();
			if (attack == &glb_attackdefs[ATTACK_MISUSED] ||
			    attack == &glb_attackdefs[ATTACK_MISUSED_BUTWEAPON] ||
			    attack == &glb_attackdefs[ATTACK_MISTHROWN])
			{
			    break;
			}

			// Valid choice!
			validchoice[numchoice++] = slot;
			break;
		    }
		    
		    // I really can't think of a way that the
		    // amulet can be used.    
		    case ITEMSLOT_AMULET:
		    // The right hand is covered by normal combat.    
		    case ITEMSLOT_RHAND:
		    // This ensures we cover all valid code paths
		    case NUM_ITEMSLOTS:
			break;
		}
	    }

	    // Given our base procchance of procchance,
	    // and a number of choices of numchoice, we want a combined
	    // choice total.
	    // This is so that the probability of procing with both
	    // a helmet and a shield is greater than with just one.
	    // Our formula is:
	    // 1 - (1 - procchance)^numchoice
	    // This is slightly unfair as we don't allow two attacks
	    // in a single round so you get a penalty for double equipping,
	    // but I'm hoping it is close enough.
	    
	    // First, we handle common & trivial cases.
	    if (!numchoice)
		procchance = 0;
	    else if (numchoice > 1)	// Equality leaves it unchanged.
	    {
		int		negchance, iter, basechance;

		negchance = 100 - procchance;
		// Make base 1024.
		negchance *= 1024;
		negchance += 50;
		negchance /= 100;
		basechance = negchance;
		// Repeatedly exponentiate and normalize the base.
		// Start at 1 as we already have a single chance.
		for (iter = 1; iter < numchoice; iter++)
		{
		    negchance *= basechance;
		    // Remove the 1024 base.
		    negchance >>= 10;
		}
		// Back to base 100
		negchance *= 100;
		negchance += 512;
		negchance >>= 10;
		// And invert...
		procchance = 100 - negchance;
	    }

	    // Choose a random value if possible
	    if (rand_chance(procchance))
	    {
		slot = validchoice[rand_choice(numchoice)];
		ITEM		*weapon;

		weapon = getEquippedItem(slot);
		UT_ASSERT(weapon != 0);

		// Create some pretty text.
		switch (slot)
		{
		    case ITEMSLOT_HEAD:
			formatAndReport("%U head <butt> with %r %Iu.", weapon);
			break;
		    case ITEMSLOT_BODY:
			formatAndReport("%U body <slam> with %r %Iu.", weapon);
			break;
		    case ITEMSLOT_LHAND:
			formatAndReport("%U shield <slam> with %r %Iu.", weapon);
			break;
		    case ITEMSLOT_RRING:
		    case ITEMSLOT_LRING:
			formatAndReport("%U <punch> with %r %Iu.", weapon);
			break;
		    case ITEMSLOT_FEET:
			formatAndReport("%U <kick> with %r %Iu.", weapon);
			break;

		    // These should not occur.
		    case ITEMSLOT_AMULET:
		    case ITEMSLOT_RHAND:
		    case NUM_ITEMSLOTS:
			UT_ASSERT(!"Invalid itemslot selected");
			break;
		}

		// This triggers us to try once more with the
		// new weapon.
		attack = weapon->getAttack();
	    }
	}
    }

    return true;
}

bool
MOB::actionSwap(int dx, int dy)
{
    int		 nx, ny;
    MOB		*mob;
    bool	 doswap, taketurn;

    applyConfusion(dx, dy);

    nx = getX() + dx;
    ny = getY() + dy;

    if (!canMoveDelta(dx, dy, false))
    {
	formatAndReport("%U cannot go there, so cannot swap places.");
	return false;
    }

    if (hasIntrinsic(INTRINSIC_INPIT))
    {
	formatAndReport("%U cannot swap places while in a pit.");
	return false;
    }
    if (hasIntrinsic(INTRINSIC_INTREE))
    {
	formatAndReport("%U cannot swap places while up a tree.");
	return false;
    }
    if (hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	formatAndReport("%U cannot swap places while submerged.");
	return false;
    }
    // Check to see if there is an item in the way.  Note that
    // you can swap places with a boulder!
    if (!canMoveDelta(dx, dy, true))
    {
	ITEM		*boulder;

	boulder = glbCurLevel->getItem(nx, ny);
	if (boulder && boulder->getDefinition() == ITEM_BOULDER)
	{
	    // Check to see if you have anything equipped.
	    if (hasAnyEquippedItems())
	    {
		formatAndReport("%U <be> wearing too much to swap places with %IU.", boulder);
		return false;
	    }

	    // We can swap with the boulder.
	    // Should be strength check?
	    formatAndReport("%U <swap> positions with %IU.", boulder);

	    // Move the boulder.
	    glbCurLevel->dropItem(boulder);
	    glbCurLevel->acquireItem(boulder, getX(), getY(), this);

	    // Move the avatar.
	    move(nx, ny);

	    // Report what we stepped on...
	    // As both mob & this moved, we report if either of them is
	    // the avatar.
	    // If either died, the MOB::getAvatar should be reset to 0,
	    // so neither of these paths should be taken.
	    if (isAvatar())
	    {
		glbCurLevel->describeSquare(getX(), getY(), 
					    hasIntrinsic(INTRINSIC_BLIND),
					    false);
	    }
	    return true;
	}
	else if (boulder)
	    formatAndReport("%U cannot swap places with %IU.", boulder);
	else
	    formatAndReport("%U cannot go there, so cannot swap places.");

	return false;
    }
    
    // We can only swap with a monster.
    mob = glbCurLevel->getMob(nx, ny);
    if (!mob)
    {
	reportMessage("There is no one there to swap places with.");
	return false;
    }

    // If either ourselves or the MOB is giant sized, we can't swap.
    if (getSize() >= SIZE_GARGANTUAN)
    {
	formatAndReport("%U <be> too big to swap places.");
	return false;
    }
    if (mob->getSize() >= SIZE_GARGANTUAN)
    {
	formatAndReport("%MU <M:be> too big to swap places.", mob);
	return false;
    }

    // Check that the mob can move onto our square.
    if (!mob->canMove(getX(), getY(), true))
    {
	formatAndReport("%MU cannot step here!", mob);
	return false;
    }

    // If either you or the mob is in a pit, you can't swap,
    // no free exits!
    // We check for yourself at the start so we don't allow swapping
    // with boulders to escape.
    if (mob->hasIntrinsic(INTRINSIC_INPIT))
    {
	formatAndReport("%U cannot swap places with %MU because %Mp <M:be> in a pit.", mob);
	return false;
    }
    if (mob->hasIntrinsic(INTRINSIC_INTREE))
    {
	formatAndReport("%U cannot swap places with %MU because %Mp <M:be> up a tree.", mob);
	return false;
    }
    if (mob->hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	formatAndReport("%U cannot swap places with %MU because %Mp <M:be> submerged.", mob);
	return false;
    }

    // If mob is friendly, swap for free.
    // If neutral, swap with strength check
    // If hostile, no go.
    doswap = false;
    taketurn = false;
    switch (mob->getAttitude(this))
    {
	case ATTITUDE_FRIENDLY:
	    doswap = true;
	    taketurn = true;
	    formatAndReport("%U <swap> places with %MU.", mob);
	    break;
	case ATTITUDE_NEUTRAL:
	    taketurn = true;
	    // TODO: Chance of becoming hostile?
	    switch (strengthCheck(mob->getStrength()))
	    {
		case -1:
		    formatAndReport("%U cannot budge %MU.", mob);
		    doswap = false;
		    break;
		case 0:
		    formatAndReport("%U cannot quite shift %MU.", mob);
		    doswap = false;
		    break;
		case 1:
		    formatAndReport("%U <push> %MU out of the way.", mob);
		    doswap = true;
		    break;
		case 2:
		    formatAndReport("%U <muscle> past %MU.", mob);
		    doswap = true;
		    break;
	    }
	    break;

	case ATTITUDE_HOSTILE:
	    taketurn = false;
	    doswap = false;
	    formatAndReport("%U cannot swap places with someone %p <be> fighting!");
	    break;

	case NUM_ATTITUDES:
	    UT_ASSERT(!"UNKNOWN ATTITUDE!");
	    taketurn = false;
	    doswap = false;
	    break;
    }

    if (doswap)
    {
	// Swap...
	// Note either party may die here, but we don't use them
	// herein.
	mob->move(getX(), getY());
	move(nx, ny);
	// Report what we stepped on...
	// As both mob & this moved, we report if either of them is
	// the avatar.
	// If either died, the MOB::getAvatar should be reset to 0,
	// so neither of these paths should be taken.
	if (isAvatar())
	{
	    glbCurLevel->describeSquare(getX(), getY(), 
					hasIntrinsic(INTRINSIC_BLIND),
					false);
	}
	if (mob->isAvatar())
	{
	    glbCurLevel->describeSquare(mob->getX(), mob->getY(), 
					mob->hasIntrinsic(INTRINSIC_BLIND),
					false);
	}
    }
    return taketurn;
}

bool
MOB::actionSleep()
{
    formatAndReport("%U <lie> down to sleep.");

    if (hasIntrinsic(INTRINSIC_RESISTSLEEP))
    {
	formatAndReport("%U <stay> wide awake!");
	return true;
    }

    // Set our sleep flag.
    setTimedIntrinsic(this, INTRINSIC_ASLEEP, 50);

    return true;
}

static MAGICTYPE_NAMES
getSudokuType(int idx, unsigned int seed)
{
    int			x, y, i, j;
    MAGICTYPE_NAMES	type;

    int		pattern[4][4] = 
    {
	{ 0, 1,   2, 3 },
	{ 2, 3,   0, 1 },

	{ 1, 0,   3, 2 },
	{ 3, 2,   1, 0 }
    };

    MAGICTYPE_NAMES		reorder[4] =
    { MAGICTYPE_WAND, MAGICTYPE_SCROLL, MAGICTYPE_POTION, MAGICTYPE_RING };

    // Reorder according to reorder pattern.
    for (i = 0; i < 4; i++)
    {
	j = i + (seed % (4 - i));
	seed = rand_wanginthash(seed);
	if (i != j)
	{
	    type = reorder[i];
	    reorder[i] = reorder[j];
	    reorder[j] = type;
	}
    }
    for (y = 0; y < 4; y++)
	for (x = 0; x < 4; x++)
	    pattern[y][x] = reorder[pattern[y][x]];

    // Now, permute our pattern according to the seed.
    if (seed & 1)
    {
	for (y = 0; y < 4; y++)
	{
	    i = pattern[y][0];
	    pattern[y][0] = pattern[y][1];
	    pattern[y][1] = i;
	}
    }
    if (seed & 2)
    {
	for (y = 0; y < 4; y++)
	{
	    i = pattern[y][2];
	    pattern[y][2] = pattern[y][3];
	    pattern[y][3] = i;
	}
    }
    if (seed & 4)
    {
	for (x = 0; x < 4; x++)
	{
	    i = pattern[0][x];
	    pattern[0][x] = pattern[1][x];
	    pattern[1][x] = i;
	}
    }
    if (seed & 8)
    {
	for (x = 0; x < 4; x++)
	{
	    i = pattern[2][x];
	    pattern[2][x] = pattern[3][x];
	    pattern[3][x] = i;
	}
    }

    // We should now permute entire squares, but I am drunk now.
    // But, I just realized that swapping squares == flipping on x
    // plus two column flips.
    x = idx & 3;
    y = idx >> 2;

    // Flip/flop 
    if (seed & 16)
	x = 3 - x;
    if (seed & 32)
	y = 3 - y;
    if (seed & 64)
    {
	i = x;
	x = y;
	y = i;
    }

    return (MAGICTYPE_NAMES) pattern[y][x];
}

static int
consumeSudokuKeys(ITEMSTACK &stack, int idx, long seed, int &eaten)
{
    int		 i;
    int		 valid = 0;
    ITEM	*item;
    MAGICTYPE_NAMES	type;
    BUF			buf;

    type = getSudokuType(idx, seed);

    for (i = 0; i < stack.entries(); i++)
    {
	item = stack(i);
	if (item->getMagicType() != type)
	{
	    item->formatAndReport("%U <disappear> in a puff of smoke.");
	    glbCurLevel->dropItem(item);
	    delete item;
	    eaten++;
	}
	else
	{
	    // A valid item, vibrate and set true
	    item->formatAndReport("%U <vibrate>.");
	    valid = 1;
	}
    }

    return valid;
}

bool
MOB::actionMagicDoorEffect(int x, int y)
{
    // Search the level for the sudoku keys (ie, altars...)
    int		numaltar = 0, numused, eaten;
    int		altarx[16], altary[16];
    int		mx, my;
    int		idx;
    ITEMSTACK	items;
    
    for (my = 0; my < MAP_HEIGHT; my++)
    {
	if (numaltar >= 16)
	    break;
	for (mx = 0; mx < MAP_WIDTH; mx++)
	{
	    if (numaltar >= 16)
		break;
	    if (glbCurLevel->getTile(mx, my) == SQUARE_FLOORALTAR)
	    {
		altarx[numaltar] = mx;
		altary[numaltar] = my;
		numaltar++;
	    }
	}
    }

    // If we don't have 16 altars, nothing to do.
    if (numaltar != 16)
    {
	encyc_viewentry("MAGICDOOR", MAGICDOOR_NOTENOUGHDOORS);
	return false;
    }

    // Inspect the contents of each altar.
    numused = 0;
    eaten = 0;
    for (idx = 0; idx < 16; idx++)
    {
	items.clear();
	glbCurLevel->getItemStack(items, altarx[idx], altary[idx]);

	numused += consumeSudokuKeys(items, idx, glbCurLevel->getSeed(), eaten);
    }

    if (numused < 16 && eaten)
    {
	encyc_viewentry("MAGICDOOR", MAGICDOOR_INSUFFICIENTITEMS);
	return true;
    }
    else if (numused < 16)
    {
	encyc_viewentry("MAGICDOOR", MAGICDOOR_NOITEMS);
	return true;
    }

    // Door opens!
    glbCurLevel->setTile(x, y, SQUARE_OPENDOOR);
    encyc_viewentry("MAGICDOOR", MAGICDOOR_SUCCESS);

    return true;
}

bool
MOB::actionOpen(int dx, int dy)
{
    SQUARE_NAMES		tile;
    bool	consumed = false, doopen = false;

    applyConfusion(dx, dy);

    // Now check to see if the square itself is openable.
    tile = glbCurLevel->getTile(getX() + dx, getY() + dy);

    switch (tile)
    {
	case SQUARE_BLOCKEDDOOR:
	    if (!isAvatar())
	    {
		formatAndReport("$U <find> the door latched.");
		break;
	    }
	    // FALL THROUGH
	case SQUARE_DOOR:
	    // Check if we were strong enough.
	    switch (strengthCheck(STRENGTH_HUMAN))
	    {
		case -1:
		    formatAndReport("%U cannot budge the door!");
		    break;
		    
		case 0:
		    formatAndReport("%U cannot quite shift the door.");
		    break;
		    
		case 1:
		    doopen = true;
		    formatAndReport("%U <force> the door open.");
		    break;

		case 2:    
		    doopen = true;
		    formatAndReport("%U <open> the door smoothly.");
		    break;
	    }
	    
	    if (doopen)
	    {
		glbCurLevel->setTile(getX() + dx, getY() + dy, SQUARE_OPENDOOR);
	    }
	    consumed = true;
	    break;
	case SQUARE_OPENDOOR:
	    // Can't reopen...
	    reportMessage("The door is already open.");
	    break;
	case SQUARE_MAGICDOOR:
	    if (!isAvatar())
	    {
		// Non-avatar's can't trigger this door.
		return false;
	    }
	    consumed = actionMagicDoorEffect(getX() + dx, getY() + dy);
	    break;
	default:
	    reportMessage("That is not openable!");
	    break;
    }
    return consumed;
}

bool
MOB::actionClose(int dx, int dy)
{
    SQUARE_NAMES		tile;
    bool	consumed = false, doclose = false;
    MOB		*mob;
    ITEM	*item;

    applyConfusion(dx, dy);

    // Now check to see if the square itself is openable.
    tile = glbCurLevel->getTile(getX() + dx, getY() + dy);

    switch (tile)
    {
	case SQUARE_OPENDOOR:
	    // Check if there is a mob in the way.
	    mob = glbCurLevel->getMob(getX() + dx, getY() + dy);
	    if (mob)
	    {
		formatAndReport("%MU <M:hold> the door open!", mob);
		return true;
	    }

	    // Check to see if there is an item.
	    item = glbCurLevel->getItem(getX() + dx, getY() + dy);
	    if (item)
	    {
		formatAndReport("The door is blocked by %IU.", item);
		return true;
	    }
	    
	    // Check if we were strong enough.
	    // Doors are much easier to close than open.
	    switch (strengthCheck(STRENGTH_WEAK))
	    {
		case -1:
		    formatAndReport("%U cannot budge the door!");
		    break;
		    
		case 0:
		    formatAndReport("%U cannot quite shift the door!");
		    break;
		    
		case 1:
		    doclose = true;
		    formatAndReport("%U <close> the door.");
		    break;

		case 2:    
		    doclose = true;
		    formatAndReport("%U <slam> the door shut.");
		    break;
	    }
	    
	    if (doclose)
	    {
		glbCurLevel->setTile(getX() + dx, getY() + dy, SQUARE_DOOR);
	    }
	    consumed = true;
	    break;
	case SQUARE_BLOCKEDDOOR:
	case SQUARE_DOOR:
	    // Can't reclose...
	    reportMessage("The door is already closed.");
	    break;
	default:
	    reportMessage("That is not closeable!");
	    break;
    }
    return consumed;
}

extern bool glbHasJustSearched;

bool
MOB::actionSearch()
{
    int		 dx, dy;
    bool	 found = false;
    ITEM	*item;
    bool	 shouldreveal = false;

    if (isAvatar())
    {
	glbHasJustSearched = true;
    }

    // We always map everything around us.
    // No need for LOS here.
    // No need for LOS?
    // Really?
    // Even if an NPC decides to invoke actionSearch?
    // Because of, perhaps, wielding a ring of searching?
    // Past-Jeff was stupid.
    // The easy answer is to just make this test if we are the avatar,
    // but I know those people at armoredtactis will be reading this change,
    // so I might as well throw in an exception whereby your pets will
    // be allowed to map the level for you.
    // Not that is particularly useful since they either follow or stay,
    // but some glorious future they may get an explore command?
    if (isAvatar() || hasCommonMaster(MOB::getAvatar()))
	shouldreveal = true;

    if (shouldreveal)
	glbCurLevel->markMapped(getX(), getY(), 1, 1, false);

    // Now, examine every square around us...
    for (dy = -1; dy <= 1; dy++)
    {
	if (getY() + dy < 0 || getY() + dy >= MAP_HEIGHT)
	    continue;
	for (dx = -1; dx <= 1; dx++)
	{
	    if (getX() + dx < 0 || getX() + dx >= MAP_WIDTH)
		continue;

	    // We want to mark any items on this square as
	    // known.
	    // For speed, we just mark the top item as mapped.
	    // Really, we should mark the entire stack...
	    if (shouldreveal)
	    {
		item = glbCurLevel->getItem(getX()+dx, getY()+dy);
		if (item)
		    item->markMapped();
	    }
	    
	    // Don't examine own square, may kill us!
	    if (!dx && !dy)
		continue;

	    found |= searchSquare(getX() + dx, getY() + dy, true);
	}
    }

    // Last to search is our own square, in case we die.
    found |= searchSquare(getX(), getY(), true);

    if (!found)
    {
	// I think this is too noisy so have axed it.
	// NOTE: Not safe to enable it, because we may be dead now!
#if 0
	formatAndReport("%U <search> and find nothing.");
#endif
    }

    // Consume...
    return true;
}

bool
MOB::actionClimb()
{
    SQUARE_NAMES		tile;
    int				trap;

    tile = glbCurLevel->getTile(getX(), getY());

    trap = glb_squaredefs[tile].trap;
    switch ((TRAP_NAMES) trap)
    {
	case TRAP_TELEPORT:
	    return actionClimbDown();
	    
	case TRAP_NONE:
	case TRAP_POISONVENT:
	case TRAP_SMOKEVENT:
	case NUM_TRAPS:
	    break;

	case TRAP_HOLE:
	    return actionClimbDown();

	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	    if (hasIntrinsic(INTRINSIC_INPIT))
		return actionClimbUp();
	    else
		return actionClimbDown();
    }

    // If we are submerged, it is straight forward: climbup.
    if (hasIntrinsic(INTRINSIC_SUBMERGED))
	return actionClimbUp();

    switch (tile)
    {
	case SQUARE_LADDERUP:
	    return actionClimbUp();

	case SQUARE_LADDERDOWN:
	    return actionClimbDown();

	case SQUARE_DIMDOOR:
	    return actionClimbDown();

	// If you are in the relevant liquid, you will have already
	// gone to climb up.
	case SQUARE_LAVA:
	    return actionClimbDown();
	case SQUARE_WATER:
	    return actionClimbDown();
	case SQUARE_ACID:
	    return actionClimbDown();

	case SQUARE_FOREST:
	case SQUARE_FORESTFIRE:
	    if (hasIntrinsic(INTRINSIC_INTREE))
		return actionClimbDown();
	    else
		return actionClimbUp();

	default:
	    formatAndReport("%U <try> to climb thin air!");
	    return false;
    }
}

bool
MOB::actionClimbUp()
{
    SQUARE_NAMES tile;
    int		trap;
    BUF		buf;

    tile = glbCurLevel->getTile(getX(), getY());

    trap = glb_squaredefs[tile].trap;

    switch ((TRAP_NAMES) trap)
    {
	case TRAP_HOLE:
	case TRAP_NONE:
	case TRAP_TELEPORT:
	case TRAP_POISONVENT:
	case TRAP_SMOKEVENT:
	case NUM_TRAPS:
	    break;

	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	{
	    if (hasIntrinsic(INTRINSIC_INPIT))
	    {
		if (canFly())
		{
		    // Trivially escape the pit.
		    formatAndReport("%U <fly> out of the pit.");
		    clearIntrinsic(INTRINSIC_INPIT);
		    return true;
		}
		// Try to climb out...
		makeEquipNoise(ITEMSLOT_FEET);
		if (rand_chance(50))
		{
		    // Success.
		    formatAndReport("%U <climb> out of the pit.");
		    clearIntrinsic(INTRINSIC_INPIT);
		}
		else
		{
		    formatAndReport("%U <slip> and <fall> back into the pit.");
		}
		return true;
	    }
	    else
	    {
		// You are already out of the pit!
		// Will fall through to thin air.
	    }
	}
    }

    if (hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	// Try to swim to the surface...
	int		chance;
	BUF		success;
	bool		haswaterwalk;

	haswaterwalk = hasIntrinsic(INTRINSIC_WATERWALK);

	switch (tile)
	{
	    case SQUARE_LAVA:
		chance = 90;		// Easy to get out of lava.
		buf = formatToString("%U <struggle> in the lava.",
			    this, 0, 0, 0);
		if (haswaterwalk)
		    success = formatToString("%U <step> out of the lava.",
				this, 0, 0, 0);
		else
		    success = formatToString("%U <pull> out of the lava.",
				this, 0, 0, 0);
		if (haswaterwalk)
		    chance = 100;
		break;
	    case SQUARE_WATER:
		chance = 50;		// Harder to keep on top of.
		buf = formatToString("%U <flounder> in the water.",
			    this, 0, 0, 0);
		if (haswaterwalk)
		    success = formatToString("%U <step> to the surface.",
				this, 0, 0, 0);
		else
		    success = formatToString("%U <swim> to the surface.",
				this, 0, 0, 0);
		if (haswaterwalk)
		    chance = 100;
		break;
	    case SQUARE_ACID:
		chance = 50;		// Harder to keep on top of.
		buf = formatToString("%U <flounder> in the acid.",
			    this, 0, 0, 0);
		if (haswaterwalk)
		    success = formatToString("%U <step> to the surface.",
				this, 0, 0, 0);
		else
		    success = formatToString("%U <swim> to the surface.",
				this, 0, 0, 0);
		if (haswaterwalk)
		    chance = 100;
		break;
	    case SQUARE_ICE:
		chance = 0;
		buf = formatToString("%U <pound> on the ice in vain.",
				this, 0, 0, 0);
		break;
	    default:
		// Buried alive.
		chance = 0;
		buf = formatToString("The enclosing rock fastly imprisons %U.",
				this, 0, 0, 0);
		break;
	}

	if (chance && rand_chance(chance))
	{
	    reportMessage(success);
	    clearIntrinsic(INTRINSIC_SUBMERGED);
	}
	else
	{
	    reportMessage(buf);
	}

	return true;
    }

    switch (tile)
    {
	case SQUARE_LADDERUP:
	{
	    // Climb to the given map...
	    MAP		*nextmap;
	    int		 x, y, ox, oy;

	    if (!canFly())
		makeEquipNoise(ITEMSLOT_FEET);

	    // Special case of climbing top stair case...
	    if (glbCurLevel->getDepth() == 1)
	    {
		if (isAvatar())
		{
		    // Determine if we have the heart of baezl'bub...
		    bool		 hasheart = false;
		    
		    hasheart = hasItem(ITEM_BLACKHEART);

		    if (hasheart)
		    {
			formatAndReport("%U <return> to the sweet air of the surface world.");
		    }
		    else
		    {
			formatAndReport("Until %U <have> slain Baezl'bub and "
				        "have his black heart, the surface "
					"world cannot help %p.");
			return true;
		    }
		}
		else
		{
		    // Some critter is trying to escape, they think
		    // better of it.
		    formatAndReport("%U <recoil> from the light of the surface world.");
		    return true;
		}
	    }

	    nextmap = glbCurLevel->getMapUp();
	    if (!nextmap)
	    {
		formatAndReport("%U <bonk> %r head on an invisible barrier.");
		return true;
	    }

	    // See if there is room...
	    if (!nextmap->findTile(SQUARE_LADDERDOWN, x, y) ||
		!nextmap->findCloseTile(x, y, getMoveType()))
	    {
		formatAndReport("%U cannot find room at the end of the ladder.");
		return true;
	    }

	    if (canFly())
		formatAndReport("%U <fly> up the ladder.");
	    else
		formatAndReport("%U <climb> the ladder.");

	    ox = getX();
	    oy = getY();

	    // Move to it...
	    glbCurLevel->unregisterMob(this);
	    if (!move(x, y, true))
		return true;
	    nextmap->registerMob(this);

	    glbCurLevel->chaseMobOnStaircase(SQUARE_LADDERDOWN, 
					     ox, oy, this, nextmap);

	    if (isAvatar())
	    {
		glbSuppressAutoClimb = true;

		MAP::changeCurrentLevel(nextmap);
		// Need to rebuild our screen!  Refreshing will be handled
		// by the message wait code.
		gfx_scrollcenter(getX(), getY());
		glbCurLevel->buildFOV(getX(), getY(), 7, 5);
		if (!hasIntrinsic(INTRINSIC_BLIND))
		    glbCurLevel->markMapped(getX(), getY(), 7, 5);

		glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
	    }
	    
	    return true;
	}

	case SQUARE_FOREST:
	case SQUARE_FORESTFIRE:
	{
	    if (!hasIntrinsic(INTRINSIC_INTREE))
	    {
		if (canFly())
		    formatAndReport("%U <fly> up into the tree.");
		else
		    formatAndReport("%U <climb> up the tree.");

		setIntrinsic(INTRINSIC_INTREE);

		return true;
	    }
	    else
	    {
		formatAndReport("%U <be> already at the top of the tree.");
		return false;
	    }
	}

	default:
	    if (canFly())
		formatAndReport("%U <fly> up and hit the roof.");
	    else
		formatAndReport("%U <try> to climb up thin air!");
	    return false;
    }
}

bool
MOB::actionClimbDown()
{
    SQUARE_NAMES tile;
    TRAP_NAMES	trap;

    tile = glbCurLevel->getTile(getX(), getY());
    
    trap = (TRAP_NAMES) glb_squaredefs[tile].trap;

    switch (trap)
    {
	case TRAP_NONE:
	case TRAP_POISONVENT:
	case TRAP_SMOKEVENT:
	case NUM_TRAPS:
	    break;

	case TRAP_TELEPORT:
	{
	    // Trigger the teleport trap.
	    if (canFly())
		formatAndReport("%U <fly> into the teleporter.");
	    else
		formatAndReport("%U <climb> into the teleporter.");
	    return actionTeleport();
	}

	case TRAP_HOLE:
	{
	    // Climb to the given map...
	    MAP		*nextmap;
	    int		 x, y;

	    makeEquipNoise(ITEMSLOT_FEET);
	    nextmap = glbCurLevel->getMapDown();
	    if (!nextmap || !nextmap->isUnlocked())
	    {
		formatAndReport("%U <be> stopped by an invisible barrier.");
		return true;
	    }

	    // See if there is room...
	    // We just go somewhere random (but moveable!)
	    if (!nextmap->findRandomLoc(x, y, getMoveType(), false,
					getSize() >= SIZE_GARGANTUAN,
					false, false, false, true))
	    {
		formatAndReport("%U <find> the hole blocked.");
		return true;
	    }

	    // Move to it...
	    glbCurLevel->unregisterMob(this);
	    if (!move(x, y, true))
		return true;
	    nextmap->registerMob(this);

	    if (canFly())
		formatAndReport("%U <fly> down the hole.");
	    else
		formatAndReport("%U <jump> into the hole.");

	    if (isAvatar())
	    {
		MAP::changeCurrentLevel(nextmap);
		glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
	    }
	    
	    return true;
	}
	    
	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	{
	    if (hasIntrinsic(INTRINSIC_INPIT))
	    {
		// Already inside the pit, can't dig deeper.
		// Will fall through to scramble on ground.
	    }
	    else
	    {
		makeEquipNoise(ITEMSLOT_FEET);
		// You can safely descend into the pit with
		// no chance of damage.
		formatAndReport("%U carefully <climb> into the pit.");
		setIntrinsic(INTRINSIC_INPIT);

		// Report what we stepped on...
		if (isAvatar())
		{
		    glbCurLevel->describeSquare(getX(), getY(), 
						hasIntrinsic(INTRINSIC_BLIND),
						false);
		}

		return true;
	    }
	}
    }

    switch (tile)
    {
	case SQUARE_LADDERDOWN:
	{
	    // Climb to the given map...
	    MAP		*nextmap;
	    int		 x, y;
	    int		 ox, oy;

	    makeEquipNoise(ITEMSLOT_FEET);
	    nextmap = glbCurLevel->getMapDown();
	    if (!nextmap)
	    {
		formatAndReport("%U <be> stopped by an invisible barrier.");
		return true;
	    }

	    if (!nextmap->isUnlocked())
	    {
		formatAndReport("%U <find> that the ladder is locked.");

		attemptunlock();
		return false;
	    }

	    // See if there is room...
	    if (!nextmap->findTile(SQUARE_LADDERUP, x, y) ||
		!nextmap->findCloseTile(x, y, getMoveType()))
	    {
		formatAndReport("%U cannot find room at the end of the ladder.");
		return true;
	    }

	    if (canFly())
		formatAndReport("%U <fly> down the ladder.");
	    else
		formatAndReport("%U <climb> the ladder.");

	    ox = getX();
	    oy = getY();

	    // Move to it...
	    glbCurLevel->unregisterMob(this);
	    if (!move(x, y, true))
		return true;
	    nextmap->registerMob(this);

	    glbCurLevel->chaseMobOnStaircase(SQUARE_LADDERUP, 
					     ox, oy, this, nextmap);

	    if (isAvatar())
	    {
		glbSuppressAutoClimb = true;

		MAP::changeCurrentLevel(nextmap);
		// Need to rebuild our screen!  Refreshing will be handled
		// by the message wait code.
		gfx_scrollcenter(getX(), getY());
		glbCurLevel->buildFOV(getX(), getY(), 7, 5);
		if (!hasIntrinsic(INTRINSIC_BLIND))
		    glbCurLevel->markMapped(getX(), getY(), 7, 5);

		glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
	    }
	    
	    return true;
	}

	case SQUARE_DIMDOOR:
	{
	    // Climb to the portal map.
	    MAP		*nextmap;
	    int		 x, y;
	    int		 ox, oy;

	    // Make sure you have the dimdoor key!
	    if (!hasItem(ITEM_YRUNE))
	    {
		formatAndReport("The portal refuses to open without the key.");
		return true;
	    }

	    makeEquipNoise(ITEMSLOT_FEET);
	    nextmap = glbCurLevel->getMapBranch();
	    if (!nextmap || !nextmap->isUnlocked())
	    {
		formatAndReport("%U <recoil> from a seeming infinite abyss.");
		return true;
	    }

	    // See if there is room...
	    if (!nextmap->findTile(SQUARE_DIMDOOR, x, y) ||
		!nextmap->findCloseTile(x, y, getMoveType()))
	    {
		formatAndReport("%U cannot find room at the end of the portal.");
		return true;
	    }

	    if (canFly())
		formatAndReport("%U <fly> into the portal.");
	    else
		formatAndReport("%U <climb> into the portal.");

	    ox = getX();
	    oy = getY();

	    // Move to it...
	    glbCurLevel->unregisterMob(this);
	    if (!move(x, y, true))
		return true;
	    nextmap->registerMob(this);

	    glbCurLevel->chaseMobOnStaircase(SQUARE_DIMDOOR, 
					     ox, oy, this, nextmap);

	    if (isAvatar())
	    {
		MAP::changeCurrentLevel(nextmap);
		// Need to rebuild our screen!  Refreshing will be handled
		// by the message wait code.
		gfx_scrollcenter(getX(), getY());
		glbCurLevel->buildFOV(getX(), getY(), 7, 5);
		if (!hasIntrinsic(INTRINSIC_BLIND))
		    glbCurLevel->markMapped(getX(), getY(), 7, 5);

		glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
		BRANCH_NAMES	branch = glbCurLevel->branchName();
		formatAndReport(glb_branchdefs[branch].welcome);
	    }
	    
	    return true;
	}

	case SQUARE_LAVA:
	{
	    formatAndReport("%U <dive> into the lava.");
	    setIntrinsic(INTRINSIC_SUBMERGED);
	    // Report what we stepped on...
	    if (isAvatar())
	    {
		glbCurLevel->describeSquare(getX(), getY(), 
					    hasIntrinsic(INTRINSIC_BLIND),
					    false);
	    }

	    return true;
	}

	case SQUARE_WATER:
	{
	    formatAndReport("%U <dive> into the water.");
	    setIntrinsic(INTRINSIC_SUBMERGED);
	    // Report what we stepped on...
	    if (isAvatar())
	    {
		glbCurLevel->describeSquare(getX(), getY(), 
					    hasIntrinsic(INTRINSIC_BLIND),
					    false);
	    }

	    return true;
	}
	
	case SQUARE_ACID:
	{
	    formatAndReport("%U <dive> into the acid.");
	    setIntrinsic(INTRINSIC_SUBMERGED);
	    // Report what we stepped on...
	    if (isAvatar())
	    {
		glbCurLevel->describeSquare(getX(), getY(), 
					    hasIntrinsic(INTRINSIC_BLIND),
					    false);
	    }

	    return true;
	}

	case SQUARE_FOREST:
	case SQUARE_FORESTFIRE:
	    if (hasIntrinsic(INTRINSIC_INTREE))
	    {
		if (canFly())
		    formatAndReport("%U <fly> down from the tree.");
		else
		    formatAndReport("%U <climb> down the tree.");

		clearIntrinsic(INTRINSIC_INTREE);

		return true;
	    }
	    else
	    {
		// FALLTHROUGH
	    }
	
	default:
	    formatAndReport("%U <scrabble> at the floor!");
	    return false;
    }
}

bool
MOB::actionPickUp(ITEM *item, ITEM **newitem)
{
    BUF		buf;
    bool		 submerged;

    submerged = hasIntrinsic(INTRINSIC_INPIT) || 
		hasIntrinsic(INTRINSIC_SUBMERGED);

    if (hasIntrinsic(INTRINSIC_INTREE))
    {
	// Items never stay in tree branches, so people in tree
	// can't get anything.
	formatAndReport("%U cannot reach %IU.", item);
	return false;
    }

    if (!item)
    {
	formatAndReport("%U <grope> on the floor foolishly!");
	return false;
    }

    // Verify the item is in square.
    if (item->getX() != getX() || item->getY() != getY())
    {
	formatAndReport("%U cannot reach %IU.", item);
	return false;
    }

    // Verify we can reach it.
    if (item->isBelowGrade() != submerged)
    {
	formatAndReport("%U cannot reach %IU.", item);
	return false;
    }

    glbCurLevel->dropItem(item);

    // Try to acquire the item ourselves....
    // We must get the item name first as the item may be deleted by
    // acquireItem.
    BUF		itemname = item->getName();

    if (acquireItem(item, newitem))
    {
	// Success!
	buf = formatToString("%U <pick> up %B1.", this, 0, 0, 0,
		itemname.buffer());
	reportMessage(buf);
    }
    else
    {
	// Drop back on the floor.
	glbCurLevel->acquireItem(item, getX(), getY(), this, newitem);
	buf = formatToString("%U <try> to pick up %B1 and <fail>.", 
		this, 0, 0, 0,
		itemname.buffer());
	reportMessage(buf);

	return false;
    }

    return true;
}

bool
MOB::actionDrop(int ix, int iy)
{
    ITEM		*item, *drop;

    item = getItem(ix, iy);

    // Verify there is an item.
    if (!item)
    {
	formatAndReport("%U <drop> nothing.");
	return true;
    }

    // Verify the item is not equipped.
    if (!ix)
    {
	formatAndReport("%U cannot drop %IU because it equipped.", item);
	return true;
    }

    // Drop the item...
    drop = dropItem(ix, iy);
    UT_ASSERT(drop == item);

    // Report the item dropped before acquiring to the ground,
    // this prevents out of order messages if it then sinks.
    formatAndReport("%U <drop> %IU.", item);

    // Mark the item below grade if we are.
    if (hasIntrinsic(INTRINSIC_INPIT) ||
	hasIntrinsic(INTRINSIC_SUBMERGED))
    {
	drop->markBelowGrade(true);
    }

    // Check if item falls to the ground
    if (hasIntrinsic(INTRINSIC_INTREE))
    {
	formatAndReport("%IU <I:drop> to the ground.", item);
	// Try to break the item
	if (item->doBreak(10, this, 0, glbCurLevel, getX(), getY()))
	    return true;
    }

    glbCurLevel->acquireItem(drop, getX(), getY(), this);
    return true;
}

static int
glb_itemsortcompare(const void *v1, const void *v2)
{
    const ITEM	*i1, *i2;

    i1 = *((const ITEM **) v1);
    i2 = *((const ITEM **) v2);

    // Favorite items always go first.
    if (i1->isFavorite() && !i2->isFavorite())
	return -1;
    if (!i1->isFavorite() && i2->isFavorite())
	return 1;

    if (i1->getItemType() < i2->getItemType())
	return -1;
    if (i1->getItemType() > i2->getItemType())
	return 1;

    // Itemtypes are equal, sort by exact item.
    if (i1->getDefinition() < i2->getDefinition())
	return -1;
    if (i1->getDefinition() > i2->getDefinition())
	return 1;

    int			comp;

    // Compare our item names.
    comp = i1->getName().strcmp(i2->getName());
    if (comp != 0)
	return comp;
    return 0;
}

bool
MOB::actionSort()
{
    formatAndReport("%U <rummage> through %r backpack.");

    // Sort all the items...
    ITEM		*item;
    int			 numitems, i, j;

    // Count how many items we have so we can make a flat array.
    numitems = 0;
    for (item = myInventory; item; item = item->getNext())
    {
	// Only counts if item is not equipped, as we don't sort
	// equipped items.
	if (item->getX() == 0)
	    continue;
	
	numitems++;
    }

    // This is a trivially true path.
    if (!numitems)
	return false;

    ITEM		**flatarray;

    // Create a flat array of all the items.
    flatarray = new ITEM *[numitems];
    
    i = 0;
    for (item = myInventory; item; item = item->getNext())
    {
	// Only counts if item is not equipped, as we don't sort
	// equipped items.
	if (item->getX() == 0)
	    continue;

	flatarray[i] = item;
	i++;
    }

    // Now, we sort the item pointers according to our rules:
    qsort(flatarray, numitems, sizeof(ITEM *), glb_itemsortcompare);

    // Now merge adjacent items.  We rely on identical items
    // getting sorted together.  This likely fails for id vs non id
    // cases.
    // We thus escape this by continuing to look for merge potentials
    // until we get the first item with a different definition.
#if 1
    for (i = 0; i < numitems; i++)
    {
	// If this entry is null, we already merged it
	if (!flatarray[i])
	    continue;

	for (j = i+1; j < numitems; j++)
	{
	    // Skip already merged items.
	    if (!flatarray[j])
		continue;
	    
	    // If the items have different definitions, we must be done.
	    if (flatarray[i]->getDefinition() != flatarray[j]->getDefinition())
		break;

	    // Check to see if we can merge item j onto item i.
	    if (flatarray[i]->canMerge(flatarray[j]))
	    {
		ITEM		*drop;
		
		drop = dropItem(flatarray[j]->getX(), flatarray[j]->getY());
		if (drop != flatarray[j])
		{
		    UT_ASSERT(!"Drop failed!");
		    break;
		}
		else
		{
		    flatarray[i]->mergeStack(flatarray[j]);
		    delete flatarray[j];
		    flatarray[j] = 0;
		}
	    }
	}
    }
#endif

    // Now, using the flat array, assign new item positions for everything.
    j = 0;
    for (i = 0; i < numitems; i++)
    {
	if (flatarray[i])
	{
	    flatarray[i]->setPos( (j / MOBINV_HEIGHT) + 1,
				   j % MOBINV_HEIGHT );
	    j++;
	}
    }

    delete [] flatarray;
    
    return false;
}

bool
MOB::actionEat(int ix, int iy)
{
    ITEM		*item, *drop, *newitem = 0, *stackitem = 0;
    BUF		buf;
    bool		 doeat = false, triedeat = false;
    bool		 skippoison = false;
    int			 foodval = 0;

    item = getItem(ix, iy);

    // Verify there is an item.
    if (!item)
    {
	formatAndReport("%U <eat> thin air.");
	return false;
    }

    // Verify the item is not equipped.
    if (!ix)
    {
	// Exception - you can eat from left and right hands.
	if (iy != ITEMSLOT_LHAND && iy != ITEMSLOT_RHAND)
	{
	    formatAndReport("%U cannot eat %IU because it is equipped.", item);
	    return false;
	}
    }

    if (getHungerLevel() >= HUNGER_FULL)
    {
	formatAndReport("%U <be> too full to eat anything more.");
	return false;
    }

    if (item->getStackCount() > 1)
    {
	stackitem = item;
	item = stackitem->splitStack(1);
    }

    switch (item->getMagicType())
    {
	case MAGICTYPE_POTION:
	{
	    int		magicclass;
	    
	    doeat = true;
	    triedeat = true;

	    // Magic potions have a food value of 10.
	    foodval = 10;
	    
	    newitem = ITEM::create(ITEM_BOTTLE, false, true);

	    // Print the drink message....
	    formatAndReport("%U <drink> %IU.", item);

	    // Get the magic type...
	    magicclass = item->getMagicClass();
	    UT_ASSERT(item->getMagicType() == MAGICTYPE_POTION);

	    // If it is ided on drinking, make it so!
	    // If it is ided on drinking, also id it if the avatar
	    // can see the drinker.
	    if (glb_potiondefs[magicclass].autoid && 
		(isAvatar() ||
		 MOB::getAvatar() && MOB::getAvatar()->canSense(this)))
		item->markIdentified();
		
	    // Apply any bonus effects due to it being an artifact.
	    if (item->isArtifact())
	    {
		const ARTIFACT		*art;
		const char		*intrinsics;
		int  			 min_range, max_range;

		if (item->isBlessed())
		    max_range = 2000;
		else
		    max_range = 500;
		if (item->isCursed())
		    min_range = 2;
		else
		    min_range = 300;

		art = item->getArtifact();
		intrinsics = art->intrinsics;
		while (intrinsics && *intrinsics)
		{
		    setTimedIntrinsic(this,
			    (INTRINSIC_NAMES) *intrinsics,
			    rand_range(min_range, max_range));
		    intrinsics++;
		}
	    }
	    
	    // Apply the effect:
	    switch ((POTION_NAMES) magicclass)
	    {
		case POTION_HEAL:
		    {
			int		heal;

			if (item->isBlessed())
			    heal = rand_dice(1, 10, 10);
			else if (item->isCursed())
			    heal = rand_dice(1, 10, 0);
			else
			    heal = rand_dice(1, 20, 0);

			if (receiveHeal(heal, this))
			{
			    formatAndReport("The %Iu <I:heal> %U.", item);
			    
			    if (isAvatar() || 
				(MOB::getAvatar() && 
				 MOB::getAvatar()->canSense(this)))
				item->markIdentified();
			}
			else
			{
			    // Grant bonus magic...
			    if (item->isBlessed())
				heal = rand_dice(3, 2, -2);
			    else if (item->isCursed())
				heal = 1;
			    else
				heal = rand_dice(2, 2, -1);

			    incrementMaxHP(heal);
			    
			    formatAndReport("%U <feel> more robust.");
			    if (isAvatar() || 
				(MOB::getAvatar() && 
				 MOB::getAvatar()->canSense(this)))
				item->markIdentified();
			}
			break;
		    }

		case POTION_ACID:
		    if (!receiveDamage(ATTACK_ACIDPOTION, 0, item, 0,
					ATTACKSTYLE_MISC))
		    {
			// Do nothing more if we die.
			// TODO: This will then drop the bottle we ate???
			if (stackitem)
			    delete item;
			return true;
		    }
		    break;

		case POTION_BLIND:
		    {
			int		turns;
			
			// You get 3d20 turns of blindness.
			if (item->isBlessed())
			    turns = rand_dice(3, 10, 30);
			else if (item->isCursed())
			    turns = rand_dice(3, 10, 0);
			else
			    turns = rand_dice(3, 20, 0);

			// Determine if we notice anything...
			if (isAvatar() && 
			    !hasIntrinsic(INTRINSIC_BLIND))
			{
			    item->markIdentified();
			}

			setTimedIntrinsic(this, INTRINSIC_BLIND, turns);
		    }
		    break;

		case POTION_POISON:
		    {
			int		turns;
			POISON_NAMES	poison;

			turns = rand_dice(4, 5, 0);
			if (item->isBlessed())
			    poison = POISON_STRONG;
			else if (item->isCursed())
			    poison = POISON_MILD;
			else
			    poison = POISON_NORMAL;

			setTimedIntrinsic(this, (INTRINSIC_NAMES)
				          glb_poisondefs[poison].intrinsic, 
					  turns);
			break;
		    }

		case POTION_CURE:
		    {
			bool		add_resist, cured;

			add_resist = item->isBlessed();
			cured = receiveCure();
			
			if (add_resist)
			{
			    // We grant poison resistance for a 5 + 1d5 turns.
			    formatAndReport("%R metabolism stabilizes.");
			    setTimedIntrinsic(this, INTRINSIC_RESISTPOISON,
						    rand_dice(1, 5, 6));
			}
			if (cured)
			{
			    formatAndReport("The poison is expunged from %R veins.");
			}

			// Check for identification
			if (cured || add_resist)
			{
			    if (isAvatar() || 
				(MOB::getAvatar() && 
				 MOB::getAvatar()->canSense(this)))
				item->markIdentified();
			}
			else
			{
			    formatAndReport("Nothing happens.");
			}
			break;
		    }

		case POTION_GREEKFIRE:
		    {
			formatAndReport("The liquid catches on fire as it leaves the bottle!  Burning liquid covers %U!");

			if (!receiveDamage(ATTACK_GREEKFIREPOTION, 0, item, 0,
					    ATTACKSTYLE_MISC))
			{
			    // Do nothing more if we die.
			    // TODO: Does this get rid of the bottle?
			    if (stackitem)
				delete item;
			    return true;
			}
			break;
		    }

		case POTION_ENLIGHTENMENT:
		    {
			formatAndReport("%U <gain> insight.");

			if (isAvatar())
			{
			    glbShowIntrinsic(this);
			}
			break;
		    }

		case POTION_SMOKE:
		    {
			formatAndReport("Smoke billows out of %IU.", item);

			ITEM::setZapper(this);

			// Identification is handled by the grenadecallback.
			glbCurLevel->fireBall(getX(), getY(), 1, true,
					ITEM::grenadeCallbackStatic,
					item);
			break;
		    }

		case POTION_MANA:
		    {
			int		mana;

			if (item->isBlessed())
			    mana = rand_dice(3, 10, 30);
			else if (item->isCursed())
			    mana = rand_dice(3, 10, 0);
			else
			    mana = rand_dice(3, 20, 0);

			if (receiveMana(mana, this))
			{
			    formatAndReport("The %Iu <I:recharge> %U.", item);
			    
			    if (isAvatar() || 
				(MOB::getAvatar() && 
				 MOB::getAvatar()->canSense(this)))
				item->markIdentified();
			}
			else
			{
			    // Grant bonus magic...
			    if (item->isBlessed())
				mana = rand_dice(3, 2, -2);
			    else if (item->isCursed())
				mana = 1;
			    else
				mana = rand_dice(2, 2, -1);

			    incrementMaxMP(mana);
			    
			    formatAndReport("%U <receive> a boost.");
			    if (isAvatar() || 
				(MOB::getAvatar() && 
				 MOB::getAvatar()->canSense(this)))
				item->markIdentified();
			}
			break;
		    }

		case NUM_POTIONS:
		    UT_ASSERT(!"Unknown potion class!");
		    break;
	    }

	    break;
	}
	
	case MAGICTYPE_SPELLBOOK:
	{
	    SPELLBOOK_NAMES	book = (SPELLBOOK_NAMES) item->getMagicClass();

	    const u8		*spells;
	    int  		 min_range, max_range;

	    if (!canDigest(item) || glb_itemdefs[item->getDefinition()].isquest)
	    {
		// If you can't actually eat the book, you shouldn't
		// gain the bonus.
		break;
	    }
	    
	    if (item->isBlessed())
		max_range = 200;
	    else
		max_range = 80;
	    if (item->isCursed())
		min_range = 2;
	    else
		min_range = 30;

	    min_range *= item->getCharges();
	    max_range *= item->getCharges();

	    spells = (const u8 *) glb_spellbookdefs[book].spells;
	    while (spells && *spells)
	    {
		setTimedIntrinsic(this,
			(INTRINSIC_NAMES) glb_spelldefs[*spells].intrinsic,
			rand_range(min_range, max_range));
		spells++;
	    }
	    spells = (const u8 *) glb_spellbookdefs[book].skills;
	    while (spells && *spells)
	    {
		setTimedIntrinsic(this,
			(INTRINSIC_NAMES) glb_skilldefs[*spells].intrinsic,
			rand_range(min_range, max_range));
		spells++;
	    }
	    break;
	}

	case MAGICTYPE_NONE:
	case MAGICTYPE_SCROLL:
	case MAGICTYPE_RING:
	case MAGICTYPE_HELM:
	case MAGICTYPE_WAND:
	case MAGICTYPE_AMULET:
	case MAGICTYPE_BOOTS:
	case MAGICTYPE_STAFF:
	    break;
	
	case NUM_MAGICTYPES:
	    UT_ASSERT(!"Invalid magic type");
	    break;
    }

    switch (item->getDefinition())
    {
	case ITEM_WATER:
	{
	    doeat = true;
	    triedeat = true;
	    foodval = 1;
	    
	    newitem = ITEM::create(ITEM_BOTTLE, false, true);

	    // Print the drink message....
	    formatAndReport("%U <drink> %IU.", item);

	    // Apply any bonus effects due to it being an artifact.
	    if (item->isArtifact())
	    {
		const ARTIFACT		*art;
		const char		*intrinsics;
		int  			 min_range, max_range;

		if (item->isBlessed())
		    max_range = 2000;
		else
		    max_range = 500;
		if (item->isCursed())
		    min_range = 2;
		else
		    min_range = 300;

		art = item->getArtifact();
		intrinsics = art->intrinsics;
		while (intrinsics && *intrinsics)
		{
		    setTimedIntrinsic(this,
			    (INTRINSIC_NAMES) *intrinsics,
			    rand_range(min_range, max_range));
		    intrinsics++;
		}
	    }

	    // If you are undead, should not be a pleasant experience.
	    if (item->isBlessed() && getMobType() == MOBTYPE_UNDEAD)
	    {
		if (!receiveDamage(ATTACK_ACIDPOTION, 0, item, 0,
				ATTACKSTYLE_MISC))
		{
		    // Do nothing more if we die.
		    // TODO: This will then drop the bottle we ate???
		    if (stackitem)
			delete item;
		    return true;
		}
	    }
	    break;
	}

	case ITEM_BOTTLE:
	    doeat = true;
	    triedeat = true;
	    foodval = 0;
	    formatAndReport("%U <munch> on broken glass.");
	    if (!receiveDamage(ATTACK_GLASSSHARDS, 0, item, 0,
				ATTACKSTYLE_MISC))
	    {
		// Do nothing more if we die.
		// TODO: This will then drop the bottle we ate???
		if (stackitem)
		    delete item;
		return true;
	    }
	    break;

	case ITEM_BLACKHEART:
	    {
		formatAndReport("%U <consider> eating %IU, but sanity prevails.", item);
		doeat = false;
		triedeat = true;
		break;
	    }

	default:
	    break;
    }

    if (!triedeat)
    {
	// Determine if it is the sort of thing we 
	// can digest.
	if (canDigest(item) && item->defn().isquest)
	{
	    triedeat = true;
	    formatAndReport("Powerful magic forces prevent %U from eating %IU.", item);
	}
	else if (canDigest(item))
	{
	    MOB		*corpse;
	    const char	*verb;
	    const char	*verbchoice[] = 
	    { 
		"%U <consume> %IU.", 
		"%U <eat> %IU.", 
		"%U <dine> on %IU.", 
		"%U <munch> on %IU.",
		0
	    };

	    verb = rand_string(verbchoice);
	    
	    triedeat = true;
	    formatAndReport(verb, item);

	    // How much health do we get?  Depends on the item...
	    // Using weight seemed clever at the time, but tiny corpses
	    // like mice have a foodval of 50!
	    // Thus the 10 times multiplier
	    foodval = item->getWeight() * 10;
	    corpse = item->getCorpseMob();
	    if (corpse)
	    {
		bool		isbones;
		INTRINSIC_NAMES	intrinsic;

		isbones = item->getDefinition() == ITEM_BONES;

		foodval = glb_sizedefs[corpse->getSize()].foodval;
		if (isbones)
		    foodval /= 2;

		// Bonus food if you have butchery.
		if (hasSkill(SKILL_BUTCHERY))
		    foodval += foodval/2;

		if (corpse->defn().acidiccorpse)
		{
		    clearIntrinsic(INTRINSIC_STONING);
		    if (!hasIntrinsic(INTRINSIC_RESISTACID) ||
			 hasIntrinsic(INTRINSIC_VULNACID))
		    {
			if (isAvatar())
			{
			    formatAndReport("%r stomach burns!");
			}

			if (!receiveDamage(ATTACK_ACIDICCORPSE, 0, item, 0,
					    ATTACKSTYLE_MISC))
			{
			    // Do nothing more if we die.
			    if (stackitem)
				delete item;
			    return true;
			}
		    }
		}

		for (intrinsic = INTRINSIC_NONE; intrinsic < NUM_INTRINSICS;
		     intrinsic = (INTRINSIC_NAMES) (intrinsic+1))
		{
		    if (INTRINSIC::hasIntrinsic(
				corpse->defn().eatgrant,
				intrinsic))
		    {
			// Don't grant poison if we can butcher
			// properly.
			if (hasSkill(SKILL_BUTCHERY))
			{
			    if (glb_intrinsicdefs[intrinsic].ispoison)
				continue;
			}
			if (hasSkill(SKILL_BUTCHERY) || rand_chance(50))
			{
			    int		duration;

			    // We cap poison duration to something sensible,
			    // most inflicted poison attacks are 5d3, so
			    // we just go with the 5d5
			    if (glb_intrinsicdefs[intrinsic].ispoison)
				duration = rand_dice(5, 5, 0);
			    else
				duration = foodval + rand_choice(foodval);

			    setTimedIntrinsic(this, intrinsic, duration);
			}
		    }
		}

		// If we have the butchery skill, we can also 
		// remove any poison from the surface
		if (hasSkill(SKILL_BUTCHERY))
		    skippoison = true;

		// If the corpse grants magic, add a bonus.
		int 	mpbonus;

		mpbonus = rand_dice(corpse->defn().eatmp);
		if (mpbonus)
		{
		    incrementMaxMP(mpbonus);
		    formatAndReport("%U <receive> a boost.");
		}
	    }
	    
	    doeat = true;
	}
	else
	{
	    formatAndReport("%U cannot digest %IU.", item);
	}
    }

    if (doeat)
    {
	pietyEat(item, foodval);
	if (!skippoison)
	    item->applyPoison(this, this, true);

	// Remove the item from our inventory.
	// If it was stacked, we can just delete item.
	if (stackitem)
	{
	    delete item;
	}
	else
	{
	    drop = dropItem(ix, iy);
	    UT_ASSERT(drop == item);
	    delete drop;
	}

	// Place the result, if any.
	if (newitem)
	{
	    if (!acquireItem(newitem))
	    {
		// We failed to acquire the item.  Likely out of room.
		formatAndReport("%U <discard> %IU on the ground.", newitem);

		glbCurLevel->acquireItem(newitem, getX(), getY(), this);
	    }
	}

	// Feed the mob the foodvalue.
	feed(foodval);
    }
    else
    {
	// Failed to eat.  If we ripped this off a stack,
	// we need to add it back to a stack.
	if (stackitem)
	{
	    stackitem->mergeStack(item);
	    delete item;
	}
    }

    return true;
}

bool
MOB::actionRead(int ix, int iy)
{
    ITEM		*item, *drop, *newitem = 0, *itemstack = 0;
    BUF			 buf;
    bool		 doread = false;
    bool		 costturn = true;

    item = getItem(ix, iy);

    // Verify there is an item.
    if (!item)
    {
	formatAndReport("%U <read> thin air.");
	return false;
    }

    // Check to ensure we are not blind.  Blind people can't read.
    if (hasIntrinsic(INTRINSIC_BLIND))
    {
	formatAndReport("%U cannot read %IU, as %p <be> blind.", item);
	return false;
    }

    // There is no objection to readding equipped scrolls.
    // Perhaps we should give a bonus some day?

    if (item->getStackCount() > 1)
    {
	itemstack = item;
	item = itemstack->splitStack(1);
    }

    // Check if its magic class is scroll..
    if (item->getMagicType() == MAGICTYPE_SCROLL)
    {
	int		magicclass;
	
	doread = true;
	
	// Print the read message...
	formatAndReport("%U <read> %IU.", item);

	// Apply any bonus effects due to it being an artifact.
	if (item->isArtifact())
	{
	    const ARTIFACT		*art;
	    const char		*intrinsics;
	    int  			 min_range, max_range;

	    if (item->isBlessed())
		max_range = 2000;
	    else
		max_range = 500;
	    if (item->isCursed())
		min_range = 2;
	    else
		min_range = 300;

	    art = item->getArtifact();
	    intrinsics = art->intrinsics;
	    while (intrinsics && *intrinsics)
	    {
		setTimedIntrinsic(this,
			(INTRINSIC_NAMES) *intrinsics,
			rand_range(min_range, max_range));
		intrinsics++;
	    }
	}

	magicclass = item->getMagicClass();

	// If it autoids on reading do so.
	if (glb_scrolldefs[magicclass].autoid && 
	    (isAvatar() ||
	     (MOB::getAvatar() && MOB::getAvatar()->canSense(this))) )
	    item->markIdentified();

	// Apply any contact poison.
	item->applyPoison(this, this);

	// Apply the effect:
	switch ((SCROLL_NAMES) magicclass)
	{
	    case SCROLL_ID:
	    {
		formatAndReport("%U <be> more aware of %r possessions!");

		if (isAvatar())
		{
		    // ID all items...
		    ITEM		*id;
		    int		 x, y;

		    for (y = 0; y < MOBINV_HEIGHT; y++)
			for (x = 0; x < MOBINV_WIDTH; x++)
			{
			    id = getItem(x, y);
			    if (id)
				id->markIdentified();
			}
		}
		break;
	    }
	    case SCROLL_REMOVECURSE:
	    {
		if (item->isCursed())
		{
		    receiveCurse(false, false);
		}
		else
		{
		    receiveUncurse(item->isBlessed());
		}
		break;
	    }
	    case SCROLL_FIRE:
	    {
		int		x, y;
		MOB		*oldself;
		MOBREF		myself;
		
		formatAndReport("A pillar of fire engulfs %U!");
		
		x = getX();	y = getY();
		oldself = this;
		myself.setMob(this);
		ourEffectAttack = ATTACK_FIRESCROLL;
		glbCurLevel->fireBall(getX(), getY(), 1, true,
				      areaAttackCBStatic, &myself);

		// If die, return immediately.
		if (oldself != glbCurLevel->getMob(x, y))
		{
		    if (itemstack)
			delete item;
		    return true;
		}
		break;
	    }
	    case SCROLL_LIGHT:
	    {
		int		radius = 5;
		bool		islight = true;
		
		if (item->isCursed())
		{
		    radius = 7;
		    islight = false;
		}
		if (item->isBlessed())
		    radius = 7;

		if (islight)
		{
		    formatAndReport("A warm light spreads from %U.");
		}
		else
		{
		    formatAndReport("An inky blackness spreads from %U.");
		}
			
		// Draw a square radius light.
		glbCurLevel->applyFlag(SQUAREFLAG_LIT, getX(), getY(), radius,
					false, islight);
		break;
	    }
	    case SCROLL_MAP:
	    {
		if (item->isCursed())
		    formatAndReport("%U <be> less aware of %r surroundings.");
		else if (item->isBlessed())
		    formatAndReport("%U <be> very aware of %r surroundings.");
		else
		    formatAndReport("%U <be> more aware of %r surroundings.");
		
		// If blessed, get entire level.
		// If regular, get 10 radius (almost entire level)
		// If cursed, lose level :>
		if (isAvatar())
		{
		    if (item->isCursed())
			glbCurLevel->applyFlag(SQUAREFLAG_MAPPED,
				    getX(), getY(), MAP_WIDTH, false,
				    false);
		    else if (item->isBlessed())
			glbCurLevel->markMapped(getX(), getY(), 
						MAP_WIDTH, MAP_HEIGHT,
						false);
		    else
			glbCurLevel->markMapped(getX(), getY(), 
						10, 10,
						false);
		}
		break;
	    }
	    case SCROLL_HEAL:
	    {
		formatAndReport("%R wounds are healed.");
		receiveHeal(15 - (item->isCursed() * 5) + (item->isBlessed() * 5), this);
		break;
	    }
	    case SCROLL_TELEPORT:
	    {
		actionTeleport(!item->isCursed(), item->isBlessed());
		break;
	    }
	    case SCROLL_ENCHANTARMOUR:
	    {
		int		 enchantment;

		enchantment = 1;
		if (item->isCursed())
		{
		    enchantment = -1;
		}
		if (item->isBlessed())
		{
		    enchantment += rand_choice(2);	// 50% chance of +2.
		}
		    
		receiveArmourEnchant((ITEMSLOT_NAMES) -1, enchantment, true);
		
		break;
	    }
	    case SCROLL_ENCHANTWEAPON:
	    {
		int		 enchantment;

		enchantment = 1;
		if (item->isCursed())
		{
		    enchantment = -1;
		}
		if (item->isBlessed())
		{
		    if (rand_choice(2))
		    {
			enchantment++;
		    }
		}

		receiveWeaponEnchant(enchantment, true);
		break;
	    }
	    case SCROLL_MAKEARTIFACT:
	    {
		ITEM		*item;

		item = getEquippedItem(ITEMSLOT_RHAND);
		if (!item)
		{
		    // Oops!  This was wasted :>
		    formatAndReport("%U <feel> omnipotent!");
		    formatAndReport("The feeling passes.");
		    // Do not consume the scroll!  That is just evil!
		    doread = false;
		    break;
		}

		// We have a valid item to make into an artifact.
		// If it is already an artifact, nothing happens.
		if (item->isArtifact())
		{
		    formatAndReport("%R %IU is already as all-powerful as it can get.", item);
		    // Do not consume the scroll!  That is just evil!
		    doread = false;
		    break;
		}

		// Transform the item into an artifact.
		formatAndReport("Eldritch energies from planes both higher and lower swirl through the air.");
		formatAndReport("They condense into a thick plasma which surrounds %R %Iu!", item);
		formatAndReport("%IU <I:be> now named by the gods!", item);
		
		item->makeArtifact();

		formatAndReport("%Ip <I:be> now known as %IU!", item);

		break;
	    }
	    case NUM_SCROLLS:
	    {
		UT_ASSERT(!"INVALID SCROLL NUMBER!");
		break;
	    }
	}
    }
    else if (item->getMagicType() == MAGICTYPE_SPELLBOOK)
    {
	int		magicclass;
	
	doread = true;
	
	// Print the read message...
	formatAndReport("%U <read> %IU.", item);

	magicclass = item->getMagicClass();

	// Apply any contact poison.
	item->applyPoison(this, this);

	// Ask the user what to read.
	if (!isAvatar())
	{
	    UT_ASSERT(!"Reading not supported for non-avatar!");
	    doread = false;
	}
	else
	{
	    int		 num_options;
	    SKILL_NAMES	 skill = SKILL_NONE;
	    SPELL_NAMES  spell = SPELL_NONE;

	    num_options = strlen(glb_spellbookdefs[magicclass].spells) +
			  strlen(glb_spellbookdefs[magicclass].skills);

	    if (!num_options)
	    {
		// Nothing you can learn from this book!
		doread = false;
	    }
	    else
	    {
		char	**options;
		int	 *enumvalue;
		bool	 *isspell;
		int	  i;

		// Remind the user the number of free slots.
		if (strlen(glb_spellbookdefs[magicclass].spells))
		{
		    buf.sprintf("You have %d free %s %s.",
			    getFreeSpellSlots(),
			    "spell",
			    ((getFreeSpellSlots() == 1) ? "slot" : "slots"));
		}
		else
		{
		    buf.sprintf("You have %d free %s %s.",
			    getFreeSkillSlots(),
			    "skill",
			    ((getFreeSkillSlots() == 1) ? "slot" : "slots"));
		}
		reportMessage(buf);
		
		options = new char *[num_options+2];
		enumvalue = new int[num_options];
		isspell = new bool[num_options];

		const u8	*stuff;

		i = 0;
		for (stuff = (const u8 *)glb_spellbookdefs[magicclass].spells;
		     *stuff;
		     stuff++)
		{
		    // Add a spell.
		    spell = (SPELL_NAMES) *stuff;

		    options[i] = new char [strlen(glb_spelldefs[spell].name) + 5];

		    strcpy(options[i], intrinsicFromWhatLetter((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic));

		    strcat(options[i], glb_spelldefs[spell].name);
		    isspell[i] = true;
		    enumvalue[i] = spell;
		    
		    i++;
		}
		for (stuff = (const u8 *)glb_spellbookdefs[magicclass].skills;
		     *stuff;
		     stuff++)
		{
		    // Add a skill.
		    skill = (SKILL_NAMES) *stuff;

		    options[i] = new char [strlen(glb_skilldefs[skill].name) + 5];
		    strcpy(options[i], intrinsicFromWhatLetter((INTRINSIC_NAMES) glb_skilldefs[skill].intrinsic));

		    strcat(options[i], glb_skilldefs[skill].name);
		    isspell[i] = false;
		    enumvalue[i] = skill;
		    
		    i++;
		}

		UT_ASSERT(i == num_options);

		// Add in the cancel option.
		options[i++] = "cancel";

		// Null terminate
		options[i] = 0;

		// And prompt for choice:
		spell = SPELL_NONE;
		skill = SKILL_NONE;

		bool		tryagain = true;

		while (tryagain)
		{
		    static int		lastchoice = 0;
		    int			choice;
		    int			aorb;

		    tryagain = false;

		    choice = gfx_selectmenu(15, 3, (const char **) options, aorb, lastchoice, true);
		    if (choice == num_options)
			choice = -1;

		    if (choice >= 0)
			lastchoice = choice;

		    if (choice >= 0 &&
			(aorb == BUTTON_SELECT || aorb == BUTTON_X))
		    {
			// Give long description..
			if (isspell[choice])
			    encyc_viewSpellDescription((SPELL_NAMES) enumvalue[choice]);
			else
			    encyc_viewSkillDescription((SKILL_NAMES) enumvalue[choice]);
			tryagain = true;
		    }

		    if (aorb != BUTTON_A)
			choice = -1;

		    if (choice >= 0)
		    {
			if (isspell[choice])
			    spell = (SPELL_NAMES) enumvalue[choice];
			else
			    skill = (SKILL_NAMES) enumvalue[choice];
		    }
		}

		// Delete everything
		for (i = 0; i < num_options; i++)
		{
		    delete [] options[i];
		}
		delete [] options;
		delete [] isspell;
		delete [] enumvalue;
		
		{
		    int		y;
		    for (y = 3; y < 19; y++)
			gfx_cleartextline(y);
		}

		// Give long description..
		// Lets the user know what they are in for.
		const char *name = 0;
		if (spell != SPELL_NONE)
		{
		    encyc_viewSpellDescription(spell);
		    name = glb_spelldefs[spell].name;
		}
		else if (skill != SKILL_NONE)
		{
		    encyc_viewSkillDescription(skill);
		    name = glb_skilldefs[skill].name;
		}

		if (name)
		{

		    BUF		spelltext;

		    spelltext.sprintf("Learn %s?", name);

		    if (!gfx_yesnomenu(spelltext))
		    {
			// A request to cancel
			spell = SPELL_NONE;
			skill = SKILL_NONE;
		    }

		}
		
		// Learn the relevant spell or skill
		doread = false;
		if (spell != SPELL_NONE)
		{
		    if (canLearnSpell(spell, true))
		    {
			doread = true;
			learnSpell(spell);
		    }
		}
		if (skill != SKILL_NONE)
		{
		    if (canLearnSkill(skill, true))
		    {
			doread = true;
			learnSkill(skill);
		    }
		}

		if (spell == SPELL_NONE && skill == SKILL_NONE)
		{
		    // If the user aborts reading the spell book,
		    // ie: just glances to see the titles,
		    // we want to not cost a turn.
		    costturn = false;
		}
	    }
	}

	// Autoid spellbooks...
	// We don't want to markIdentified as we don't insta-id the
	// charges.
	// Because the avatar sees the menu of choices, there is no
	// case in which we *don't* want to identify.
	if (isAvatar())
	    item->markClassKnown();
	
	if (doread)
	{
	    // Use a charge from the spellbook:
	    item->useCharge();
	    // If no charges, fade away.
	    if (item->getCharges())
	    {
		doread = false;
		// Chance of figuring out the number of pages left.
		if (isAvatar() && !item->isKnownCharges())
		{
		    if (!rand_choice(20))
		    {
			item->markChargesKnown();
			formatAndReport("%U <determine> the number of pages in %IU.", item);
		    }
		}
	    }
	    else
	    {
		doread = true;
		formatAndReport("%IU <I:fade> away.", item);
	    }
	}
    }
    else
    {
	// Try to read the specific object type.
	switch (item->getDefinition())
	{
	    default:
		doread = false;
		formatAndReport("%U <look> at %IU but cannot find any writing.", item);
		break;
	}
    }

    if (doread)
    {
	// Remove the item from our inventory.
	if (itemstack)
	{
	    delete item;
	}
	else
	{
	    drop = dropItem(ix, iy);
	    UT_ASSERT(drop == item);
	    delete drop;
	}

	// Place the result, if any.
	if (newitem)
	    acquireItem(newitem);
    }
    else
    {
	// Merge the scroll back if required.
	// Doing a mergeStack will be wrong, as spell books
	// would be falsely merged and fail to decrement
	// their usage.  Instead, we reacquire the item.
	if (itemstack)
	{
	    if (!acquireItem(item))
	    {
		// Not enough room, drop on the ground.
		glbCurLevel->acquireItem(item, getX(), getY(), this);
	    }
	}
    }

    return costturn;
}

bool
MOB::actionDip(int potionx, int potiony, int ix, int iy)
{
    ITEM		*potion, *item, *drop, *newpotion, *newitem;
    ITEM		*potionstack = 0;
    bool		 consumed = false;

    // First, get the items and ensure they exist (and are of the right
    // type)

    potion = getItem(potionx, potiony);
    if (!potion)
    {
	formatAndReport("%U <try> to dip into thin air!");
	return false;
    }

    item = getItem(ix, iy);
    if (!item)
    {
	formatAndReport("%U <dip> nothing into %IU.  Suprisingly, nothing happens!", potion);
	return false;
    }

    // If the potion is the same as the item, bad stuff will happen.
    // Thus we avert that with a humerous message.
    if (item == potion)
    {
	formatAndReport("%IU <I:be> not a klein bottle!", potion);
	return false;
    }

    // If the potion is being wielded, we forbid the dip.
    // This avoids being able to dequip items by dipping stuff into them.
    if (!potionx)
    {
	formatAndReport("%U cannot dip anything into an equipped item.");
	return false;
    }

    // Now, note that we can dip a stack.  However, we always dip
    // into one potion.
    
    // Remove the items from our inventory for the duration of the dip...
    if (potion->getStackCount() > 1)
    {
	potionstack = potion;
	potion = potionstack->splitStack(1);
    }
    else
    {
	drop = dropItem(potionx, potiony);
	UT_ASSERT(drop == potion);
    }
    drop = dropItem(ix, iy);
    UT_ASSERT(drop == item);

    // Do the dip....
    consumed = potion->actionDip(this, item, newpotion, newitem);

    // Acquire the new items!
    // If the actionDip failed to dip and the same potion was returned,
    // this will also implicitly merge it into our stack.  In the
    // case it generates an empty bottle, it will have deleted our
    // split potion for us.
    if (newitem)
    {
	if (!acquireItem(newitem, ix, iy))
	{
	    formatAndReport("Lacking room, %U <drop> %IU", newitem);

	    // Mark the item below grade if we are.
	    if (hasIntrinsic(INTRINSIC_INPIT) ||
		hasIntrinsic(INTRINSIC_SUBMERGED))
	    {
		newitem->markBelowGrade(true);
	    }

	    glbCurLevel->acquireItem(newitem, getX(), getY(), this);
	}
    }
    if (newpotion)
    {
	if (!acquireItem(newpotion))
	{
	    formatAndReport("Lacking room, %U <drop> %IU.", newpotion);

	    // Mark the item below grade if we are.
	    if (hasIntrinsic(INTRINSIC_INPIT) ||
		hasIntrinsic(INTRINSIC_SUBMERGED))
	    {
		newpotion->markBelowGrade(true);
	    }

	    glbCurLevel->acquireItem(newpotion, getX(), getY(), this);
	}
    }

    // If ix == 0, this is an equpped item.  Dipping may thus
    // change what we have equipped, so we must rebuild appearance.
    if (ix == 0)
	rebuildAppearance();

    return consumed;
}

bool
MOB::actionThrow(int ix, int iy, int dx, int dy, int dz)
{
    ITEM		*missile, *launcher, *stack = 0;
    BUF			 buf;
    const ATTACK_DEF	*missileattack;

    applyConfusion(dx, dy, dz);

    missile = getItem(ix, iy);
    
    if (!missile)
    {
	formatAndReport("%U <mime> throwing something.");
	return false;
    }

    // Can't throw items you have equipped.
    if (!ix)
    {
	formatAndReport("%U cannot throw equipped items.");
	return false;
    }

    if (!dx && !dy && !dz)
    {
	formatAndReport("%U <toss> %IU from hand to hand.", missile);
	return false;
    }

    if (missile->getStackCount() > 1)
    {
	stack = missile;
	missile = stack->splitStack(1);
    }
    else
    {
	missile = dropItem(ix, iy);
    }

    // Figure out what our launcher, if any, is.
    launcher = 0;
    
    if (missile->requiresLauncher())
    {
	launcher = getEquippedItem(ITEMSLOT_RHAND);
	if (launcher)
	{
	    if (!missile->canLaunchThis(launcher))
		launcher = 0;
	}
	// Try also our left hand.  Either hand is allowed to have the launcher.
	if (!launcher)
	{
	    launcher = getEquippedItem(ITEMSLOT_LHAND);
	    if (launcher)
	    {
		if (!missile->canLaunchThis(launcher))
		    launcher = 0;
	    }
	}
    }
    
    // If we should have a launcher, but don't, we need to use misused.
    missileattack = missile->getThrownAttack();
    if (missile->requiresLauncher() && !launcher)
	missileattack = &glb_attackdefs[ATTACK_MISTHROWN];

    if (!dz)
    {
	// Throwing outwards.  If this is cursed, we want a chance
	// to fumble.
	if (missile->isCursed() && 
	    (missile->getItemType() == ITEMTYPE_WEAPON ||
	     missile->getItemType() == ITEMTYPE_POTION))
	{
	    // Check we don't have misthrown, don't want to id
	    // swords this way.
	    if (missile->defn().thrownattack != ATTACK_MISTHROWN)
	    {
		if (rand_chance(10))
		{
		    formatAndReport("%U <fumble>!");
		    // Reset throw direction to the ground
		    dx = dy = 0;
		    dz = -1;
		    // If thrown by avatar, note
		    if (isAvatar())
		    {
			missile->markCursedKnown();
			if (stack)
			    stack->markCursedKnown();
		    }
		}
	    }
	}
    }

    if (dz)
    {
	if (dz < 0)
	{
	    formatAndReport("%U <throw> %IU onto the ground.", missile);

	    if (missile->doBreak(10, this, 0, glbCurLevel, getX(), getY()))
		return true;

	    glbCurLevel->acquireItem(missile, getX(), getY(), this);
	    return true;
	}
	else
	{
	    int		dropx, dropy;
	    bool	selflives;
	    
	    // Throwing into the air, it lands on you :>
	    formatAndReport("%U <throw> %IU into the air.", missile);

	    dropx = getX();
	    dropy = getY();

	    formatAndReport("%IU <I:fall> on %A!", missile);

	    selflives = receiveDamage(missileattack, this, missile, launcher,
			    ATTACKSTYLE_THROWN);

	    if (missile->doBreak(10, (selflives ? this : 0), 0, glbCurLevel, dropx, dropy))
		return true;

	    glbCurLevel->acquireItem(missile, dropx, dropy, this);

	    return true;
	}
    }

    if (launcher)
	formatAndReport("%U <fire> %IU.", missile);
    else
	formatAndReport("%U <throw> %IU.", missile);

    bool		ricochet = false;

    // Check for special skill...
    if (missile->defn().specialskill == SKILL_WEAPON_RICOCHET)
    {
	// Requires a launcher
	if (launcher && hasSkill(SKILL_WEAPON_RICOCHET))
	{
	    ricochet = true;
	}
    }

    MOBREF		myself;

    myself.setMob(this);

    glbCurLevel->throwItem(getX(), getY(), dx, dy,
		missile, stack, launcher, 
		this,
		missileattack,
		ricochet);

    // If we died, there is no point worrying about floating back
    // on star squares.
    // This is also necessary if we de-poly with the splash damage.
    MOB		*mob = myself.getMob();
    if (!mob || mob->hasIntrinsic(INTRINSIC_DEAD))
	return true;

    // Check if you are on a star square, in which case you float
    // in opposite direction
    SQUARE_NAMES	tile = glbCurLevel->getTile(mob->getX(), mob->getY());
    if (glb_squaredefs[tile].isstars)
    {
	mob->formatAndReport("%U <float> in the opposite direction.");
	mob->actionWalk(-dx, -dy, true);
    }

    return true;
}

bool
MOB::actionFire(int dx, int dy, int dz)
{
    // Find an item in the quiver.
    ITEM		*ammo;

    for (ammo = myInventory; ammo; ammo = ammo->getNext())
    {
	// Ignore worn items.
	if (ammo->getX() == 0)
	    continue;

	if (ammo->isQuivered())
	    break;
    }

    if (!ammo)
    {
	formatAndReport("%U <have> nothing quivered.");
	return false;
    }

    // We succeeded!  Throw!
    return actionThrow(ammo->getX(), ammo->getY(), dx, dy, dz);
}

bool
MOB::actionQuiver(int ix, int iy)
{
    ITEM	*ammo;

    ammo = getItem(ix, iy);

    if (!ammo)
    {
	formatAndReport("%U <quiver> empty air!");
	return false;
    }

    // Toggle state.
    if (ammo->isQuivered())
    {
	formatAndReport("%U <dequiver> %IU.", ammo);
	ammo->markQuivered(false);
    }
    else
    {
	formatAndReport("%U <quiver> %IU.", ammo);
	ammo->markQuivered(true);
    }
    return false;
}

bool
MOB::ableToZap(int ix, int iy, int dx, int dy, int dz, bool quiet)
{
    ITEM		*wand;

    wand = getItem(ix, iy);
    if (!wand)
    {
	if (!quiet)
	    formatAndReport("%U <try> to zap nothing!");

	return false;
    }

    return true;
}

bool
MOB::actionZap(int ix, int iy, int dx, int dy, int dz)
{
    ITEM		*wand;
    bool		 consumed = false;

    applyConfusion(dx, dy, dz);

    // First, get the items and ensure they exist (and are of the right
    // type)
    if (!ableToZap(ix, iy, dx, dy, dz, false))
	return false;

    wand = getItem(ix, iy);

    // If the wand was in a stack, unpeel it from the stack before zapping.
    if (wand->getStackCount() > 1)
    {
	int		sx, sy;
	
	if (findItemSlot(sx, sy))
	{
	    wand = wand->splitStack();
	    acquireItem(wand, sx, sy);
	}
	else
	{
	    formatAndReport("%U <have> no free space!");
	    return false;
	}
    }

    // Do the zap....
    consumed = wand->actionZap(this, dx, dy, dz);

    return consumed;
}

void
MOB::cancelSpell(SPELL_NAMES spell)
{
    // Un-Pay the spell cost...
    myHP += glb_spelldefs[spell].hpcost;
    myMP += glb_spelldefs[spell].mpcost;
    myExp += glb_spelldefs[spell].xpcost;
}

bool
MOB::actionCast(SPELL_NAMES spell, int dx, int dy, int dz)
{
    MOB		*target;
    BUF		buf;
    int		 i, n;
    
    const char	*directionflavour = 0;
    bool	 targetself = false;
    bool	 istargetlit = false;

    applyConfusion(dx, dy, dz);

    // Targetted square.
    int		tx = getX() + dx;
    int		ty = getY() + dy;
    MOBREF	myself;

    myself.setMob(this);

    tx &= MAP_WIDTH-1;
    ty &= MAP_HEIGHT-1;

    // Determine if our target is lit...
    if ((abs(dx) <= 1 && abs(dy) <= 1) ||
	glbCurLevel->isLit(tx, ty))
    {
	istargetlit = true;
    }
    
    // TODO: Sanity test...

    // Text for casting the spell...
    buf = formatToString("%U <cast> %B1.",
		this, 0, 0, 0,
		glb_spelldefs[spell].name);
    reportMessage(buf);

    // Pay the spell cost...
    myHP -= glb_spelldefs[spell].hpcost;
    myMP -= glb_spelldefs[spell].mpcost;
    myExp -= glb_spelldefs[spell].xpcost;

    target = glbCurLevel->getMob(tx, ty);

    if (!dx && !dy && !dz)
    {
	targetself = true;
	directionflavour = getReflexive();
    }
    if (dz)
    {
	if (dz < 0)
	    directionflavour = "the floor";
	else
	    directionflavour = "the ceiling";
    }

    // Switch on spell...
    switch (spell)
    {
	case SPELL_NONE:
	{
	    formatAndReport("%U <gesture> in vain.");
	    return true;
	}
	case SPELL_FLASH:
	    // Even if no creature was targeted, the square
	    // hit becomes lit.
	    glbCurLevel->setFlag(tx, ty, SQUAREFLAG_LIT, true);
	    if (!target)
	    {
		formatAndReport("%U <illuminate> empty air.");
		return true;
	    }

	    formatAndReport("%U <direct> a blinding flash at %MU.", target);

	    pietyCastSpell(spell);
	    target->receiveDamage(ATTACK_SPELL_FLASH, this, 0, 0, ATTACKSTYLE_SPELL);
	    return true;

	case SPELL_STICKYFLAMES:
	    if (!target)
	    {
		formatAndReport("%U <cast> at thin air.");
		return true;
	    }

	    formatAndReport("%U <set> %MU alight.", target);

	    target->setTimedIntrinsic(this, 
				INTRINSIC_AFLAME, rand_dice(3, 2, 3));

	    pietyCastSpell(spell);
	    
	    return true;

	case SPELL_CHILL:
	    if (!target)
	    {
		formatAndReport("%U <cast> at thin air.");
		return true;
	    }

	    formatAndReport("%U <touch> %MU.", target);

	    pietyCastSpell(spell);
	    target->receiveDamage(ATTACK_SPELL_CHILL, this, 0, 0, ATTACKSTYLE_SPELL);
	    return true;

	case SPELL_SPARK:
	    formatAndReport("Electrical sparks fly from %U.");
	    pietyCastSpell(spell);

	    ourEffectAttack = ATTACK_SPELL_SPARK;
	    glbCurLevel->fireBall(getX(), getY(), 1, false,
				areaAttackCBStatic, &myself);
	    return true;

	case SPELL_MAGICMISSILE:
	    formatAndReport("%U <send> a magic missile from %r fingertip.");

	    ourZapSpell = spell;
	    ourFireBallCount = getMagicDie();
	    
	    pietyCastSpell(spell);

	    if (targetself || dz)
	    {
		if (dz)
		    reportMessage("The ray bounces.");
		zapCallbackStatic(getX(), getY(), false, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 5 + getMagicDie(),
				 MOVE_STD_FLY, true,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;

	case SPELL_ACIDSPLASH:
	    if (!target)
	    {
		formatAndReport("%U <cast> at thin air.");
		return true;
	    }

	    formatAndReport("%U <spray> corrosive acid.");
	    pietyCastSpell(spell);
	    target->receiveDamage(ATTACK_SPELL_ACIDSPLASH, this, 
				  0, 0, ATTACKSTYLE_SPELL);

	    // Destroy a random inventory item.
	    // If we targeted ourself, we may be dead, however.
	    if ((myself.getMob() == this) && !hasIntrinsic(INTRINSIC_DEAD))
	    {
		int		 ix, iy;
		ITEM		*item;

		ix = rand_range(0, MOBINV_WIDTH-1);
		iy = rand_range(0, MOBINV_HEIGHT-1);
		item = getItem(ix, iy);

		if (item)
		{
		    formatAndReport("The backsplash drenches %r %Iu!", item);

		    if (item->canDissolve())
		    {
			// Peel off one instance if stacked, otherwise
			// take the whole thing.
			if (item->getStackCount() > 1)
			{
			    item = item->splitStack(1);
			}
			else
			{
			    item = dropItem(item->getX(), item->getY());
			}
			formatAndReport("%IU <I:be> dissolved.", item);

			delete item;

			// Rebuild our appearance.
			if (!ix)
			    rebuildAppearance();
		    }
		    else if (item->getDefinition() == ITEM_BOTTLE)
		    {
			// Fill bottle with acid!
			formatAndReport("Acid fills %IU.", item);
			item->setDefinition(ITEM::lookupMagicItem(MAGICTYPE_POTION, POTION_ACID));

			// If we can see this happening, we now know
			// what acid potions are.
			if (MOB::getAvatar() && MOB::getAvatar()->canSense(this))
			    item->markClassKnown();
		    }
		    else
		    {
			formatAndReport("%IU <I:be> unaffected.", item);
		    }
		}
	    }
	    return true;

	case SPELL_ACIDICMIST:
	    // Determine if we can see the coords.
	    if (!hasIntrinsic(INTRINSIC_BLIND) &&
		glbCurLevel->canMove(tx, ty, MOVE_STD_FLY) &&
		istargetlit &&
		glbCurLevel->hasLOS(getX(), getY(),
				    tx, ty))
	    {
		pietyCastSpell(spell);
		formatAndReport("%U <condense> an acidic mist.");
	    
		glbCurLevel->setSmoke(tx, ty, SMOKE_ACID, this);
		return true;
	    }

	    // The creature cannot see its destination - acidic mist is
	    // illegal!
	    formatAndReport("%U <concentrate> in vain.");
	    cancelSpell(spell);
	    return true;

	case SPELL_CORROSIVEEXPLOSION:
	    if (!hasIntrinsic(INTRINSIC_BLIND) &&
		glbCurLevel->canMove(tx, ty, MOVE_STD_FLY) &&
		istargetlit &&
		glbCurLevel->hasLOS(getX(), getY(),
				    tx, ty))
	    {
		ITEMSTACK		itemstack;
		int			numitems, i;

		glbCurLevel->getItemStack(itemstack, tx, ty);

		formatAndReport("%U <create> a highly volatile blob of acid."); 

		numitems = 0;
		for (i = 0; i < itemstack.entries(); i++)
		{
		    if (itemstack(i)->canDissolve())
		    {
			const char *flavour[] =
			{
			    "devours", 
			    "consumes", 
			    "dissolves", 
			    "eats",
			    0
			};
			BUF		itemname = itemstack(i)->getName();
			// Stacks of arrows become very powerful  :>
			numitems += itemstack(i)->getStackCount();
			buf.sprintf("The blob %s %s.",
				    rand_string(flavour),
				    itemname.buffer());
			glbCurLevel->reportMessage(buf, tx, ty);
			glbCurLevel->dropItem(itemstack(i));
			delete itemstack(i);
		    }
		}

		if (!numitems)
		{
		    glbCurLevel->reportMessage("With nothing to sustain it, the blob fizzles away.", tx, ty);
		    return true;
		}

		// Successful cast!
		pietyCastSpell(spell);

		// Explosion!!!
		int		radius;
		int		reps;
		radius = numitems - 1;
		if (radius > 10)
		    radius = 10;
		reps = (numitems / 4) + 1;

		for (i = 0; i < reps; i++)
		{
		    glbCurLevel->reportMessage("The acid blob explodes violently!", tx, ty);
		    ourEffectAttack = ATTACK_SPELL_CORROSIVEEXPLOSION;
		    glbCurLevel->fireBall(tx, ty, radius, true,
					areaAttackCBStatic, &myself);
		    // See if we are done.
		    if (reps > 1 && (i == reps-1))
			glbCurLevel->reportMessage("The acid blob has dissipated.", tx, ty);
		}

		return true;
	    }
	    // The creature cannot see its destination - corrosive explosion is
	    // illegal!
	    // (I had a very promising character die to a potion of blindness
	    // on Klaskov's level when he had leap-attacked into the middle
	    // of the orcs in preparation for a massive corrosive
	    // explosion...)
	    formatAndReport("%U <concentrate> in vain.");
	    cancelSpell(spell);
	    return true;

	case SPELL_ACIDPOOL:
	    if (!hasIntrinsic(INTRINSIC_BLIND) &&
		istargetlit &&
		glbCurLevel->hasLOS(getX(), getY(),
				    tx, ty))
	    {
		formatAndReport("%U <summon> a fountain of acid.");
		pietyCastSpell(spell);
		glbCurLevel->floodSquare(SQUARE_ACID, tx, ty, this);
		return true;
	    }
	    
	    // Creature can't see where to put the pool, not possible.
	    formatAndReport("%U <concentrate> in vain.");
	    cancelSpell(spell);
		
	    return true;

	case SPELL_MINDACID:
	{
	    ITEM		*item;
	    
	    // Determine if we have hands.
	    if (!getSlotName(ITEMSLOT_RHAND))
	    {
		formatAndReport("A blob of mind-acid appears beside %U and then dissipates.");
		return true;
	    }

	    // Determine if we can disarm the main weapon.
	    if (getEquippedItem(ITEMSLOT_RHAND))
	    {
		actionDequip(ITEMSLOT_RHAND);
		item = getEquippedItem(ITEMSLOT_RHAND);
		if (item)
		{
		    // Failed to dequip, likely due to curse or
		    // out of inventory.
		    formatAndReport("%R mind acid is blocked by %r %Iu.", item);
		    return true;
		}
	    }

	    pietyCastSpell(spell);

	    // The right hand is now free.  Create our mind-acid
	    // object.
	    // We do not want artifact mind acid!
	    item = ITEM::create(ITEM_MINDACIDHAND, false);

	    // 5 turns to dispell it.
	    item->addCharges(5);
	    // We don't want to have an enchanted hand, as that
	    // may create a hand that does no damage.
	    item->enchant(-item->getEnchantment());
	    // Let user count down the charges.
	    item->markChargesKnown();

	    buf.sprintf("Mind acid coats %s %s.",
			getPossessive(),
			getSlotName(ITEMSLOT_RHAND));
	    reportMessage(buf);
	    acquireItem(item, 0, ITEMSLOT_RHAND);

	    return true;
	}

	case SPELL_DISINTEGRATE:
	{
	    // Much like the hoe of destruction, does half damage to all
	    // foes.
	    if (!target)
	    {
		reportMessage("The air boils as its contituent atoms are rent asunder.");
		return true;
	    }

	    pietyCastSpell(spell);
	    formatAndReport("%MR atoms are rent asunder by %R dread magic.", target);
	    
	    // Build our own attack def.
	    ATTACK_DEF		attack;
	    DICE		dice;
	    attack = glb_attackdefs[ATTACK_SPELL_DISINTEGRATE];

	    dice.myNumdie = 0;
	    dice.mySides = 1;
	    dice.myBonus = (target->getMaxHP() + 1) / 2;
	    attack.damage = dice;

	    target->receiveDamage(&attack, this, 0, 0, ATTACKSTYLE_SPELL);
	    
	    return true;
	}

	case SPELL_FORCEBOLT:
	    formatAndReport("A bolt of force slams forward.");
	    ourZapSpell = spell;
	    pietyCastSpell(spell);

	    if (targetself || dz)
	    {
		if (dz)
		{
		    buf.sprintf("The %s seems unaffected.",
				((dz < 0) ? "floor" : "ceiling"));
		    reportMessage(buf);
		}
		else
		{
		    zapCallbackStatic(getX(), getY(), false, &myself);
		}
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				    dx, dy,
				    1,
				    MOVE_ALL, false,
				    zapCallbackStatic,
				    &myself);
		
		glbCurLevel->knockbackMob(tx, ty, dx, dy, false);
	    }

	    return true;

	case SPELL_FORCEWALL:
	{
	    formatAndReport("A wall of force builds around %U and then slams outwards.");
	    ourZapSpell = spell;

	    pietyCastSpell(spell);

	    // No target, always run in all directions.
	    int			 rad, numwalls;
	    bool		*lastwallstatus;
	    int			 rdx = 0, rdy = 0, i, circle;
	    int			 wdx, wdy, symbol = TILE_VOID;
	    int			 cwdx = 0, cwdy = 0;
	    int			 maxwalls = 25;

	    numwalls = 0;

	    lastwallstatus = new bool[11*11];
	    memset(lastwallstatus, 0, sizeof(bool) * 11 * 11);
	    // The current position of the avatar is marked as
	    // having a force wall to start from.
	    lastwallstatus[5*11+5] = true;
	    for (rad = 1; rad < 6 && numwalls < maxwalls; rad++)
	    {
		// Circle around...
		//        11112
		// 112    4   2
		// 4 2 -> 4   2
		// 433    4   2
		//        43333
		for (circle = 0; circle < 4 && numwalls < maxwalls; circle++)
		{
		    for (i = 0; i < rad * 2 && numwalls < maxwalls; i++)
		    {
			switch (circle)
			{
			    case 0:
				rdy = 5 - rad;
				rdx = 5 - rad + i;
				break;
			    case 1:
				rdx = 5 + rad;
				rdy = 5 - rad + i;
				break;
			    case 2:
				rdx = 5 + rad - i;
				rdy = 5 + rad;
				break;
			    case 3:
				rdx = 5 - rad;
				rdy = 5 + rad - i;
				break;
			}

			// Disable the ray if it can't move into this square.
			if (!glbCurLevel->canRayMove(getX()+rdx-5,
						     getY()+rdy-5,
						     MOVE_STD_FLY,
						     false))
			{
			    continue;
			}

			// rdx, rdy are now our entry into lastwallstatus.
			// We need to check if any previous direction
			// has a wall, and if so, what symbol we should use.
			// TODO: We should bias our direction based
			// on circle!
			symbol = TILE_VOID;
			for (wdy = -1; wdy <= 1; wdy++)
			{
			    for (wdx = -1; wdx <= 1; wdx++)
			    {
				if (!(wdy || wdx))
				    continue;
				// Test if in bounds...
				if (rdx + wdx < 0 || rdx + wdx >= 11)
				    continue;
				if (rdy + wdy < 0 || rdy + wdy >= 11)
				    continue;

				// Valid test, see if there was a wall here!
				if (lastwallstatus[rdx + wdx + (rdy+wdy)*11])
				{
				    // Aesthetics: always replace slashes
				    // with non slashes.
				    if (wdx && wdy)
				    {
					if (symbol != TILE_VOID)
					    continue;
					if (wdx * wdy < 0)
					    symbol = TILE_RAYSLASH;
					else
					    symbol = TILE_RAYBACKSLASH;
				    }
				    else
				    {
					if (wdx)
					    symbol = TILE_RAYPIPE;
					else
					    symbol = TILE_RAYDASH;
				    }
				    cwdx = wdx;
				    cwdy = wdy;
				}
			    }
			}


			if (symbol != TILE_VOID)
			{
			    // We found a direction that is stored in
			    // cwdx, cwdy.  Note this is the *from*
			    // direction.
			    glbCurLevel->fireRay(getX() + rdx - 5 + cwdx, 
				    getY() + rdy - 5 + cwdy,
				    -cwdx, -cwdy,
				    1,
				    MOVE_STD_FLY, false,
				    zapCallbackStatic,
				    &myself);

			    // Knock back mobs so they get hit again!
			    glbCurLevel->knockbackMob(
				    getX() + rdx - 5, 
				    getY() + rdy - 5,
				    -cwdx, -cwdy, 
				    true);
			    
			    // Mark this square as a valid source
			    lastwallstatus[rdx+rdy*11] = true;

			    // Increment number of dudes
			    numwalls++;
			}
		    }
		}
	    }

	    delete [] lastwallstatus;

	    return true;
	}
	    
	case SPELL_FROSTBOLT:
	    formatAndReport("A bolt of ice speeds from %r hands.");
	    ourZapSpell = spell;
	    
	    pietyCastSpell(spell);

	    if (targetself || dz)
	    {
		if (dz)
		    reportMessage("The ray fizzles out.");
		else
		    zapCallbackStatic(getX(), getY(), false, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 4,
				 MOVE_STD_FLY, false,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;
	    
	case SPELL_LIVINGFROST:
	{
	    // Determine if an frost could be there...
	    tx = getX();
	    ty = getY();

	    if (!glbCurLevel->findCloseTile(tx, ty, MOVE_STD_FLY))
	    {
		glbCurLevel->reportMessage(
		    "The air chills briefly.",
		    tx, ty);
		return true;
	    }

	    MOB		*frost;

	    pietyCastSpell(spell);
	    
	    frost = MOB::create(MOB_LIVINGFROST);
	    // Summoned creatures never have inventory.
	    frost->destroyInventory();

	    frost->move(tx, ty, true);

	    glbCurLevel->registerMob(frost);

	    frost->formatAndReport("%U <freeze> into existence!");

	    frost->makeSlaveOf(this);
	    frost->setTimedIntrinsic(this, INTRINSIC_SUMMONED, 8);

	    return true;
	}
	case SPELL_BLIZZARD:
	{
	    bool		candetonate = false;
	    
	    // Determine if the given coordinates are visable..
	    // If we can sense the target by any means, we are allowed
	    // to detonate it!
	    if (target && ((target == this) || canSense(target)))
		candetonate = true;

	    if (!candetonate && 
		(!istargetlit ||
		 !glbCurLevel->hasLOS(getX(), getY(),
				     tx, ty)))
	    {
		// Cannot see the position, illegal.
		formatAndReport("%U <gesture> into the darkness.");
		cancelSpell(spell);
		return true;
	    }

	    // Check if no creature is there.
	    if (!target)
	    {
		formatAndReport("%U <try> to detonate thin air.");
		cancelSpell(spell);
		return true;
	    }

	    // Verify the target is an undead.
	    if (target->getDefinition() != MOB_LIVINGFROST)
	    {
		formatAndReport("%MU <M:be> insufficiently cold to be a source.", target);
		return true;
	    }

	    // Note we do *not* need to command the living frost!
	    // Ice daemons are our friends :>

	    // Phew!  We have a winner...
	    formatAndReport("%U <unravel> the magics that stablize %MU.", target);
	    
	    // Destroy the living frost.
	    glbCurLevel->unregisterMob(target);
	    target->triggerAsDeath(ATTACK_BLIZZARD_SOURCE, this, 0, false);
	    delete target;

	    // Success!
	    pietyCastSpell(spell);

	    // Create the blizzard.
	    glbCurLevel->reportMessage("A large blizzard fills the air with sleet and snow!", tx, ty);
	    ourEffectAttack = ATTACK_SPELL_BLIZZARD;
	    glbCurLevel->fireBall(tx, ty, 3, true, areaAttackCBStatic, &myself);
	    
	    return true;
	}

	case SPELL_DIG:
	    formatAndReport("%U magically <excavate> earth.");
	    pietyCastSpell(spell);

	    if (!dz && (dx || dy) &&
		glbCurLevel->isDiggable(getX()+dx, getY()+dy))
	    {
		MOBREF		myself;
		myself.setMob(this);

		ourEffectAttack = ATTACK_SPELL_SANDBLAST;

		int	angle = rand_dirtoangle(dx, dy);
		int	rdx, rdy;
		int	sx = getX(), sy = getY();
		rand_angletodir(angle+3, rdx, rdy);
		glbCurLevel->fireRay(sx+dx, sy+dy,
				rdx, rdy,
				1,
				MOVE_STD_FLY, false,
				areaAttackCBStatic,
				&myself);
		rand_angletodir(angle+5, rdx, rdy);
		glbCurLevel->fireRay(sx+dx, sy+dy,
				rdx, rdy,
				1,
				MOVE_STD_FLY, false,
				areaAttackCBStatic,
				&myself);
		if (!myself.getMob())
		    return true;
	    }

	    return actionDig(dx, dy, dz, 1, true);

	case SPELL_CREATEPIT:
	    // You can excavate anything you have LOS on, regardless
	    // of sight rays.
	    if (glbCurLevel->hasLOS(getX(), getY(), tx, ty))
	    {
		pietyCastSpell(spell);
		formatAndReport("Dirt flies as %U <focus> %r will.");
	    
		glbCurLevel->digPit(tx, ty, this);
		return true;
	    }

	    // The creature cannot see its destination, excavate
	    // is illegal.
	    formatAndReport("%U <concentrate> in vain.");
	    cancelSpell(spell);
	    return true;

	case SPELL_SANDSTORM:
	{
	    if ((!istargetlit ||
		 !glbCurLevel->hasLOS(getX(), getY(),
				     tx, ty)))
	    {
		// Cannot see the position, illegal.
		formatAndReport("%U <gesture> into the darkness.");
		cancelSpell(spell);
		return true;
	    }

	    // Determine if the given coordinates are valid..
	    if (!glbCurLevel->isDiggable(tx, ty))
	    {
		formatAndReport("There is nothing for %U to build a sandstorm from.");
		cancelSpell(spell);
		return true;
	    }

	    // Phew!  We have a winner...
	    formatAndReport("%U <detonate> the rock and <wrap> it in a cyclone of air.");

	    // Success!
	    pietyCastSpell(spell);

	    // Create the sandstorm.
	    glbCurLevel->reportMessage("A sandstorm fills the air with abrasive dirt!", tx, ty);
	    ourEffectAttack = ATTACK_SPELL_SANDSTORM;
	    int	wx, wy;
	    glbCurLevel->getAmbientWindDirection(wx, wy);
	    glbCurLevel->fireBall(tx+wx, ty+wy, 1, true, areaAttackCBStatic, &myself);

	    // Now dig!
	    glbCurLevel->digSquare(tx, ty, myself.getMob());
	    
	    return true;
	}

	case SPELL_GROWFOREST:
	    // You can grow anywhere you have LOS.
	    if (glbCurLevel->hasLOS(getX(), getY(), tx, ty))
	    {
		pietyCastSpell(spell);
		formatAndReport("Verdant forest springs from the ground.");
		glbCurLevel->growForest(tx, ty, this);
		return true;
	    }

	    // The creature cannot see its destination, excavate
	    // is illegal.
	    formatAndReport("%U <concentrate> in vain.");
	    cancelSpell(spell);
	    return true;

	case SPELL_ANIMATEFOREST:
	    // You can grow anywhere you have LOS.
	    if (glbCurLevel->hasLOS(getX(), getY(), tx, ty))
	    {
		SQUARE_NAMES		 tile;

		tile = glbCurLevel->getTile(tx, ty);
		if (tile == SQUARE_FORESTFIRE)
		{
		    formatAndReport("The flames prevent %R magic from taking effect.");
		    return true;
		}
		if (tile != SQUARE_FOREST)
		{
		    formatAndReport("Without trees to focus on, %R animation magic fizzles.");
		    return true;
		}

		if (glbCurLevel->getMob(tx, ty))
		{
		    formatAndReport("A creature's presences prevents the forest from awakening.");
		    return true;
		}
		
		formatAndReport("The forest comes to life!");
		glbCurLevel->setTile(tx, ty, SQUARE_CORRIDOR);
		MOB		*tree;

		// LIVINGTREE... as opposed to DEADTREE?
		// (This nitpick is made whilst half way between
		// TO and SF at some inordinate altitude)
		tree = MOB::create(MOB_LIVINGTREE);
		tree->destroyInventory();
		tree->move(tx, ty, true);
		glbCurLevel->registerMob(tree);
		tree->makeSlaveOf(this);
		tree->setTimedIntrinsic(this, INTRINSIC_SUMMONED, 30);

		pietyCastSpell(spell);
		return true;
	    }

	    // The creature cannot see its destination, excavate
	    // is illegal.
	    formatAndReport("%U <focus> in vain.");
	    cancelSpell(spell);
	    return true;

	case SPELL_DOWNPOUR:
	    // You can create a downpour where you have LOS.
	    if (glbCurLevel->hasLOS(getX(), getY(), tx, ty))
	    {
		pietyCastSpell(spell);
		formatAndReport("%U <summon> thick clouds that release a torrential downpour.");
		glbCurLevel->downPour(tx, ty, this);
		return true;
	    }

	    // The creature cannot see its destination, excavate
	    // is illegal.
	    formatAndReport("%U <concentrate> in vain.");
	    cancelSpell(spell);
	    return true;

	case SPELL_ROLLINGBOULDER:
	{
	    SQUARE_NAMES		 tile;
	    bool		 	 hadboulder = false;
	    ITEM			*boulder;
	    MOB				*victim;
	    MOB				*rollee = 0;
	    
	    // You need to have LOS on the target wall.
	    if (!glbCurLevel->hasLOS(getX(), getY(), tx, ty))
	    {
		formatAndReport("%U <waste> %r energy.");
		return true;
	    }

	    // Check to see that it is a wall...
	    tile = glbCurLevel->getTile(tx, ty);
	    switch (tile)
	    {
		case SQUARE_EMPTY:
		case SQUARE_WALL:
		case SQUARE_SECRETDOOR:
		    break;
		default:
		    // Check to see if we have a boulder.
		    boulder = glbCurLevel->getItem(tx, ty);
		    if (boulder && boulder->getDefinition() == ITEM_BOULDER)
		    {
			hadboulder = true;
			// Describe that the boulder starts to move..
			glbCurLevel->reportMessage("A large boulder starts to roll.", tx, ty);
			
		    }
		    else
		    {
			// Might be able to roll the monster
			rollee = glbCurLevel->getMob(tx, ty);
			if (!rollee)
			{
			    formatAndReport("%U <try> to get rock from air.");
			    return true;
			}
			if (rollee->getMaterial() != MATERIAL_STONE)
			{
			    formatAndReport("%R magic slips off %MU.", rollee);
			    return true;
			}
			// Pull them out of pits or water
			rollee->clearIntrinsic(INTRINSIC_INPIT);
			rollee->clearIntrinsic(INTRINSIC_SUBMERGED);
		    }
		    break;
	    }

	    pietyCastSpell(spell);
	    
	    // The wall in question turns into a boulder.
	    // Unless there was already a boulder.
	    if (!hadboulder && !rollee)
	    {
		boulder = ITEM::create(ITEM_BOULDER, false, true);
		glbCurLevel->setTile(tx, ty, SQUARE_CORRIDOR);

		// We do the message after the change so we might get
		// a map update...
		glbCurLevel->reportMessage("A large boulder is carved out of the wall.", tx, ty);

		// In case acquiring does something odd..
		glbCurLevel->acquireItem(boulder, tx, ty, this);
	    }

	    // Now, try to roll the boulder.
	    // First, determine the likely direction.
	    int			 bdx, bdy, range;
	    bool		 cansee;
	    MOBREF		 thismob;
	    ITEM		*blocker;
	    int			 oldoverlay;

	    thismob.setMob(this);

	    bdx = getX() - tx;
	    bdy = getY() - ty;

	    // If one is bigger than the other, zero the other.
	    // (ie, usually do horizontal moves)
	    if (abs(bdx) > abs(bdy))
		bdy = 0;
	    else if (abs(bdx) < abs(bdy))
		bdx = 0;

	    bdx = SIGN(bdx);
	    bdy = SIGN(bdy);

	    // If we are casting on ourself (perhaps to escape
	    // entombment?) point randomly.
	    if (!bdx && !bdy)
		rand_direction(bdx, bdy);

	    oldoverlay = -1;

	    for (range = 0; range < 5; range++)
	    {
		// Let the user see the boulder here...
		cansee = glbCurLevel->getFlag(tx, ty, SQUAREFLAG_FOV);

		// Retrieve the boulder...
		if (rollee)
		{
		    MOB *nmob = glbCurLevel->getMob(tx, ty);
		    // Check if died or fell in a hole
		    if (nmob != rollee)
			break;
		    // If they are in a pit or submerged stop them
		    if (rollee->hasIntrinsic(INTRINSIC_INPIT) ||
			rollee->hasIntrinsic(INTRINSIC_SUBMERGED))
			break;
		}
		else
		{
		    boulder = glbCurLevel->getItem(tx, ty);
		    // The boulder may have falling into a pit or what not...
		    if (!boulder || boulder->getDefinition() != ITEM_BOULDER)
			break;
		}

		victim = glbCurLevel->getMob(tx+bdx, ty+bdy);
		blocker = glbCurLevel->getItem(tx+bdx, ty+bdy);
		
		// Verify the boulder can move...
		// Huge creatures just have boulder hit their feet
		// for no damage!
		if (!glbCurLevel->canMove(tx+bdx, ty+bdy, MOVE_STD_FLY) ||
		    (victim && victim->getSize() >= SIZE_GARGANTUAN) ||
		    (blocker && !blocker->isPassable()))
		{
		    if (rollee)
			rollee->formatAndReport("%U <shudder> to a stop.");
		    else
			glbCurLevel->reportMessage("The boulder shudders to a stop.", tx, ty);
		    break;
		}

		// Erase old overlay...
		if (oldoverlay >= 0)
		{
		    gfx_setoverlay(tx, ty, oldoverlay);
		}

		// Move the boulder...
		if (rollee)
		{
		    tx += bdx;
		    ty += bdy;

		    // And put the item in as the new overlay.
		    if (cansee)
		    {
			oldoverlay = gfx_getoverlay(tx, ty);
			gfx_setoverlay(tx, ty, rollee->getTile());
			gfx_sleep(6);
		    }
		    else
			oldoverlay = -1;

		    // Crush the poor person!
		    victim = glbCurLevel->getMob(tx, ty);
		    if (victim)
		    {
			rollee->actionAttack(bdx, bdy, 1);
			break;
		    }
		    if (!rollee->move(tx, ty))
			break;
		}		
		else
		{
		    glbCurLevel->dropItem(boulder);
		    tx += bdx;
		    ty += bdy;

		    // And put the item in as the new overlay.
		    if (cansee)
		    {
			oldoverlay = gfx_getoverlay(tx, ty);
			gfx_setoverlay(tx, ty, TILE_BOULDER);
			gfx_sleep(6);
		    }
		    else
			oldoverlay = -1;

		    // Crush the poor person!
		    victim = glbCurLevel->getMob(tx, ty);
		    if (victim)
		    {
			victim->receiveAttack(ATTACK_ROLLINGBOULDER, 
					    thismob.getMob(), boulder, 0, 
					    ATTACKSTYLE_MISC);
		    }
		    
		    glbCurLevel->acquireItem(boulder, tx, ty, thismob.getMob());
		}
	    }
	    // Clean up...
	    if (oldoverlay >= 0)
	    {
		gfx_setoverlay(tx, ty, oldoverlay);
	    }

	    return true;
	}

	case SPELL_ENTOMB:
	{
	    int		tile;
	    bool	anyeffect = false;
	    
	    for (dy = -1; dy <= 1; dy++)
	    {
		if (ty+dy < 0 || ty+dy >= MAP_HEIGHT)
		    continue;
		for (dx = -1; dx <= 1; dx++)
		{
		    if (tx+dx < 0 || tx+dx >= MAP_HEIGHT)
			continue;

		    if (!dx && !dy)
			continue;
		    
		    // Change this tile into solid rock, if possible.

		    // Can't entomb critters.
		    if (glbCurLevel->getMob(tx + dx, ty + dy))
			continue;

		    tile = glbCurLevel->getTile(tx+dx, ty+dy);

		    if (glb_squaredefs[tile].invulnerable)
			continue;

		    // Set the tile.
		    if (tile != SQUARE_EMPTY)
			anyeffect = true;
		    glbCurLevel->setTile(tx+dx, ty+dy, SQUARE_EMPTY);
		}
	    }

	    if (!anyeffect)
	    {
		formatAndReport("%U <feel> the ground shudder.  Blocked, the rock summoned from the plane of earth returs whence it came.");
	    }
	    else
	    {
		formatAndReport("Solid rock rises from the ground, entombing %U.");
		pietyCastSpell(spell);
	    }

	    return true;
	}
	    
	    
	case SPELL_FIREBALL:
	    ourZapSpell = spell;
	    ourFireBallCount = 0;
	    
	    pietyCastSpell(spell);

	    if (targetself || dz)
	    {
		if (dz)
		{
		    buf.sprintf("The ray hits the %s.", directionflavour);
		    reportMessage(buf);
		}
		zapCallbackStatic(getX(), getY(), true, &myself);
	    }
	    else
	    {
		// After much soul searching, the bouncing fireballs
		// have been axed during a flight over the atlantic, this
		// time on return form IRDC'09 which was held at CERN.
		// For reasons of economy, however, my own departure is a few
		// weeks later and via Paris.
		// Due to this airbus 320 lacking any power outlet, I am
		// at the mercy of the p1610's 5 hours of battery life.
		// Since I also plan on playing some Civ 4, this session
		// must be truncated to fit withing the 8 hour flight.
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 6,
				 MOVE_STD_FLY, false,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;
	    
	case SPELL_LIGHTNINGBOLT:
	    formatAndReport("Lightning flies from %r hand.");
	    
	    pietyCastSpell(spell);

	    ourZapSpell = spell;
	    if (targetself || dz)
	    {
		if (dz)
		{
		    reportMessage("The ray bounces.");
		}
		zapCallbackStatic(getX(), getY(), false, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 6,
				 MOVE_STD_FLY, true,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;
	    
	case SPELL_CHAINLIGHTNING:
	    pietyCastSpell(spell);

	    ourZapSpell = spell;
	    ourFireBallCount = 0;
	    if (targetself || dz)
	    {
		if (dz)
		{
		    reportMessage("The ray bounces.");
		}
		zapCallbackStatic(getX(), getY(), false, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 6,
				 MOVE_STD_FLY, true,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;

	case SPELL_SUNFIRE:
	{
	    int		delay;
	    
	    formatAndReport("%U <talk> to the sun...");

	    if (hasIntrinsic(INTRINSIC_TIRED))
	    {
		formatAndReport("The sun ignores %r prayers!");
		return true;
	    }

	    pietyCastSpell(spell);

	    // Report that the sun answers...
	    formatAndReport("The sun answers %r prayers!");
	    reportMessage("Darkness falls on the surface world as the sun's "
			  "energies are focussed.");

	    reportMessage("A blinding blast of light falls from the roof.");

	    // First we blast everyone in the area with the sunfire
	    // blast.
	    ourEffectAttack = ATTACK_SUNFIREBLAST;
	    glbCurLevel->fireBall(getX(), getY(), 3, false,
				 areaAttackCBStatic, &myself);

	    // Next, melt the rocks.
	    reportMessage("The very rocks melt!");
	    glbCurLevel->fireBall(getX(), getY(), 3, false,
				meltRocksCBStatic, &myself);

	    delay = rand_dice(20, 20, 20);
	    setTimedIntrinsic(this, INTRINSIC_TIRED, delay);
	    return true;
	}

	case SPELL_REGENERATE:
	    if (!target || ((target != this) && !canSense(target)))
	    {
		formatAndReport("%U <cast> at thin air.");
		cancelSpell(spell);
		return true;
	    }

	    // You get 30 + 3d10 turns of REGENERATION.
	    // You get 5 + 1d10 turns of MAGICDRAIN.
	    target->setTimedIntrinsic(this, INTRINSIC_REGENERATION, 
				      rand_dice(3, 10, 30));
	    target->setTimedIntrinsic(this, INTRINSIC_MAGICDRAIN,
				      rand_dice(1, 10, 5));
	    
	    pietyCastSpell(spell);

	    target->formatAndReport("%U <be> surrounded by a healthy aura.");
	    return true;

	case SPELL_HEAL:
	    if (!target || ((target != this) && !canSense(target)))
	    {
		formatAndReport("%U <cast> at thin air.");
		cancelSpell(spell);
		return true;
	    }
	    if (target->receiveHeal(10, this))
	    {
		target->formatAndReport("%R wounds close.");
		
		pietyCastSpell(spell);
	    }
	    else
	    {
		target->formatAndReport("%U <look> no better.");
	    }
	    return true;

	case SPELL_SLOWPOISON:
	    if (!target || ((target != this) && !canSense(target)))
	    {
		formatAndReport("%U <cast> at thin air.");
		cancelSpell(spell);
		return true;
	    }
#if 0
	    if (target->receiveSlowPoison())
	    {
		target->formatAndReport("The poison slows in %R veins.");
		
		pietyCastSpell(spell);
	    }
	    else
	    {
		target->formatAndReport("%U <be> not poisoned.");
	    }
#else
	    // We grant poison resistance for a 5 + 1d5 turns.
	    target->formatAndReport("%R metabolism stabilizes.");
	    target->setTimedIntrinsic(this, INTRINSIC_RESISTPOISON,
				    rand_dice(1, 5, 6));
#endif
	    return true;
	    
	case SPELL_MAJORHEAL:
	    if (!target || ((target != this) && !canSense(target)))
	    {
		formatAndReport("%U <cast> at thin air.");
		cancelSpell(spell);
		return true;
	    }
	    if (target->receiveHeal(30, this))
	    {
		target->formatAndReport("%R wounds close.");
		
		pietyCastSpell(spell);
	    }
	    else
	    {
		target->formatAndReport("%U <look> no better.");
	    }
	    return true;
	    
	case SPELL_CUREPOISON:
	    if (!target || ((target != this) && !canSense(target)))
	    {
		formatAndReport("%U <cast> at thin air.");
		cancelSpell(spell);
		return true;
	    }
	    if (target->receiveCure())
	    {
		formatAndReport("The poison is expunged from %MR veins.", target);
		pietyCastSpell(spell);
	    }
	    else
	    {
		formatAndReport("%M <M:be> not poisoned.", target);
	    }
	    return true;

	case SPELL_RESURRECT:
	    // Determine the corpse that is pointed at...
	    if (target)
	    {
		target->formatAndReport("%U <be> not in need of aid.");
		cancelSpell(spell);
		return true;
	    }

	    // Resurrect all corpses at the target square...
	    if (!glbCurLevel->resurrectCorpses(tx, ty, this))
	    {
		formatAndReport("The stones fail to come to life.");
		return true;
	    }
	    else
		pietyCastSpell(spell);
	    
	    return true;

	case SPELL_SUMMON_FAMILIAR:
	{
	    // Find a location for the familiar
	    tx = getX();
	    ty = getY();

	    if (!glbCurLevel->findCloseTile(tx, ty, MOVE_WALK))
	    {
		glbCurLevel->reportMessage(
		    "A famliar briefly appears before thinking better of it.",
		    tx, ty);
		return true;
	    }

	    MOB		*familiar;

	    familiar = MOB::create(MOB_BAT);

	    // Bats always start with their full possible health.
	    // No reason to be evil with 1 HP bats :>
	    familiar->forceHP(10);

	    // Likewise, they get some magic points to play with
	    familiar->incrementMaxMP(10 - familiar->getMaxMP());
	    
	    // Summoned creatures never have inventory.
	    familiar->destroyInventory();

	    familiar->move(tx, ty, true);

	    glbCurLevel->registerMob(familiar);

	    familiar->formatAndReport("%U <come> to %MR call!", this);

	    familiar->makeSlaveOf(this);
	    familiar->setIntrinsic(INTRINSIC_FAMILIAR);

	    pietyCastSpell(spell);
	    
	    return true;
	}

	case SPELL_TRANSFER_KNOWLEDGE:
	{
	    if (!target || !canSense(target))
	    {
		// Unable to transfer to destination.
		formatAndReport("%U watch helplessly as %R hard earned experience evaporates.");
		return true;
	    }
	    target->formatAndReport("%U <be> wiser.");
	    // Thanks to the pyramid scheme, you will get half the experience
	    // back. This might thus be too powerful.
	    target->receiveExp(250);

	    pietyCastSpell(spell);
	    return true;
	}

	case SPELL_FETCH:
	{
	    bool		somethinghappened = false;
	    int			i;
	    ITEM		*item;

	    // You don't need visible coordinates.
	    // You do need to either recall the item at the coordinates
	    // or sense the creature at the coordinates.
	    if (target && canSense(target))
	    {
		int		fx, fy;

		// Summon the creature to your presence!
		formatAndReport("%U <summon> %MU.", target);

		// We also want the summoned creature to know what happened.
		target->formatAndReport("%U <be> summoned to %MR presence.", this);

		// Find the closest valid square to us.
		fx = getX();
		fy = getY();
		glbCurLevel->findCloseTile(fx, fy, target->getMoveType());

		target->actionTeleport(false, true, fx, fy);
		somethinghappened = true;
	    }

	    ITEMSTACK	stack;

	    // Return a list of all mapped items.
	    glbCurLevel->getItemStack(stack, tx, ty, -1, true);
	    for (i = 0; i < stack.entries(); i++)
	    {
		item = stack(i);
		if (item->hasIntrinsic(INTRINSIC_TELEFIXED))
		{
		    item->formatAndReport("%U <shudder>.");
		    continue;
		}
		formatAndReport("%U <fetch> %IU.", item);
		glbCurLevel->dropItem(item);
		glbCurLevel->acquireItem(item, getX(), getY(), this);
		somethinghappened = true;
	    }

	    if (!somethinghappened)
	    {
		formatAndReport("%R summons go unheeded.");
	    }
	    else
	    {
		pietyCastSpell(spell);
	    }
	    return true;
	}


	case SPELL_BLINK:
	    // Determine if the given coordinates are visable..
	    if (!hasIntrinsic(INTRINSIC_BLIND) &&
		istargetlit &&
		glbCurLevel->hasLOS(getX(), getY(),
				    tx, ty))
	    {
		if (target && target != this)
		{
		    formatAndReport("%U <stare> at %MU.", target);
		    return true;
		}
		else
		{
		    pietyCastSpell(spell);
		    return actionTeleport(false, true, 
					  tx, ty);
		}
	    }

	    // The creature cannot see its destination - blink is
	    // illegal!
	    formatAndReport("%U <stare> into darkness.");
	    cancelSpell(spell);
	    return true;

	case SPELL_FLAMESTRIKE:
	    if (target && ((target == this) || canSense(target)))
	    {
		formatAndReport("%U <call> on %r god's power!");

		pietyCastSpell(spell);
		if (isAvatar())
		{
		    // Compute our piety...
		    ATTACK_DEF		attack;
		    memcpy(&attack, &glb_attackdefs[ATTACK_FLAMESTRIKE], sizeof(ATTACK_DEF));
		    // Every doubling of piety adds another die.
		    // Zero or less piety gets no dice
		    // We know we are capped at 10k, or 2^13 for an awesome
		    // 13d6 attack.
		    int		piety = piety_chosengodspiety();
		    attack.damage.myNumdie = 0;
		    while (piety > 1)
		    {
			attack.damage.myNumdie++;
			piety >>= 1;
		    }
		    target->receiveAttack(&attack, this, 0, 0,
					    ATTACKSTYLE_SPELL);
		}
		else
		    target->receiveAttack(ATTACK_FLAMESTRIKE, this, 0, 0,
					    ATTACKSTYLE_SPELL);
	    }
	    else
	    {
		formatAndReport("%U foolishly <ask> %r god to strike thin air.");
		cancelSpell(spell);
	    }
	    return true;
	    
	case SPELL_TELEPORT:
	    pietyCastSpell(spell);
	    return actionTeleport();

	case SPELL_TELEWITHCONTROL:
	    pietyCastSpell(spell);
	    return actionTeleport(true, true);

	case SPELL_DETECTCURSE:
	{
	    // All items in our inventory have their status marked.
	    // Only get credit if this changed something.
	    if (!isAvatar())
		return true;
	    ITEM		*item;
	    bool		 didid = false;

	    formatAndReport("%U <become> attuned to %r possessions...");
	    
	    for (item = myInventory; item; item = item->getNext())
	    {
		if (!item->isKnownCursed())
		{
		    const char		*colour = "grey";
		    if (item->isBlessed())
			colour = "white";
		    else if (item->isCursed())
			colour = "black";

		    formatAndReport("%R %Iu <I:glow> %B1.", item, colour);
		    item->markCursedKnown();
		    didid = true;
		}
	    }
	    if (didid)
	    {
		pietyCastSpell(spell);
	    }
	    else
	    {
		formatAndReport("%U <sense> nothing new.");
	    }
	    return true;
	}

	case SPELL_IDENTIFY:
	{
	    // This had better be the player.
	    UT_ASSERT(isAvatar());
	    if (!isAvatar())
		return true;

	    // No UI during stress test.
	    if (glbStressTest)
		return true;

	    // Select item to identify.
	    int			selectx, selecty;
	    ITEM		*item = 0;
	    
	    gfx_showinventory(this);
	    gfx_getinvcursor(selectx, selecty);
	    if (gfx_selectinventory(selectx, selecty))
	    {
		item = getItem(selectx, selecty);
	    }

	    gfx_hideinventory();
	    writeGlobalActionBar(true);

	    // We perform the identify after hiding the inventory
	    // so we don't lose the final line of the identification
	    // to the inventory clear.
	    if (item)
	    {
		item->markIdentified();
		formatAndReport("%U <identify> %IU.", item);
		pietyCastSpell(spell);
	    }
	    else
	    {
		// Undo cost...
		cancelSpell(spell);
		return false;
	    }

	    return true;
	}

	case SPELL_LIGHT:
	    formatAndReport("Light spreads out from %U.");

	    pietyCastSpell(spell);
	    glbCurLevel->applyFlag(SQUAREFLAG_LIT,
		    getX() + dx,
		    getY() + dy,
		    5, false, true);
	    return true;

	case SPELL_DARKNESS:
	    formatAndReport("Darkness spreads out from %U.");

	    pietyCastSpell(spell);
	    glbCurLevel->applyFlag(SQUAREFLAG_LIT,
		    getX() + dx,
		    getY() + dy,
		    5, false, false);
	    return true;

	case SPELL_MAGICMAP:
	    // Only applicable to avatar.
	    if (!isAvatar())
		return true;
	    
	    formatAndReport("A map forms in %r mind");
	    pietyCastSpell(spell);
	    glbCurLevel->markMapped(getX() + dx, getY() + dy, 
				    10, 10,
				    false);
	    return true;

	case SPELL_KNOCK:
	    if (!hasIntrinsic(INTRINSIC_BLIND) &&
		istargetlit &&
		glbCurLevel->hasLOS(getX(), getY(),
				    tx, ty))
	    {
		SQUARE_NAMES	tile;

		// Determine if there is a door or secret door
		// in the targetted square...
		tile = glbCurLevel->getTile(tx, ty);

		switch (tile)
		{
		    case SQUARE_OPENDOOR:
		    {
			// Find a dx/dy.
			int	dx, dy;
			MOB	*mob;
			ITEM	*item;

			// Knock is always away from caster
			dx = SIGN(tx - getX());
			dy = SIGN(ty - getY());

			if (glbCurLevel->canMove(tx+dx,ty,MOVE_STD_FLY,false))
			{
			    dy = 0;
			}
			else if (glbCurLevel->canMove(tx,ty+dy,MOVE_STD_FLY,false))
			{
			    dx = 0;
			}
			else
			{
			    dy = dx = 0;
			}
			
			if (dx || dy)
			{
			    // Check to see if a creature or item
			    // blocks the square.
			    mob = glbCurLevel->getMob(tx, ty);
			    if (mob)
			    {
				if (glbCurLevel->knockbackMob(tx, ty, dx, dy, true))
				{
				    mob = glbCurLevel->getMob(tx+dx, ty+dy);
				    if (mob)
					mob->receiveDamage(
						ATTACK_DOORSLAM,
						this,
						0, 0,
						ATTACKSTYLE_SPELL);
				}
			    }
			}

			// Check for any blockers.
			mob = glbCurLevel->getMob(tx, ty);
			if (mob)
			{
			    mob->formatAndReport("%U <hold> the door open!");
			    return true;
			}

			// Check to see if there is an item.
			item = glbCurLevel->getItem(tx, ty);
			if (item)
			{
			    formatAndReport("The door is blocked by %IU.", item);
			    return true;
			}

			reportMessage("The door closes.");
			glbCurLevel->setTile(tx, ty, SQUARE_DOOR);
			pietyCastSpell(spell);
			break;
		    }
		    case SQUARE_DOOR:
		    case SQUARE_BLOCKEDDOOR:
			reportMessage("The door opens.");
			glbCurLevel->setTile(tx, ty, SQUARE_OPENDOOR);
			pietyCastSpell(spell);
			break;
		    case SQUARE_MAGICDOOR:
			reportMessage("A voice booms: \"My test is not so easily bypassed!\"");
			break;
		    case SQUARE_SECRETDOOR:
			reportMessage("A secret door opens.");
			glbCurLevel->setTile(tx, ty, SQUARE_OPENDOOR);
			pietyCastSpell(spell);
			break;

		    default:
			reportMessage("Nothing happens.");
			break;
		}
		return true;
	    }
	    formatAndReport("%U <hear> a distant knock.");
	    return true;

	case SPELL_TRACK:
	    // Determine if there is a creature there that
	    // we can sense.
	    if (target && ((target == this) || canSense(target)))
	    {
		formatAndReport("%U <build> a standing wave of magic.");
		target->formatAndReport("%U <be> revealed to all.");

		target->setTimedIntrinsic(this,
				INTRINSIC_POSITIONREVEALED,
				100);

		pietyCastSpell(spell);
	    }
	    else
	    {
		formatAndReport("%U <try> to track the wind.");
		cancelSpell(spell);
	    }
	    return true;

	case SPELL_DIAGNOSE:
	    // Determine if there is a creature there that
	    // we can sense.
	    if (target && ((target == this) || canSense(target)))
	    {
		formatAndReport("%U <peer> into %MU with your magic.", target);

		target->characterDump(false, false, false);

		pietyCastSpell(spell);
	    }
	    else
	    {
		formatAndReport("%U <find> the empty air in surprisingly good condition.");
		cancelSpell(spell);
	    }
	    return true;

	case SPELL_POSSESS:
	{
	    if (target)
	    {
		// Control target
		// Attempt a mental roll.
		int		check = smartsCheck(target->getSmarts());

		switch (check)
		{
		    case -1:
			// Force failure.
			formatAndReport("%MU easily <M:rebuff> %R attempt at control.", target);
			break;
		    case 0:
			// Failure, but could work.
			formatAndReport("%MU <M:rebuff> %R attempt at control.", target);
			break;
		    case 1:
			// Success, but could fail.
			formatAndReport("%U <overpower> %MR mental defences.", target);
			break;
		    case 2:
			// Success always
			formatAndReport("%U easily <overpower> %MR mental defences.", target);
			break;
		    default:
			UT_ASSERT(!"Invalid smarts check");
			break;
		}
		if (check >= 1)
		{
		    actionPossess(target, rand_range(50, 100));
		    target->pietyCastSpell(spell);
		}
		else
		{
		    // Still get noticed by the gods, as this is equivalent
		    // to missing.
		    pietyCastSpell(spell);
		}
	    }
	    else
	    {
		formatAndReport("%U <find> nothing to possess.");
		cancelSpell(spell);
	    }
	    break;
	}

	case SPELL_PRESERVE:
	{
	    ITEMSTACK	stack;
	    bool	anythinghappen = false;

	    formatAndReport("%U <form> a stasis field.");

	    if (target)
	    {
		// Preserve the target.
		target->formatAndReport("%U <be> encased in a purple glow.");

		// Designed to be long enough to prevent stoning.
		target->setTimedIntrinsic(this, 
					INTRINSIC_UNCHANGING,
					6);

		anythinghappen = true;
	    }

	    glbCurLevel->getItemStack(stack, tx, ty);
	    n = stack.entries();
	    for (i = 0; i < n; i++)
	    {
		// If this is a corpse, set its charges to 255
		// to preserve it.
		if (stack(i)->getDefinition() == ITEM_BONES ||
		    stack(i)->getDefinition() == ITEM_CORPSE)
		{
		    stack(i)->formatAndReport("%U <be> encased in a purple glow.");
		    stack(i)->setAsPreserved();
		    anythinghappen = true;
		}
	    }

	    if (!anythinghappen)
		reportMessage("Nothing happens.");
	    else
		pietyCastSpell(spell);
	    return true;
	}

	case SPELL_PETRIFY:
	{
	    bool		anythinghappen = false;
	    ITEMSTACK		stack;

	    if (target)
	    {
		// Petrify the target.
		anythinghappen = true;
		formatAndReport("A layer of dust covers %MU.", target);
		if (target->hasIntrinsic(INTRINSIC_RESISTSTONING))
		{
		    formatAndReport("The dust falls to the floor.");
		}
		else
		    target->setTimedIntrinsic(this,
				INTRINSIC_STONING,
				3);
	    }

	    glbCurLevel->getItemStack(stack, tx, ty);
	    n = stack.entries();
	    for (i = 0; i < n; i++)
		anythinghappen |= stack(i)->petrify(glbCurLevel, this);
		
	    if (!anythinghappen)
		reportMessage("Nothing happens.");
	    else pietyCastSpell(spell);

	    return true;
	}

	case SPELL_STONETOFLESH:
	{
	    bool		anythinghappen = false;
	    ITEMSTACK		stack;
	    SQUARE_NAMES	tile;

	    if (target)
	    {
		anythinghappen = true;
		formatAndReport("A layer of dandruff covers %MU.", target);

		// Grant temporary stoning resistance.
		target->setTimedIntrinsic(this, INTRINSIC_RESISTSTONING,
					rand_dice(1, 5, 6));

		// UnPetrify the target.  This is bad news
		// for earth elementals :>
		target->actionUnPetrify();
	    }

	    glbCurLevel->getItemStack(stack, tx, ty);
	    n = stack.entries();
	    for (i = 0; i < n; i++)
		anythinghappen |= stack(i)->unpetrify(glbCurLevel, this);
	    
	    // Check to see if the tile is stone, in which case
	    // we turn it to a mound of flesh.
	    tile = glbCurLevel->getTile(tx, ty);

	    switch (tile)
	    {
		case SQUARE_WALL:
		case SQUARE_EMPTY:
		case SQUARE_SECRETDOOR:
		{
		    ITEM		*flesh;
		    
		    anythinghappen = true;
		    glbCurLevel->reportMessage("The wall slumps into a large mound of flesh.", tx, ty);
		    glbCurLevel->setTile(tx, ty, SQUARE_CORRIDOR);
		    flesh = ITEM::create(ITEM_MOUNDFLESH, false, true);
		    glbCurLevel->acquireItem(flesh, tx, ty, this);
		    
		    break;
		}

		default:
		    break;
	    }
		
	    if (!anythinghappen)
		reportMessage("Nothing happens.");
	    else pietyCastSpell(spell);

	    return true;
	}

	case SPELL_DIRECTWIND:
	{
	    // I have never understood wind names.
	    const char *direction[9] =
		    { "southeast", "south", "southwest",
		      "east", "invalid", "west",
		      "northeast", "north", "northwest" };
	    const char *windname;
	    int		windidx;

	    windidx = (dx + 1) + (dy + 1) * 3;
	    windname = direction[windidx];

	    if (dx || dy)
	    {
		buf.sprintf("A strong wind blows from the %s.  ",
			     windname);
	    }
	    else
	    {
		buf.reference("The air stills to an unnatural calm.  ");
	    }
	    // Global message.
	    msg_report(buf);
	    glbCurLevel->setWindDirection(dx, dy, 20 + rand_choice(20));

	    pietyCastSpell(spell);
	    break;
	}
	    
	case SPELL_RAISE_UNDEAD:
	{
	    // Determine if the given coordinates are visable..
	    if (hasIntrinsic(INTRINSIC_BLIND) ||
		!istargetlit ||
		!glbCurLevel->hasLOS(getX(), getY(),
				     tx, ty))
	    {
		// Cannot see the position, illegal.
		formatAndReport("%U <gesture> into the darkness.");
		cancelSpell(spell);
		return true;
	    }

	    // Check to see if a living creature is there.
	    if (target)
	    {
		formatAndReport("%MU <M:be> still twitching.", target);
		return true;
	    }

	    // Try to raise a zombie first.
	    if (!glbCurLevel->raiseZombie(tx, ty, this))
	    {
		// If that fails, raise a skeleton.
		if (!glbCurLevel->raiseSkeleton(tx, ty, this))
		{
		    // All failed.
		    formatAndReport("Without any corpses or bones, the magic is wasted.");
		    return true;
		}
	    }

	    // Success!
	    pietyCastSpell(spell);
	    return true;
	}

	case SPELL_RECLAIM_SOUL:
	{
	    // Determine if the given coordinates are visable..
	    if (hasIntrinsic(INTRINSIC_BLIND) ||
		!istargetlit ||
		!glbCurLevel->hasLOS(getX(), getY(),
				     tx, ty))
	    {
		// Cannot see the position, illegal.
		formatAndReport("%U <gesture> into the darkness.");
		cancelSpell(spell);
		return true;
	    }

	    // Check if no creature is there.
	    if (!target)
	    {
		formatAndReport("%U <try> to extract a soul from empty air.");
		return true;
	    }

	    // Verify the target is an undead.
	    if (target->defn().mobtype != MOBTYPE_UNDEAD)
	    {
		formatAndReport("The soul of %MU is still bound too tightly.", target);
		return true;
	    }

	    // Verify we are not oneself.
	    if (target == this)
	    {
		formatAndReport("The laws of thermodynamics forbid %U from using %MU as a power source.", target);
		return true;
	    }

	    // Verify we command the undead.
	    if (!target->isSlave(this))
	    {
		formatAndReport("%MU is not under the control of %U.", target);
		return true;
	    }

	    // Phew!  We have a winner...
	    formatAndReport("%U <send> vile tendrils that envelope %MU and yank free the last vestiges of unlife.", target);
	    
	    // Increase our health.
	    int		extrahp, maxhp;

	    // Maximum heal is the target's strength
	    extrahp = target->getHP();

	    // We cap again according to the undead type.
	    switch (target->getDefinition())
	    {
		case MOB_ZOMBIE:
		    maxhp = 10;
		    break;
		case MOB_SKELETON:
		    maxhp = 5;
		    break;
		case MOB_GHAST:
		    maxhp = 20;
		    break;
		default:
		    // I don't think any other type can fall through to here,
		    // but let us prepare for inevitable programmer error.
		    maxhp = 30;
		    break;
	    }

	    if (extrahp > maxhp)
		extrahp = maxhp;

	    // Heal ourself.
	    receiveHeal(extrahp, this, true);

	    // Destroy the target.
	    glbCurLevel->unregisterMob(target);
	    target->triggerAsDeath(ATTACK_RECLAIM_SOUL, this, 0, false);
	    delete target;

	    // Success!
	    pietyCastSpell(spell);
	    return true;
	}

	case SPELL_DARK_RITUAL:
	{
	    int		dx, dy;
	    ATTACK_NAMES     datk[4] = { ATTACK_SPELL_DARK_RITUAL_EAST,
					 ATTACK_SPELL_DARK_RITUAL_SOUTH,
					 ATTACK_SPELL_DARK_RITUAL_WEST,
					 ATTACK_SPELL_DARK_RITUAL_NORTH };
	    int		dir;
	    int		numkilled = 0;
	    MOBREF	myself;

	    formatAndReport("%U <scatter> blood in a rough pentagram and start the unspeakable ritual.");

	    myself.setMob(this);

	    for (dir = 0; dir < 4; dir++)
	    {
		getDirection(dir, dx, dy);

		// Determine if their is an owned undead in this direction.
		target = glbCurLevel->getMob(getX() + dx, getY() + dy);
		if (!target)
		    continue;

		// See if undead & owned.
		if (target->defn().mobtype != MOBTYPE_UNDEAD)
		    continue;

		if (!target->isSlave(this))
		    continue;

		numkilled++;

		// Transform the undead & empower the attack.
		formatAndReport("%U <power> a deadly attack by consuming %MU.", target);

		// Destroy the target.
		glbCurLevel->unregisterMob(target);
		target->triggerAsDeath(ATTACK_DARK_RITUAL_CONSUME, this, 0, false);
		delete target;

		// Apply the attack.
		ourEffectAttack = datk[dir];
		// Todo, maybe cone attack instead?
		glbCurLevel->fireBall(getX() + dx * 2,
				      getY() + dy * 2,
				      1, true,
				      areaAttackCBStatic, &myself);

		// Check if we died...
		if (!myself.getMob() ||
		    myself.getMob() != this ||	
		    myself.getMob()->hasIntrinsic(INTRINSIC_DEAD))
		{
		    return true;
		}
	    }
	    if (numkilled)
	    {
		pietyCastSpell(spell);
		if (numkilled == 4)
		{
		    // Charge us up!
		    setTimedIntrinsic(0, INTRINSIC_LICHFORM, 2);
		}
	    }
	    else
	    {
		// No visual effect.
		formatAndReport("With no willing victims, %R ritual has no effect.", target);
		return true;
	    }
	    return true;
	}

	case SPELL_GHASTIFY:
	    // You must be targetting a creature.
	    if (!target)
	    {
		formatAndReport("Cruel magics envelope thin air and dissipate.");
		cancelSpell(spell);
		return true;
	    }
	    
	    // The target must have some life force.
	    if (glb_mobdefs[target->getDefinition()].mobtype == MOBTYPE_UNDEAD)
	    {
		formatAndReport("%MU <M:have> no life force to convert.", target);
		return true;
	    }

	    // The target must be at death's gate.
	    if ((target->getHP() >= target->getMaxHP()) ||
		(target->getHP() != 1))
	    {
		formatAndReport("%MU, not being at death's gate, <M:resist>.", target);
		return true;
	    }

	    if (target->hasIntrinsic(INTRINSIC_UNCHANGING))
	    {
		target->formatAndReport("%U <shudder> as malevolent magics dissipate.");
		return true;
	    }

	    // GHASTIFY!
	    formatAndReport("Malevolent magics surround %MU, twisting %MA into a ghast!", target);

	    pietyCastSpell(spell);

	    // Yes, they keep all of their powers.  Perhaps this is too evil
	    // We'll see...
	    target->myDefinition = MOB_GHAST;

	    // All of their health is instantly returned.
	    // This allows this to be used as a "save self" spell.
	    target->setMaximalHP();

	    target->makeSlaveOf(this);

	    return true;

	case SPELL_BINDSOUL:
	{
	    ITEMSTACK		 stack;
	    ITEM		*item, *corpse = 0;
	    int			 i, n;

	    glbCurLevel->getItemStack(stack, tx, ty);

	    n = stack.entries();
	    for (i = 0; i < n; i++)
	    {
		item = stack(i);
		if (item->getDefinition() == ITEM_BONES ||
		    item->getDefinition() == ITEM_CORPSE)
		{
		    corpse = item;
		}
	    }

	    if (!corpse)
	    {
		formatAndReport("With no corpse, %R energies dissipate.");
		return true;
	    }

	    if (!corpse->getCorpseMob())
	    {
		reportMessage("The soul has left the corpse.");
		return true;
	    }
	    
	    // If there is target, we want to poly it into
	    // the given corpse tyoe
	    if (target)
	    {
		bool		forcetame = false;
		MOBREF		newpet;
		
		formatAndReport("Dark energies swirl around %MU.", target);
		pietyCastSpell(spell);

		if (target != this)
		    forcetame = true;

		newpet.setMob(target);

		target->actionPolymorph(false, false,
				    corpse->getCorpseMob()->getDefinition());

		// WARNING this is now invalid if we self-polymorphed!

		// Only enslave if poly success.
		if (target == newpet.getMob())
		    forcetame = false;

		target = newpet.getMob();
		if (forcetame)
		{
		    target->makeSlaveOf(this);
		}

		// Destroy the corpse.
		glbCurLevel->dropItem(corpse);
		delete corpse;
		
		return true;
	    }

	    // Check to see if there is a mound of flesh.  Then
	    // we can make a Flesh Golem.
	    // In case of a boulder, we get a stone golem.
	    {
		ITEM		*mound = 0;
		bool		 isboulder = false;
		
		n = stack.entries();
		for (i = 0; i < n; i++)
		{
		    item = stack(i);
		    if (item->getDefinition() == ITEM_MOUNDFLESH)
		    {
			mound = item;
			isboulder = false;
		    }
		    if (item->getDefinition() == ITEM_BOULDER)
		    {
			mound = item;
			isboulder = true;
		    }
		}

		if (mound)
		{
		    BUF		corpsename = corpse->getName();
		    BUF		moundname = mound->getName();
		    buf.sprintf("Shadowy tendrils yank the soul from %s and infuse it in %s.",
			    corpsename.buffer(), moundname.buffer());
		
		    reportMessage(buf);

		    pietyCastSpell(spell);

		    // Destroy the corpse.
		    glbCurLevel->dropItem(corpse);
		    delete corpse;
		    
		    // Destory the mound of flesh.
		    glbCurLevel->dropItem(mound);
		    delete mound;

		    // Create the appropriate golem.
		    MOB		*golem;

		    if (isboulder)
			golem = MOB::create(MOB_STONEGOLEM);
		    else
			golem = MOB::create(MOB_FLESHGOLEM);
			
		    // Make tame and summoned.
		    golem->makeSlaveOf(this);
		    golem->setTimedIntrinsic(this, INTRINSIC_SUMMONED,
					    isboulder ? 500 : 2000);

		    // Add to the map.
		    golem->move(tx, ty, true);
		    glbCurLevel->registerMob(golem);

		    return true;
		}
	    }

	    // Check if we can create a stone golem out of
	    // solid rock.  These are permament.
	    SQUARE_NAMES		tile;

	    tile = glbCurLevel->getTile(tx, ty);
	    if (tile == SQUARE_WALL || tile == SQUARE_EMPTY)
	    {
		BUF		corpsename = corpse->getName();
		buf.sprintf("The wall collapses, reforming itself with the soul of %s.",
			    corpsename.buffer());
		reportMessage(buf);

		pietyCastSpell(spell);

		glbCurLevel->setTile(tx, ty, SQUARE_CORRIDOR);

		// Destroy the corpse.
		glbCurLevel->dropItem(corpse);
		delete corpse;

		// Create the permament golem.
		MOB		*golem;

		golem = MOB::create(MOB_STONEGOLEM);
		    
		// Make tame
		golem->makeSlaveOf(this);

		// Add to the map.
		golem->move(tx, ty, true);
		glbCurLevel->registerMob(golem);
		
		return true;
	    }

	    if ((tile == SQUARE_CORRIDOR || tile == SQUARE_FLOOR) &&
		corpse->isBelowGrade()) 
	    {
		// Dig a pit for the golem.
		formatAndReport("The ground churns as %IU <I:infuse> it with power.", corpse);

		pietyCastSpell(spell);

		// Destroy the corpse.
		glbCurLevel->dropItem(corpse);
		delete corpse;

		glbCurLevel->digPit(tx, ty, 0);

		// Create the permament golem.
		MOB		*golem;

		golem = MOB::create(MOB_STONEGOLEM);
		    
		// Make tame
		golem->makeSlaveOf(this);

		// Add to the map.
		golem->move(tx, ty, true);
		glbCurLevel->registerMob(golem);
		
		return true;
	    }

	    formatAndReport("%IU <I:twitch>.", corpse);
	    return true;
	}

	case SPELL_SOULSUCK:
	{
	    if (target && ((target == this) || canSense(target)))
	    {
		target->formatAndReport("A black cloud surrounds %U.");
		formatAndReport("Oily smoke, inky black, streams to %U.");

		// Check if target already amnesiac
		if (target->hasIntrinsic(INTRINSIC_AMNESIA))
		{
		    formatAndReport("The smoke has no substance.");
		    return true;
		}

		// Steal all of the target's spells for the duration
		int		duration;
		duration = rand_range(100, 250);

		if (target != this)
		{
		    SPELL_NAMES		spell;
		    SKILL_NAMES		skill;

		    FOREACH_SPELL(spell)
		    {
			if (target->hasSpell(spell, false))
			{
			    learnSpell(spell, duration);
			}
		    }
		    FOREACH_SKILL(skill)
		    {
			if (target->hasSkill(skill, false))
			{
			    learnSkill(skill, duration);
			}
		    }
		}

		// Apply amnesia
		target->setTimedIntrinsic(this, INTRINSIC_AMNESIA, duration);
		target->setTimedIntrinsic(this, INTRINSIC_CONFUSED, 5);

		pietyCastSpell(spell);
	    }
	    else
	    {
		formatAndReport("%U <fail> in %r attempt to extract a soul from thin air.");
		cancelSpell(spell);
	    }
	    return true;
	}

	case SPELL_WIZARDSEYE:
	{
	    // Determine if an imp could be there...
	    tx = getX();
	    ty = getY();

	    if (!glbCurLevel->findCloseTile(tx, ty, MOVE_STD_FLY))
	    {
		glbCurLevel->reportMessage(
		    "A floating eye briefly appears before thinking better of it.",
		    tx, ty);
		return true;
	    }

	    MOB		*eye;

	    eye = MOB::create(MOB_WIZARDSEYE);
	    // Summoned creatures never have inventory.
	    eye->destroyInventory();

	    eye->move(tx, ty, true);

	    glbCurLevel->registerMob(eye);

	    eye->formatAndReport("%U <appear> in a puff of smoke!");

	    // Possess the eye for a few turns
	    eye->setTimedIntrinsic(this, INTRINSIC_SUMMONED, 30);
	    // Also make it tame.
	    eye->makeSlaveOf(this);
	    actionPossess(eye, 29);

	    eye->pietyCastSpell(spell);
	    
	    return true;
	}
	case SPELL_SUMMON_IMP:
	{
	    // Determine if an imp could be there...
	    tx = getX();
	    ty = getY();

	    if (!glbCurLevel->findCloseTile(tx, ty, MOVE_WALK))
	    {
		glbCurLevel->reportMessage(
		    "An imp briefly appears before thinking better of it.",
		    tx, ty);
		return true;
	    }

	    MOB		*imp;

	    imp = MOB::create(MOB_IMP);
	    // Summoned creatures never have inventory.
	    imp->destroyInventory();

	    imp->move(tx, ty, true);

	    glbCurLevel->registerMob(imp);

	    imp->formatAndReport("%U <appear> in a puff of smoke!");

	    imp->makeSlaveOf(this);
	    imp->setTimedIntrinsic(this, INTRINSIC_SUMMONED, 30);

	    pietyCastSpell(spell);
	    
	    return true;
	}

	case SPELL_SUMMON_DEMON:
	{
	    // Determine if an demon could be there...
	    tx = getX();
	    ty = getY();

	    if (!glbCurLevel->findCloseTile(tx, ty, MOVE_STD_FLY))
	    {
		glbCurLevel->reportMessage(
		    "A daemon briefly appears before thinking better of it.",
		    tx, ty);
		return true;
	    }

	    MOB		*imp;

	    imp = MOB::create(MOB_DAEMON);
	    // Summoned creatures never have inventory.
	    imp->destroyInventory();

	    imp->move(tx, ty, true);

	    glbCurLevel->registerMob(imp);

	    imp->formatAndReport("%U <appear> in a puff of smoke!");

	    imp->makeSlaveOf(this);
	    imp->setTimedIntrinsic(this, INTRINSIC_SUMMONED, 100);

	    pietyCastSpell(spell);
	    
	    return true;
	}

	case SPELL_POISONITEM:
	{
	    ITEM		*item;

	    item = getEquippedItem(ITEMSLOT_RHAND);

	    if (!item)
	    {
		// No item to poison.
		buf = formatToString("A greenish glow appears around %R %B1.",
			    this, 0, 0, 0,
			    (getSlotName(ITEMSLOT_RHAND) ?
				getSlotName(ITEMSLOT_RHAND) :
				"vicinity"),
			    0);
		reportMessage(buf);
		return true;
	    }

	    // If the item is an empty bottle, make a poison potion.
	    if (item->getDefinition() == ITEM_BOTTLE)
	    {
		formatAndReport("A thick slime fills %IU.", item);
		pietyCastSpell(spell);
		item->setDefinition(ITEM::lookupMagicItem(MAGICTYPE_POTION, POTION_POISON));

		// It is safe to assume the user knows what casting
		// poison item on a bottle would fill the bottle with.
		if (MOB::getAvatar() && MOB::getAvatar()->canSense(this))
		    item->markClassKnown();
			
		return true;
	    }

	    // Poison the item.
	    formatAndReport("A thick slime coats %R %Iu.", item);
	    pietyCastSpell(spell);
	    item->makePoisoned(POISON_NORMAL, 5);
	    if (MOB::getAvatar() && MOB::getAvatar()->canSense(this))
		item->markPoisonKnown();

	    return true;
	}

	case SPELL_POISONBOLT:
	    formatAndReport("A poison bolt shoots from %r fingertip.");
	    
	    pietyCastSpell(spell);

	    ourZapSpell = spell;

	    if (targetself || dz)
	    {
		if (dz)
		    reportMessage("The ray fizzles.");
		else
		    zapCallbackStatic(getX(), getY(), false, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 6,
				 MOVE_STD_FLY, false,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;

	case SPELL_CLOUDKILL:
	    formatAndReport("%U <spit>.");

	    pietyCastSpell(spell);
	    ourZapSpell = spell;

	    if (targetself || dz)
	    {
		zapCallbackStatic(getX(), getY(), true, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 6,
				 MOVE_STD_FLY, false,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;

	case SPELL_FINGEROFDEATH:
	    formatAndReport("A jet black ray flies from %r eyes.");
	    
	    pietyCastSpell(spell);

	    ourZapSpell = spell;

	    if (targetself || dz)
	    {
		if (dz)
		    reportMessage("The ray bounces.");
		zapCallbackStatic(getX(), getY(), false, &myself);
	    }
	    else
	    {
		glbCurLevel->fireRay(getX(), getY(),
				 dx, dy,
				 6,
				 MOVE_STD_FLY, true,
				 zapCallbackStatic,
				 &myself);
	    }
	    return true;

	case NUM_SPELLS:
	    UT_ASSERT(!"UNHANDLED SPELL!");
	    return false;
    }
	    
    return false;
}

bool
MOB::actionDig(int dx, int dy, int dz, int range, bool magical)
{
    applyConfusion(dx, dy, dz);

    // TODO: Handle mundane digging.
    if (!magical)
    {
	// Special messages for special equipment...
	ITEM	*item = getSourceOfIntrinsic(INTRINSIC_DIG);

	if (item)
	{
	    formatAndReport("%U <dig> with %r %Iu.", item);
	}
	else
	{
	    formatAndReport("%U <dig>.");
	}
    }

    // Do actual excavation:

    // Dig yourself.
    if (!dx && !dy && !dz)
    {
	formatAndReport("%r stomach feels empty.");
	starve(500);
	return true;
    }

    // Dig upwards.
    if (dz > 0)
    {
	bool		fancymove = false;
	
	// If you are submerged, digging upwards will
	// clear a path.  If you are buried alive,
	// it will turn your square into a pit.
	// If you are under ice, it will break the ice
	// and raise you up.
	if (hasIntrinsic(INTRINSIC_SUBMERGED))
	{
	    SQUARE_NAMES		tile, newtile;
	    bool	setinpit = false;

	    tile = glbCurLevel->getTile(getX(), getY());
	    newtile = tile;

	    // We let the creature escape first.  This way you see the report
	    // message even if they were invisible when entombed.
	    clearIntrinsic(INTRINSIC_SUBMERGED);

	    switch (tile)
	    {
		case SQUARE_WATER:
		    formatAndReport("The water parts above %U.");
		    fancymove = true;
		    break;
		case SQUARE_ACID:
		    formatAndReport("The acid parts above %U.");
		    fancymove = true;
		    break;
		case SQUARE_LAVA:
		    formatAndReport("The lava parts above %U.");
		    fancymove = true;
		    break;
		case SQUARE_ICE:
		    formatAndReport("The ice breaks above %U.");
		    newtile = SQUARE_WATER;
		    fancymove = true;
		    break;
		default:
		    formatAndReport("The rocks shatter above %U.");
		    setinpit = true;
		    fancymove = true;
		    break;
	    }
	    
	    if (newtile != tile)
	    {
		glbCurLevel->setTile(getX(),
				     getY(),
				     newtile);
		glbCurLevel->dropItems(getX(),
				    getY(),
				    this);
	    }
	    if (setinpit)
	    {
		// Don't identify us as zapper as
		// not normal type.
		glbCurLevel->digPit(getX(), getY(), 0);
		setIntrinsic(INTRINSIC_INPIT);
	    }
	}
	
	if (!fancymove)
	{
	    if (glbCurLevel->branchName() == BRANCH_TRIDUDE)
	    {
		formatAndReport("The metal roof is unaffected.");
	    }
	    else if (glbCurLevel->getDepth() == 0)
	    {
		formatAndReport("The clear blue sky remains unimpressed.");
	    }
	    else
	    {
		// Zap at the ceiling, rocks fall down.
		formatAndReport("Rocks fall from the ceiling onto %r head.");

		ITEM *rocks = ITEM::create(ITEM_ROCK, false);
		glbCurLevel->acquireItem(rocks, getX(), getY(), this);
		// TODO: We would like a nicer message?
		receiveDamage(ATTACK_CEILINGROCKS, this, 
					0, 0,
					ATTACKSTYLE_MISC);
	    }
	}

	return true;
    }

    if (dz < 0)
    {
	// If we are digging into water or lava, we have a no-op.
	// If we are digging into ice, we break the ice and end up
	// in the drink.  (Unless we are submerged.)
	switch (glbCurLevel->getTile(getX(), getY()))
	{
	    case SQUARE_WATER:
		formatAndReport("The water briefly parts below %U..");
		// Another chance to drop due to the air being revealed.
		glbCurLevel->dropMobs(getX(), getY(), false, this);
		return true;
	    case SQUARE_ACID:
		formatAndReport("The acid briefly parts below %U.");
		// Another chance to drop due to the air being revealed.
		glbCurLevel->dropMobs(getX(), getY(), false, this);
		return true;
	    case SQUARE_LAVA:
		formatAndReport("The lava briefly parts below %U.");
		// Another chance to drop due to the air being revealed.
		glbCurLevel->dropMobs(getX(), getY(), false, this);
		return true;
	    case SQUARE_ICE:
		if (!hasIntrinsic(INTRINSIC_SUBMERGED))
		{
		    formatAndReport("The ice breaks below %U.");
		    glbCurLevel->setTile(getX(),
					 getY(),
					 SQUARE_WATER);
		    glbCurLevel->dropMobs(getX(), getY(), false, this);
		}
		else
		{
		    formatAndReport("The water briefly parts below %U..");
		}
		return true;
	    default:
		break;
	}
    
	// Zap at the floor.  We make a pit, and fall down it.
	// The map handles falling through it.
	// The exception is if we are digging a pit, ie: short range.
	// The exception with that is if you are already in a pit.
	if (range > 1 || hasIntrinsic(INTRINSIC_INPIT))
	    glbCurLevel->digHole(getX(), getY(), this);
	else
	    glbCurLevel->digPit(getX(), getY(), this);

	return true;
    }

    // Now, the theoritically easy part...
    ourZapSpell = SPELL_DIG;

    MOBREF		myself;
    myself.setMob(this);

    glbCurLevel->fireRay(getX(), getY(),
		    dx, dy,
		    range,
		    MOVE_ALL, false,
		    zapCallbackStatic,
		    &myself);

    return true;
}

bool
MOB::actionTeleport(bool allowcontrol, bool givecontrol, int xloc, int yloc)
{
    int			tx, ty;
    BUF		buf;

    if (hasIntrinsic(INTRINSIC_TELEFIXED))
    {
	formatAndReport("%U <shake>.");
	return true;
    }

    // Check for failure due to being on a level that prohibits
    // teleportation
    if (!glbCurLevel->allowTeleportation())
    {
	// We allow people to blink as it isn't quite so bad a form
	// of cheating...  This may change :>
	if (xloc >= 0 && yloc >= 0)
	{
	    // Blink, allow.
	}
	else
	{
	    formatAndReport("The dungeon briefly distorts around %U.");
	    return true;
	}
    }

    if (allowcontrol && 
        (givecontrol || hasIntrinsic(INTRINSIC_TELEPORTCONTROL)))
    {
	if (!glbStressTest && isAvatar())
	{
	    buf.sprintf("Where do you want to teleport?");
	    reportMessage(buf);
	    tx = getX();
	    ty = getY();
	    if (gfx_selecttile(tx, ty))
	    {
		// Check to see if that is a legal location.
		if (canMove(tx, ty, true) && !glbCurLevel->getMob(tx, ty)
		    && !glbCurLevel->getFlag(tx, ty, SQUAREFLAG_NOMOB))
		{
		    // A legal move!
		    formatAndReport("%U <teleport>.");
		}
		else
		{
		    // If you decide to stay put, don't report that you
		    // are blocked (since you will, of course, fail to
		    // move into your own square!)
		    if (tx == getX() && ty == getY())
		    {
			formatAndReport("%U <decide> to stay put.");
		    }
		    else
		    {
			formatAndReport("%U <be> blocked!");
			tx = getX();
			ty = getY();
		    }
		}
	    }
	    else
	    {
		tx = getX();
		ty = getY();
		formatAndReport("%U <decide> to stay put.");
	    }
	}
	else
	{
	    // Use AI.  Try to teleport to the stairs.
	    if (glbCurLevel->findTile(SQUARE_LADDERUP, tx, ty) &&
		glbCurLevel->findCloseTile(tx, ty, getMoveType()))
	    {
		formatAndReport("%U <teleport> away.");
	    }
	    else
	    {
		// Go somewhere random instead of the stairs as they
		// are blocked.
		if (glbCurLevel->findRandomLoc(tx, ty, getMoveType(),
			    false, getSize() >= SIZE_GARGANTUAN,
			    false, true, true, true))
		{
		    formatAndReport("%U <teleport> away.");
		}
		else
		{
		    // Teleport to where we are.
		    formatAndReport("%U <blink> out of sight briefly.");
		    tx = getX();
		    ty = getY();
		}
	    }
	}
    }
    else
    {
	// Determine a random destination...
	if (xloc >= 0 && yloc >= 0)
	{
	    formatAndReport("%U <blink>.");
	    tx = xloc;
	    ty = yloc;
	}
	else if (glbCurLevel->findRandomLoc(tx, ty, getMoveType(),
		    false, getSize() >= SIZE_GARGANTUAN,
		    false, false, false, true))
	{
	    formatAndReport("%U <teleport>.");
	}
	else
	{
	    // Failed to teleport, you shudder...
	    tx = getX();
	    ty = getY();
	    formatAndReport("%U <shudder> violently.");
	}
    }

    bool		didmove = false;

    if (tx != getX() || ty != getY())
	didmove = true;

    // Move to the given destination.
    if (!move(tx, ty, !didmove))
	return true;
    
    // Report what we stepped on...
    if (didmove && isAvatar())
    {
	// Need to rebuild our screen!  Refreshing will be handled
	// by the message wait code.
	gfx_scrollcenter(getX(), getY());
	glbCurLevel->buildFOV(getX(), getY(), 7, 5);

	glbCurLevel->describeSquare(getX(), getY(), 
				    hasIntrinsic(INTRINSIC_BLIND),
				    false);
    }

    return true;
}

bool
MOB::actionUnPolymorph(bool silent, bool systemshock)
{
    MOB		*origmob;

    if (hasIntrinsic(INTRINSIC_UNCHANGING))
    {
	// Explicitly prevent polymorphing...
	if (!silent)
	    formatAndReport("%U <shudder>.");
	return true;
    }

    origmob = myBaseType.getMob();

    if (!origmob)
    {
	// Nothing to do!
	return true;
    }

    // Be paranoid and trigger a mobref check.
#ifdef STRESS_TEST
    if (glbCurLevel)
    {
	if (!glbCurLevel->verifyMob())
	{
	    msg_report("MOBREF corruption: pre-unpoly.  ");
	}
    }
#endif

    if (!silent)
	formatAndReport("%U <return> to %r original shape!");

    bool	shouldwallcrush = false;

    if (origmob->getSize() >= SIZE_GARGANTUAN &&
	getSize() < SIZE_GARGANTUAN)
    {
	shouldwallcrush = true;
    }
    
    // Transfer the name of the mob to the base level.  This accounts
    // for cases where the player has named the mob.
    origmob->myName = myName;
    
    // Transfer all the items.
    // We could just steal the inventory pointer, but that would be
    // evil if items ever get back references.  There is also no
    // guarantee that the intrinsics/rebuild will work correctly.
    ITEM		*item, *next, *drop;
    bool		 anydrop = false;
    for (item = myInventory; item; item = next)
    {
	next = item->getNext();
	
	drop = dropItem(item->getX(), item->getY());
	UT_ASSERT(drop == item);

	if (!origmob->acquireItem(item, item->getX(), item->getY()))
	{
	    // We have to report the dropping before doing it
	    // as dropping may merge stacks and delete item.
	    // Note that we don't want to use newitem as that
	    // may report an unusual new stack count.
	    formatAndReport("%IU <I:drop> on the ground!", item);
	    // If we fail to acquire, we have to drop on the ground.
	    glbCurLevel->acquireItem(item, getX(), getY(), this);
	    anydrop = true;
	}
    }
    // Make sure stuff ends up in the water, etc.
    if (anydrop)
	glbCurLevel->dropItems(getX(), getY(), 0);

    UT_ASSERT(!myInventory);

    // Transfer the raw mob pointers...
    MOBREF		tmpref;
    INTRINSIC_NAMES	intrinsic;

    tmpref = myOwnRef;
    myOwnRef = origmob->myOwnRef;
    origmob->myOwnRef = tmpref;

    // Fix up the underlying pointers to match the global level.
    if (!myOwnRef.transferMOB(this))
    {
	UT_ASSERT(!"UnPoly Fail Transfer To");
    }
    if (!origmob->myOwnRef.transferMOB(origmob))
    {
	UT_ASSERT(!"UnPoly Fail Transfer From");
    }

    origmob->move(getX(), getY(), true);
    
    // Clear out our base type.  (It points to ourself now,
    // so would be rather disasterous to delete it as is!)
    myBaseType.setMob(0);
    
    glbCurLevel->unregisterMob(this);
    glbCurLevel->registerMob(origmob);

    // We don't need to transfer back experience as it has been
    // accumulating for us.

    // Transfer food level.
    origmob->myFoodLevel = myFoodLevel;

    // Transfer all the intrinsics that should survive.
    for (intrinsic = INTRINSIC_NONE; intrinsic < NUM_INTRINSICS;
	 intrinsic = (INTRINSIC_NAMES) (intrinsic + 1))
    {
	if (glb_intrinsicdefs[intrinsic].surviveunpoly &&
	    hasIntrinsic(intrinsic))
	{
	    INTRINSIC_COUNTER		*counter;

	    // We may already have the intrinsic, so don't want
	    // to double count things.
	    origmob->clearIntrinsic(intrinsic);
	    
	    counter = getCounter(intrinsic);
	    if (counter)
	    {
		origmob->setTimedIntrinsic(counter->myInflictor.getMob(),
					    intrinsic,
					    counter->myTurns);
	    }
	    else
		origmob->setIntrinsic(intrinsic);
	}

	// If the intrinsic shouldn't survive, remove ourselves.
	if (glb_intrinsicdefs[intrinsic].clearonpoly)
	    clearIntrinsic(intrinsic);
    }

    // Transfer the ai target.
    origmob->myAITarget.setMob(getAITarget());
    
    delete this;

    origmob->rebuildAppearance();
    origmob->rebuildWornIntrinsic();

    // Put the system shock to the end so the message makes sense.
    // Apply system shock: The base types max hit points are damaged
    // as a result of the pain.
    if (systemshock)
    {
	origmob->systemshock();
    }

    // Crush surrounding walls if we return to gargantuan size
    if (shouldwallcrush)
    {
	glbCurLevel->wallCrush(origmob);
    }

#ifdef STRESS_TEST
    // Be paranoid and trigger a mobref check.
    if (glbCurLevel)
    {
	if (!glbCurLevel->verifyMob())
	{
	    msg_report("MOBREF corruption: unpoly.  ");
	}
    }
#endif

    // All done!  No one should be the wiser!
    return true;
}

bool
MOB::actionPolymorph(bool allowcontrol, bool forcecontrol, MOB_NAMES newdef)
{
    MOB		*newmob;
    BUF		buf;

    if (hasIntrinsic(INTRINSIC_UNCHANGING))
    {
	// Explicitly prevent polymorphing...
	formatAndReport("%U <shudder>.");
	return true;
    }

#ifdef STRESS_TEST
    // Be paranoid and trigger a mobref check.
    if (glbCurLevel)
    {
	if (!glbCurLevel->verifyMob())
	{
	    msg_report("MOBREF corruption: pre-poly.  ");
	}
    }
#endif

    // If we are already polyed, we first return to our original state
    // before going on to polymorph...
    if (myBaseType.getMob())
    {
	newmob = myBaseType.getMob();

	actionUnPolymorph(true);

	return newmob->actionPolymorph(allowcontrol, forcecontrol, newdef);
    }

    // newdef = MOB_CRETAN_MINOTAUR;
    
    if (allowcontrol && (forcecontrol || hasIntrinsic(INTRINSIC_POLYCONTROL)))
    {
	formatAndReport("%U <take> control of the transformation!");
	if (!glbStressTest && isAvatar())
	{
	    // Make a list of all killed critters and prompt.
	    MOB_NAMES		 mobtypes[NUM_MOBS];
	    const char 		*moblist[NUM_MOBS+2]; // room for null, nochange
	    int			 i, numtype, aorb, selection;

	    victory_buildSortedKillList(mobtypes, &numtype);

	    // Option to remain unchanged...
	    moblist[0] = "yourself";
	    
	    // Create names
	    for (i = 0; i < numtype; i++)
	    {
		moblist[i+1] = glb_mobdefs[mobtypes[i]].name;
	    }
	    // Null terminate
	    moblist[numtype+1] = 0;

	    while (1)
	    {
		int		y;

		gfx_printtext(0, 3, "Transform into what?");
		selection = gfx_selectmenu(5, 4, (const char **) moblist, aorb);
		for (y = 3; y < 19; y++)
		    gfx_cleartextline(y);

		// You are not allowed to cancel.
		if (!aorb)
		    break;
	    }
	    if (!selection)
	    {
		// Person chose to resist.
		// No shudder message as this might involve turning
		// *back* to your original form.
		return true;
	    }

	    selection--;
	    newdef = mobtypes[selection];
	}
	else
	{
	    // Find the end of our advancment and turn into it.
	    newdef = getDefinition();
	    while (glb_mobdefs[newdef].evolvetarget != MOB_NONE)
	    {
		newdef = (MOB_NAMES) glb_mobdefs[newdef].evolvetarget;
	    }

	    // If I have no evolve target, go for random poly unless
	    // we are in good health.
	    if (newdef == getDefinition())
	    {
		if (getHP() >= (getMaxHP() >> 3) + 1)
		{
		    // We are happy with what we are.
		    return true;
		}
		else
		{
		    // We may be seeking escape.
		    newdef = MOB_NONE;
		}
	    }
	}
    }

    // If newdef is not MOB_NONE, that is the forced value.
    // We manually add an upper bound as one may have locked
    // the NPC to only be of a certain type, that will then infinite
    // loop
    int maxtries = 100;
    while (newdef == MOB_NONE)
    {
	newdef = MOB::chooseNPC(100);

	// Reject self-polymorph as it is embarrassing.
	if (newdef == getDefinition() && (maxtries --> 0))
	    newdef = MOB_NONE;
    }

    newmob = MOB::create(newdef);

    // This bit of logic was cut and paste on the Narita Express whilst
    // somewhat drunk on beer and sake.  Not enough beer and sake, obviously,
    // since I'm still able to cut and paste.  Not much to see outside at
    // 8:15, however, it is already very dark.  Clearly I'm preparing
    // for my more northerly summers and expectations of the sun setting
    // some time after dusk.
    bool	shouldwallcrush = false;

    if (newmob->getSize() >= SIZE_GARGANTUAN &&
	getSize() < SIZE_GARGANTUAN)
    {
	shouldwallcrush = true;
    }
    
    // The new mob doesn't get to keep any cool possessions!
    newmob->destroyInventory();

    // Transfer all the items.
    // We could just steal the inventory pointer, but that would be
    // evil if items ever get back references.  There is also no
    // guarantee that the intrinsics/rebuild will work correctly.
    ITEM		*item, *next, *drop;
    bool		 anydrop = false;

    for (item = myInventory; item; item = next)
    {
	next = item->getNext();
	
	drop = dropItem(item->getX(), item->getY());
	UT_ASSERT(drop == item);

	if (!newmob->acquireItem(item, item->getX(), item->getY()))
	{
	    // We have to report the dropping before doing it
	    // as dropping may merge stacks and delete item.
	    // Note that we don't want to use newitem as that
	    // may report an unusual new stack count.
	    // If we fail to acquire, we have to drop on the ground.
	    formatAndReport("%IU <I:drop> on the ground!", item);
	    glbCurLevel->acquireItem(item, getX(), getY(), this);
	    anydrop = true;
	}
    }
    // Make sure stuff ends up in the water, etc.
    if (anydrop)
	glbCurLevel->dropItems(getX(), getY(), 0);
    
    UT_ASSERT(!myInventory);

    // Move the getX()/getY() so we don't get "transforms into it".
    newmob->move(getX(), getY(), true);
    // Set the mob's dlevel so we don't get transforms into it.
    newmob->setDLevel(getDLevel());
    // Set the mob's name to match our own name.
    newmob->myName = myName;

    // Spam the effect!
    // If we are the avatar, we force the new form to be visible.  Otherwise,
    // polying whilst invisible and no see invisible would be 
    // "you turn into it"
    // Except.... That is a feature.  After all, you have no
    // idea of knowing what your new form is?
    // Bah, disable for now until we properly use "something".
    // We now use soemething, so it is a feature again.
    formatAndReport("%U <transform> into %B1!", 
		    newmob->getName(true, false, false /*isAvatar()*/));

    // We want all mob references to go to newmob rather than this:
    MOBREF		tmpref;
    INTRINSIC_NAMES	intrinsic;

    tmpref = myOwnRef;
    myOwnRef = newmob->myOwnRef;
    newmob->myOwnRef = tmpref;

    // Fix up the underlying pointers to match the global level.
    if (!myOwnRef.transferMOB(this))
    {
	UT_ASSERT(!"Poly transfer to");
    }
    if (!newmob->myOwnRef.transferMOB(newmob))
    {
	UT_ASSERT(!"Poly transfer from");
    }
    
    // Important to do this AFTER the transfer!
    newmob->myBaseType.setMob(this);
    
    glbCurLevel->unregisterMob(this);
    glbCurLevel->registerMob(newmob);

    // Transfer any experience to the new mob.
    newmob->myExp = myExp;

    // Transfer food level.
    newmob->myFoodLevel = myFoodLevel;

    // Transfer all the intrinsics that should survive.
    for (intrinsic = INTRINSIC_NONE; intrinsic < NUM_INTRINSICS;
	 intrinsic = (INTRINSIC_NAMES) (intrinsic + 1))
    {
	if (glb_intrinsicdefs[intrinsic].survivepoly &&
	    hasIntrinsic(intrinsic))
	{
	    INTRINSIC_COUNTER		*counter;
	    
	    counter = getCounter(intrinsic);
	    if (counter)
	    {
		newmob->setTimedIntrinsic(counter->myInflictor.getMob(),
					    intrinsic,
					    counter->myTurns);
	    }
	    else
		newmob->setIntrinsic(intrinsic);
	}

	// If the intrinsic shouldn't survive, remove ourselves.
	if (glb_intrinsicdefs[intrinsic].clearonpoly)
	    clearIntrinsic(intrinsic);
    }

    // Transfer the ai target.
    newmob->myAITarget.setMob(getAITarget());

    newmob->rebuildAppearance();
    newmob->rebuildWornIntrinsic();

    // Crush surrounding walls if we return to gargantuan size
    if (shouldwallcrush)
    {
	glbCurLevel->wallCrush(newmob);
    }

    // All done!  No one should be the wiser!

#ifdef STRESS_TEST
    // Be paranoid and trigger a mobref check.
    if (glbCurLevel)
    {
	if (!glbCurLevel->verifyMob())
	{
	    msg_report("MOBREF corruption: poly.  ");
	}
    }
#endif
    
    return true;
}

bool
MOB::actionPetrify()
{
    // Ensure we are made of flesh.  Otherwise, not much to do.
    if (getMaterial() != MATERIAL_FLESH || hasIntrinsic(INTRINSIC_UNCHANGING))
    {
	formatAndReport("%U <feel> momentarily stony.");
	return false;
    }

    // Special case: Flesh golems become stone golems.
    if (getDefinition() == MOB_FLESHGOLEM)
    {
	formatAndReport("%U <transform> into a stone golem!");
	myDefinition = MOB_STONEGOLEM;
	return true;
    }

    // Special case: Trolls become cave trolls.
    if (getDefinition() == MOB_TROLL)
    {
	formatAndReport("%U <transform> into a cave troll!");
	myDefinition = MOB_CAVETROLL;
	return true;
    }

    // Determine if the user can save their life.
    if (attemptLifeSaving("%U <harden> into a statue!"))
    {
	return true;
    }

#ifdef STRESS_TEST
    if (glbCurLevel)
    {
	if (!glbCurLevel->verifyMob())
	{
	    msg_report("MOBREF corruption: pre-petrify.  ");
	}
    }
#endif

    formatAndReport("%U <harden> into a statue!");

    // Release any possession
    if (hasIntrinsic(INTRINSIC_POSSESSED))
	actionReleasePossession(true);

    // Create the new statue.
    ITEM		*statue;

    glbCurLevel->unregisterMob(this);
    statue = ITEM::createStatue(this);
    glbCurLevel->acquireItem(statue, getX(), getY(), this);
    
    if (isAvatar())
    {
	// Note we want to auto-map our own statue so the user
	// gets to see it.
	statue->markMapped();
    }
    triggerAsDeath(ATTACK_TURNED_TO_STONE, 0, 0, true);

    // Clear their death intrinsics so if they are ressed, they
    // don't keep burning...
    clearDeathIntrinsics(true);
	
    // One may be better categorized as in stasis, but this does
    // mean that people should stop following you.
    setIntrinsic(INTRINSIC_DEAD);

#ifdef STRESS_TEST
    // Be paranoid and trigger a mobref check.
    if (glbCurLevel)
    {
	if (!glbCurLevel->verifyMob())
	{
	    msg_report("MOBREF corruption: petrify.  ");
	}
    }
#endif
    
    return true;
}

bool
MOB::actionUnPetrify()
{
    // Clear out the stoning intrinsic
    clearIntrinsic(INTRINSIC_STONING);
    
    // Ensure we are made of stone.  Otherwise, not much to do.
    if (getMaterial() != MATERIAL_STONE || hasIntrinsic(INTRINSIC_UNCHANGING))
    {
	formatAndReport("%U <feel> momentarily fleshy.");
	return false;
    }

    // Special case: Flesh golems become stone golems.
    if (getDefinition() == MOB_STONEGOLEM)
    {
	formatAndReport("%U <transform> into a flesh golem!");
	myDefinition = MOB_FLESHGOLEM;
	return true;
    }

    // Special case: Cave trolls become trolls.
    if (getDefinition() == MOB_CAVETROLL)
    {
	formatAndReport("%U <soften> into a troll!");
	myDefinition = MOB_TROLL;
	return true;
    }

    // Check if we can save our life...
    if (attemptLifeSaving("%U <sag> into a mound of featureless flesh!"))
    {
	return true;
    }

    // Ew!  Icky!
    formatAndReport("%U <collapse> into a mound of flesh!");

    // Create the new flesh mound.
    ITEM		*flesh;

    glbCurLevel->unregisterMob(this);
    
    // Drop our inventory on the ground.
    {
	ITEM *cur;

	// Drop our inventory on this square...
	while ((cur = myInventory))
	{
	    // We manually drop the item for efficiency.
	    // (Creature is dead, so no need for efficiency)
	    myInventory = cur->getNext();
	    cur->setNext(0);
	    glbCurLevel->acquireItem(cur, getX(), getY(), this);
	}
    }
    
    flesh = ITEM::create(ITEM_MOUNDFLESH, false, true);
    glbCurLevel->acquireItem(flesh, getX(), getY(), this);
    
    if (isAvatar())
    {
	// Note we want to auto-map our own mound of flesh so the user
	// gets to see it.
	flesh->markMapped();
    }
    // WHile you do become an item, there is no restoration possibility...
    triggerAsDeath(ATTACK_TURNED_TO_FLESH, 0, 0, false);

    // Self is most definitely dead.
    delete this;

    return true;
}

bool
MOB::ableToEquip(int ix, int iy, ITEMSLOT_NAMES slot, bool quiet)
{
    // First, ensure there is an item...
    ITEM		*item, *olditem, *otheritem;
    BUF		buf;
    bool		 fit;
    bool		 dualdequip = false;

    // Determine if we have the given slot...
    if (!hasSlot(slot))
    {
	if (!quiet)
	{
	    formatAndReport("%U <have> no %B1!", 
			glb_itemslotdefs[slot].bodypart);
	}
	return false;
    }

    item = getItem(ix, iy);
    if (!item)
    {
	if (!quiet)
	    formatAndReport("%U <try> to equip nothing!");
	return false;
    }

    // Check to see if it can fit.  Note anything will fit in the right
    // hand, as anything can be used as a weapon.
    fit = false;
    switch (slot)
    {
	case ITEMSLOT_HEAD:
	    fit = item->isHelmet();
	    break;
	case ITEMSLOT_RHAND:
	    fit = true;

	    // We need two hands to equip large items.
	    if (item->getSize() > getSize())
	    {
		// Check to see if the right hand is free.  Specifically,
		// if the left hand has an item, we can't fit a large weapon.
		otheritem = getEquippedItem(ITEMSLOT_LHAND);
		if ((otheritem && otheritem->getDefinition() != ITEM_BUCKLER) 
			|| !hasSlot(ITEMSLOT_LHAND))
		{
		    // We demand a double dequip
		    dualdequip = true;
		}
	    }
	    break;
	case ITEMSLOT_LHAND:
	    // Anything fits in the right hand.
	    // However, we still prohibit stacking (as that is only
	    // allowed for doing enchantments)
	    // We only idac if it is a shield.
	    fit = true;
	    // Check to see if the left hand is free.  Specifically,
	    // if the right hand has a large weapon in it, we can't
	    // equip in the left hand.
	    // Note: If you have a left hand, you always have a right hand.
	    // Note: Bucklers are special because you strap them to
	    // your forearm.
	    otheritem = getEquippedItem(ITEMSLOT_RHAND);
	    if (otheritem && (otheritem->getSize() > getSize()) &&
		(item->getDefinition() != ITEM_BUCKLER))
	    {
		if (!quiet)
		{
		    formatAndReport("%U <need> two hands to wield %IU!", otheritem);
		}
		return false;
	    }
	    break;
	case ITEMSLOT_BODY:
	    fit = item->isJacket();
	    break;
	case ITEMSLOT_FEET:
	    fit = item->isBoots();
	    break;
	case ITEMSLOT_AMULET:
	    fit = item->isAmulet();
	    break;
	case ITEMSLOT_RRING:
	case ITEMSLOT_LRING:
	{	
	    // If we have the missing finger intrinsic, this will
	    // only work if we are replacing the ring or have no
	    // rings, ie, the other slot is free.
	    if (hasIntrinsic(INTRINSIC_MISSINGFINGER))
	    {
		if (getEquippedItem(
			(slot==ITEMSLOT_RRING)
			    ? ITEMSLOT_LRING
			    : ITEMSLOT_RRING))
		{
		    if (!quiet)
		    {
			formatAndReport("%U <be> surprised to find no %B1!",
				getSlotName(slot));
		    }
		    // Yes, you get to find out for free...
		    return false;
		}
	    }
	    fit = item->isRing();
	    break;
	}
	default:
	{
	    UT_ASSERT(!"Unhandled ITEMSLOT");
	    fit = false;
	    break;
	}
    }

    if (!fit)
    {
	if (!quiet)
	{
	    buf = formatToString("%U <fail> to put %IU %B1 %r %B2.",
		    this, 0, 0, item,
		    glb_itemslotdefs[slot].preposition,
		    getSlotName(slot));
	    reportMessage(buf);
	}
	return false;
    }

    // Verify we are able to dequip the old slot.
    olditem = getEquippedItem(slot);
    if (olditem)
    {
	if (!ableToDequip(slot, quiet))
	    return false;
    }
    if (dualdequip)
    {
	if (!ableToDequip(ITEMSLOT_LHAND, quiet))
	    return false;
    }

    return true;
}

bool
MOB::actionEquip(int ix, int iy, ITEMSLOT_NAMES slot, bool quiet)
{
    // First, ensure there is an item...
    ITEM		*item, *olditem, *offhand;
    BUF		buf;
    bool		 idac = false;	// Do we now know the ac?
    bool		 allowstack = false;

    // Verify this is a legal operation.
    if (!ableToEquip(ix, iy, slot, quiet))
	return false;

    item = getItem(ix, iy);

    switch (slot)
    {
	case ITEMSLOT_HEAD:
	    idac = true;
	    break;
	case ITEMSLOT_RHAND:
	    allowstack = item->defn().equipstack;
	    break;
	case ITEMSLOT_LHAND:
	    idac = item->isShield();
	    break;
	case ITEMSLOT_BODY:
	    idac = true;
	    break;
	case ITEMSLOT_FEET:
	    idac = true;
	    break;
	case ITEMSLOT_AMULET:
	    break;
	case ITEMSLOT_RRING:
	case ITEMSLOT_LRING:
	    break;
	default:
	{
	    UT_ASSERT(!"Unhandled ITEMSLOT");
	    break;
	}
    }

    // It fits.  First, we remove whatever was there...
    olditem = getEquippedItem(slot);
    if (olditem)
    {
	actionDequip(slot);
    }
    // It is possible dequiping failed.  In that case, we abort early.
    // (We do check that we are ableToDequip, but let's still be paranoid)
    if (getEquippedItem(slot))
	return true;

    // Check to see if we need secondary dequip.
    if (slot == ITEMSLOT_RHAND &&
	item->getSize() > getSize())
    {
	offhand = getEquippedItem(ITEMSLOT_LHAND);

	// See if we need to dequip the left hand or not.
	// Bucklers are special.
	// Only do so if an item is present to avoid spam.
	// (Written on GO train, less prestigous than first class...)
	if (offhand && offhand->getDefinition() != ITEM_BUCKLER)
	{
	    // Force double dequip.
	    actionDequip(ITEMSLOT_LHAND, quiet);
	    // Check for failure, possibly full inventory?
	    // (Cursed should be handled in ableToDequip...)
	    if (getEquippedItem(ITEMSLOT_LHAND))
		return true;
	}
    }

    // Strip off the top item of a stack.
    ITEM		*itemstack = 0;
    if (!allowstack)
    {
	if (item->getStackCount() > 1)
	{
	    itemstack = item;
	    item = itemstack->splitStack(1);
	}
    }

    // Move the old item into the slot which we equipped from.
    // Note that we require a spare slot to dequip into, but we
    // will ensure the cursor is over the dequipped item, so one
    // can easily swap weapons.
    // Note that if we equipped from a stack, the stack will
    // remain in the old pos, so thus we don't want to do this.
    if (olditem && !itemstack)
    {
	// If we are equipping from a slot, we have to verify
	// it is legal to put the old item into that slot.
	// Or you can end up wearing a sword.
	if (ix != 0 || ableToEquip(olditem->getX(), olditem->getY(), (ITEMSLOT_NAMES) iy, true))
	    olditem->setPos(item->getX(), item->getY());
    }
    
    // Move this item into the slot...
    if (!itemstack)
	item->setPos(0, slot);
    else
    {
	// As we have verified above that the slot is empty,
	// this will not fail nor result in item being deleted.
	acquireItem(item, 0, slot);
    }

    if (!quiet)
    {
	buf = formatToString("%U <put> %IU %B1 %r %B2.",
		this, 0, 0, item,
		glb_itemslotdefs[slot].preposition,
		getSlotName(slot));
	reportMessage(buf);
    }

    // Poison ourselves if the item is poisoned:
    item->applyPoison(this, this);
    
    // If we now can figure out the ac, we mark it as known.
    if (idac && isAvatar())
	item->markEnchantKnown();

    // Auto identify the item if it is cursed and we didn't know
    // the status.
    if (!item->isKnownCursed() && item->isCursed() && isAvatar())
    {
	if (!quiet)
	{
	    formatAndReport("%IU <I:chill> %r %B1!", item,
			getSlotName(slot));
	}
	item->markCursedKnown();
    }

    // If the avatar equips something that doesn't curse them,
    // the avatar can note it is now not cursed.
    if (!item->isKnownNotCursed() && !item->isCursed() && isAvatar())
    {
	// No spam for this.
	item->markNotCursedKnown();
    }
    
    // If the new item grants missing finger, we should ensure we have
    // a free finger...
    if (item->hasIntrinsic(INTRINSIC_MISSINGFINGER))
    {
	dropOneRing();
    }

    rebuildAppearance();
    rebuildWornIntrinsic();

    return true;
}

bool
MOB::ableToDequip(ITEMSLOT_NAMES slot, bool quiet)
{
    ITEM		*item;
    BUF		buf;
    int			 ix, iy;

    item = getEquippedItem(slot);

    if (!item)
    {
	// Trying to dequip something you don't have on.
	if (!hasSlot(slot))
	{
	    buf = formatToString("%U <have> no %B1!",
			this, 0, 0, 0,
			glb_itemslotdefs[slot].bodypart);
	}
	else
	{
	    buf = formatToString("%U <have> nothing %B1 %r %B2!",
			this, 0, 0, 0,
			glb_itemslotdefs[slot].preposition,
			getSlotName(slot));
	}
	if (!quiet)
	    reportMessage(buf);
	return false;
    }

    // Check if the item is cursed.  Can't remove cursed items.
    if (item->isCursed())
    {
	if (!quiet)
	{
	    formatAndReport("%U <try> to remove %IU, but %Ip <I:be> evil!", item);
	}
	if (isAvatar())
	    item->markCursedKnown();

	return false;
    }

    // Find a slot for it to go to.
    if (!findItemSlot(ix, iy))
    {
	if (!quiet)
	{
	    formatAndReport("%U <have> no room to hold %IU.", item);
	}
	
	return false;
    }

    return true;
}

bool
MOB::actionDequip(ITEMSLOT_NAMES slot, bool quiet)
{
    ITEM		*item;
    BUF		buf;
    int			 ix, iy;

    if (!ableToDequip(slot, quiet))
	return false;

    item = getEquippedItem(slot);

    // Find a slot for it to go to.
    findItemSlot(ix, iy);

    // Take the item off...
    item->setPos(ix, iy);
    if (!quiet)
	formatAndReport("%U <remove> %IU from %r %B1.",
		    item, getSlotName(slot));

    rebuildAppearance();
    rebuildWornIntrinsic();

    return true;
}

bool
MOB::actionPray()
{
    // Non avatars have it simple.
    if (!isAvatar())
    {
	formatAndReport("%U <utter> a quick prayer.");
	return true;
    }

    formatAndReport("%U <query> the gods...");

    pietyReport();
    
    return true;
}

bool
MOB::actionPossess(MOB *target, int duration)
{
    if (target == this)
    {
	formatAndReport("%U already <control> %MU.", target);
	return true;
    }
    target->setTimedIntrinsic(this, INTRINSIC_POSSESSED, duration);
    formatAndReport("%U <gain> control of %MU.", target);
    // Transfer avatarhood.
    if (isAvatar())
	setAvatar(target);
    // The source creature is marked as braindead.
    setIntrinsic(INTRINSIC_BRAINDEAD);
    return true;
}

bool
MOB::actionReleasePossession(bool systemshock)
{
    if (!hasIntrinsic(INTRINSIC_POSSESSED))
    {
	formatAndReport("%U <be> already of one mind.");
	return true;
    }

    MOB		*orig;

    orig = getInflictorOfIntrinsic(INTRINSIC_POSSESSED);
    if (!orig || orig->hasIntrinsic(INTRINSIC_DEAD))
    {
	formatAndReport("%U <realize> %r original body is no more!");
	// Clear status to return original controller
	clearIntrinsic(INTRINSIC_POSSESSED);
	// This counts as another death - which handles the avatar
	// suiciding here.
	// Hmm... If our orig exists we technically should be becameitem
	// since we are now a corpse...  Though, on further thought,
	// tha twas handled *when* it became a corpse and this is
	// about the victory screen.  If we allow the user to wait
	// for their body to decay, this needs to be tightned
	// up anyways.
	triggerAsDeath(ATTACK_LOSTBODY, 0, 0, false);
	return true;
    }

    // Remove any braindead status
    orig->clearIntrinsic(INTRINSIC_BRAINDEAD);

    // If we are the avatar, restore avatar pointer
    if (isAvatar())
    {
	MAP		*avatarmap;

	avatarmap = glbCurLevel->findMapWithMob(orig);
	setAvatar(orig);
	if (avatarmap && avatarmap != glbCurLevel)
	{
	    // Technically, we want to change levels right away
	    // However, that is a very dangerous thing to do inside of
	    // actionReleasePossession as people like receiveDamage
	    // naively assume that glbCurLevel remains constant.  You
	    // can't just fix those guys either, as they are called
	    // multiple times - say if an area attack fireball kills
	    // the possessed critter.
	    // MAP::changeCurrentLevel(avatarmap);
	    MAP::delayedLevelChange(avatarmap);
	}
    }
    
    // Clear status to return original controller
    orig->formatAndReport("%U <release> control of %MU.", this);
    clearIntrinsic(INTRINSIC_POSSESSED);

    // Apply system shock now.
    if (systemshock)
	orig->systemshock();
    
    return true;
}

bool
MOB::actionForget(SPELL_NAMES spell, SKILL_NAMES skill)
{
    bool		 canforget = false;
    BUF		buf;
    ITEM		*item;
    int			 i;
    const char		*prereq;
    
    if (spell == SPELL_NONE && skill == SKILL_NONE)
    {
	// Nothing to foget.
	formatAndReport("%U <suffer> from deja vu.");
	return true;
    }

    // Each in turn.
    if (spell != SPELL_NONE)
    {
	if (!hasSpell(spell))
	{
	    formatAndReport("%U <forget> %B1 before knowing it.  Impressive.",
		    glb_spelldefs[spell].name);
	}
	else if ((item = getSourceOfIntrinsic((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic)))
	{
	    formatAndReport("%U cannot forget %B1 while using %IU.",
			item, glb_spelldefs[spell].name);
	}
	else
	{
	    canforget = true;
	    
	    // Check that nothing we know prevents us from forgetting it.
	    for (i = 0; i < NUM_SPELLS; i++)
	    {
		if (i == spell)
		    continue;

		if (!hasSpell((SPELL_NAMES) i))
		    continue;

		prereq = glb_spelldefs[i].prereq;
		while (*prereq)
		{
		    if (*((u8 *) prereq) == spell)
		    {
			canforget = false;
			buf.sprintf("Knowing %s prevents %s from being forgotten.",
				    glb_spelldefs[i].name,
				    glb_spelldefs[spell].name);
			reportMessage(buf);
		    }
		    prereq++;
		}
	    }

	    if (canforget)
	    {
		formatAndReport("%U <try> to forget %B1.",
			    glb_spelldefs[spell].name);
		clearIntrinsic((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic);
		// Remove from base type, if present.
		MOB		*base;
		base = getBaseType();
		if (base)
		    base->clearIntrinsic((INTRINSIC_NAMES) glb_spelldefs[spell].intrinsic);
		
		// Report new number of slots.
		formatAndReport("%U <have> %B1.",
			gram_createcount("free spell slot",
					 getFreeSpellSlots(),
					 true));
	    }
	}
    }

    if (skill != SKILL_NONE)
    {
	if (!hasSkill(skill))
	{
	    formatAndReport("%U <forget> %B1 before knowing it.  Impressive.",
		    glb_skilldefs[skill].name);
	}
	else if ((item = getSourceOfIntrinsic((INTRINSIC_NAMES) glb_skilldefs[skill].intrinsic)))
	{
	    formatAndReport("%U cannot forget %B1 while using %IU.",
		    item,
		    glb_skilldefs[skill].name);
	}
	else
	{
	    canforget = true;
	    
	    // Check that nothing we know prevents us from forgetting it.
	    for (i = 0; i < NUM_SKILLS; i++)
	    {
		if (i == skill)
		    continue;

		if (!hasSkill((SKILL_NAMES) i))
		    continue;

		prereq = glb_skilldefs[i].prereq;
		while (*prereq)
		{
		    if (*((u8 *) prereq) == spell)
		    {
			canforget = false;
			buf.sprintf("Knowing %s prevents %s from being forgotten.",
				    glb_skilldefs[i].name,
				    glb_skilldefs[skill].name);
			reportMessage(buf);
		    }
		    prereq++;
		}
	    }

	    if (canforget)
	    {
		formatAndReport("%U <try> to forget %B1.",
			    glb_skilldefs[skill].name);
		clearIntrinsic((INTRINSIC_NAMES) glb_skilldefs[skill].intrinsic);
		// Remove from base type, if present.
		MOB		*base;
		base = getBaseType();
		if (base)
		    base->clearIntrinsic((INTRINSIC_NAMES) glb_skilldefs[spell].intrinsic);
		// Report new number of slots.
		// Report new number of slots.
		formatAndReport("%U <have> %B1.",
			gram_createcount("free skill slot",
					 getFreeSkillSlots(),
					 true));
	    }
	}
    }

    return canforget;
}

// We store what our action bars are bound to in this packed
// structure.  This both reduces the size required and ensures
// we can store both spells and actions.
//
// It also secretly clamps our number of spells at 127 and number
// of action commands to 127.  The latter is likely not a problem,
// but I look forward to some day running into the former limit
// and cursing the fool that left it here.

u8
action_packStripButton(ACTION_NAMES action)
{
    return action;
}

u8
action_packStripButton(SPELL_NAMES spell)
{
    return spell | 128;
}

void
action_unpackStripButton(u8 value, ACTION_NAMES &action, SPELL_NAMES &spell)
{
    spell = SPELL_NONE;
    action = ACTION_NONE;
    if (value & 128)
	spell = (SPELL_NAMES) (value & 127);
    else
	action = (ACTION_NAMES) value;
}

SPRITE_NAMES 
action_spriteFromStripButton(u8 value, int *grey)
{
    SPELL_NAMES		spell;
    ACTION_NAMES	action;

    action_unpackStripButton(value, action, spell);

    if (grey)
	*grey = 257;

    if (spell != SPELL_NONE)
    {
	if (grey)
	{
	    MOB		*avatar = MOB::getAvatar();
	    if (avatar)
	    {
		*grey = avatar->spellCastability(spell);
	    }
	    else
	    {
		// Dead people have no spells!
		*grey = 0;
	    }
	}
	return (SPRITE_NAMES) glb_spelldefs[spell].tile;
    }
    else
	return (SPRITE_NAMES) glb_actiondefs[action].tile;
}

void
action_indexToOverlayPos(int index, int &x, int &y)
{
    if (index < 15)
    {
	x = index * 2;
	y = -2;
    }
    else if (index < 30)
    {
	x = index - 15;
	x *= 2;
	y = 20;
    }
    else if (index < 40)
    {
	x = -1;
	y = (index - 30) * 2;
    }
    else if (index < 50)
    {
	x = 29;
	y = (index - 40) * 2;
    }
    else
    {
	UT_ASSERT(!"Invalid buttons strip");
	x = 7 * 2;
	y = 5 * 2;
    }
}
    
