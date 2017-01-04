/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        mobref.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Implementation of MOBREFs.
 *	
 *	MOBREFs are the preferred way to refer to specific MOBs rather than
 *	using MOB *.  When a MOB is deleted, its MOBREF will start
 *	returning null (rather than the MOB *, which will point to deleted
 *	data.)  Furthermore, MOBREFs are safe to save & load.
 */

#include <stdio.h>
#include "mobref.h"
#include "creature.h"
#include "assert.h"
#include "sramstream.h"
#include "msg.h"

struct MOBREF_REF
{
    MOB		*ptr;
    u16		 count;
    
    // Note this isn't masked by free ref!  Thus, we can use it
    // to share data between the two.
    u16		 isfree;
};

struct MOBREF_FREEREF
{
    u16		prev;
    u16		next;
};

union MOBREF_DATA
{
    MOBREF_REF		ref;
    MOBREF_FREEREF	list;
};

// We have 20 arrays of 256 entries - maximum number of refs is 5,000.
#define		MAX_REF_TABLES		20

MOBREF_DATA	*glb_refdata[MAX_REF_TABLES];
u16		 glb_freelist = 0;
u16		 glb_numrefs = 0;

void
mobref_init()
{
    memset(glb_refdata, 0, sizeof(MOBREF_DATA *) * MAX_REF_TABLES);
}

void
mobref_clearallrefs()
{
    int		i;

    for (i = 0; i < MAX_REF_TABLES; i++)
    {
	delete [] glb_refdata[i];
	glb_refdata[i] = 0;
    }
    glb_freelist = 0;
    glb_numrefs = 0;
}

int
mobref_getnumrefs()
{
    return glb_numrefs;
}

int
mobref_getnumtables()
{
    int		i;
    int		numtable = 0;

    for (i = 0; i < MAX_REF_TABLES; i++)
    {
	if (glb_refdata[i])
	    numtable++;
    }
    return numtable;
}

// Very basic reference handlers:

// Finds the data without creating.  Result may be a free ref.
MOBREF_DATA *
mobref_findraw(u16 id)
{
    int		table, off;

    if (!id)
	return 0;

    id--;
    table = id >> 8;
    off = id & 255;

    if (!glb_refdata[table])
	return 0;

    return &glb_refdata[table][off];
}

// Allocates the given table & updates the free list.
void
mobref_alloctable(int table)
{
    if (glb_refdata[table])
	return;
    
    glb_refdata[table] = new MOBREF_DATA [256];

    int		i, tableidx;

    // Address of first entry in table:
    tableidx = table << 8;
    tableidx++;
    for (i = 0; i < 256; i++)
    {
	glb_refdata[table][i].ref.isfree = 1;
	glb_refdata[table][i].list.prev = tableidx + i - 1; 
	glb_refdata[table][i].list.next = tableidx + i + 1; 
    }
    // The very last guy points to current free list, the first guy
    // becomes the free list.
    glb_refdata[table][255].list.next = glb_freelist;
    glb_refdata[table][0].list.prev = 0;

    // The previous of the current free list must point to our very last guy.
    // This should be a no-op because we should be calling this because
    // we ran out of freelist.
    // However, on load we have to create tables as they are touched.
    // If we load two isolated mobs on different tables, we'd create a table
    // while the free list is non-empty.  This will cause our double-linked
    // list to become broken.
    // Unfortunately, the only damage this seems able to cause is to
    // cause us to falsely move the glbfreelist forward, thereby
    // forcing us to allocate more mob tables than we actually need.
    MOBREF_DATA		*head;

    head = mobref_findraw(glb_freelist);
    if (head)
	head->list.prev = tableidx + 255;

    glb_freelist = tableidx;
}

// Creates & allocs the refdata for the given id.
MOBREF_DATA *
mobref_findorcreate(u16 id)
{
    int		 table, off;
    MOBREF_DATA	*data;

    // one-correct the id.
    id--;

    table = id >> 8;
    off = id & 255;
    
    UT_ASSERT(table < MAX_REF_TABLES);
    
    if (!glb_refdata[table])
    {
	mobref_alloctable(table);
    }
    data = &glb_refdata[table][off];

    // Check if already freed...
    if (data->ref.isfree)
    {
	// Yuck, gotta free out this reference.
	// Get adjoining references...
	MOBREF_DATA	*prev, *next;

	prev = mobref_findraw(data->list.prev);
	next = mobref_findraw(data->list.next);

	if (prev)
	{
	    prev->list.next = data->list.next;
	}
	else
	{
	    glb_freelist = data->list.next;
	}
	if (next)
	{
	    next->list.prev = data->list.prev;
	}
	
	// The following turns data into a normal ref.
	data->ref.count = 0;
	data->ref.ptr = 0;
	data->ref.isfree = 0;

	// printf("Create ref given ID %d\n", id);

	glb_numrefs++;
    }

    return data;
}

// Allocs a new refdata, giving you the index.
u16
mobref_allocid()
{
    if (!glb_freelist)
    {
	// Find & allocate a new table, this will update this.
	int		i;
	for (i = 0; i < MAX_REF_TABLES; i++)
	{
	    if (!glb_refdata[i])
	    {
		mobref_alloctable(i);
		break;
	    }
	}
	// This fails implies we are screwed!
	UT_ASSERT(i < MAX_REF_TABLES);
    }

    if (!glb_freelist)
	return 0;

    int			 newid;
    MOBREF_DATA		*newdata, *next;
    
    newid = glb_freelist;
#if 0
    {
	BUF	buf;
	buf.sprintf("new id: %d.  ", newid);
	msg_report(buf);
    }
#endif
    newdata = mobref_findraw(newid);
    next = mobref_findraw(newdata->list.next);
    if (next)
	next->list.prev = 0;
    glb_freelist = newdata->list.next;
#if 0
    {
	BUF		buf;
	buf.sprintf("new free: %d.  ", glb_freelist);
	msg_report(buf);
    }
#endif
    newdata->ref.isfree = 0;
    newdata->ref.ptr = 0;
    newdata->ref.count = 0;

    // printf("Create ref %d\n", newid);
    glb_numrefs++;

    return newid;
}

// Add a reference to id.  increferences must balance decreferences.
// If no one has inced this reference before, it is allocated & the
// pointer is set to 0.  This way load() works seamlessly (eventually
// the guilty party will do a transferMOB)
// It is valid to increment a null id.
void
mobref_increference(u16 id)
{
    if (!id)
	return;

    MOBREF_DATA		*data;

    data = mobref_findorcreate(id);
    data->ref.count++;
}

// Removes a reference, freeing if it hits zero.
// Valid to call with null ids - is a noop then.
void
mobref_decreference(u16 id)
{
    if (!id)
	return;

    MOBREF_DATA		*data;

    data = mobref_findraw(id);

    // If this id is too old, it may point to garbage, sanity check:
    if (!data)
    {
	UT_ASSERT(!"Invlaid dec reference!");
	return;
    }
    
    if (data->ref.isfree)
    {
	// WTF? Already freed!
	UT_ASSERT(!"Ref already freed!");
	return;
    }

    // If the count is 0 or 1, we will delete.
    // Zero is in case some one allocs and decs before incing...
    if (data->ref.count <= 1)
    {
	data->ref.count = 0;
	data->ref.isfree = 1;

	MOBREF_DATA		*head;

	head = mobref_findraw(glb_freelist);

	// Setup the next/prev pointers & put at front of list.
	data->list.next = glb_freelist;
	data->list.prev = 0;

	if (head)
	    head->list.prev = id;

	glb_freelist = id;

	// printf("Id %d done\n", id);

	glb_numrefs--;
    }
    else
    {
	// Straightforward...
	data->ref.count--;
    }
}

// Takes the given id & sets the mob to point to it.
bool
mobref_setptr(u16 id, MOB *mob)
{
    if (!id)
	return true;
    
    MOBREF_DATA		*data;

    data = mobref_findraw(id);
    if (!data)
    {
	UT_ASSERT(!"Stale ref, no data, in setptr!");
	return false;
    }
	
    if (data->ref.isfree)
    {
	UT_ASSERT(!"Stale ref, freed, in setptr!");
	return false;
    }

    data->ref.ptr = mob;
    return true;
}

MOB *
mobref_lookupptr(u16 id)
{
    if (!id)
	return 0;

    MOBREF_DATA		*data;

    data = mobref_findraw(id);
    if (!data || data->ref.isfree)
    {
	UT_ASSERT(!"Stale reference in lut!");
	return 0;
    }

    return data->ref.ptr;
}


//
// MOBREF definitions
//

MOBREF::MOBREF()
{
    // Clear our id.
    // No need to ref count nulls.
    myId = 0;
}

MOBREF::MOBREF(const MOBREF &ref)
{
    // Another person interested in it (ourselves)
    mobref_increference(ref.myId);
    myId = ref.myId;
}

MOBREF::~MOBREF()
{
    // Nulls ignored by decreference.
    mobref_decreference(myId);

    // Zero out our ID in case someone double-deletes a mobref.
    // Of course, that is a bug for other reasons and won't survive
    // a debug build, but at least it minimizes the damage on the GBA.
    myId = 0;
}

MOBREF &
MOBREF::operator=(const MOBREF &ref)
{
    mobref_increference(ref.myId);
    mobref_decreference(myId);
    myId = ref.myId;
    return *this;
}

void
MOBREF::load(SRAMSTREAM &is)
{
    int		val;
    
    mobref_decreference(myId);
    is.uread(val, 16);
    myId = val;
    mobref_increference(myId);
}

void
MOBREF::save(SRAMSTREAM &os) const
{
    os.write(myId, 16);
}

void
MOBREF::createAndSetMOB(MOB *mob)
{
    // Clear our old ID.
    mobref_decreference(myId);

    // If mob is null, we want a null reference.
    if (!mob)
    {
	myId = 0;
	return;
    }

    // Find a new one...
    myId = mobref_allocid();
    mobref_increference(myId);
    if (!mobref_setptr(myId, mob))
    {
	UT_ASSERT(!"Set failed in create");
    }
}

bool
MOBREF::transferMOB(MOB *mob)
{
    // If we are null, this is a problem if mob is not null!
    if (!myId && mob)
    {
	UT_ASSERT(!"Invalid transfer!");
	return false;
    }

    if (!mobref_setptr(myId, mob))
    {
	UT_ASSERT(!"Set ptr failed in transfer");
	return false;
    }

    // If new mob is null, we instantly clear ourselves.
    if (!mob)
    {
	mobref_decreference(myId);
	myId = 0;
    }

    return true;
}

void
MOBREF::setMob(const MOB *mob)
{
    if (!mob)
    {
	mobref_decreference(myId);
	myId = 0;
    }
    else
    {
	*this = mob->getMobRef();
    }
}

MOB *
MOBREF::getMob() const
{
    if (!myId)
	return 0;

    MOB		*mob;

    mob = mobref_lookupptr(myId);
    if (!mob)
    {
	// Clear out our reference, it's stale.
	mobref_decreference(myId);
	myId = 0;
    }
    return mob;
}
