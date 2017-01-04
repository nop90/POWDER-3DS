/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        speed.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Implementation of the speed system.
 *
 *	Speed is INTENTIONALLY round based.  The goal is to allow
 *	predictable movement, rather than the tendency to floating
 *	point speed systems that have confused (IMO) later roguelikes.
 *
 *	As such, there are three speed modifiers: Fast (F), Quick (Q),
 *	and Slow (S).  There are also 4 types of phases: Fast (F),
 *	Quick (Q), Slow (S), and Normal (N).
 *
 *	doHeartbeat style functions execute only on N & S phases.
 *
 *	doAI movement operates on the phases according to the following
 *	chart:
 *
 *	    F N S Q N Speed	
 *	N     x x   x  100%
 *	F   x x x   x  133%
 *	Q     x x x x  133%
 *	FQ  x x x x x  167%
 *	S     x     x   67%
 *	FS  x x     x  100%
 *	QS    x   x x  100%
 *	FQS x x   x x  133%
 */

#include "speed.h"
#include "sramstream.h"
#include "assert.h"

int glbGameMoves = 0;
int glbHeartbeatCount = 0;

// We cache this so speed_getphase is fast.
PHASE_NAMES	glbCurPhase;

#define PHASE_SEQ_LEN		5

// This is the prebuilt table of phases.  We use it to determine
// our current phase given our time tick.
const PHASE_NAMES glbPhaseOrder[PHASE_SEQ_LEN] =
{
    PHASE_FAST,
    PHASE_NORMAL,
    PHASE_SLOW,
    PHASE_QUICK,
    PHASE_NORMAL
};

void
speed_init()
{
    glbGameMoves = 0;
    glbHeartbeatCount = 0;
    // Initial tick to initialize glbCurPhase.
    speed_ticktime();
}

void
speed_reset()
{
    glbGameMoves = 0;
    glbHeartbeatCount = 0;
    speed_ticktime();
}

void
speed_ticktime()
{
    glbGameMoves++;
    glbCurPhase = glbPhaseOrder[glbGameMoves % PHASE_SEQ_LEN];
    if (speed_isheartbeatphase())
	glbHeartbeatCount++;
}

int
speed_gettime()
{
    return glbGameMoves;
}

PHASE_NAMES
speed_getphase()
{
    return glbCurPhase;
}

bool
speed_isheartbeatphase()
{
    // Determine if we are in a proper phase.
    switch (speed_getphase())
    {
	case PHASE_FAST:
	case PHASE_QUICK:
	    // Do not hearbeat on these phases.
	    return false;

	case PHASE_NORMAL:
	case PHASE_SLOW:
	    // we want a heartbeat.
	    return true;

	case NUM_PHASES:
	    UT_ASSERT(!"Invalid phase!");
	    break;
    }

    // Should not go here.
    return true;
}

bool
speed_matchheartbeat(int modulus)
{
    if (modulus < 2)
	return true;
    
    // Avoid division where possible.
    switch (modulus)
    {
	case 2:
	    if (glbHeartbeatCount & 1)
		return false;
	    return true;
	case 4:
	    if (glbHeartbeatCount & 3)
		return false;
	    return true;
	case 8:
	    if (glbHeartbeatCount & 7)
		return false;
	    return true;
    }

    if (glbHeartbeatCount % modulus)
	return false;
    return true;
}

void
speed_save(SRAMSTREAM &os)
{
    os.write(glbGameMoves, 32);
    os.write(glbHeartbeatCount, 32);
}

void
speed_load(SRAMSTREAM &is)
{
    is.read(glbGameMoves, 32);
    is.read(glbHeartbeatCount, 32);
}
