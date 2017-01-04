/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        name.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This handles tracking the name library.
 */

#include "mygba.h"
#include <stdio.h>
#include "sramstream.h"
#include "assert.h"
#include "name.h"
#include "msg.h"

#define INVALID_NAME 65535

int    glb_numnames = 0;
char **glb_names = 0;
u8    *glb_namerefs = 0;

void
name_init(int numnames)
{
    int			i;

    for (i = 0; i < glb_numnames; i++)
    {
	if (glb_names[i])
	    free(glb_names[i]);
    }
    delete [] glb_names;
    delete [] glb_namerefs;
    
    if (numnames < 64)
	numnames = 64;
    glb_numnames = numnames;
    glb_names = new char *[glb_numnames];
    glb_namerefs = new u8[glb_numnames];
    memset(glb_names, 0, sizeof(char *) * glb_numnames);
    memset(glb_namerefs, 0, sizeof(u8) * glb_numnames);
}

int
name_alloc(const char *name)
{
    int		i;

    for (i = 0; i < glb_numnames; i++)
    {
	if (!glb_namerefs[i])
	{
	    // Found a free name!
	    break;
	}
    }
    if (i == glb_numnames)
    {
	// Need to realloc.
	int		  newsize;
	char		**newnames;
	u8		 *newref;

	newsize = glb_numnames * 2;
	newnames = new char *[newsize];
	newref = new u8[newsize];
	memset(newnames, 0, sizeof(char *) * newsize);
	memset(newref, 0, sizeof(u8) * newsize);

	// Copy old ones.
	memcpy(newnames, glb_names, sizeof(char *) * glb_numnames);
	memcpy(newref, glb_namerefs, sizeof(u8) * glb_numnames);

	// Delete & change over.
	// Note that i is now a valid index.
	glb_numnames = newsize;
	delete [] glb_names;
	glb_names = newnames;
	delete [] glb_namerefs;
	glb_namerefs = newref;
    }

    UT_ASSERT(glb_namerefs[i] == 0);

    glb_namerefs[i] = 1;
    glb_names[i] = strdup(name);
    return i;
}

void
name_addref(int idx)
{
    if (idx < 0)
	return;
    if (idx == INVALID_NAME)
	return;
    
    if (idx >= glb_numnames)
    {
	UT_ASSERT(0);
	return;
    }
    glb_namerefs[idx]++;
}

void
name_decref(int idx)
{
    if (idx < 0)
	return;
    if (idx == INVALID_NAME)
	return;
    
    if (idx >= glb_numnames)
    {
	UT_ASSERT(0);
	return;
    }
    if (glb_namerefs[idx] == 0)
    {
	UT_ASSERT(0);
	return;
    }
    glb_namerefs[idx]--;
    if (!glb_namerefs[idx])
    {
	free(glb_names[idx]);
	glb_names[idx] = 0;
    }
}

void
name_load(SRAMSTREAM &is)
{
    int		i;
    int		numnames;
    char	buf[40];

    is.uread(numnames, 16);

    name_init(numnames);
    
    for (i = 0; i < numnames; i++)
    {
	is.readString(buf, 30);
	if (buf[0])
	{
	    // This is a real string...
	    glb_names[i] = strdup(buf);
	}
    }
}

void
name_save(SRAMSTREAM &os)
{
    int		numnames, i;

    // Count backward to find number of real names.
    for (numnames = glb_numnames-1; numnames >= 0 && !glb_names[numnames];
	    numnames--);

    // Include the last guy found.
    numnames++;

    os.write(numnames, 16);

    for (i = 0; i < numnames; i++)
    {
	os.writeString(glb_names[i], 30);
    }
}

//
// Now for the actual NAME definitions
//

NAME::NAME()
{
    myIdx = INVALID_NAME;
}

NAME::~NAME()
{
    name_decref(myIdx);
}

NAME::NAME(const NAME &name)
{
    myIdx = name.myIdx;
    name_addref(myIdx);
}

NAME &
NAME::operator=(const NAME &name)
{
    name_addref(name.myIdx);
    name_decref(myIdx);
    myIdx = name.myIdx;

    return *this;
}

const char *
NAME::getName() const
{
    if (myIdx == INVALID_NAME)
	return 0;

    if (myIdx >= glb_numnames)
    {
	UT_ASSERT(0);
	return 0;
    }

    return glb_names[myIdx];
}

void
NAME::setName(const char *name)
{
    name_decref(myIdx);
    if (name && *name)
	myIdx = name_alloc(name);
    else
	myIdx = INVALID_NAME;
}

void
NAME::save(SRAMSTREAM &os) const
{
    os.write(myIdx, 16);
}

void
NAME::load(SRAMSTREAM &is)
{
    int		val;

    name_decref(myIdx);

    is.uread(val, 16);

    myIdx = val;
    name_addref(myIdx);
}
