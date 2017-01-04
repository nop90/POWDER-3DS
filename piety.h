/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        piety.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This handles most of the deity related code.
 *	All the piety access functions that alter the global
 *	piety values are triggered through here.
 */

#ifndef __piety__
#define __piety__

class SRAMSTREAM;
class MOB;
class ITEM;

// Reset all the piety scores to zero for a new game.
void piety_init();

// Load piety & god choice.
void piety_load(SRAMSTREAM &is);

// Save the current piety & god choice.
void piety_save(SRAMSTREAM &os);

GOD_NAMES piety_chosengod();
int piety_chosengodspiety();
void piety_setgod(GOD_NAMES god);

#endif
