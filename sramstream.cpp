/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        sramstream.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	The SRAMSTREAM class provides the routines to push bits
 *	into the saved memory space.  It also does implicit zero
 *	compression, so the more zeros present, the less space
 *	required.
 */

#include "sramstream.h"
#include <stdio.h>

#include "sramstream.h"
#include "msg.h"
#include "assert.h"

#include "mygba.h"

// Crappy flashers require one to specify your save type.
// Since we aren't using the official HDK, we have to roll our own
// signifier:
const char *glbSaveType = "SRAM_V113";

static void
delay()
{
    static volatile int delay = 0;

    delay *= delay + 1;
}

SRAMSTREAM::SRAMSTREAM()
{
    mySRAMPtr = 0;
    myPos = 0;
    myBit = 0;
    myNumBlocks = 0;
}

SRAMSTREAM::~SRAMSTREAM()
{
}

void
SRAMSTREAM::rewind()
{
    mySRAMPtr = 0;
    myPos = 0;
    myBit = 0;
    myNumBlocks = 0;
}

bool
SRAMSTREAM::writeRaw(const char *data, int length)
{
    char		*startsram = hamfake_writeLockSRAM(), *sram;
    int			 i;

    if (mySRAMPtr + length > 63000)
    {
	// Fail to write!
	return false;
    }
    
    sram = startsram;
    sram += mySRAMPtr;
    for (i = 0; i < length; i++)
    {
	*sram++ = *data++;
	delay();
    }
    mySRAMPtr += length;

    hamfake_writeUnlockSRAM(startsram);

    return true;
}

bool
SRAMSTREAM::readRaw(char *data, int len)
{
    char		*sram, *startsram = hamfake_readLockSRAM();
    int			 i;

    sram = startsram;
    sram += mySRAMPtr;
    
    for (i = 0; i < len; i++)
    {
	*data++ = *sram++;
	delay();
    }

    mySRAMPtr += len;
    
    hamfake_readUnlockSRAM(startsram);
    return true;
}

bool
SRAMSTREAM::write(int val, int bits)
{
    
    // Only support full int writes for now.
    while (bits > 0)
    {
	myBuffer[myPos++] = val & 0xff;
	val >>= 8;
	bits -= 8;
	
	if (myPos >= 1024)
	{
	    writeToSRAM(myBuffer, 1024);
	    myPos = 0;
	}
    }
    // Does not yet handle fractional.
    UT_ASSERT(!bits);
    return true;
}

bool
SRAMSTREAM::uread(int &val, int bits)
{
    int		offset = 0;
    int		newval;
    
    val = 0;
    while (bits > 0)
    {
	if (!myPos && !myBit)
	{
	    // myBuffer not yet read...
	    readFromSRAM((char *)myBuffer, 256 * sizeof(int));
	}
	newval = myBuffer[myPos++];
	newval &= 0xff;
	val |= newval << offset;
	offset += 8;
	bits -= 8;
	
	if (myPos >= 1024)
	    myPos = 0;
    }
    UT_ASSERT(!bits);		// Should be even bytes.
    return true;
}

bool
SRAMSTREAM::read(int &val, int bits)
{
    uread(val, bits);
    if (bits < 32)
    {
	if (val & (1 << (bits-1)))
	{
	    // Sign extend...
	    val |= ~(int)((1 << bits) - 1);
	}
    }

    return true;
}

bool
SRAMSTREAM::readString(char *dst, int maxlen)
{
    int		i;
    int		val;

    for (i = 0; i < maxlen-1; i++)
    {
	if (!uread(val, 8))
	{
	    dst[i] = 0;
	    return false;
	}
	dst[i] = val;
	if (!dst[i])
	    return true;		// success.
    }
    // Clipped, likely corrupt now.
    dst[i] = 0;
    return false;
}

bool
SRAMSTREAM::writeString(const char *src, int maxlen)
{
    int		i;
    int		val;

    if (!src)
	src = "";

    for (i = 0; i < maxlen-1; i++)
    {
	val = src[i];
	if (!write(val, 8))
	{
	    return false;
	}
	if (!src[i])
	    return true;		// success.
    }
    // Clip the string.
    val = 0;
    if (!write(val, 8))
	return false;
    return true;
}

bool
SRAMSTREAM::flush()
{
    BUF		buf;

    if (myPos || myBit)
    {
	int		i;
	// We want to zero out the end parts to reduce the saved size...
	for (i = myPos+1; i < 256; i++)
	{
	    myBuffer[i] = 0;
	}
	writeToSRAM((char *)myBuffer, 256 * sizeof(int));
    }
    buf.sprintf("Completed save of %d bytes, %d blocks.",
	    mySRAMPtr,
	    myNumBlocks);
    msg_report(buf);
    return true;
}

void
SRAMSTREAM::writeToSRAM(char *data, int len)
{
    char	*sram = hamfake_writeLockSRAM();

    if (mySRAMPtr > 63000)
    {
	// Fail to write!
	return;
    }

    sram += mySRAMPtr;
    UT_ASSERT(len == 1024);
    mySRAMPtr += compress(sram, data, len);
    myNumBlocks++;

    hamfake_writeUnlockSRAM(sram);
}

void 
SRAMSTREAM::readFromSRAM(char *data, int len)
{
    char	*sram = hamfake_readLockSRAM();

    sram += mySRAMPtr;
    UT_ASSERT(len == 1024);
    mySRAMPtr += decompress(data, sram, len);
    myNumBlocks++;

    hamfake_readUnlockSRAM(sram);
}

// This will compress 1k block into dst.
int
SRAMSTREAM::compress(char *dst, char *src, int blocklen)
{
    // First, we determine the best method...
    int			zero = 0;		// Number zeros..
    int			rlesave = 0;		// Amount saved by RLE
    int			zeroprebyte = 0;	// Zeros with byte preproc
    int			zeropreshort = 0;	// Zeros with short preproc
    int			zeropreint = 0;		// Zeros with int preproc
    int			currunlen, minzero;
    int			isconstant = 1;
    int			zerotablesize;
    int			i;
    PREPROCESS_TYPE	preprocess;
    bool		istopblock = false;

#if 1
    if (blocklen == 1024)
	istopblock = true;
#endif

    // Number of bytes needed for a zerotable of this blocklen.
    zerotablesize = (blocklen + 7) >> 3;

    currunlen = 0;
    for (i = 0; i < blocklen; i++)
    {
	if (src[i] == 0)
	    zero++;
	if (i && src[i] == src[i-1])
	{
	    currunlen++;
	}
	else
	    currunlen = 1;		// Difference, run of 1.
	if (currunlen > 3)
	    rlesave++;			// After three codes we save...
	if ((!i && !src[i]) || (src[i] == src[i-1]))
	    zeroprebyte++;
	if ((i < 2 && !src[i]) || (src[i] == src[i-2]))
	    zeropreshort++;
	if ((i < 4 && !src[i]) || (src[i] == src[i-4]))
	    zeropreint++;
	else
	    isconstant = 0;
    }
    if (0)
    {
	BUF		buf;
	buf.sprintf("Z:%d,B:%d,S:%d,I:%d",
		zero, zeroprebyte, zeropreshort, zeropreint);
	msg_report(buf);
    }

    // Find best preprocess...
    if (zero >= zeroprebyte)
    {
	minzero = zero;
	preprocess = PREPROCESS_NONE;
    }
    else
    {
	minzero = zeroprebyte;
	preprocess = PREPROCESS_BYTE;
    }
    if (minzero < zeropreshort)
    {
	preprocess = PREPROCESS_SHORT;
	minzero = zeropreshort;
    }
    if (minzero < zeropreint)
    {
	preprocess = PREPROCESS_INT;
	minzero = zeropreint;
    }

    // Determine best method...
    if (isconstant)
    {
	//msg_report("Constant...");
	if (istopblock)
	    msg_report("_");
	return compressConstant(dst, src);
    }

    // May have to punt for uncompressed...
    if ((blocklen - minzero + zerotablesize) > blocklen) // &&
	// (blocklen + 8 - rlesave) > blocklen)
    {
	// msg_report("None...");
	if (istopblock)
	    msg_report("x");
	return compressNone(dst, src, blocklen);
    }
    
    //if ((blocklen - minzero + zerotablesize) < (blocklen - rlesave))
    if (1)
    {
	//msg_report("ZeroTable...");
	if (istopblock)
	    msg_report(".");
	return compressZeroTable(dst, src, preprocess, blocklen);
    }
    else
    {
	if (istopblock)
	    msg_report("r");
	return compressRLE(dst, src, blocklen);
    }
}

// This will decompress into a 1k block.
int
SRAMSTREAM::decompress(char *dst, char *src, int blocklen)
{
    PREPROCESS_TYPE		preprocess;
    COMPRESS_TYPE		compress;
    char			c;
    bool			istopblock = false;

#if 1
    if (blocklen == 1024)
	istopblock = true;
#endif

    // Get type code...
    c = *src++;
    delay();
    
    compress = (COMPRESS_TYPE) (c & 0xf);
    preprocess = (PREPROCESS_TYPE) ((c >> 4) & 0xf);

    switch (compress)
    {
	case COMPRESS_NONE:
	    //msg_report("None...");
	    if (istopblock)
		msg_report("x");
	    return decompressNone(dst, src, blocklen) + 1;
	case COMPRESS_CONSTANT:
	    //msg_report("Constant...");
	    if (istopblock)
		msg_report("_");
	    return decompressConstant(dst, src, blocklen) + 1;
	case COMPRESS_ZEROTABLE:
	    //msg_report("ZeroTable...");
	    if (istopblock)
		msg_report(".");
	    return decompressZeroTable(dst, src, preprocess, blocklen) + 1;
	case COMPRESS_RLE:
	    //msg_report("RLE...");
	    if (istopblock)
		msg_report("r");
	    return decompressRLE(dst, src, blocklen);
	default:
	    UT_ASSERT(!"Unknown compression type!");
	    return 0;
    }
}

int
SRAMSTREAM::compressConstant(char *dst, char *src)
{
    // Write out the constant code, than the four bytes needed...
    *dst++ = COMPRESS_CONSTANT;
    delay();
    *dst++ = *src++;
    delay();
    *dst++ = *src++;
    delay();
    *dst++ = *src++;
    delay();
    *dst++ = *src++;
    delay();

    return 5;		// Fixed 5 bytes..
}

int
SRAMSTREAM::decompressConstant(char *dst, char *src, int len)
{
    int		i;

    UT_ASSERT(! (len & 3));
    for (i = 0; i < (len >> 2); i++)
    {
	*dst++ = src[0];
	delay();
	*dst++ = src[1];
	delay();
	*dst++ = src[2];
	delay();
	*dst++ = src[3];
	delay();
    }
    return 4;
}

int
SRAMSTREAM::compressNone(char *dst, char *src, int len, bool header)
{
    int		i;
    
    if (header)
    {
        *dst++ = COMPRESS_NONE;
	delay();
    }

    for (i = 0; i < len; i++)
    {
	*dst++ = *src++;
	delay();
    }
    return len + (header ? 1 : 0);	// Fixed size..
}

int
SRAMSTREAM::decompressNone(char *dst, char *src, int len)
{
    int		i;

    for (i = 0; i < len; i++)
    {
	*dst++ = *src++;
	delay();
    }
    return len;
}

int
SRAMSTREAM::compressZeroTable(char *dst, char *src, PREPROCESS_TYPE preprocess, int blocklen)
{
    char		zerotable[128];
    int			zerotablelen;
    int			i, j, pos, prelen;
    char		*dststart = dst;

    zerotablelen = (blocklen + 7) >> 3;

    UT_ASSERT(! (blocklen & 7));
    
    *dst++ = COMPRESS_ZEROTABLE | (preprocess << 4);
    delay();

    switch (preprocess)
    {
	case PREPROCESS_NONE:
	    //msg_report("prenone");
	    prelen = 0;
	    break;
	case PREPROCESS_BYTE:
	    //msg_report("prebyte");
	    prelen = 1;
	    break;
	case PREPROCESS_SHORT:
	    //msg_report("preshort");
	    prelen = 2;
	    break;
	case PREPROCESS_INT:
	    //msg_report("preint");
	    prelen = 4;
	    break;
	default:
	    UT_ASSERT(0);
	    prelen = 1024;
	    break;
    }

    // First, we build the zero table...
    pos = 0;
    for (i = 0; i < zerotablelen; i++)
    {
	zerotable[i] = 0;
	for (j = 0; j < 8; j++)
	{
	    // If before prelen, act as if pre was zero...
	    if (pos < prelen)
	    {
		if (src[pos])
		    zerotable[i] |= 1 << j;
	    }
	    else
	    {
		switch (preprocess)
		{
		    case PREPROCESS_NONE:
			if (src[pos])
			    zerotable[i] |= 1 << j;
			break;
		    case PREPROCESS_BYTE:
		    case PREPROCESS_SHORT:
		    case PREPROCESS_INT:
			if (src[pos] != src[pos - prelen])
			    zerotable[i] |= 1 << j;
			break;
		}
	    }
	    pos++;
	}
    }

    // Now save it using simple RLE..
    //dst += compressRLE(dst, zerotable, 128, false);

    // If zerotablelen is short enough, go to no compression.
    if (zerotablelen < 20)
        dst += compressNone(dst, zerotable, zerotablelen, false);
    else
	dst += compress(dst, zerotable, zerotablelen);

    // And now save the non-zero entries...
    pos = 0;
    for (i = 0; i < zerotablelen; i++)
    {
	for (j = 0; j < 8; j++)
	{
	    if (pos < prelen || (preprocess == PREPROCESS_NONE))
	    {
		if (zerotable[i] & (1 << j))
		{
		    *dst++ = src[pos];
		    delay();
		}
	    }
	    else
	    {
		// Modulate our result by the prelen previous one..
		if (zerotable[i] & (1 << j))
		{
		    *dst++ = src[pos] ^ src[pos-prelen];
		    delay();
		}
	    }
	    pos++;
	}
    }

    return (dst - dststart);
}

int
SRAMSTREAM::decompressZeroTable(char *dst, char *src, PREPROCESS_TYPE preprocess, int blocklen)
{
    char	zerotable[128];
    int		zerotablelen;
    char	*srcstart = src;
    int		prelen;
    int		i, j, pos;
    char	c;

    zerotablelen = (blocklen + 7) >> 3;
    UT_ASSERT(! (blocklen & 7) );

    // Fetch the zero table...
    //src += decompressRLE(zerotable, src, 128);
    if (zerotablelen < 20)
	src += decompressNone(zerotable, src, zerotablelen);
    else
	src += decompress(zerotable, src, zerotablelen);

    switch (preprocess)
    {
	case PREPROCESS_NONE:
	    //msg_report("prenone");
	    prelen = 0;
	    break;
	case PREPROCESS_BYTE:
	    //msg_report("prebyte");
	    prelen = 1;
	    break;
	case PREPROCESS_SHORT:
	    //msg_report("preshort");
	    prelen = 2;
	    break;
	case PREPROCESS_INT:
	    //msg_report("preint");
	    prelen = 4;
	    break;
	default:
	    UT_ASSERT(0);
	    prelen = 1024;
	    break;
    }

    pos = 0;
    for (i = 0; i < zerotablelen; i++)
    {
	for (j = 0; j < 8; j++)
	{
	    if (pos < prelen || (preprocess == PREPROCESS_NONE))
	    {
		c = 0;
	    }
	    else
	    {
		c = dst[-prelen];
	    }
	    if (zerotable[i] & (1 << j))
	    {
		c ^= *src++;
	    }
	    *dst++ = c;
	    delay();
	    pos++;
	}
    }
    
    return (src - srcstart);
}

int
SRAMSTREAM::compressRLE(char *dst, char *src, int len, bool header)
{
    char		*dststart = dst;
    int			 currun, i;
    int			 runsame;
    int			 n, j;

    UT_ASSERT(!"RLE IS BROKEN!");
    
    if (header)
    {
	*dst++ = COMPRESS_RLE;
	delay();
    }
    
    // The rule is if we see something 3 long, we do a run.  This doesn't
    // save anything, except it might collapse one of the 8 mandatory
    // restarts in an 8k table.

    currun = 0;
    runsame = 0;
    i = 0;
    while (i < 1024)
    {
	if (currun && (src[i-1] == src[i]))
	    runsame++;
	else
	{
	    // We have hit the end of a same run..  Properly write out
	    // if 3 or greater.
	    if (runsame > 3)
	    {
		// TODO: Is this off by one?
		j = i - currun;		// Get back to the start...
		n = currun - runsame;	// number different prior to same.
		if (n)
		{
		    *dst++ = n-1;	// No high bit, raw data...
		    delay();
		    while (n--)
		    {
			*dst++ = src[j++];
			delay();
		    }
		}
		// Now, dump out the run proper...
		*dst++ = (runsame-1) | 0x80;	// high bit for rle
		delay();
		*dst++ = src[i-1];		// Last valid...
		delay();
		
		// We now want i to be on the stack, so a currun of 1
		// and runsame of 1.
		runsame = currun = 1;
		i++;
		continue;
	    }
	    // Otherwise, we keep trucking resetting runsame to 1.
	    runsame = 1;
	}
	currun++;
	if (currun == 128)
	{
	    // We need to dump out this sequence and reset...
	    // If runsame == currun, we had 128 in a row the same.
	    // Otherwise, it is raw data time...
	    if (runsame == currun)
	    {
		// NOTE: This errors out on MSVC.
		// Good thing we don't use runlength due to some
		// other error
		*dst++ = (char) (unsigned char)0xFF; // (128 - 1) | 0x80
		delay();
		*dst++ = src[i];
		delay();
	    }
	    else
	    {
		j = i - currun;
		n = 128;
		*dst++ = n-1;	// No high bit, raw data...
		delay();
		while (n--)
		{
		    *dst++ = src[j++];
		    delay();
		}
	    }

	    // Now we have dumped out a full block, reset everything..
	    runsame = currun = 0;
	    i++;
	    continue;
	}

	i++;
    }

    // i hit 1024.  Need to dump what is left...
    if (currun)
    {
	if (runsame == currun)
	{
	    *dst++ = 0x80 | (currun - 1); 
	    delay();
	    *dst++ = src[i];
	    delay();
	}
	else
	{
	    j = i - currun;
	    n = currun;
	    *dst++ = n-1;	// No high bit, raw data...
	    delay();
	    while (n--)
	    {
		*dst++ = src[j++];
		delay();
	    }
	}
    }

    return (dst - dststart);
}

int
SRAMSTREAM::decompressRLE(char *dst, char *src, int len)
{
    int			n;
    char		c;
    char		*srcstart = src;
    
    while (len > 0)
    {
	c = *src++;
	delay();
	if (c & 0x80)
	{
	    // RLE...
	    n = c & 0x7f;
	    n++;
	    c = *src++;
	    delay();
	    while (n--)
	    {
		*dst++ = c;
		len--;
	    }
	}
	else
	{
	    // Raw data...
	    n = c & 0x7f;
	    n++;
	    while (n--)
	    {
		*dst++ = *src++;
		len--;
		delay();
	    }
	}
    }

    return (src - srcstart);
}

//
// Hamfake methods defined in the non-SDL world.
//
#if !defined(USING_SDL) && !defined(USING_DS) && !defined(_3DS)

char *
hamfake_readLockSRAM()
{
    return (char *) 0xE000000;
}

void
hamfake_readUnlockSRAM(char *)
{
}

char *
hamfake_writeLockSRAM()
{
    return (char *) 0xE000000;
}

void
hamfake_writeUnlockSRAM(char *)
{
}

// This is being written whilst travelling at a great land speed.
// I'm on the Nozomi Super Express Shinkasen, speeding past Mt. Fuji.
void
hamfake_endWritingSession()
{
}

#endif
