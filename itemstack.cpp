/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        itemstack.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	Implementation of the itemstack functions
 */

#ifndef __itemstack_c__
#define __itemstack_c__


#include <string.h>
#include "assert.h"
#include "itemstack.h"

template <typename PTR>
PTRSTACK<PTR>::PTRSTACK()
{
    myEntries = 0;
    myExtraSize = 0;
    myExtraList = 0;
}

template <typename PTR>
PTRSTACK<PTR>::~PTRSTACK()
{
    delete [] myExtraList;
}

template <typename PTR>
void
PTRSTACK<PTR>::append(PTR item)
{
    if (myEntries >= PTRSTACK_MINSIZE)
    {
	// Need to add to the extra stack.
	if (myEntries >= PTRSTACK_MINSIZE + myExtraSize)
	{
	    // Need to expand the extra stack.
	    PTR		*newlist;

	    myExtraSize = myExtraSize * 2 + PTRSTACK_MINSIZE;
	    newlist = new PTR[myExtraSize];
	    if (myExtraList)
		memcpy(newlist, myExtraList, 
			sizeof(PTR) * (myEntries - PTRSTACK_MINSIZE));
	    delete [] myExtraList;
	    myExtraList = newlist;
	}
	myExtraList[myEntries - PTRSTACK_MINSIZE] = item;
    }
    else
	myLocal[myEntries] = item;

    myEntries++;
}

template <typename PTR>
void
PTRSTACK<PTR>::set(int idx, PTR item)
{
    UT_ASSERT(idx < entries());
    UT_ASSERT(idx >= 0);
    if (idx < 0)
	return;
    if (idx >= entries())
	return;

    if (idx >= PTRSTACK_MINSIZE)
    {
	idx -= PTRSTACK_MINSIZE;
	myExtraList[idx] = item;
    }
    else
	myLocal[idx] = item;
}

template <typename PTR>
void
PTRSTACK<PTR>::setEntries(int entries)
{
    UT_ASSERT(entries <= myEntries);
    if (entries > myEntries)
	return;

    myEntries = entries;
}

template <typename PTR>
PTR
PTRSTACK<PTR>::operator()(int idx) const
{
    UT_ASSERT(idx >= 0 && idx < myEntries);
    if (idx < 0 || idx >= myEntries)
    {
	// This is so you only have to implement an int constructor.
	return 0;
    }

    if (idx < PTRSTACK_MINSIZE)
	return myLocal[idx];

    idx -= PTRSTACK_MINSIZE;
    return myExtraList[idx];
}


template <typename PTR>
int
PTRSTACK<PTR>::entries() const
{
    return myEntries;
}

template <typename PTR>
void
PTRSTACK<PTR>::clear()
{
    setEntries(0);
}

template <typename PTR>
void
PTRSTACK<PTR>::collapse()
{
    int		i, j;

    for (i = 0, j = 0; i < entries(); i++)
    {
	if (!(*this)(i))
	{
	    // Don't increment j!
	}
	else
	{
	    if (i != j)
		set(j++, (*this)(i));
	    else
		j++;
	}
    }
    // Update number of entries.
    setEntries(j);
}

template <typename PTR>
void
PTRSTACK<PTR>::reverse()
{
    PTR		 tmp;
    int		 i, n, r;

    n = entries();
    for (i = 0; i < n / 2; i++)
    {
	// Reversed entry.
	r = n - 1 - i;
	tmp = (*this)(i);
	set(i, (*this)(r));
	set(r, tmp);
    }
}

#endif
