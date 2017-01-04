/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        victory.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	This is where the code associated with victory goes...
 * 	It handles the UI associated with listing how cool the avatar is,
 * 	or how much they sucked if didwin == false.
 */

#ifndef __victory_h__
#define __victory_h__

#include "buf.h"

// We are really the only one who cares about this now...
extern char glbAvatarName[100];
extern int  glbGameStartFrame;
extern bool glbWizard;
extern bool glbTutorial;
extern bool glbHasAlreadyWon;
extern bool glbStressTest;
extern bool glbMapStats;
extern bool glbOpaqueTiles;
// This is the gender you *selected*, not the one you *are*.
extern s8   glbGender;

void
glbShowIntrinsic(MOB *mob);
void
victory_buildSortedKillList(MOB_NAMES *mobtypes, int *numtypes=0, int *totalkill=0);
void
glbVictoryScreen(bool didwin, const ATTACK_DEF *attack, MOB *src, ITEM *weapon);

BUF
victory_formattime(int endtime, int *scale = 0);

#endif
