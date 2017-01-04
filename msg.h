/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        msg.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	Platform independent message handler.
 *
 *	This is the message glue library.  It handles the writing
 *	of status messages or prompts to the display.  It deals with
 *	-more-, scrollback, wordwrap, etc.
 *
 *	For displaying a block of scrollable text, see the gfx_
 *	functions in gfx_engine.
 */

#ifndef __msg_h__
#define __msg_h__

#include "buf.h"

void msg_init();

// Waits for any net-new keypress.
void msg_awaitaccept();

// Send a standard message across.  This program will handle line wrapping,
// pausing for user to read long messages or for the previous message
// to clear, etc.
void msg_report(const char *msg, bool wait=true);
void msg_report(BUF buf, bool wait=true);

// Much like msg_report, except the message is first broadcast across
// the center of the screen.  It is then placed into the message buffer
// for scrollback purposes.
void msg_announce(const char *msg);
void msg_announce(BUF buf);

// This does an append-style message.  It will allow this message to break
// between lines (but not "pages"), and be tacked on after other messages.
void msg_append(const char *msg);

// This message requires an accept before it continues.
void msg_alert(const char *msg);

// Asks the user for a questions, return false if cancelled
bool msg_askdir(const char *question, int &dx, int &dy, bool allowdiag=false);

// Clear all messages.
void msg_clear();

// Reports if queue is empty.
bool msg_isempty();

// Restores the last message that was cleared.
// Using this repeatedly will get earlier messages, and restorelast
// of false will get later messages.  If it is outside its valid range,
// false is returned.
bool msg_restore(bool restorelast);

// Shows the entire message history in the pager.
void msg_displayhistory();

#endif
