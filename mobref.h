/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        mobref.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	Definition for mob references
 *
 * 	These light weight classes use reference counting internally
 * 	to allow you to track MOB * without having to worry about
 * 	the MOB being deleted (you will then just get 0 back)
 * 	It also allows swapping MOB * behind the users back, 
 * 	potentially compressing MOBs, etc.  Also, you can always save/reload
 * 	the raw MOBREFs rather than MOB * which don't save & load very well.
 */

#ifndef __mobref_h__
#define __mobref_h__

// needed for u16.
#include "mygba.h"

class SRAMSTREAM;
class MOB;

// Required one time initialization.
void mobref_init();

// This wipes all the tables.  Do not call if you still have MOBREF
// pointers lying around!
// It's provided to make it easier to shoot yourself in the foot.
void mobref_clearallrefs();

// Debugging information.
int mobref_getnumrefs();
int mobref_getnumtables();

class MOBREF
{
public:
    MOBREF();
    ~MOBREF();

    // Copy constructor & assignment operators.
    MOBREF(const MOBREF &ref);
    MOBREF &operator=(const MOBREF &ref);

    void	 load(SRAMSTREAM &is);
    void	 save(SRAMSTREAM &os) const;

    // Clears this reference & then creates a new ref for MOB.
    // Should only be done by MOB itself!
    void	 createAndSetMOB(MOB *mob);

    // Changes the underlying mob * to the new one.
    // returns false if something really bad happened.
    bool	 transferMOB(MOB *mob);

    // Changes our reference to be the given mob's reference.  This
    // handles the pain of dealing with null MOB*.
    void	 setMob(const MOB *mob);

    MOB		*getMob() const;

    // Returns if the reference is null, NOT if what it points to
    // is null!
    bool	 isNull() const { return myId == 0; }
		    
    // Returns the base id.  Should only be used to hash mobs!
    // Note that these *are* recycled!  And change on reload!
    int		 getId() const { return myId; }
    
private:
    mutable u16		myId;
};

#endif
