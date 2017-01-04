/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        assert.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Standard UT_ASSERT definitions.  These should be used rather than
 *	system dependent ASSERT or assert.
 *	(The builtin HAM assert is rather annoying as it corrupts vram)
 *
 * 	When an assert is triggered, it freezes game play with the 
 *	relevant message until both R & L are pressed & released.
 */

#ifndef __assert__
#define __assert__

//#define USEASSERT

#ifdef STRESS_TEST
#define USEASSERT
#endif

#ifdef USING_SDL
#ifdef _DEBUG
#define USEASSERT
#endif
#endif

#ifdef USEASSERT

void triggerassert(int cond, const char *msg, int line, const char *file);

#define UT_ASSERT(cond) \
	triggerassert(cond, #cond, __LINE__, __FILE__)
#define UT_ASSERT2(cond, msg) \
	triggerassert(cond, msg, __LINE__, __FILE__)

#else

#define UT_ASSERT(cond)     
#define UT_ASSERT2(cond, msg)     

#endif

#endif
