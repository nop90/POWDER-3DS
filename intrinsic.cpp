/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        intrinsic.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Bit field functions for handling intrinsics...
 */

#include "intrinsic.h"
#include "assert.h"
#include "sramstream.h"

INTRINSIC::INTRINSIC()
{
    memset(myData, 0, (NUM_INTRINSICS + 7) / 8);
}

INTRINSIC::INTRINSIC(const char *rawstr)
{
    loadFromString(rawstr);
}

void
INTRINSIC::loadFromString(const char *rawstr)
{
    clearAll();
    mergeFromString(rawstr);
}

void
INTRINSIC::mergeFromString(const char *rawstr)
{
    if (!rawstr)
	return;

    while (*rawstr)
    {
	setIntrinsic( (INTRINSIC_NAMES) ((u8)*rawstr) );
	rawstr++;
    }
}

void
INTRINSIC::clearAll()
{
    memset(myData, 0, (NUM_INTRINSICS + 7) / 8);
}

void
INTRINSIC::save(SRAMSTREAM &os)
{
    int		i;
    
    for (i = 0; i < ((NUM_INTRINSICS + 7) / 8); i++)
    {
	os.write(myData[i], 8);
    }
}

void
INTRINSIC::load(SRAMSTREAM &is)
{
    int		i, val;
    
    for (i = 0; i < ((NUM_INTRINSICS + 7) / 8); i++)
    {
	is.uread(val, 8);
	myData[i] = val;
    }
}

bool
INTRINSIC::hasIntrinsic(const char *rawstr, INTRINSIC_NAMES intrinsic)
{
    if (!rawstr)
	return false;

    while (*rawstr)
    {
	if ( ((u8)*rawstr) == intrinsic )
	    return true;
	rawstr++;
    }
    return false;
}

bool
INTRINSIC::hasIntrinsic(INTRINSIC_NAMES intrinsic) const
{
    int		byte, bit, mask;

    if (intrinsic == INTRINSIC_NONE)
	return false;

    UT_ASSERT(intrinsic < NUM_INTRINSICS);

    bit = intrinsic - 1;
    byte = bit >> 3;
    bit &= 7;
    mask = 1 << bit;
    
    return (myData[byte] & mask) ? true : false;
}

bool
INTRINSIC::toggleIntrinsic(INTRINSIC_NAMES intrinsic)
{
    int		byte, bit, mask;

    if (intrinsic == INTRINSIC_NONE)
	return false;

    UT_ASSERT(intrinsic < NUM_INTRINSICS);

    bit = intrinsic - 1;
    byte = bit >> 3;
    bit &= 7;
    mask = 1 << bit;

    if (myData[byte] & mask)
    {
	myData[byte] &= ~mask;
	return true;
    }
    
    myData[byte] |= mask;
    return true;
}

bool
INTRINSIC::setIntrinsic(INTRINSIC_NAMES intrinsic, bool newval)
{
    if (!newval)
	return clearIntrinsic(intrinsic);
    
    if (intrinsic == INTRINSIC_NONE)
	return false;

    int		byte, bit, mask;
    bool	ret = false;

    UT_ASSERT(intrinsic < NUM_INTRINSICS);

    bit = intrinsic - 1;
    byte = bit >> 3;
    bit &= 7;
    mask = 1 << bit;

    if (myData[byte] & mask)
	ret = true;
    else
	myData[byte] |= mask;

    return ret;
}

bool
INTRINSIC::clearIntrinsic(INTRINSIC_NAMES intrinsic)
{
    int		byte, bit, mask;
    bool	ret = false;

    if (intrinsic == INTRINSIC_NONE)
	return false;

    UT_ASSERT(intrinsic < NUM_INTRINSICS);

    bit = intrinsic - 1;
    byte = bit >> 3;
    bit &= 7;
    mask = 1 << bit;

    if (myData[byte] & mask)
    {
	ret = true;
	myData[byte] &= ~mask;
    }
    
    return ret;
}
