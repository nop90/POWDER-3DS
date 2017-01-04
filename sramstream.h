/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        sramstream.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	This is an attempt to provide a bit level stream to the SRAM
 * 	for backup & restoration.
 */

#ifndef __sramstream_h__
#define __sramstream_h__

//
// Hamfake methods for accessing save data.
//
char *hamfake_readLockSRAM();
void hamfake_readUnlockSRAM(char *);

char *hamfake_writeLockSRAM();
void hamfake_writeUnlockSRAM(char *);

void hamfake_endWritingSession();

enum COMPRESS_TYPE
{
    COMPRESS_NONE,
    COMPRESS_CONSTANT,
    COMPRESS_ZEROTABLE,
    COMPRESS_RLE
};

enum PREPROCESS_TYPE
{
    PREPROCESS_NONE,
    PREPROCESS_BYTE,
    PREPROCESS_SHORT,
    PREPROCESS_INT
};

class SRAMSTREAM
{
public:
    SRAMSTREAM();
    ~SRAMSTREAM();

    // Rest read/write pointer to start of SRAM.
    void		rewind();
    
    bool		writeRaw(const char *data, int numbytes);
    bool		readRaw(char *data, int numbytes);

    // Write given number of bits into stream.  Returns false if failed.
    bool		write(int val, int bits);
    // Reads a given number of bits.  Also returns false if failed.
    bool		read(int &val, int bits);
    // Unsigned read.
    bool		uread(int &val, int bits);

    // Reads up to maxlen-1, then null terminates.
    // false for failure.
    // We have max len on both sides to ensure the two are in sync.
    bool		readString(char *dst, int maxlen);
    bool		writeString(const char *src, int maxlen);
    
    // Flushes buffer into SRAM.  Do at end of writes to guarantee last
    // bit is written.
    bool		flush();

protected:
    void		writeToSRAM(char *data, int len);
    void		readFromSRAM(char *data, int len);

    // Compression methods...
    int			compress(char *dst, char *src, int blocklen);
    int			compressConstant(char *dst, char *src);
    int			compressNone(char *dst, char *src, int len, bool header=true);
    int			compressZeroTable(char *dst, char *src, PREPROCESS_TYPE preprocess, int len);
    int			compressRLE(char *dst, char *src, int len, bool header=true);
    int			decompress(char *dst, char *src, int blocklen);
    int			decompressConstant(char *dst, char *src, int len);
    int			decompressNone(char *dst, char *src, int len);
    int			decompressZeroTable(char *dst, char *src, PREPROCESS_TYPE preprocess, int blocklen);
    int			decompressRLE(char *dst, char *src, int len);

private:
    int			mySRAMPtr;		// char offset in SRAM
    int			myPos;			// char offset in myBuffer
    int			myBit;			// bit offset in myBuffer[myPos]
    char		myBuffer[1024];
    int			myNumBlocks;
};

#endif
