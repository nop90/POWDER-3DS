/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        ai.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	ai.cpp contains all the AI for MOBs in POWDER.
 *
 *	The hook for all these routines is doAI(), which is called when
 *	MOBs get an action phase.  This will call a series of utility
 *	functions with the "ai" prefix to perform what would be the
 *	best action for this MOB at this time.
 *
 *	Actually performing the action is done by invoking the correct
 *	"action" prefixed method.  These ai methods thus *do* nothing,
 *	they merely determine what should be done, and then call action*
 *	to do it.
 */

#include "mygba.h"

#include <stdio.h>
#include <ctype.h>
#include "assert.h"
#include "creature.h"
#include "glbdef.h"
#include "rand.h"
#include "map.h"
#include "item.h"
#include "itemstack.h"
#include "intrinsic.h"
#include "control.h"
#include "sramstream.h"

struct MOB_MEMORY
{
    // Who remembers this fact
    MOBREF		id;
    // How many turns it will be remembered for.
    u8			turns;
    // What element this consists of.
    u8			element;
};

PTRSTACK<MOB_MEMORY *>	glbMobMemory;

void
ai_init()
{
    ai_reset();
}

void
ai_reset()
{
    int		idx;

    for (idx = 0; idx < glbMobMemory.entries(); idx++)
    {
	delete glbMobMemory(idx);
    }
    glbMobMemory.clear();
}

void
ai_save(SRAMSTREAM &os)
{
    int		idx;

    // FIrst, collapse list so no zeros
    glbMobMemory.collapse();
    
    os.write(glbMobMemory.entries(), 32);

    for (idx = 0; idx < glbMobMemory.entries(); idx++)
    {
	glbMobMemory(idx)->id.save(os);
	os.write(glbMobMemory(idx)->turns, 8);
	os.write(glbMobMemory(idx)->element, 8);
    }
}

void
ai_load(SRAMSTREAM &is)
{
    // Reset global list
    ai_reset();

    int		val, i, count;

    is.uread(count, 32);
    for (i = 0; i < count; i++)
    {
	MOB_MEMORY	*mem;
	mem = new MOB_MEMORY;

	mem->id.load(is);
	is.uread(val, 8);
	mem->turns = val;
	is.uread(val, 8);
	mem->element = val;
    }
}

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

bool
mob_shouldCast(int rarity = 0)
{
    return !rand_choice(3+rarity);
}

//
// doAI: This runs the AI script for any MOB.   It should handle all
// MOB types.
//
void
MOB::doAI()
{
    int			diffx, diffy;
    int			dx, dy;
    int			distx, disty;
    int			range;
    bool		skipmovecheck;
    int			ai = glb_mobdefs[myDefinition].ai;
    bool		targetinfov = false;

    MOB			*killtarget;
    MOB			*aitarget;
    MOB			*avatar = MOB::getAvatar();
    MOB			*owner = getMaster();

    // Determine if our FSM is currently on the just-born
    // delay state.  If so, revert to standard command.
    if (myAIFSM == AI_FSM_JUSTBORN)
    {
	myAIFSM = AI_FSM_NORMAL;
	return;
    }

    if (hasIntrinsic(INTRINSIC_BRAINDEAD))
    {
	// The AI has been deactivated, likely due to having
	// possessed another creature.
	return;
    }

    // Specific types of AI have specific types of actions...
    if (getAI() == AI_MOUSE)
    {
	if (aiDoMouse())
	    return;
    }

    // These are the highest priority moves.  Stuff like healing, etc.
    if (aiDoUrgentMoves())
	return;

    // Check to see if we can see our enemy...
    killtarget = 0;
    
    // Process our AI state.
    switch (myAIFSM)
    {
	case AI_FSM_JUSTBORN:
	    myAIFSM = AI_FSM_NORMAL;
	    break;
	case AI_FSM_NORMAL:
	    if (owner)
	    {
		myAIFSM = AI_FSM_GUARD;
	    }
	    break;

	case AI_FSM_ATTACK:
	    // If our target is dead, return to guarding.
	    aitarget = getAITarget();
	    if (!aitarget || aitarget->hasIntrinsic(INTRINSIC_DEAD))
	    {
		// Return to guarding.
		myAIFSM = AI_FSM_GUARD;
	    }
	    break;

	case AI_FSM_GUARD:
	    if (!owner)
		myAIFSM = AI_FSM_NORMAL;
	    else
	    {
		// Look at our owner's ai target.  We do NOT target ourselves,
		// even if our owner did.
		// We also refuse to target any pets, or fellow pets.
		// (They should have never set their ai target anyways,
		// but life is full of surprises.)
		aitarget = owner->getAITarget();

		if (aitarget && aitarget != this &&
		    !aitarget->hasCommonMaster(this))
		{
		    // Convert to kill!
		    myAIFSM = AI_FSM_ATTACK;
		    setAITarget(aitarget);
		}
	    }
	    break;

	case AI_FSM_STAY:
	    if (!owner)
		myAIFSM = AI_FSM_NORMAL;

	    // Always stay!
	    break;
    }

    aitarget = getAITarget();
    if (aitarget)
    {
	// Double check our aitarget lives.  It may have died
	// and we just a have a corpse reference.
	if (aitarget->hasIntrinsic(INTRINSIC_DEAD))
	{
	    clearAITarget();
	    aitarget = 0;
	}
	else
	{
	    if (canSense(aitarget))
		killtarget = aitarget;
	}
    }

    // If our ai type attacks the avatar, check that...
    if (!killtarget && avatar && (avatar != owner))
    {
	// Check to see if we are an avatar hunter.  
	// ie, the hostile flag is set.
	if (getAttitude(avatar) == ATTITUDE_HOSTILE)
	{
	    // If we are in the avatar's FOV, we will charge...
	    if (canSense(avatar))
	    {
		// One final test if the avatar is part of the same
		// chain as we are.  This way imps summoned by demons
		// won't target you.
		if (!hasCommonMaster(avatar))
		    killtarget = avatar;
	    }
	}
    }

    if (killtarget)
    {
	// Determine shortest direction...
	diffx = killtarget->getX() - getX();
	diffy = killtarget->getY() - getY();
	dx = sign(diffx);
	dy = sign(diffy);
	distx = abs(diffx);
	disty = abs(diffy);
	range = MAX(distx, disty);

	targetinfov = true;

	// If the kill target is the avatar,
	// check to see if we have a pure LOS to the avatar,
	// ie, if we are in the FOV.  If not, we want to use the smell
	// gradient.
	if (killtarget->isAvatar() && !glbCurLevel->hasFOV(getX(), getY()))
	{
	    glbCurLevel->getAvatarSmellGradient(this, getX(), getY(), dx, dy);
	    targetinfov = false;
	}
    }
    
    // If so, we try to kill it.
    if (killtarget)
    {
	// We only get here if we know where the target is, so don't
	// need to recheck LOS.

	// We now know the location of our enemy, so mark it.
	if (glb_aidefs[getAI()].markattackpos)
	{
	    myAIState &= ~AI_LAST_HIT_LOC;
	    myAIState |= killtarget->getX() | killtarget->getY() * MAP_WIDTH;
	}

	// Check if we have a straight line.
	if (!dx || !dy || (distx == disty) )
	{
	    if (targetinfov && aiDoRangedAttack(dx, dy, range))
		return;
	}

	// Check to see if any way is passable...
	// Because we have someone we hate, we try not to make
	// any new enemies.

	skipmovecheck = false;

	// If we are one distance away, don't need to check.
	// This allows us to attack people on unmoveable terrain.
	if (((distx == 1) && (disty == 0)) ||
	    ((distx == 0) && (disty == 1)))    
	{
	    skipmovecheck = true;;
	}
	
	// Grid bugs are allowed to skip if both are one.
	if (canMoveDiabolically() &&
	    distx == 1 && disty == 1)
	{
	    skipmovecheck = true;
	}

	// If we are 2x2 in size, we can attack with certain
	// other combinations.
	if (getSize() >= SIZE_GARGANTUAN)
	{
	    // Walk east.
	    if (dx == 1 && distx == 2 && disty <= 1 && (dy == 0 || dy == 1))
	    {
		skipmovecheck = true;
		dx = 1;
		dy = 0;
	    }
	    // Walk west:
	    if (dx == -1 && distx == 1 && disty <= 1 && (dy == 0 || dy == 1))
	    {
		skipmovecheck = true;
		dx = -1;
		dy = 0;
	    }
	    // Walk south:
	    if (dy == 1 && disty == 2 && distx <= 1 && (dx == 0 || dx == 1))
	    {
		skipmovecheck = true;
		dx = 0;
		dy = 1;
	    }
	    // Walk north:
	    if (dy == -1 && disty == 1 && distx <= 1 && (dx == 0 || dx == 1))
	    {
		skipmovecheck = true;
		dx = 0;
		dy = -1;
	    }
	}

	// If we haven't set skipmove check, we'll be trying to close
	// in.  We, however, don't want to do this if we have battle
	// prep stuff (Buffs, etc.)
	if (!skipmovecheck)
	    if (aiDoBattlePrep(diffx, diffy))
		return;

	// If we are currently in melee, we may be wanting to flee.
	if (skipmovecheck)
	{
	    if ( (killtarget->getExpLevel() > getExpLevel() + 2) ||
		 (getHP() < 5 && getMaxHP() > 20) )
	    {
		if (aiDoFleeMelee(killtarget))
		    return;
	    }

	    // Continue good old melee.
	    actionBump(dx, dy);
	    return;
	}

	// Walk in the dx/dy direction.
	if (aiMoveInDirection(dx, dy, distx, disty, killtarget))
	    return;
    }

    if (!killtarget &&
	 aitarget && glb_aidefs[getAI()].markattackpos &&
	(myAIState & AI_LAST_HIT_LOC))
    {
	// Try to move where the attack came from.
	dx = sign( (myAIState & 31) - getX() );
	dy = sign( ((myAIState >> 5) & 31) - getY() );
	distx = abs( (myAIState & 31) - getX() );
	disty = abs( ((myAIState >> 5) & 31) - getY() );
	range = MAX(distx, disty);

	if (!dx && !dy)
	{
	    // We got to the loc, but found no one!  Clear out
	    // the loc with 30% chance.
	    // This will have the mob become bored of the loc eventually.
	    if (rand_chance(30))
		myAIState &= ~AI_LAST_HIT_LOC;
	}
	else
	{
	    MOBREF	thismobref;

	    thismobref.setMob(this);
	    if (aiMoveInDirection(dx, dy, distx, disty, aitarget))
	    {
		// We *may* now know the location of our enemy.
		// By double checking here a mob chasing you can see you
		// at the end of  its turn and thus catch up.
		MOB	*thismob;

		thismob = thismobref.getMob();
		if (thismob && glb_aidefs[thismob->getAI()].markattackpos)
		{
		    aitarget = thismob->getAITarget();
		    if (aitarget && thismob->canSense(aitarget))
		    {
			thismob->myAIState &= ~AI_LAST_HIT_LOC;
			thismob->myAIState |= aitarget->getX() | 
					      aitarget->getY() * MAP_WIDTH;
		    }
		}
		return;
	    }
	}
    }

    if (aiUseInventory())
	return;

    if (aiEatStuff())
	return;

    if (aiBePackrat())
	return;

    // Pets told to stay will always stay in place.
    if (myAIFSM == AI_FSM_STAY)
	return;

    // Follow our master...
    if (owner)
    {
#if 0
	dx = sign(owner->getX() - getX());
	dy = sign(owner->getY() - getY());
	distx = abs(owner->getX() - getX());
	disty = abs(owner->getY() - getY());
	range = MAX(distx, disty);

	// If we are beside our owner, no need to follow.  That would
	// just cause us to swap places, which would be annoying.
	if (range > 1)
	{
	    // Cancel out invalid follows.
	    if (dx)
	    {
		if (!canMove(getX()+dx, getY(), true,
			    aiWillOpenDoors()) || 
		    glbCurLevel->getMob(getX()+dx, getY()))
		{
		    dx = 0;
		}
	    }
	    if (dy)
	    {
		if (!canMove(getX(), getY()+dy, true,
			    aiWillOpenDoors()) || 
		    glbCurLevel->getMob(getX(), getY()+dy))
		{
		    dy = 0;
		}
	    }

	    // If both set, unset one of them.
	    if (dx && dy && !canMoveDiabolically())
	    {
		if (rand_choice(2))
		    dx = 0;
		else
		    dy = 0;
	    }

	    // Move in the chosen direction, if it exists.
	    if (dx || dy)
	    {
		actionBump(dx, dy);
		return;
	    }
	}
#else
	distx = abs(owner->getX() - getX());
	disty = abs(owner->getY() - getY());
	range = MAX(distx, disty);

	if (range > 1)
	{
	    if (owner->isAvatar() &&
		glbCurLevel->getAvatarSmellGradient(this, getX(), getY(), dx, dy))
	    {
		// Target is 0 because we don't want to swap
		// with the avatar.
		if (aiMoveInDirection(dx, dy, distx, disty, 0))
		{
		    // Only return if we successful use the hint.
		    return;
		}
	    }
	    
#if 0
	    // This code works, but is SLOW.
	    if (glbCurLevel->findPath(
			 getX(), getY(), 
			 owner->getX(), owner->getY(),
			 (MOVE_NAMES) getMoveType(), 
			 false, false, 
			 dx, dy))
	    {
		if (dx || dy)
		{
		    actionBump(dx, dy);
		    return;
		}
	    }
#endif
	}
#endif
    }

    // if we are the avatar and are standing on downstairs, dive deeper!
    if (isAvatar())
    {
	SQUARE_NAMES		tile;
	tile = glbCurLevel->getTile(getX(), getY());
	if (tile == SQUARE_LADDERDOWN)
	{
	    if (actionClimb())
		return;
	}

	// TODO: it would be nice if we tried something a bit more
	// interesting here.
	// Like, find nearest unexplored square and go to it.
    }
	    

    // Random movement...
    // We have a 25% chance of just waiting.
    // After all, we have no hurry.  This allows the avatar
    // to position himself, and doesn't guarantee a flee/attack
    // in narrow corridors.
    if (!rand_choice(4))
	return;
    
    findRandomValidMove(dx, dy, owner);

    actionBump(dx, dy);
}

bool
MOB::aiMoveInDirection(int dx, int dy, int distx, int disty,
		    MOB *target)
{
    bool		 skipmovecheck = false;
    MOB			*blocker;
    int			 delta;

    // Grid bugs need to do a move check on the diagonal.
    // If diagonal works, skipmovecheck.
    if (canMoveDiabolically() && dx && dy)
    {
	if (canMoveDelta(dx, dy, true, aiWillOpenDoors(), true))
	{
	    // Check to see if we hit someone other than
	    // our target...
	    blocker = glbCurLevel->getMob(getX() + dx, getY() + dy);
	    if (!blocker || (blocker == target))
	    {
		// dx+dy is a valid move...
		skipmovecheck = true;
	    }
	}
    }
    
    if (!skipmovecheck && dx != 0)
    {
	if (canMove(getX() + dx, getY(), true, aiWillOpenDoors(), true))
	{
	    delta = dx;
	    if (getSize() >= SIZE_GARGANTUAN)
	    {
		if (delta > 0)
		    delta *= 2;
	    }
	    // Check to see if we hit someone other than
	    // our target...
	    blocker = glbCurLevel->getMob(getX() + delta, getY());
	    if (!blocker || (blocker == target))
	    {
		// dx maybe a valid move.
		if (getSize() >= SIZE_GARGANTUAN)
		{
		    blocker = glbCurLevel->getMob(getX() + delta, getY()+1);
		    if (blocker && blocker != target)
			dx = 0;
		}
	    }
	    else
		dx = 0;
	}
	else
	    dx = 0;
    }
    if (!skipmovecheck && dy != 0)
    {
	if (canMove(getX(), getY() + dy, true, aiWillOpenDoors(), true))
	{
	    delta = dy;
	    if (getSize() >= SIZE_GARGANTUAN)
	    {
		if (delta > 0)
		    delta *= 2;
	    }

	    // Check to see if we hit someone other than
	    // our target...
	    blocker = glbCurLevel->getMob(getX(), getY() + delta);
	    if (!blocker || (blocker == target))
	    {
		// dy maybe a valid move...
		if (getSize() >= SIZE_GARGANTUAN)
		{
		    blocker = glbCurLevel->getMob(getX()+1, getY() + delta);
		    if (blocker && blocker != target)
			dy = 0;
		}
	    }
	    else
		dy = 0;
	}
	else
	    dy = 0;
    }

    // If both dx & dy are set, zero out one of them at random.
    // Grid bugs are the exception, of course.  However, if they did
    // not skip the movecheck, the diagonal was an invalid direction.
    if (dx && dy && (!canMoveDiabolically() || !skipmovecheck))
    {
	if (rand_choice(2))
	    dx = 0;
	else
	    dy = 0;
    }

    // If either is set, bump!
    if (dx || dy)
    {
	// Holy fuck does this get complicated quickly.
	// I never meant to create this monstrosity of a conditional!
	// (Those who were just grepping for swear words: Shame!)
	if (( (distx > 2 && dx) || (disty > 2 && dy) ) &&
	    hasIntrinsic(INTRINSIC_JUMP) &&
	    canMoveDelta(dx * 2, dy * 2, true, false, true) &&
	    !hasIntrinsic(INTRINSIC_BLIND) &&
	     glbCurLevel->isLit(getX() + dx * 2, getY() + dy * 2) &&
	    (!glbCurLevel->getMob(getX() + dx * 2, getY() + dy * 2) ||
	     (glbCurLevel->getMob(getX() + dx * 2, getY() + dy * 2) == this)))
	{
	    int		tile;
	    
	    // Verify the ground is solid enough to jump.
	    tile = glbCurLevel->getTile(getX(), getY());
	    if (!(glb_squaredefs[tile].movetype & MOVE_WALK) &&
		 (!hasIntrinsic(INTRINSIC_WATERWALK) ||
		      !(glb_squaredefs[tile].movetype & MOVE_SWIM)))
	    {
		// The ground isn't solid enough!
		return actionBump(dx, dy);
	    }
	    else
	    {
		// Note we will still try and jump from buried or
		// space walk situations, but these problems aren't
		// any better off with bumping, so no need to special
		// case.
		// Jump to close quickly.
		return actionJump(dx, dy);
	    }
	}
	else
	    return actionBump(dx, dy);
    }

    return false;
}

bool
MOB::aiCanTargetResist(MOB *target, ELEMENT_NAMES element) const
{
    // We only track info on avatar, so early exit if invalid
    if (!target || !target->isAvatar())
	return false;
    
    MOB		*master;
    master = getMaster();
    if (master)
    {
	// Check master's knowledge base.
	if (master->aiCanTargetResist(target, element))
	    return true;
	// We may know locally as well, so still check our own setting.
    }

    // Search through the memory list.
    int		idx;

    for (idx = 0; idx < glbMobMemory.entries(); idx++)
    {
	if (!glbMobMemory(idx))
	    continue;

	if (glbMobMemory(idx)->id.getMob() == this &&
	    glbMobMemory(idx)->element == element)
	{
	    // We have a match!
	    return true;
	}
    }

    // Not found, fail
    return false;
}

void
MOB::aiDecayKnowledgeBase()
{
    // Currently only avatar has such a table
    if (!isAvatar())
	return;

    int		idx;

    for (idx = 0; idx < glbMobMemory.entries(); idx++)
    {
	if (glbMobMemory(idx))
	{
	    if (!glbMobMemory(idx)->id.getMob() ||
		!glbMobMemory(idx)->turns)
	    {
		delete glbMobMemory(idx);
		glbMobMemory.set(idx, 0);
	    }
	    else
		glbMobMemory(idx)->turns--;
	}
    }

    // Collpase the list
    glbMobMemory.collapse();
}

void
MOB::aiNoteThatTargetHasIntrinsic(MOB *target, INTRINSIC_NAMES intrinsic)
{
    switch (intrinsic)
    {
	case INTRINSIC_RESISTFIRE:
	    aiNoteThatTargetCanResist(target, ELEMENT_FIRE);
	    break;
	case INTRINSIC_RESISTCOLD:
	    aiNoteThatTargetCanResist(target, ELEMENT_COLD);
	    break;
	case INTRINSIC_RESISTACID:
	    aiNoteThatTargetCanResist(target, ELEMENT_ACID);
	    break;
	case INTRINSIC_RESISTSHOCK:
	    aiNoteThatTargetCanResist(target, ELEMENT_SHOCK);
	    break;
	case INTRINSIC_BLIND:
	    aiNoteThatTargetCanResist(target, ELEMENT_LIGHT);
	    break;
	case INTRINSIC_RESISTPHYSICAL:
	    aiNoteThatTargetCanResist(target, ELEMENT_PHYSICAL);
	    break;

	// Fake elements:
	case INTRINSIC_RESISTPOISON:
	    aiNoteThatTargetCanResist(target, ELEMENT_POISON);
	    break;
	case INTRINSIC_REFLECTION:
	    aiNoteThatTargetCanResist(target, ELEMENT_REFLECTIVITY);
	    break;
	case INTRINSIC_RESISTSLEEP:
	    aiNoteThatTargetCanResist(target, ELEMENT_SLEEP);
	    break;

	default:
	    // Everything else is ignored.
	    break;
    }
}

void
MOB::aiNoteThatTargetCanResist(MOB *target, ELEMENT_NAMES element)
{
    // If the target isn't the avatar, we don't track
    if (!target || !target->isAvatar())
	return;

    // 50% chance of noting resistance
    if (!rand_chance(50))
	return;

    aiTrulyNoteThatTargetCanResist(target, element);
}

void
MOB::aiTrulyNoteThatTargetCanResist(MOB *target, ELEMENT_NAMES element)
{
    // Pass on to our master, if any.
    MOB		*master;

    master = getMaster();
    if (master)
    {
	master->aiTrulyNoteThatTargetCanResist(target, element);
    }

    // Verify we are not the avatar, as we don't care about our own
    // references :>
    if (isAvatar())
	return;

    // Check the avatar resist list for this mob.
    int		idx;
    for (idx = 0; idx < glbMobMemory.entries(); idx++)
    {
	if (glbMobMemory(idx))
	{
	    if (glbMobMemory(idx)->id.getMob() == this &&
		glbMobMemory(idx)->element == element)
	    {
		// Restore the counter.
		glbMobMemory(idx)->turns = 100;
		return;
	    }
	}
    }

    // Failed to find on list, append.
    MOB_MEMORY		*mem = new MOB_MEMORY;
    mem->id.setMob(this);
    mem->element = element;
    mem->turns = 100;
    glbMobMemory.append(mem);
}

bool
MOB::aiDoMouse()
{
    int		lastdir, dir, newdir;
    int		dx, dy;
    bool	found = false;

    // Least two bits track our last direction.
    lastdir = myAIState & 3;

    // We want to try and track a wall.  We want this wall to
    // be on our right hand side.
    // Mob looking right:
    // ?@  <- turn south to keep wall.
    // X.
    //
    // ?@. <= Bad state, no old wall!  Keep going straight.
    // ..?
    //
    // ?@. <= Go straight
    // ?X?
    //
    // ?.?
    // ?@X <= Turn and head north.
    // XX?
    //
    // ?X?
    // .@X <= Turn around.
    // ?X?  

    // Is there a wall to the right?
    getDirection((lastdir + 1) & 3, dx, dy);
    
    if (!canMoveDelta(dx, dy, true, aiWillOpenDoors()) || 
	glbCurLevel->getMob(getX() + dx, getY() + dy))
    {
	// Got a wall to the right.  Try first available direction.
	dir = lastdir;

	while (1)
	{
	    getDirection(dir, dx, dy);
	    if (canMoveDelta(dx, dy, true, aiWillOpenDoors()) &&
		!glbCurLevel->getMob(getX() + dx, getY() + dy))    
	    {
		newdir = dir;
		break;
	    }
	    // Keeping wall on right means turning left first!
	    dir--;
	    dir &= 3;
	    if (dir == lastdir)
	    {
		newdir = -1;
		break;
	    }
	}
    }
    else
    {
	// No wall to the right.  If there is a wall to the right &
	// behind us, we want to turn right.
	// Otherwise, we're in a bad state and keep going.
	int		bx, by;

	getDirection( (lastdir + 2) & 3, bx, by);

	// This neat trick gets us the back right wall:
	bx |= dx;
	by |= dy;

	if (!canMoveDelta(bx, by, true, aiWillOpenDoors()) ||
	    glbCurLevel->getMob(getX() + bx, getY() + by))
	{
	    // There is a wall, want to turn right.
	    newdir = lastdir + 1;
	    newdir &= 3;
	}
	else
	{
	    // Go straight.
	    newdir = lastdir;
	}
    }

    if (newdir == -1)
	newdir = lastdir;
    else
	found = true;

    // Store our last direction.
    myAIState &= ~3;
    myAIState |= newdir;

    if (found)
    {
	return actionBump(dx, dy);
    }

    // Mouse is cornered!  Return to normal AI.
    return false;
}

bool
MOB::aiDoUrgentMoves()
{
    // This determines if there is any condition that should
    // trump everything else.

    // 0) If turning to stone, stop it.
    if (hasIntrinsic(INTRINSIC_STONING))
    {
	if (aiDoStopStoningSelf())
	    return true;
    }

    // 1) If health is low, heal.
    int		safelevel;
    safelevel = (getMaxHP() >> 3) + 1;
    if ((safelevel < getMaxHP()) && (getHP() <= safelevel))
    {
	if (aiDoHealSelf())
	    return true;
    }

    // 2) If poisoned, cure it!
    if (aiIsPoisoned())
    {
	if (aiDoCurePoisonSelf())
	    return true;
    }

    //  2.5, if on dangerous square, deal with it!
    SQUARE_NAMES	tile = glbCurLevel->getTile(getX(), getY());
    bool		escape = false;

    switch (tile)
    {
	case SQUARE_LAVA:
	case SQUARE_FORESTFIRE:
	    if (!hasIntrinsic(INTRINSIC_RESISTFIRE) ||
		 hasIntrinsic(INTRINSIC_VULNFIRE))
	    {
		if (aiDoFreezeMySquare())
		    return true;
		escape = true;
	    }
	    break;
	case SQUARE_WATER:
	    if (!(getMoveType() & MOVE_FLY) && 
		!hasIntrinsic(INTRINSIC_NOBREATH))
	    {
		escape = true;
	    }
	    break;
	case SQUARE_ACID:
	    if (!(getMoveType() & MOVE_FLY) &&
	       (!hasIntrinsic(INTRINSIC_RESISTACID) ||
		 hasIntrinsic(INTRINSIC_VULNACID)))
	    {
		escape = true;
	    }
	    break;
	case SQUARE_STARS1:
	case SQUARE_STARS2:
	case SQUARE_STARS3:
	case SQUARE_STARS4:
	case SQUARE_STARS5:
	    if (!hasIntrinsic(INTRINSIC_NOBREATH))
	    {
		escape = true;
	    }
	    break;

	default:
	    break;
    }

    if (escape)
    {
	if (aiDoEscapeMySquare())
	    return true;
    }

    // 3) If on fire, douse ourselves with a bottle.
    if (hasIntrinsic(INTRINSIC_AFLAME))
    {
	if (aiDoDouseSelf())
	    return true;
    }

    // Nothing urgent.
    return false;
}

bool
MOB::aiDoFleeMelee(MOB *target)
{
    ITEM		*item;
    WAND_NAMES		 wand;
    MAGICTYPE_NAMES	 mtype;
    SQUARE_NAMES	 tile;
    TRAP_NAMES		 trap;

    int			 dx = 0, dy = 0;

    if (target)
    {
	dx = SIGN(target->getX()-getX()); 
	dy = SIGN(target->getY()-getY());
    }
    // Check to see if we can jump down a hole or climb stairs.
    tile = glbCurLevel->getTile(getX(), getY());
    trap = (TRAP_NAMES) glb_squaredefs[tile].trap;
    switch (trap)
    {
	case TRAP_NONE:
	case TRAP_PIT:
	case TRAP_SPIKEDPIT:
	case TRAP_SMOKEVENT:
	case TRAP_POISONVENT:
	case NUM_TRAPS:
	    break;
	    
	case TRAP_TELEPORT:
	    return actionClimbDown();
	    
	case TRAP_HOLE:
	    return actionClimbDown();
    }

    switch (tile)
    {
	case SQUARE_LADDERUP:
	    // We don't want this triggered in stress test.
	    if (isAvatar())
		break;
	    return actionClimbUp();

	case SQUARE_LADDERDOWN:
	    return actionClimbDown();

	default:
	    break;
    }
    
    // Search our items for a way out.
    if (glb_aidefs[getAI()].zapwands)
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    mtype = item->getMagicType();
	    if (mtype == MAGICTYPE_WAND && (item->getCharges() > 0))
	    {
		wand = (WAND_NAMES) item->getMagicClass();
		
		switch (wand)
		{
		    case WAND_TELEPORT:
			if (target &&
			    (hasIntrinsic(INTRINSIC_TELEFIXED) ||
			     rand_chance(60)))
			{
			    // Teleport away the attacker.
			    // This is preferable as it is more annoying
			    // to the player and better matches how players
			    // react to a single powerful threat.
			    return actionZap(item->getX(), item->getY(), 
					dx, dy, 0);
			}
			// Zap at self.
			return actionZap(item->getX(), item->getY(), 0, 0, 0);
			
		    case WAND_DIGGING:
			// Zap downwards.
			return actionZap(item->getX(), item->getY(), 0, 0, -1);

		    case WAND_SLEEP:
			// If target isn't sleep resist and not already
			// asleep, zap to escape.
			if (target &&
			    !target->hasIntrinsic(INTRINSIC_ASLEEP) &&
			    !aiCanTargetResist(target, ELEMENT_SLEEP))
			{
			    // Try to put our enemy asleep.
			    return actionZap(item->getX(), item->getY(), 
					dx, dy, 0);
			}
			break;
			
		    case WAND_FIRE:
		    case WAND_ICE:
		    case WAND_LIGHT:
		    case WAND_NOTHING:
		    case WAND_CREATETRAP:
		    case WAND_INVISIBLE:
		    case WAND_CREATEMONSTER:
		    case WAND_POLYMORPH:
		    case WAND_SPEED:
		    case WAND_SLOW:
			// Nothing to do with these wands.
			break;

		    case NUM_WANDS:
			UT_ASSERT(!"Invalid wand!");
			break;
		}
	    }
	}
    }

    // Try different spells.
    if (canCastSpell(SPELL_TELEPORT))
    {
	return actionCast(SPELL_TELEPORT, 0, 0, 0);
    }
    if (canCastSpell(SPELL_TELEWITHCONTROL))
    {
	return actionCast(SPELL_TELEWITHCONTROL, 0, 0, 0);
    }
    // TODO: Support blinking!

    // We gotta stand and fight.
    return false;
}

bool
MOB::aiUseInventory()
{
    // Only search if we have something new.
    if (!(myAIState & AI_DIRTY_INVENTORY))
	return false;

    if (glb_aidefs[getAI()].throwitems)
    {
	ITEM		*item;
	bool		 anythrowable = false;

	for (item = myInventory; item; item = item->getNext())
	{
	    if (aiIsItemThrowable(item))
	    {
		anythrowable = true;
		break;
	    }
	}

	if (anythrowable)
	    myAIState |= AI_HAS_THROWABLE;
	else
	    myAIState &= ~AI_HAS_THROWABLE;
    }
    if (glb_aidefs[getAI()].zapwands)
    {
	ITEM		*item;
	bool		 anyzappable = false;

	for (item = myInventory; item; item = item->getNext())
	{
	    if (aiIsItemZapAttack(item))
	    {
		anyzappable = true;
		break;
	    }
	}

	if (anyzappable)
	    myAIState |= AI_HAS_ATTACKWAND;
	else
	    myAIState &= ~AI_HAS_ATTACKWAND;
    }
    
    if (!glb_aidefs[getAI()].useitems)
	return false;
    
    // Check to see if there is anything to equip.
    ITEM		*item;
    ITEMSLOT_NAMES	 slot;

    for (item = myInventory; item; item = item->getNext())
    {
	// Ignore already equipped items.
	if (!item->getX())
	    continue;
	
	// Determine the optimal slot.
	slot = (ITEMSLOT_NAMES) -1;
	if (item->isHelmet())
	    slot = ITEMSLOT_HEAD;
	if (item->isBoots())
	    slot = ITEMSLOT_FEET;
	if (item->isShield())
	    slot = ITEMSLOT_LHAND;
	if (item->isJacket())
	    slot = ITEMSLOT_BODY;
	if (item->isAmulet())
	    slot = ITEMSLOT_AMULET;
	if (item->isRing())
	{
	    slot = ITEMSLOT_RRING;
	    if (getEquippedItem(ITEMSLOT_RRING)
		&& !getEquippedItem(ITEMSLOT_LRING))
		slot = ITEMSLOT_LRING;
	}

	if (slot == -1 &&
	    item->getAttackName() != ATTACK_MISUSED &&
	    item->getAttackName() != ATTACK_MISUSED_BUTWEAPON
	    )
	{
	    // Consider as a weapon.
	    slot = ITEMSLOT_RHAND;
	}

	// Check to see if they could actually equip there...
	if (slot != -1 &&
	    !ableToEquip(item->getX(), item->getY(), slot, true))
	{
	    slot = (ITEMSLOT_NAMES) -1;
	}

	if (slot != -1)
	{
	    // Verify that the new item is better.
	    if (aiIsItemCooler(item, slot))
	    {
		return actionEquip(item->getX(), item->getY(), slot);
	    }
	}
    }

    // Nothing found, clear our ai state.
    myAIState &= ~AI_DIRTY_INVENTORY;

    return false;
}

bool
MOB::aiEatStuff()
{
    if (getHungerLevel() > HUNGER_FORAGE)
    {
	// Full, so don't eat
	return false;
    }
    
    if (!(myAIState & AI_HAS_EDIBLE))
    {
	// We don't know of anything...
	return false;
    }

    ITEM		*item;

    for (item = myInventory; item; item = item->getNext())
    {
	if (canDigest(item) && safeToDigest(item))
	{
	    return actionEat(item->getX(), item->getY());
	}
    }

    // We must not have any edible food after all.
    myAIState &= ~AI_HAS_EDIBLE;

    return false;
}

bool
MOB::aiBePackrat()
{
    bool		hungry = true;
    bool		packrat;

    packrat = glb_aidefs[getAI()].packrat;

    if (getHungerLevel() > HUNGER_FORAGE)
	hungry = false;
    if (!hungry && !packrat)
	return false;
    
    // Pick stuff up.
    ITEM		*item;
    bool		 submerged;

    if (hasIntrinsic(INTRINSIC_INTREE))
	return false;

    submerged = hasIntrinsic(INTRINSIC_INPIT) || 
		hasIntrinsic(INTRINSIC_SUBMERGED);
    item = glbCurLevel->getItem(getX(), getY(), submerged);

    if (item)
    {
	// If we are not a packrat, we only pick up food
	if (!packrat)
	{
	    if (!canDigest(item))
		return false;
	}
	// Check to see if our inventory is full.
	int			 sx, sy;
	if (!findItemSlot(sx, sy))
	    return false;

	return actionPickUp(item);
    }

    return false;
}

bool
MOB::aiDoBattlePrep(int diffx, int diffy)
{
    // We have sighted the enemy, but don't have a straight LOS.
    // Thus it is time to do some cool stuff.
    MOB		*target = glbCurLevel->getMob(getX()+diffx, getY()+diffy);
    int		 range = ABS(diffx) + ABS(diffy);

    // Try to use our items:
    if (glb_aidefs[getAI()].useitems)
    {
	ITEM			*item;
	MAGICTYPE_NAMES		 mtype;
	WAND_NAMES		 wand;

	for (item = myInventory; item; item = item->getNext())
	{
	    mtype = item->getMagicType();

	    if (mtype == MAGICTYPE_POTION)
	    {
		// TODO: Potion cases?
	    }
	    else if (mtype == MAGICTYPE_SCROLL)
	    {
		// TODO: Scroll cases?
	    }
	    else if (mtype == MAGICTYPE_WAND && item->getCharges() > 0 &&
		     glb_aidefs[getAI()].zapwands)
	    {
		wand = (WAND_NAMES) item->getMagicClass();
		switch (wand)
		{
		    case WAND_INVISIBLE:
			// Zap at ourself.
			// If we are already invisble, no need to duplicate.
			if (hasIntrinsic(INTRINSIC_INVISIBLE))
			    break;

			return actionZap(item->getX(), item->getY(), 0, 0, 0);

		    case WAND_SPEED:
			// Zap at ourself.
			// If we are already fast, don't redo.
			if (hasIntrinsic(INTRINSIC_QUICK))
			    break;

			return actionZap(item->getX(), item->getY(), 0, 0, 0);
			
		    case WAND_CREATEMONSTER:
		    {
			// Find a valid spot to make a mob.
			int		dx, dy;

			// Evil laugh.
			for (dy = -1; dy <= 1; dy++)
			{
			    if (getY() + dy < 0 || getY() + dy > MAP_HEIGHT)
				continue;
			    for (dx = -1; dx <= 1; dx++)
			    {
				if (getX() + dx < 0 || getX() + dx > MAP_WIDTH)
				    continue;
				
				if (!dx && !dy)
				    continue;
				
				if (glbCurLevel->canMove(getX() + dx, 
						    getY() + dy, MOVE_WALK) &&
				    !glbCurLevel->getMob(getX() + dx,
						    getY() + dy))
				{
				    return actionZap(item->getX(), item->getY(),
						     dx, dy, 0);
				}
			    }
			}
		    }

		    case WAND_SLOW:
		    case WAND_TELEPORT:
		    case WAND_DIGGING:
		    case WAND_SLEEP:
		    case WAND_FIRE:
		    case WAND_ICE:
		    case WAND_LIGHT:
		    case WAND_NOTHING:
		    case WAND_CREATETRAP:
		    case WAND_POLYMORPH:
			// Nothing to do with these wands.
			break;

		    case NUM_WANDS:
			UT_ASSERT(!"Invalid wand!");
			break;
		}
	    }
	}
    }

    // Try to use spells.
    
    // Keep the target visible to us!
    if (canCastSpell(SPELL_TRACK) && mob_shouldCast())
    {
	// Only do it if our target isn't already 
	if (target && !target->hasIntrinsic(INTRINSIC_POSITIONREVEALED))
	{
	    return actionCast(SPELL_TRACK, diffx, diffy, 0);
	}
    }
    
    // Bring the target into melee range
    if (target && canCastSpell(SPELL_FETCH) && mob_shouldCast(6))
    {
	return actionCast(SPELL_FETCH, diffx, diffy, 0);
    }
    
    if (canCastSpell(SPELL_SUMMON_DEMON) && mob_shouldCast())
    {
	return actionCast(SPELL_SUMMON_DEMON, 0, 0, 0);
    }

    if (canCastSpell(SPELL_SUMMON_IMP) && mob_shouldCast())
    {
	return actionCast(SPELL_SUMMON_IMP, 0, 0, 0);
    }

    if (canCastSpell(SPELL_LIVINGFROST) && mob_shouldCast())
    {
	return actionCast(SPELL_LIVINGFROST, 0, 0, 0);
    }

    // Poison our item.
    if (canCastSpell(SPELL_POISONITEM))
    {
	ITEM		*item;
	
	item = getEquippedItem(ITEMSLOT_RHAND);
	if (item && !item->isPoisoned())
	{
	    return actionCast(SPELL_POISONITEM, 0, 0, 0);
	}
    }
    
    if (!hasIntrinsic(INTRINSIC_REGENERATION) && canCastSpell(SPELL_REGENERATE))
    {
	return actionCast(SPELL_REGENERATE, 0, 0, 0);
    }

    if (!hasIntrinsic(INTRINSIC_RESISTPOISON) && canCastSpell(SPELL_SLOWPOISON))
    {
	return actionCast(SPELL_SLOWPOISON, 0, 0, 0);
    }

    if (canCastSpell(SPELL_FLAMESTRIKE) && mob_shouldCast() &&
	!aiCanTargetResist(target, ELEMENT_FIRE))
    {
	if (target && canSense(target))
	    return actionCast(SPELL_FLAMESTRIKE, diffx, diffy, 0);
    }

    // Fire elementals get a special surprise :>
    if (target && target->getDefinition() == MOB_FIREELEMENTAL &&
	canCastSpell(SPELL_DOWNPOUR) && mob_shouldCast() &&
	canSense(target))
    {
	return actionCast(SPELL_DOWNPOUR, diffx, diffy, 0);
    }

    // If the target is in a pit, consider drowning...
    if (target && target->hasIntrinsic(INTRINSIC_INPIT) &&
	canCastSpell(SPELL_DOWNPOUR) && mob_shouldCast() &&
	canSense(target))
    {
	return actionCast(SPELL_DOWNPOUR, diffx, diffy, 0);
    }

    // Yes, I am evil.
    if (canCastSpell(SPELL_FORCEWALL) && 
	range < 4 &&	    
	mob_shouldCast(6) &&
	!aiCanTargetResist(target, ELEMENT_PHYSICAL))
    {
	return actionCast(SPELL_FORCEWALL, diffx, diffy, 0);
    }

    if (canCastSpell(SPELL_ROLLINGBOULDER) && mob_shouldCast())
    {
	// This is a little bit tricky...
	const static int	sdx[4] = { -1, 1,  0, 0 };
	const static int	sdy[4] = {  0, 0, -1, 1 };

	int		i, r;
	int		bx, by;
	bool		found;
	SQUARE_NAMES	tile;
	ITEM		*boulder;

	for (i = 0; i < 4; i++)
	{
	    // Check to see if there is a boulder or earth tile
	    // in range...
	    bx = getX() + diffx;
	    by = getY() + diffy;
	    
	    found = false;
	    for (r = 0; r < 5; r++)
	    {
		bx += sdx[i];
		by += sdy[i];

		if (bx < 0 || bx >= MAP_WIDTH)
		    break;
		if (by < 0 || by >= MAP_WIDTH)
		    break;

		// See if we found something good.
		tile = glbCurLevel->getTile(bx, by);

		switch (tile)
		{
		    case SQUARE_EMPTY:
		    case SQUARE_WALL:
		    case SQUARE_SECRETDOOR:
			found = true;
			break;
		    default:
			// Not a rolling boulder candidate.
			// If it is not possible to fly through
			// this square, it will block any potential
			// boulders so should stop us...
			if (!glbCurLevel->canMove(bx, by, MOVE_STD_FLY))
			{
			    r = 100;
			}
			break;
		}

		// See if there is a boulder.
		boulder = glbCurLevel->getItem(bx, by);
		if (boulder && boulder->getDefinition() == ITEM_BOULDER)
		    found = true;

		if (found)
		    break;
	    }

	    // If we found a valid source, see if we can start
	    // a boulder from here...
	    if (found)
	    {
		// Determine what the rolling boulder spell
		// would do if we triggered from here...
		int		bdx, bdy;

		bdx = bx - getX();
		bdy = by - getY();

		// If one is bigger than the other, zero the other.
		// (ie, usually do horizontal moves)
		if (abs(bdx) > abs(bdy))
		    bdy = 0;
		else if (abs(bdx) < abs(bdy))
		    bdx = 0;

		bdx = SIGN(bdx);
		bdy = SIGN(bdy);

		// bdx & bdy must match our signed direction
		// or we won't send the boulder in the right direction.
		if (bdx == sdx[i] && bdy == sdy[i])
		{
		    // Finally, we have to be able to see the potential
		    // boulder source...
		    if (glbCurLevel->hasLOS(getX(), getY(), bx, by))
		    {
			// Woohoo!  All passes, go for it!
			return actionCast(SPELL_ROLLINGBOULDER, 
					bx - getX(), by - getY(), 0); 
		    }
		}
	    }
	}		// for each direction.
    }

    if (canCastSpell(SPELL_CREATEPIT) && mob_shouldCast())
    {
	// Determine if target isn't already in a pit.
	switch (glbCurLevel->getTile(getX() + diffx, getY() + diffy))
	{
	    case SQUARE_CORRIDOR:
	    case SQUARE_FLOOR:
	    case SQUARE_FLOORPIT:
	    case SQUARE_PATHPIT:
	    {
		// Determine if target mob is in a pit.
		MOB		*target;

		target = glbCurLevel->getMob(getX() + diffx, getY() + diffy);
		if (target && !target->hasIntrinsic(INTRINSIC_INPIT))
		    return actionCast(SPELL_CREATEPIT, diffx, diffy, 0);
		break;
	    }
	    default:
		break;
	}
    }

    // No further preperation needed.
    return false;
}

bool
MOB::aiDoHealSelf()
{
    // Check for items that can heal...
    ITEM		*item, *polywand = 0;
    MAGICTYPE_NAMES	 mtype;
    POTION_NAMES	 potion;
    SCROLL_NAMES	 scroll;
    WAND_NAMES		 wand;

    if (glb_aidefs[getAI()].useitems)
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    mtype = item->getMagicType();
	    if (mtype == MAGICTYPE_POTION)
	    {
		potion = (POTION_NAMES) item->getMagicClass();
		if (potion == POTION_HEAL)
		{
		    return actionEat(item->getX(), item->getY());
		}
	    }
	    else if (mtype == MAGICTYPE_SCROLL)
	    {
		if (!hasIntrinsic(INTRINSIC_BLIND))
		{
		    scroll = (SCROLL_NAMES) item->getMagicClass();
		    if (scroll == SCROLL_HEAL)
		    {
			return actionRead(item->getX(), item->getY());
		    }
		}
	    }
	    else if (mtype == MAGICTYPE_WAND && (item->getCharges() > 0))
	    {
		wand = (WAND_NAMES) item->getMagicClass();
		if (wand == WAND_POLYMORPH)
		{
		    // This is a very desperate measure...
		    // Of course, it is pointless if we are already
		    // polymorphed or have unchanging.
		    if (!hasIntrinsic(INTRINSIC_UNCHANGING) &&
			!isPolymorphed())
		    {
			polywand = item;
		    }
		}
	    }
	}
    }

    // Check for spells.
    if (canCastSpell(SPELL_MAJORHEAL))
    {
	return actionCast(SPELL_MAJORHEAL, 0, 0, 0);
    }
    if (canCastSpell(SPELL_HEAL))
    {
	return actionCast(SPELL_HEAL, 0, 0, 0);
    }
    if (!hasIntrinsic(INTRINSIC_REGENERATION) && canCastSpell(SPELL_REGENERATE))
    {
	return actionCast(SPELL_REGENERATE, 0, 0, 0);
    }

    // OKay, this is a pretty desperate action, but I think it's reasonable.
    // Note that if we do this every time we fall into this path,
    // creatures with poly wands are *very* annoying.  They will keep
    // re-polying themselves until out of charges.  We thus throw
    // a 30% chance in so you can get a few more hits in before they
    // try again.
    if (polywand && rand_chance(30))
    {
	return actionZap(polywand->getX(), polywand->getY(), 0, 0, 0);
    }

    // No other methods.
    return false;
}

bool
MOB::aiDoCurePoisonSelf()
{
    // Check for items that can heal...
    ITEM		*item;
    MAGICTYPE_NAMES	 mtype;
    POTION_NAMES	 potion;

    if (hasIntrinsic(INTRINSIC_RESISTPOISON))
	return false;

    if (glb_aidefs[getAI()].useitems)
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    mtype = item->getMagicType();
	    if (mtype == MAGICTYPE_POTION)
	    {
		potion = (POTION_NAMES) item->getMagicClass();
		if (potion == POTION_CURE)
		{
		    return actionEat(item->getX(), item->getY());
		}
	    }
	}
    }

    // Check for spells.
    if (canCastSpell(SPELL_CUREPOISON))
    {
	return actionCast(SPELL_CUREPOISON, 0, 0, 0);
    }

    if (!hasIntrinsic(INTRINSIC_RESISTPOISON))
    {
	if (canCastSpell(SPELL_SLOWPOISON))
	    return actionCast(SPELL_SLOWPOISON, 0, 0, 0);
    }

    // No other methods.
    return false;
}

bool
MOB::aiDoDouseSelf()
{
    // Check for water to douse self with.
    ITEM		*item;

    if (glb_aidefs[getAI()].useitems)
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    if (item->getDefinition() == ITEM_WATER)
	    {
		// If the water is holy, and we are undead, we don't
		// want to do this.
		if (item->isBlessed() && getMobType() == MOBTYPE_UNDEAD)
		    continue;

		// Throw at the roof to douse ourselves.
		return actionThrow(item->getX(), item->getY(),
				    0, 0, 1);
	    }
	}
    }

    // Check if we can case downpour on ourself
    if (canCastSpell(SPELL_DOWNPOUR))
    {
	return actionCast(SPELL_DOWNPOUR, 0, 0, 0);
    }

    // TODO: Consider diving into pools?

    // No way to deal with it.
    return false;
}

bool
MOB::aiDoFreezeMySquare()
{
    // Look for wand to use
    ITEM		*item;

    // If we are submerged, bad idea to freeze!
    if (hasIntrinsic(INTRINSIC_SUBMERGED))
	return false;

    if (glb_aidefs[getAI()].zapwands)
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    if (item->getMagicType() == MAGICTYPE_WAND &&
		item->getMagicClass() == WAND_ICE)
	    {
		// Verify there are charges
		if (item->getCharges() > 0)
		{
		    // Zap at the ground.
		    return actionZap(item->getX(), item->getY(),
				    0, 0, -1);
		}
	    }
	}
    }

    // Check if we can cast downpour on ourself
    if (canCastSpell(SPELL_DOWNPOUR))
    {
	return actionCast(SPELL_DOWNPOUR, 0, 0, 0);
    }

    // Check if we can cast frost bolt
    if (canCastSpell(SPELL_FROSTBOLT))
    {
	return actionCast(SPELL_FROSTBOLT, 0, 0, -1);
    }

    // No way to deal with it.
    return false;
}

bool
MOB::aiDoEscapeMySquare()
{
    int		dx, dy;

    findRandomValidMove(dx, dy, 0);
    
    if (dx || dy)
    {
	// We need to bump here so we are willing to open doors!
	// Be embarassing to die on lava with a door beside you that
	// you are banging your head on.
	return actionBump(dx, dy);
    }
    return false;
}

bool
MOB::aiDoStopStoningSelf()
{
    // Check for ways to stop stoning.
    ITEM		*item;
    MAGICTYPE_NAMES	 mtype;
    POTION_NAMES	 potion;
    WAND_NAMES		 wand;

    // Check to see if we care about stoning.
    if (getMaterial() != MATERIAL_FLESH)
	return false;
    if (hasIntrinsic(INTRINSIC_UNCHANGING))
	return false;
    if (hasIntrinsic(INTRINSIC_RESISTSTONING))
	return false;

    // Flesh golem's *want* to stone.
    if (getDefinition() == MOB_FLESHGOLEM)
	return false;

    // As do trolls.
    if (getDefinition() == MOB_TROLL)
	return false;

    // The easiest cure is just to cast preservation
    if (canCastSpell(SPELL_PRESERVE))
    {
	return actionCast(SPELL_PRESERVE, 0, 0, 0);
    }
    
    // Check if we have the unpetrify spell.
    if (canCastSpell(SPELL_STONETOFLESH))
    {
	// Cast stone to flesh on ourselves to prevent the transformation.
	return actionCast(SPELL_STONETOFLESH, 0, 0, 0);
    }

    if (glb_aidefs[getAI()].useitems)
    {
	for (item = myInventory; item; item = item->getNext())
	{
	    mtype = item->getMagicType();
	    if (mtype == MAGICTYPE_POTION)
	    {
		potion = (POTION_NAMES) item->getMagicClass();

		// Quaff acid.
		if (potion == POTION_ACID)
		{
		    return actionEat(item->getX(), item->getY());
		}
	    }
	    else if (mtype == MAGICTYPE_WAND)
	    {
		wand = (WAND_NAMES) item->getMagicClass();

		if (wand == WAND_POLYMORPH && item->getCharges() > 0)
		{
		    // Polymorph ourselves!
		    // We do not need to check for unchanging, as if we
		    // had it, we wouldn't be concerned about stoning.
		    return actionZap(item->getX(), item->getY(), 0, 0, 0);
		}
	    }
	}
    }

    // Last ditch resorts:
    if (canCastSpell(SPELL_ACIDSPLASH))
    {
	return actionCast(SPELL_ACIDSPLASH, 0, 0, 0);
    }

    return false;
}

bool
MOB::aiDoRangedAttack(int dx, int dy, int range)
{
    // Check for a breath attack...
    if (hasBreathAttack() && isBreathAttackCharged())
    {
	if (range <= getBreathRange())
	{
	    // Check main damage of breath attack.
	    ATTACK_NAMES		attack;
	    MOB				*target = glbCurLevel->getMob(getX() + dx * range, getY() + dy * range);

	    attack = getBreathAttack();

	    if (!aiCanTargetResist(target, 
			(ELEMENT_NAMES) glb_attackdefs[attack].element))
		return actionBreathe(dx, dy);
	}
    }

    // Check for wand zap.
    if (aiDoWandAttack(dx, dy, range))
	return true;

    // Check for offensive spell.
    if (aiDoSpellAttack(dx, dy, range))
	return true;

    // Check for throwable.
    // We only throw with a positive range increment.
    if (((dx && dy) || (range > 1)) && aiDoThrownAttack(dx, dy, range))
	return true;

    // Nothing to do.
    return false;
}

bool
MOB::aiDoWandAttack(int dx, int dy, int range)
{
    // Range is the desired range.  Check to see if we have
    // anything in range.  If we don't have anything to throw
    // at all, clear the flag.
    if (!glb_aidefs[getAI()].zapwands)
    {
	myAIState &= ~AI_HAS_ATTACKWAND;
	return false;
    }

    // Abandon if no attack wands present.
    if (!(myAIState & AI_HAS_ATTACKWAND))
	return false;

    ITEM		*item;
    bool		 anygood = false;

    for (item = myInventory; item; item = item->getNext())
    {
	if (aiIsItemZapAttack(item))
	{
	    // Guestimate of wand ranges.
	    if (item->getCharges() > 0)
	    {
		anygood = true;

		// Default range of zap attacks is 6.
		int		rangeguess = 6;
		MOB		*target;

		target = glbCurLevel->getMob(getX() + dx*range, 
					     getY() + dy*range);
		
		if (item->getMagicType() == MAGICTYPE_WAND)
		{
		    switch (item->getMagicClass())
		    {
			case WAND_SLOW:
			    // Slow wands only have a one square effect.
			    rangeguess = 1;

			    // Check if target is slowed.
			    if (target && target->hasIntrinsic(INTRINSIC_SLOW))
				rangeguess = -1;
			    break;
			case WAND_SLEEP:
			    // Check if target is asleep.
			    if (target && target->hasIntrinsic(INTRINSIC_ASLEEP))
				rangeguess = -1;
			    break;
			case WAND_FIRE:
			    if (aiCanTargetResist(target, ELEMENT_FIRE) ||
				aiCanTargetResist(target, ELEMENT_REFLECTIVITY))
				rangeguess = -1;
			    break;
			case WAND_ICE:
			    if (aiCanTargetResist(target, ELEMENT_COLD) ||
				aiCanTargetResist(target, ELEMENT_REFLECTIVITY))
				rangeguess = -1;
			    break;
		    }
		}

		// We don't ALWAYS use the wand.  Usually do, however.
		if (range <= rangeguess && !rand_choice(3))
		{
		    // We have the one we want!
		    return actionZap(item->getX(), item->getY(),
					dx, dy, 0);
		}
	    }
	}
    }

    // We could be abandoning due to range.  Thus, we only clear
    // if nothing triggered throwable.
    if (!anygood)
    {
	myAIState &= ~AI_HAS_ATTACKWAND;
    }
    
    return false;
}

bool
MOB::aiDoSpellAttack(int dx, int dy, int range)
{
    // No magic points?  Early exit.
    if (!getMP())
	return false;
    
    // We do not want to ALWAYS cast the spells!  Otherwise, we
    // never do a variety.  We should sometimes decide to cast
    // no spell...
    // We thus use our shouldCast() function...
    MOB		*target;
    target = glbCurLevel->getMob(getX() + dx*range, getY() + dy*range);

    // Grab the soul if possible
    if (canCastSpell(SPELL_SOULSUCK) && mob_shouldCast())
    {
	if (target && canSense(target) && 
	    !target->hasIntrinsic(INTRINSIC_AMNESIA))
	{
	    return actionCast(SPELL_SOULSUCK, dx*range, dy*range, 0);
	}
    }

    // If our target is one unit away & isn't aflame, set them aflame.
    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_STICKYFLAMES) &&
	!aiCanTargetResist(target, ELEMENT_FIRE))
    {

	if (target && !target->hasIntrinsic(INTRINSIC_AFLAME))
	    return actionCast(SPELL_STICKYFLAMES, dx, dy, 0);
    }

#if 0
    // I have retreated from allowing this.  It is too close to
    // one-shot kills - anything less than half health can be killed
    // by two purple tridudes doing a fetch/disintegrate pair.
    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_DISINTEGRATE) &&
	!aiCanTargetResist(target, ELEMENT_ACID))
    {
	// Yep, I'm evil.
	return actionCast(SPELL_DISINTEGRATE, dx, dy, 0);
    }
#endif

    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_FLASH) &&
	target && !target->hasIntrinsic(INTRINSIC_BLIND))
    {
	return actionCast(SPELL_FLASH, dx, dy, 0);
    }

    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_CHILL) &&
	!aiCanTargetResist(target, ELEMENT_COLD))
    {
	if (target && !target->hasIntrinsic(INTRINSIC_SLOW))
	    return actionCast(SPELL_CHILL, dx, dy, 0);
    }

    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_ACIDSPLASH) &&
	!aiCanTargetResist(target, ELEMENT_ACID))
    {
	return actionCast(SPELL_ACIDSPLASH, dx, dy, 0);
    }

    // Spells in order of coolness.
    if (canCastSpell(SPELL_CHAINLIGHTNING) && mob_shouldCast(3) &&
	!aiCanTargetResist(target, ELEMENT_SHOCK))
    {
	return actionCast(SPELL_CHAINLIGHTNING, dx, dy, 0);
    }

    if (canCastSpell(SPELL_LIGHTNINGBOLT) && mob_shouldCast() &&
	!aiCanTargetResist(target, ELEMENT_SHOCK))
    {
	return actionCast(SPELL_LIGHTNINGBOLT, dx, dy, 0);
    }

    // Don't blow ourselves up with the fireball.
    if ((range > 1 || hasIntrinsic(INTRINSIC_RESISTFIRE)) 
	    && canCastSpell(SPELL_FIREBALL) && mob_shouldCast() &&
	    !aiCanTargetResist(target, ELEMENT_FIRE))
    {
	return actionCast(SPELL_FIREBALL, dx, dy, 0);
    }

    if (canCastSpell(SPELL_POISONBOLT) && mob_shouldCast() &&
	!aiCanTargetResist(target, ELEMENT_POISON))
    {
	return actionCast(SPELL_POISONBOLT, dx, dy, 0);
    }

    // Good old frost bolt.
    if (canCastSpell(SPELL_FROSTBOLT) && mob_shouldCast() &&
	!aiCanTargetResist(target, ELEMENT_COLD))
    {
	return actionCast(SPELL_FROSTBOLT, dx, dy, 0);
    }

    // Magic missile, the stand by of mages.
    if (canCastSpell(SPELL_MAGICMISSILE) && mob_shouldCast() &&
	!aiCanTargetResist(target, ELEMENT_PHYSICAL))
    {
	return actionCast(SPELL_MAGICMISSILE, dx, dy, 0);
    }

    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_SPARK) &&
	!aiCanTargetResist(target, ELEMENT_SHOCK))
    {
	return actionCast(SPELL_SPARK, 0, 0, 0);
    }

    if (range == 1 && mob_shouldCast() && canCastSpell(SPELL_FORCEBOLT) &&
	!aiCanTargetResist(target, ELEMENT_PHYSICAL))
    {
	return actionCast(SPELL_FORCEBOLT, dx, dy, 0);
    }

    // No offensive spells.
    return false;
}

bool
MOB::aiDoThrownAttack(int dx, int dy, int range)
{
    // Range is the desired range.  Check to see if we have
    // anything in range.  If we don't have anything to throw
    // at all, clear the flag.
    if (!glb_aidefs[getAI()].throwitems)
    {
	myAIState &= ~AI_HAS_THROWABLE;
	return false;
    }

    // Abandon if nothing is throwable in our inventory.
    if (!(myAIState & AI_HAS_THROWABLE))
	return false;

    ITEM		*item;
    bool		 anygood = false;
    MOB			*target = glbCurLevel->getMob(getX() + dx*range, getY() + dy * range);

    for (item = myInventory; item; item = item->getNext())
    {
	// If the item is equipped, we can't throw it (PC can't so,
	// why should we be able to?)
	if (!item->getX())
	    continue;
	
	if (aiIsItemThrowable(item, target))
	{
	    anygood = true;
	    
	    const ATTACK_DEF	*attack;

	    attack = item->getThrownAttack();
	    if (attack->range >= range)
	    {
		// We have the one we want!
		return actionThrow(item->getX(), item->getY(),
				    dx, dy, 0);
	    }
	}
    }

    // We could be abandoning due to range.  Thus, we only clear
    // if nothing triggered throwable.
    if (!anygood)
    {
	myAIState &= ~AI_HAS_THROWABLE;
    }
    
    return false;
}

bool
MOB::aiIsItemCooler(ITEM *item, ITEMSLOT_NAMES slot)
{
    ITEM		*old;
    bool		 defresult = false;

    // If the item is silver and we are vulnerable to silver,
    // the item is never cooler.
    if (item->getMaterial() == MATERIAL_SILVER)
    {
	if (hasIntrinsic(INTRINSIC_VULNSILVER))
	    return false;
    }

    old = getEquippedItem(slot);

    // Something is better than nothing.
    if (!old)
	defresult = true;

    // Smarts:
    // FOOL: Equips whatever is first.
    // HUMAN: Picks best AC.
    // DRAGON: Picks best effect.

    // Fools never give up their first pick.
    if (getSmarts() <= SMARTS_FOOL)
	return defresult;

    switch (slot)
    {
	case ITEMSLOT_HEAD:
	case ITEMSLOT_LHAND:
	case ITEMSLOT_BODY:
	case ITEMSLOT_FEET:
	{
	    // Pick best AC.
	    int		newac, oldac;

	    newac = item->getAC();
	    oldac = old ? old->getAC() : -1;
	    if (newac > oldac)
	    {
		// Go for swap.
		return true;
	    }
	    // Must be strictly better.
	    return defresult;
	}

	// Pick best type, so long as smart enough.
	case ITEMSLOT_LRING:
	case ITEMSLOT_RRING:
	case ITEMSLOT_AMULET:
	    if (getSmarts() <= SMARTS_HUMAN)
	    {
		// Never trade up, no idea what it is.
		return defresult;
	    }
	    else
	    {
		int		oldcool, newcool;

		newcool = item->getCoolness();
		oldcool = old ? old->getCoolness() : 0;

		if (!newcool)
		{
		    // Never trade to this.
		    return false;
		}
		else if (newcool > oldcool)
		    return true;
		
		return defresult;
	    }

	case ITEMSLOT_RHAND:
	{
	    // Weapon: Compare damages.
	    int		newdam, olddam;

	    newdam = item->calcAverageDamage();
	    olddam = old ? old->calcAverageDamage() : -1;
	    if (newdam > olddam)
		return true;
	    else
		return defresult;
	}

	case NUM_ITEMSLOTS:
	    UT_ASSERT(!"BAD SLOT NAME");
	    break;
    }

    // No idea?  Then do nothing!
    return defresult;
}

bool
MOB::aiIsItemThrowable(ITEM *item, MOB *target) const
{
    ATTACK_NAMES		attack;

    attack = item->getThrownAttackName();
    // We never really want to throw rings.
    if (attack == ATTACK_MISTHROWN || attack == ATTACK_NONE ||
	attack == ATTACK_RINGTHROWN)
	return false;

    // Until we fix launcher ai, we disallow anything that needs
    // a special launcher.
    if (item->requiresLauncher())
	return false;

    // Potions get this far, but we don't want to throw healing
    // potions at the avatar!
    if (item->getMagicType() == MAGICTYPE_POTION)
    {
	if (!glb_potiondefs[item->getMagicClass()].isgrenade)
	    return false;

	if (aiCanTargetResist(target, 
	    (ELEMENT_NAMES)glb_potiondefs[item->getMagicClass()].grenadeelement))
	    return false;
    }

    return true;
}

bool
MOB::aiIsItemZapAttack(ITEM *item) const
{
    MAGICTYPE_NAMES		mtype;

    mtype = item->getMagicType();
    if (mtype != MAGICTYPE_WAND)
    {
	return false;
    }

    WAND_NAMES			wand;

    wand = (WAND_NAMES) item->getMagicClass();
    switch (wand)
    {
	case WAND_FIRE:
	case WAND_ICE:
	case WAND_SLEEP:
	case WAND_SLOW:
	    // These can be used as offensive weapons.
	    return true;

	case WAND_SPEED:
	case WAND_LIGHT:
	case WAND_NOTHING:
	case WAND_CREATETRAP:
	case WAND_TELEPORT:
	case WAND_INVISIBLE:
	case WAND_DIGGING:	// Technically....
	case WAND_CREATEMONSTER:
	case WAND_POLYMORPH:
	    return false;
	    
	case NUM_WANDS:
	    UT_ASSERT(!"INVALID WAND TYPE");
	    break;
    }
    return false;
}

bool
MOB::aiIsPoisoned() const
{
    int		i;
    bool	haspoison = false;
    
    // If we can resist poison, we don't consider ourselves poisoned.
    // This also means gods won't waste piety curing you if you resist.
    // This could be a bad thing if the final damage would kill the
    // mob and they could have killed.  Likely the use is ready to
    // reapply the poison anyways, though.
    if (hasIntrinsic(INTRINSIC_RESISTPOISON))
    {
	return false;
    }

    for (i = 0; i < NUM_POISONS; i++)
    {
	if (hasIntrinsic((INTRINSIC_NAMES) glb_poisondefs[i].intrinsic))
	{
	    haspoison = true;
	}
    }

    return haspoison;
}

bool
MOB::aiWillOpenDoors() const
{
    return glb_aidefs[getAI()].opendoors;
}
