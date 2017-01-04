/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        mygba.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *		Global types and includes, I guess a reimp of
 *		stdafx.h.  Most of the POWDER files require
 *		this to exist.
 */

#ifndef __mygba__
#define __mygba__

#define USING_SDL

#ifdef LINUX
#define stricmp(a, b) strcasecmp(a, b)
#endif

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#endif

