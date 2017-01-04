/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        speed.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	This controls the speed system of POWDER.
 * 	The speed system is intentionaly simplistic.  More complicated
 * 	systems are attempts to kludge a continuous process on an essentially
 * 	turn oriented game.
 * 	Hopefully this decision does not become too regretable.
 */

#ifndef __speed_h__
#define __speed_h__

class SRAMSTREAM;

//
// These are the possible phases we can be in each turn.
// MOBs get actions every phase, but may want to skip their move
// or their heartbeat depending on the phase.
enum PHASE_NAMES
{
    PHASE_NORMAL,		// heartbeats, everyone gets action.
    PHASE_SLOW,			// heartbeats.  Slowed people get no action.
    PHASE_FAST,			// no heart, fast get action
    PHASE_QUICK,		// no heart, quick get action.
    NUM_PHASES
};

// One time speed initialization.
void speed_init();

// Returns the current phase.
PHASE_NAMES speed_getphase();

// Returns true if we are in a heartbeat phase.
bool	    speed_isheartbeatphase();

// Returns true if we are on this modulus of the heartbeat
bool	    speed_matchheartbeat(int modulus);

// Increments the global time
void speed_ticktime();

// Resets the time to 0 for a new game.
void speed_reset();

// Get the current time value.
int speed_gettime();

// Save the speed related information.
void speed_save(SRAMSTREAM &os);

// And load
void speed_load(SRAMSTREAM &is);

#endif
