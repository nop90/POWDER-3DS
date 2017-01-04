/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        encyc_support.h ( POWDER Library, C++ )
 *
 * COMMENTS:	This defines support functions for the encyclopedia.
 */

#ifndef __encyc_support__
#define __encyc_support__

// Views a certain enumerated key from the given
// book.  Book should be something like "GOD" or "SPELL".
void
encyc_viewentry(const char *book, int key);

// This will add the given entry to the pager. This lets you
// add your own commentary berfore / afterwards.
void
encyc_pageentry(const char *book, int key);

// Returns true if the given entry exists.
bool
encyc_hasentry(const char *book, int key);

// Displays a specific spell's info.
void
encyc_viewSpellDescription(SPELL_NAMES spell);

// Displays a specific skill's info.
void
encyc_viewSkillDescription(SKILL_NAMES skill);

#endif

