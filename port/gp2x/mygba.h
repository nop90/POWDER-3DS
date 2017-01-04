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
 *	This is a fake version of the HAM libraries mygba.h.
 *	It redefines the functions we need using hamfake.cpp.
 */

#ifndef __mygba__
#define __mygba__

#define USING_SDL
#define HAS_DISKIO
//#define HAS_KEYBOARD
//#define HAS_STYLUS
#define HAS_PRINTF

#ifdef LINUX
#define stricmp(a, b) strcasecmp(a, b)
#endif

// #define STRESS_TEST

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;

#define INT_TYPE_VBL		0
#define INT_TYPE_HBL		1

#define WIN_BG0 0x01
#define WIN_BG1 0x02
#define WIN_BG2 0x04
#define WIN_BG3 0x08
#define WIN_OBJ 0x10

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "hamfake.h"

#endif

