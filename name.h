/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        name.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	This class handles the global namespace which allows
 *	users to name specific creatures or items.  We don't
 *	want to waste 32 bits on a pointer, when there can
 *	only be a few names.  This also simplifies the marshalling,
 *	etc, of names.
 */

#ifndef __name__
#define __name__

#include "buf.h"

// Required one time initialization.
void name_init(int numnames = 0);

// Required load/save functions.  These save the
// actual data of the names.
void name_load(SRAMSTREAM &is);
void name_save(SRAMSTREAM &os);

class NAME
{
public:
    NAME();
    ~NAME();

    // Copy constructor & assignment operators.
    NAME(const NAME &name);
    NAME &operator=(const NAME &name);
    
    const char	*getName() const;

    // Because you can't change an existing name, setName will
    // actually allocate a new myIdx.
    void	 setName(const char *name);
    void	 setName(BUF buf)
		 { setName(buf.buffer()); }

    void	 save(SRAMSTREAM &os) const;
    void	 load(SRAMSTREAM &is);
    
protected:
    u16		 myIdx;
};

#endif

