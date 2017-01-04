/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        msg.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This is the message glue library.  It handles the writing
 *	of status messages or prompts to the display.  It deals with
 *	-more-, scrollback, wordwrap, etc.
 *
 *	For displaying a block of scrollable text, see the gfx_
 *	functions in gfx_engine.
 */

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "control.h"
#include "gfxengine.h"
#include "msg.h"
#include "assert.h"
#include "grammar.h"
#include "map.h"		// Needed for refresh.
#include "stylus.h"
#include "victory.h"

// Width of screen in characters
#define SCREEN_WIDTH	30
// Number of lines per page.
#define PAGE_SIZE 	3

// Just used as our default system sucks
#define HYPHEN_SYMBOL	'-'

// Buttons..
#define CONTINUE_BUTTON	BUTTON_R
#define YES_BUTTON	BUTTON_A
#define NO_BUTTON	BUTTON_B

#define	NUM_STASHES	20

// The current message.
char *glbMsg[PAGE_SIZE];
char *glbStashedMsg[NUM_STASHES][PAGE_SIZE];
int   glbCurLine;
const char *glbBlank = "                              ";
int   glbStashHead, glbStashView;
bool  glbViewingOld;

// Defined in Main.  Really ugly to do this.
// I feel great shame.
extern void writeStatusLine();
extern bool glbAutoRunEnabled;

void
msg_init()
{
    int		i, j;
    for (i = 0; i < PAGE_SIZE; i++)
    {
	glbMsg[i] = new char[SCREEN_WIDTH+1];
	for (j = 0; j < NUM_STASHES; j++)
	{
	    glbStashedMsg[j][i] = new char[SCREEN_WIDTH+1];
	    glbStashedMsg[j][i][0] = '\0';
	}
	glbMsg[i][0] = '\0';
    }
    glbCurLine = 0;
    glbViewingOld = false;
}

void
msg_awaitaccept()
{
    // We take this chance to rebuild our display so we can react
    // to changes.
    // TODO: Since we may be inside fireball code, we don't want to update
    // overlay tiles.  (Or do we?)
    if (glbCurLevel)
	glbCurLevel->refresh(true);
    writeStatusLine();

    if (glbStressTest)
	return;
    if (hamfake_forceQuit())
	return;
    
    // After some play testings, I decided that hitting anything should
    // bring the next message.  It has to be a net new press though.
    
#ifndef HAS_KEYBOARD
    // Wait for it to go off
    while (ctrl_anyrawpressed() && !hamfake_forceQuit());
    // And on
    while (!ctrl_anyrawpressed() && !hamfake_forceQuit());
    // And off...  
    while (ctrl_anyrawpressed() && !hamfake_forceQuit());
#else
    hamfake_clearKeyboardBuffer();

    // Await a new non-trivial keypress & eat it.
    while (!hamfake_getKeyPress(false) && !hamfake_forceQuit())
    {
	hamfake_awaitEvent();
    }
    // We don't want the new press to count as repeatable key.
    hamfake_clearKeyboardBuffer();
#endif

#if 0
    // Wait for L to go off..
    
    while (ctrl_rawpressed(CONTINUE_BUTTON));

    // Wait for it to go on...
    while (!ctrl_rawpressed(CONTINUE_BUTTON));

    // And off again...
    while (ctrl_rawpressed(CONTINUE_BUTTON));
#endif
}

// Updates message display - doesn't erase old!
static void
msg_refresh()
{
    int		i;

    for (i = 0; i < PAGE_SIZE; i++)
    {
	gfx_printtext(0, i, glbMsg[i]);
    }
}

// Clears the graphics text...
static void
msg_cleartext()
{
    int		i;

    for (i = 0; i < PAGE_SIZE; i++)
    {
	gfx_printtext(0, i, glbBlank);
    }
}

// Private methods...
static void
msg_write(const char *str, bool wait=true)
{
    const char		*src;
    char		*dst;
    char		*origdst;
    int			 dstlen, dstmaxlen;
    
    glbViewingOld = false;

    src = str;
    // Find first clear line...
    origdst = dst = glbMsg[glbCurLine];
    // Last line needs extra char for next symbol.
    dstmaxlen = SCREEN_WIDTH - (glbCurLine == (PAGE_SIZE-1));
    dstlen = strlen(dst);
    dst += dstlen;

    while (*src && dstlen < dstmaxlen)
    {
	*dst++ = *src++;
	dstlen++;
    }
    // If we quit because we ran out of source material, great!
    // Null terminate and quit...
    if (!*src)
    {
	*dst++ = '\0';
	msg_refresh();
	return;
    }

    // We have to wrap...  If src is a space, wrapping is trivial.
    // Other wise, we backup src until it is a space...
    while (!isspace(*src) && src > str)
    {
	src--;
	dst--;
	dstlen--;
    }

    // Now, move forward until we ate all the spaces (this seems odd
    // except if *src was a space going into the last loop)
    while (*src && isspace(*src))
    {
	src++;
    }

    // It could be trailing spaces sent us over the bounds.  We will
    // thus eat these and ignore them.
    if (!*src)
    {
	*dst++ = '\0';
	msg_refresh();
	return;
    }

    char		 prefix[SCREEN_WIDTH+1];
    prefix[0] = '\0';
    
    // Special case:  If src == str now, yet our dstlen == 0, we have a
    // single word over SCREEN_WIDTH.  We resolve this by dumping
    // dstmaxlen - 1 chars, a hyphen, and then continuing as if we
    // found a space.
    if (src == str && !dstlen)
    {
	while (*src && dstlen < dstmaxlen-1)
	{
	    *dst++ = *src++;
	    dstlen++;
	}
	// Append a hyphen...
	*dst++ = HYPHEN_SYMBOL;
    }
    // Special case: If src == str, dstlen != 0, we are tacking onto
    // the end of an existing word.  If that word ends with a space,
    // great, we wrap as expected.  However, we often build up
    // a string with a series of writes, such as "foo" followed by ", bar".
    // We want to wrap the last word on dst.  If dst doesn't have a sub
    // word, obviously we avoid this.
    // Of course, this isn't the full story.  It could be that the
    // previous line got to exactly 30 columns.  Any spaces would be 
    // silently eaten so it would look like there are no spaces.
    // To solve this, we look for special non-breaking characters that
    // we feel should be enjoined.  Specifically, a punctuation on the
    // LHS means we should break, and a punctuation on the RHS means
    // we should not.
    else if (src == str)
    {
	char		rhs, lhs;
	rhs = src[0];
	lhs = origdst[dstlen-1];
	// Check if LHS is breaking, in which case we always break
	// so ignore this code path.
	if (!isspace(lhs) && lhs != '.' && lhs != ',' && lhs != '!' && lhs != '?')
	{
	    // Find the start of the word in dst...
	    int		wordstart = dstlen - 1;
	    while (!isspace(origdst[wordstart]) && wordstart > 0)
	    {
		wordstart--;
	    }

	    // If wordstart is now zero, our previuos entry was the
	    // full length.  We might hyphenate here, but I think it
	    // is simpler just to let the break lie where we broke
	    // it before.
	    if (wordstart)
	    {
		// Since wordstart points to a space, we can safely
		// mark wordstart as null & strcpy out wordstart+1
		strcpy(prefix, &origdst[wordstart+1]);
		origdst[wordstart] = '\0';
	    }
	}
    }

    // Wrap to the next line.  We now check if this was the last line,
    // in which case we dump the next page symbol and wait, and then
    // reupdate all.  Otherwise, we just move forward a bit...
    if (glbCurLine == PAGE_SIZE-1)
    {
	// Append a next line char...
	*dst++ = SYMBOL_NEXT;
	*dst++ = '\0';
	msg_refresh();
	if (wait)
	    msg_awaitaccept();
	// Now, reset...
	msg_clear();
    }
    else
    {
	// Null append, go to next line...
	*dst++ = '\0';
	glbCurLine++;
    }
    // Write out src to the next line via recursion.
    msg_write(prefix);
    msg_write(src);
}

void
msg_report(const char *msg, bool wait)
{
    msg_write(msg, wait);
    msg_refresh();
}

void
msg_report(BUF buf, bool wait)
{
    msg_report(buf.buffer(), wait);
}

void
msg_announce(const char *msg)
{
    // Write across the center of the screen:
    int			 y, x;
    const char		*cur, *lastword;

    y = 7;
    x = 0;

    // We always break out of auto-run for these important
    // messages!
    glbAutoRunEnabled = false;

    cur = msg;
    while (*cur)
    {
	gfx_printchar(x, y, *cur);
	cur++;
	x++;
	
	// Check for line wraps.
	if (x >= SCREEN_WIDTH)
	{
	    lastword = cur;
	    if (*lastword && !isspace(*lastword))
	    {
		// We did not end on a new word.
		// We try to rewind to the last space.
		while (x > 0 && lastword > msg)
		{
		    if (isspace(*lastword))
			break;
		    lastword--;
		    x--;
		}
		if (x)
		{
		    // Erase previous line and reset our cur to
		    // last word.
		    // This fails if the single word was more than 30
		    // char, in which case we just wrap (TODO: Hyphen!)
		    cur = lastword;
		    gfx_cleartextline(y, x);
		}
	    }

	    // Eat all spaces from cur as they are oldschool.
	    while (*cur && isspace(*cur))
		cur++;

	    // Move to next line:
	    x = 0;
	    y++;
	}
    }

    // Await confirmation of having read this announcment.
    msg_awaitaccept();
    
    // Clear written to lines.
    while (y >= 7)
    {
	gfx_cleartextline(y);
	y--;
    }

    // Do old school report.  This way it ends up in our buffer.
    // We disable the wait for next as the user has already confirmed
    // reading this message.
    msg_report(msg, false);
}

void
msg_announce(BUF buf)
{
    msg_announce(buf.buffer());
}

void
msg_append(const char *msg)
{
    msg_write(msg);
    msg_refresh();
}

void
msg_alert(const char *msg)
{
    BUF		buf;
    
    if (!msg_isempty())
    {
	msg_awaitaccept();
	msg_clear();
    }
    buf.sprintf("%s%c", msg, SYMBOL_NEXT);
    
    msg_write(buf.buffer());
    msg_refresh();
    msg_awaitaccept();
    msg_clear();
    msg_refresh();
}


bool
msg_askdir(const char *question, int &dx, int &dy, bool allowdiag)
{
    bool		found = false;
    STYLUSLOCK		styluslock;

    hamfake_flushdir();
    if (!allowdiag)
    {
	styluslock.setRegion(REGION_FOURDIR);
	hamfake_buttonreq(5, 3);
    }
    else
    {
	styluslock.setRegion(REGION_TENDIR);	// Should be 8-dir!
	hamfake_buttonreq(5, 2);
    }
    
    msg_write(question);
    msg_refresh();
    
#ifdef HAS_KEYBOARD
    int			keypress = 0;
    bool		lock;
    int			dz = 0;

    while (1)
    {
	keypress = hamfake_getKeyPress(false);
	if (hamfake_forceQuit())
	    break;

	if (keypress && keypress != GFX_KEYLMB)
	{
	    ctrl_getdirfromkey(keypress, dx, dy, allowdiag);
	    found = (dx || dy);
	    break;
	}

	if (!allowdiag)
	    lock = styluslock.getfourdir(dx, dy, found);
	else
	    lock = styluslock.gettendir(dx, dy, dz, found);
	if (lock)
	{
	    found = !found;
	    if (found && !dx && !dy)
		found = false;
	    break;
	}
    }
#else
    while (1)
    {
	if (!gfx_isnewframe())
	    continue;

	if (hamfake_forceQuit())
	    break;

	ctrl_getdir(dx, dy);
	hamfake_clearKeyboardBuffer();
	if (dx || dy)
	{
	    found = true;
	    break;
	}

	if (hamfake_externaldir(dx, dy))
	{
	    found = true;
	    if (!dx && !dy)
		found = false;
	    break;
	}

	if (styluslock.getfourdir(dx, dy, found))
	{
	    found = !found;
	    if (found && !dx && !dy)
		found = false;
	    break;
	}
	// Check for B, which is abort.
	if (ctrl_hit(BUTTON_B))
	    break;
    }
#endif

    msg_clear();
    hamfake_buttonreq(5, 0);
    
    return found;
}

void
msg_clear()
{
    int		i, j;

    if (msg_isempty())
    {
	glbViewingOld = false;
	glbStashView = glbStashHead;
	return;
    }

    if (!glbViewingOld)
    {
	// Stash the message...
	glbStashHead++;
	if (glbStashHead >= NUM_STASHES)
	    glbStashHead = 0;
	for (i = 0; i < PAGE_SIZE; i++)
	{
	    strcpy(glbStashedMsg[glbStashHead][i], glbMsg[i]);

	    // Strip continuation characters from the old message
	    // as now irrelvant.  This also makes the DS look neater.
	    for (j = 0; glbStashedMsg[glbStashHead][i][j]; j++)
	    {
		switch (glbStashedMsg[glbStashHead][i][j])
		{
		    case SYMBOL_NEXT:
			glbStashedMsg[glbStashHead][i][j] = ' ';
			break;
		}
	    }

#ifdef USING_DS
	    if (glbStashedMsg[glbStashHead][i][0] != '\0')
	    {
		// Print out to console.
		iprintf(glbStashedMsg[glbStashHead][i]);
		iprintf("\n");
	    }
#endif
	}
    }
    glbViewingOld = false;
    glbStashView = glbStashHead;

    msg_cleartext();
    glbCurLine = 0;
    for (i = 0; i < PAGE_SIZE; i++)
    {
	glbMsg[i][0] = '\0';
    }
}

bool
msg_isempty()
{
    if (glbCurLine != 0)
	return false;

    if (glbMsg[0][0] != '\0')
	return false;

    return true;
}

// Returns false if did nothing.
bool
msg_restore(bool restorelast)
{
    int		i;
    int		newview;
    bool	startview = false;

    if (!glbViewingOld)
    {
	// If someone wants the next message while viewing the
	// most current, nothing to do...
	if (!restorelast)
	    return false;
	
	if (!msg_isempty())
	{
	    // We first need to stash our current message...
	    msg_clear();
	}
	else
	{
	    // We want to start off viewing the most recent message.
	    startview = true;
	}
    }

    // Determine the new message to view...
    if (!restorelast && (glbStashView == glbStashHead))
    {
	// We are viewing the most recent and asked for next,
	// so we abandon.
	return false;
    }
    if (restorelast)
    {
	if (startview)
	    newview = glbStashView;
	else
	{
	    newview = glbStashView - 1;
	    if (newview < 0)
		newview = NUM_STASHES-1;
	    if (newview == glbStashHead)
	    {
		// The user is viewing the oldest possible message
		// and asked for last - we thus fail.
		return false;
	    }
	}
    }
    else
    {
	newview = glbStashView + 1;
	if (newview >= NUM_STASHES)
	    newview = 0;
    }

    // We have a valid stashedview...
    glbStashView = newview;
    glbViewingOld = true;

    glbCurLine = 0;
    for (i = 0; i < PAGE_SIZE; i++)
    {
	strcpy(glbMsg[i], glbStashedMsg[glbStashView][i]);
	if (glbMsg[i][0] != '\0')
	    glbCurLine = i;
    }
    msg_cleartext();
    msg_refresh();

    return true;
}

void
msg_displayhistory()
{
    // First, clear out menu.
    // Also reset if we are viewing an old message.
    if (!msg_isempty() || glbViewingOld)
	msg_clear();

    // Now read out our entire list.
    int		view, line, numline;

    // Find first message.
    view = glbStashHead;

    numline = 0;
    do
    {
	// Move to newer view
	view++;
	if (view >= NUM_STASHES)
	    view -= NUM_STASHES;

	// Add this message to the pager.
	for (line = 0; line < PAGE_SIZE; line++)
	{
	    // Only add the line if it is non empty.
	    if (glbStashedMsg[view][line][0] == '\0')
		continue;

	    gfx_pager_addsingleline(glbStashedMsg[view][line]);
	    numline++;
	}
    } while (view != glbStashHead);

    gfx_pager_display(numline);
}
