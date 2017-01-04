/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        queue.h ( Letter Hunt Library, C++ )
 *
 * COMMENTS:
 *	Implements a simple queue.
 *	Is threadsafe.  Has the ability to block until an item
 * 	is available.
 */


#ifndef __queue_h__
#define __queue_h__

#include "ptrlist.h"
#include "thread.h"

template <typename PTR>
class QUEUE
{
public:
    QUEUE() { myMaxSize = 0; }
    explicit QUEUE(int maxsize) { myMaxSize = maxsize; }
    ~QUEUE() {}

    // Because we are multithreaded, this is not meant to remain valid
    // outside of a lock.
    bool		isEmpty() const
    {
	return myList.entries() == 0;
    }

    // Non-blocking empty of the queue, no freeing of the results.
    // May not be empty afterward due to multithreading!
    void		flush() 
    {
	PTR		item;
	while (remove(item));
    }

    // Removes item, returns false if failed because queue is empty.
    bool		remove(PTR &item)
    {
	AUTOLOCK	l(myLock);

	if (isEmpty())
	    return false;
	item = myList.removeFirst();
	return true;
    }

    // Blocks till an element is ready.
    PTR			waitAndRemove()
    {
	AUTOLOCK	l(myLock);
	PTR		result;

	while (!remove(result))
	{
	    myCond.wait(myLock);
	}
	return result;
    }

    bool		append(const PTR &item)
    {
	AUTOLOCK	l(myLock);

	// Fail to append if we are full.
	if (myMaxSize && myList.entries() >= myMaxSize)
	    return false;

	myList.append(item);
	myCond.trigger();

	return true;
    }

private:
    // Copying locks scares me.
    QUEUE(const QUEUE<PTR> &ref) {}
    QUEUE<PTR> &operator=(const QUEUE<PTR> &ref) {}

    PTRLIST<PTR>	myList;
    LOCK		myLock;
    CONDITION		myCond;
    int			myMaxSize;
};

#endif
