/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        buf.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 */

#include "assert.h"
#include "buf.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int		glbBufferCount = 0;

int
buf_numbufs()
{
    return glbBufferCount;
}

class BUF_int
{
public:
    BUF_int();
    // Takes ownership of the given text, will delete[] it.
    BUF_int(char *text, int len);
    // Does not take ownership of the text
    BUF_int(const char *text);

    void	 incref();
    void	 decref();
    int		 refcnt() { return myRefCount; }

    // Read methods works for shared objects
    const char	*buffer() const { return myData; }
    int		 datalen() { return myDataLen; }
    int		 strlen() { return ::strlen(myData); }

    // Write methods require you have a uniqued buffer.
    void	 resize(int newlen);
    char	*data() { return myData; }

private:
    ~BUF_int();

private:
    int		 myRefCount;
    char	*myData;
    // Allocated bytes for data.  -1 means we don't own it.
    int		 myDataLen;
};

BUF_int::BUF_int()
{
    myData = (char *) "";
    myDataLen = -1;
    myRefCount = 0;

    glbBufferCount++;
}

BUF_int::BUF_int(char *text, int len)
{
    myData = text;
    myDataLen = len;
    myRefCount = 0;

    glbBufferCount++;
}

BUF_int::BUF_int(const char *text)
{
    myData = (char *) text;
    myDataLen = -1;
    myRefCount = 0;

    glbBufferCount++;
}

BUF_int::~BUF_int()
{
    UT_ASSERT(myRefCount <= 0);
    if (myDataLen >= 0)
	delete [] myData;

    glbBufferCount--;
}

void
BUF_int::incref()
{
    myRefCount++;
}

void
BUF_int::decref()
{
    myRefCount--;
    if (myRefCount <= 0)
	delete this;
}

void
BUF_int::resize(int newlen)
{
    UT_ASSERT(myRefCount <= 1);
    
    char		*text;
    int			 slen;

    // This is an inplace resize so we can never reduce!
    slen = strlen() + 1;
    if (slen > newlen)
	newlen = slen;

    // If this is a downsize, ignore it.
    if (newlen < myDataLen)
	return;
    
    text = new char[newlen];
    ::strcpy(text, myData);

    delete [] myData;
    myData = text;
    myDataLen = newlen;
}

///
/// BUF methods
///

BUF::BUF()
{
    myBuffer = 0;
}

BUF::BUF(int len)
{
    myBuffer = 0;

    allocate(len);
}

BUF::~BUF()
{
    if (myBuffer)
	myBuffer->decref();
}

BUF::BUF(const BUF &buf)
{
    myBuffer = 0;
    *this = buf;
}

BUF &
BUF::operator=(const BUF &buf)
{
    if (buf.myBuffer)
	buf.myBuffer->incref();
    if (myBuffer)
	myBuffer->decref();
    myBuffer = buf.myBuffer;
    return *this;
}

const char *
BUF::buffer() const
{
    if (!myBuffer)
	return 0;

    return myBuffer->buffer();
}

void
BUF::steal(char *text)
{
    if (myBuffer)
	myBuffer->decref();

    if (!text)
	myBuffer = 0;
    else
    {
	myBuffer = new BUF_int(text, ::strlen(text)+1);
	myBuffer->incref();
    }
}

void
BUF::reference(const char *text)
{
    if (myBuffer)
	myBuffer->decref();

    if (!text)
	myBuffer = 0;
    else
    {
	myBuffer = new BUF_int(text);
	myBuffer->incref();
    }
}

void
BUF::strcpy(const char *src)
{
    if (myBuffer)
	myBuffer->decref();

    myBuffer = 0;

    if (src)
    {
	char	*text = new char [::strlen(src)+1];
	::strcpy(text, src);
	steal(text);
    }
}

int
BUF::strlen() const
{
    if (!myBuffer)
	return 0;

    return myBuffer->strlen();
}

int
BUF::strcmp(const char *cmp) const
{
    if (!myBuffer)
    {
	if (!cmp)
	    return 0;
	return -1;
    }

    return ::strcmp(buffer(), cmp);
}

void
BUF::strcat(const char *src)
{
    // Trivial strcat.
    if (!src)
	return;

    if (!myBuffer)
    {
	// Equivalent to strcpy
	strcpy(src);
	return;
    }

    // Find our total length
    int		srclen, mylen;

    uniquify();

    mylen = strlen();
    srclen = ::strlen(src);
    mylen += srclen + 1;

    myBuffer->resize(mylen);
    // Now safe...
    ::strcat(myBuffer->data(), src);
}

void
BUF::append(char c)
{
    char	s[2];

    s[0] = c;
    s[1] = '\0';
    strcat(s);
}

void
BUF::clear()
{
    uniquify();
    evildata()[0] = '\0';
}

bool
BUF::isstring() const
{
    if (!myBuffer)
	return false;

    if (buffer()[0])
	return true;

    return false;
}

char
BUF::lastchar(int nthlast) const
{
    if (!myBuffer)
	return 0;

    int		len;
    len = strlen();
    len -= nthlast + 1;
    if (len < 0)
	return 0;
    return buffer()[len];
}

void
BUF::allocate(int len)
{
    char *text = new char[len];

    // We always want to be null terminated!
    *text = 0;

    if (myBuffer)
	myBuffer->decref();

    myBuffer = new BUF_int(text, len);
    myBuffer->incref();
}

int
OURvsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    int		result;
#ifdef WIN32
    result = _vsnprintf(str, size, format, ap);
#else
    va_list	ap_copy;
    // Apparently va_list can't be reused on modern compilers.  Same
    // compilers require support for va_copy which older compilers
    // lack.  *sigh*
    va_copy(ap_copy, ap);
    result = vsnprintf(str, size, format, ap_copy);
    va_end(ap_copy);
#endif
    return result;
}

int
BUF::vsprintf(const char *fmt, va_list ap)
{
    int	result;
    int	cursize;

    // We wipe ourself out, so is safe to allocate.
    cursize = 100;
    while (1)
    {
	// Reallocate with the new (larger?) size.
	allocate(cursize);
	
	// Marker so we can tell if we overflowed vs having a bad
	// format tag
	myBuffer->data()[cursize-1] = '\0';
	result = OURvsnprintf(myBuffer->data(), cursize, fmt, ap);
	if (result < 0)
	{
	    // Check if the null is still there, signifying something
	    // went wrong with formatting.
	    if (myBuffer->data()[cursize-1] == '\0')
	    {
		// Treat this as the final buffer.
		result = strlen();
		break;
	    }
	}

	// We really must have a final null, thus this paranoia...
	if (result < 0 || result > cursize-1)
	{
	    cursize *= 2;
	}
	else
	{
	    // Success!
	    break;
	}
    }
    return result;
}

int
BUF::sprintf(const char *fmt, ...)
{
    va_list	marker;
    int		result;

    va_start(marker, fmt);
    result = vsprintf(fmt, marker);
    va_end(marker);

    return result;
}

int
BUF::appendSprintf(const char *fmt, ...)
{
    BUF		newtext;
    va_list	marker;
    int		result;

    va_start(marker, fmt);
    result = newtext.vsprintf(fmt, marker);
    va_end(marker);

    strcat(newtext);

    return result;
}

char *
BUF::strdup() const
{
    if (!myBuffer)
	return ::strdup("");

    return ::strdup(buffer());
}

char *
BUF::evildata()
{
    UT_ASSERT(myBuffer);
    UT_ASSERT(myBuffer->refcnt() <= 1);
    return (char *) buffer();
}

void
BUF::uniquify()
{
    if (!myBuffer)
    {
	// We want a writeable buffer
	allocate(50);
    }
    else
    {
	// If ref count is 1 or 0, we are only reference, so good.
	// UNLESS the current dude is a read only buffer.
	if (myBuffer->refcnt() <= 1 && myBuffer->datalen() >= 0)
	{
	    // Already writeable.
	    return;
	}
	// Copy the current buffer.
	int		len;
	char		*text;
	if (myBuffer->datalen() >= 0)
	    len = myBuffer->datalen();
	else
	    len = myBuffer->strlen() + 1;
	text = new char[len];
	::strcpy(text, myBuffer->buffer());

	// Throw away this buffer and make a new one.
	myBuffer->decref();
	myBuffer = new BUF_int(text, len);
	myBuffer->incref();
    }
}

void
BUF::makeFileSafe()
{
    uniquify();

    // Strip out evil characters
    unsigned char	*c;
    for (c = (unsigned char *)evildata(); *c; c++)
    {
	if (!isalnum(*c) && *c != '.')
	    *c = '_';
    }
}
