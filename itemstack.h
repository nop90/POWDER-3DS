/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        itemstack.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	This provides a method to get a stack of items,
 * 	for example, all the items at a given square.
 * 	It avoids dynamic allocation up to the minsize,
 * 	but will happily transparently go to dynamic allocation
 * 	where necessary.
 */

#ifndef __itemstack_h__
#define __itemstack_h__

class ITEM;
class MOB;

#define PTRSTACK_MINSIZE	20

template <typename PTR>
class PTRSTACK
{
public:
    PTRSTACK();
    ~PTRSTACK();

    void		 append(PTR item);

    // Reverses the order of the stack.
    void		 reverse();

    // Empties the stack
    void		 clear();

    // Collapses all entries that evaluate ! as true (ie, zero)
    void		 collapse();

    PTR			 operator()(int idx) const;
    int			 entries() const;

    void		 set(int idx, PTR item);

    void		 setEntries(int entries);

private:
    PTR			 *myExtraList;
    int			  myEntries;
    int			  myExtraSize;
    PTR 		  myLocal[PTRSTACK_MINSIZE];
};

typedef PTRSTACK<ITEM *> ITEMSTACK;
typedef PTRSTACK<MOB *> MOBSTACK;

// For crappy platforms:
#include "itemstack.cpp"

#endif
