/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        encyc_support.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:	This defines support functions for the encyclopedia.
 */

#include "glbdef.h"
#include <stdio.h>
#include "grammar.h"
#include "gfxengine.h"
#include "encyclopedia.h"
#include "encyc_support.h"

//
// Purely internal functions:
//

const encyclopedia_entry *
encyc_getentry(const char *bookname, int key)
{
    const encyclopedia_book	*book;
    int				 booknum;
    int				 entrynum;
    
    for (booknum = 0; booknum < NUM_ENCYCLOPEDIA_BOOKS; booknum++)
    {
	book = glb_encyclopedia[booknum];

	if (strcmp(book->bookname, bookname))
	    continue;
	
	for (entrynum = 0; entrynum < book->numentries; entrynum++)
	{
	    if (book->entries[entrynum]->key == key)
		return book->entries[entrynum];
	}
    }

    return 0;
}

//
// Externally available functions:
//

bool
encyc_hasentry(const char *book, int key)
{
    const encyclopedia_entry	*entry;

    entry = encyc_getentry(book, key);

    if (entry)
	return true;
    return false;
}

void
encyc_pageentry(const char *book, int key)
{
    const encyclopedia_entry	*entry;
    int				 i;

    entry = encyc_getentry(book, key);

    // Nothing to view?  Quit.
    if (!entry)
	return;

    // Build word-wrapped display list.
    for (i = 0; i < entry->numlines; i++)
    {
	gfx_pager_addtext(entry->lines[i]);
	gfx_pager_newline();
    }
}

void
encyc_viewentry(const char *book, int key)
{
    const encyclopedia_entry	*entry;

    entry = encyc_getentry(book, key);

    // Nothing to view?  Quit.
    if (!entry)
	return;

    encyc_pageentry(book, key);

    gfx_pager_display();
}

void
encyc_viewSpellDescription(SPELL_NAMES spell)
{
    BUF			buf;
    
    // Plain description:
    gfx_pager_addtext(glb_spelldefs[spell].name);
    gfx_pager_newline();

    // Add standard spell stuff
    gfx_pager_separator();

    if (glb_spelldefs[spell].mpcost)
    {
	buf.sprintf("Magic Cost: %d", glb_spelldefs[spell].mpcost);
	gfx_pager_addtext(buf);
	gfx_pager_newline();
    }
    if (glb_spelldefs[spell].hpcost)
    {
	buf.sprintf("Health Cost: %d", glb_spelldefs[spell].hpcost);
	gfx_pager_addtext(buf);
	gfx_pager_newline();
    }
    if (glb_spelldefs[spell].xpcost)
    {
	buf.sprintf("EXP Cost: %d", glb_spelldefs[spell].xpcost);
	gfx_pager_addtext(buf);
	gfx_pager_newline();
    }
    buf.sprintf("Circle: ");
    buf.strcat(gram_capitalize(glb_spelltypedefs[glb_spelldefs[spell].type].name));
    gfx_pager_addtext(buf);
    gfx_pager_newline();

    // Output any prereqs.
    const u8 *prereq;

    prereq = (const u8 *) glb_spelldefs[spell].prereq;
    if (prereq && *prereq)
    {
	gfx_pager_addtext("This requires ");

	while (*prereq)
	{
	    gfx_pager_addtext(glb_spelldefs[*prereq].name);
	    prereq++;
	    if (*prereq)
	    {
		if (!prereq[1])
		    gfx_pager_addtext(" and ");
		else
		    gfx_pager_addtext(", ");
	    }
	}
	gfx_pager_addtext(".");
	gfx_pager_newline();
    }

    // Add encyclopedia entry, if any.
    {
	const encyclopedia_entry	*entry;

	entry = encyc_getentry("SPELL", spell);
	if (entry)
	{
	    gfx_pager_separator();
	    encyc_pageentry("SPELL", spell);
	}
    }
    

    gfx_pager_display();
}

void
encyc_viewSkillDescription(SKILL_NAMES skill)
{
    // Plain description:
    gfx_pager_addtext(glb_skilldefs[skill].name);
    gfx_pager_newline();

    // Add standard skill stuff
    gfx_pager_separator();

    // Output any prereqs.
    const u8 *prereq;

    prereq = (const u8 *) glb_skilldefs[skill].prereq;
    if (prereq && *prereq)
    {
	gfx_pager_addtext("This requires ");

	while (*prereq)
	{
	    gfx_pager_addtext(glb_skilldefs[*prereq].name);
	    prereq++;
	    if (*prereq)
	    {
		if (!prereq[1])
		    gfx_pager_addtext(" and ");
		else
		    gfx_pager_addtext(", ");
	    }
	}
	gfx_pager_addtext(".");
	gfx_pager_newline();
    }

    // Add encyclopedia entry, if any.
    {
	const encyclopedia_entry	*entry;

	entry = encyc_getentry("SKILL", skill);
	if (entry)
	{
	    gfx_pager_separator();
	    encyc_pageentry("SKILL", skill);
	}
    }
    
    gfx_pager_display();
}
