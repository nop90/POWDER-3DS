/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        hiscore.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Provides routines to manage our high score list.
 */

#ifndef __hiscore__
#define __hiscore__

class MOB;
class SRAMSTREAM;
struct HISCORE_ENTRY;

extern bool glbRNGValid;
extern int glbRNGSeed;

// Inspects the SRAM to determine save version, erases save file
// if necessary.
void
hiscore_init();

// Returns version of POWDER.
int
hiscore_getversion();

// Returns the current hiscore save count.
// 0 means no save game.
// 1 means a valid save game.
// 2 or higher means a save scummed game!
int
hiscore_savecount();

// Sets the save count.
void
hiscore_setsavecount(int newval);

// Bumps the save count up one.
void
hiscore_bumpsavecount();

// Flags as a new game.
void
hiscore_flagnewgame(bool isnewgame);

// Returns if this is a new game.
bool
hiscore_isnewgame();

// Returns if we've got 15 games in high score, this is
// used to activate advanced mode.
bool
hiscore_advancedmode();

// Displays hiscore list.
void
hiscore_display(HISCORE_ENTRY *bonus, bool ischeater);

// Returns true if we add the player to our list.
// Displays the high score, along with the player's place in it.
// If ischeater is set, we will not add to the high score list.
bool
hiscore_addAndDisplayEntry(MOB *player, bool haswon, bool ischeater);

// This must be loaded first!
void
hiscore_load(SRAMSTREAM &os);

// This must be saved first!
void
hiscore_save(SRAMSTREAM &is);

#endif

