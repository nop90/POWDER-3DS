/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        intrinsic.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Definition of all bit field that can hold intrinsics...
 */

#ifndef __intrinsic_h__
#define __intrinsic_h__

#include "glbdef.h"

class SRAMSTREAM;

class INTRINSIC
{
public:
    INTRINSIC();
    // This initializes our intrinsic bit array from the given string.
    explicit INTRINSIC(const char *rawstr);

    void	loadFromString(const char *rawstr);
    void	mergeFromString(const char *rawstr);

    static bool	hasIntrinsic(const char *rawstr, 
				   INTRINSIC_NAMES intrinsic);

    bool	hasIntrinsic(INTRINSIC_NAMES intrinsic) const;
    
    // These will all return the OLD value of the intrinsic:
    bool	toggleIntrinsic(INTRINSIC_NAMES intrinsic);
    bool	setIntrinsic(INTRINSIC_NAMES intrinsic, bool newval=true);
    bool	clearIntrinsic(INTRINSIC_NAMES intrinsic);

    void	clearAll();

    void	save(SRAMSTREAM &os);
    void	load(SRAMSTREAM &os);

private:
    u8		myData[(NUM_INTRINSICS + 7) / 8];
};

#endif
