// splicebmp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <fstream>

using namespace std;

//
// This key is from line 384 to line 384+15 of gfx/classic/dungeon16_16.c
// It is the picture of the well.  It occasionally changes with palette
// shifts.
//
const int image_keylen = 8*8*2*2;
const unsigned char image_key[image_keylen] =
{
0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 
0x03, 0x03, 0x03, 0x03, 0x0a, 0x0b, 0x0a, 0x0a, 0x02, 0x02, 0x02, 0x0a, 0x12, 0x0a, 0x12, 0x12, 
0x02, 0x02, 0x0a, 0x12, 0x07, 0x07, 0x07, 0x07, 0x02, 0x02, 0x0a, 0x0b, 0x07, 0x07, 0x07, 0x07, 
0x02, 0x02, 0x0b, 0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x03, 0x03, 0x0b, 0x0a, 0x0a, 0x0b, 0x0a, 0x0a, 
0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 
0x01, 0x0b, 0x0a, 0x03, 0x03, 0x03, 0x03, 0x03, 0x12, 0x0a, 0x12, 0x0a, 0x02, 0x02, 0x02, 0x02, 
0x07, 0x07, 0x07, 0x12, 0x0b, 0x02, 0x02, 0x02, 0x07, 0x07, 0x07, 0x0a, 0x0b, 0x02, 0x02, 0x02, 
0x0a, 0x0b, 0x0a, 0x0a, 0x0b, 0x02, 0x02, 0x02, 0x0b, 0x0a, 0x0b, 0x0b, 0x0a, 0x03, 0x03, 0x03, 
0x02, 0x02, 0x0b, 0x0a, 0x0b, 0x0a, 0x0a, 0x0b, 0x02, 0x02, 0x0a, 0x0b, 0x0a, 0x0b, 0x0a, 0x0b, 
0x02, 0x02, 0x0a, 0x0b, 0x0a, 0x0a, 0x0b, 0x0a, 0x02, 0x02, 0x0b, 0x0a, 0x0b, 0x0a, 0x0b, 0x0a, 
0x03, 0x03, 0x03, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b, 0x02, 0x02, 0x02, 0x03, 0x0a, 0x0b, 0x0a, 0x0a, 
0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 
0x0a, 0x0a, 0x0b, 0x0a, 0x0b, 0x02, 0x03, 0x02, 0x0b, 0x0a, 0x0b, 0x0a, 0x0a, 0x02, 0x03, 0x02, 
0x0a, 0x0b, 0x0a, 0x0b, 0x0a, 0x02, 0x03, 0x02, 0x0a, 0x0b, 0x0a, 0x0b, 0x0a, 0x02, 0x03, 0x02, 
0x0b, 0x0a, 0x0a, 0x0b, 0x03, 0x03, 0x03, 0x03, 0x0a, 0x0b, 0x0a, 0x02, 0x02, 0x03, 0x02, 0x02, 
0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 
};

const int palette_keylen = 16;
const unsigned char palette_key[palette_keylen] =
{
    0x00, 0x00,
    0xef, 0x3d,
    0x10, 0x00,
    0x1f, 0x00,
    0x10, 0x02,
    0xff, 0x03,
    0x00, 0x40,
    0x00, 0x7c
};

int
findOffset(const unsigned char *data, int datalen, const unsigned char *key, int keylen)
{ 
    int	    off, i;

    for (off = 0; off < datalen - keylen; off++)
    {
	for (i = 0; i < keylen; i++)
	{
	    if (data[i + off] != key[i])
	    {
		// Mis match!
		break;
	    }
	}
	if (i == keylen)
	{
	    // Full match!
	    return off;
	}
    }

    // No match
    return -1;
}

#pragma pack(push, 1)

struct BMPHEAD
{
    char    id[2];
    int	    size;
    int reserved;
    int	    headersize;
    int	    infosize;
    int	    width;
    int	    depth;
    short   biplane;
    short   bits;
    int	    comp;
    int	    imagesize;
    int	    xpelperm;
    int	    ypelpm;
    int	    clrused;
    int	    clrimport;
};

#pragma pack(pop)

struct RGBQUAD
{
    unsigned char    blue;
    unsigned char    green;
    unsigned char    red;
    unsigned char    filler;
};

unsigned char *
loadbitmap(const char *name, unsigned char *palette, int &size)
{
    ifstream	    is;
    BMPHEAD	    head;

    is.open(name, ios_base::in | ios_base::binary);

    if (!is)
    {
	cerr << "Failure to open: " << name << endl;
	return 0;
    }

    is.read((char *) &head, sizeof(BMPHEAD));

    if (head.bits != 8)
    {
	cerr << "Head.bits is " << head.bits << endl;
	cerr << "Cannot support non-256 colour bitmaps." << endl;
	return 0;
    }

    unsigned char	*result;

    size = head.imagesize;
    result = new unsigned char [size];

    // Read the palette.  Stored 4 bytes per entry
    // so needs some processing
    RGBQUAD	    filepalette[256];
    int		    i;

    is.read((char *) &filepalette, sizeof(RGBQUAD) * 256);
    for (i = 0; i < 256; i++)
    {
	// Convert to 15 bit BGR.
	// Bright green is made black.
	if (filepalette[i].green == 0xff && !filepalette[i].red && !filepalette[i].blue)
	    filepalette[i].green = 0;

	// Convert all to 5 bits from 8 bits.
	filepalette[i].red >>= 3;
	filepalette[i].green >>= 3;
	filepalette[i].blue >>= 3;

	// Account for endian issues & repack
	palette[i*2+1] = (filepalette[i].blue << 2) | (filepalette[i].green >> 3);
	palette[i*2] = (filepalette[i].green << 5) | filepalette[i].red;
    }

    // Read data.  Assume width is 4 byte aligned.
    is.read((char *)result, size);

    return result;
}

void
buildpalettelut(unsigned char *palette_lut, unsigned char *newpal, unsigned char *oldpal)
{
    // We wish to alter the new palette so it matches the old palette.
    // To do this:
    // 1) Find valid colours in old palette.  (Count 0s from end)
    // 2) Find LUT of each of new palette into old palette.
    // 3) Write out unused colours.

    int		i, j, oldsize;

    for (i = 255; i >= 0; i--)
    {
	if (oldpal[i*2] || oldpal[i*2-1])
	{
	    // Valid palette,
	    break;
	}
    }

    oldsize = i;

    // palette_lut is all 0, for no mapping.  Only valid 0 mapping
    // is 0->0.
    memset(palette_lut, 0, 256);
    for (i = 1; i < 256; i++)
    {
	// Find match.
	for (j = 1; j < oldsize; j++)
	{
	    if (newpal[i*2] == oldpal[j*2] &&
		newpal[i*2+1] == oldpal[j*2+1])
	    {
		// Valid mapping
		palette_lut[i] = j;
		break;
	    }
	}
    }

    // Now, create the new palette.
    unsigned char	tmppal[512];
    // Clear out all of it.
    memset(tmppal, 0, 512);

    memcpy(tmppal, oldpal, oldsize * 2);

    // Assign all unused ones from the new list.
    j = oldsize;
    for (i = 1; i < 256; i++)
    {
	if (j > 255)
	    break;

	if (!palette_lut[i])
	{
	    palette_lut[i] = j;
	    tmppal[j*2] = newpal[i*2];
	    tmppal[j*2+1] = newpal[i*2+1];
	    j++;
	}
    }

    memcpy(newpal, tmppal, 512);
}

unsigned char *
tilify(const unsigned char *flat, int len, const unsigned char *palette_lut)
{
    // This tilifies the flat bitmap into 2x2 clusters of 8x8 tiles.
    int		width = 384, bytesperrow, tileperrow = width / 16;
    int		tx, ty, sx, sy, x, y;
    unsigned char	*result;
    int		off = 0;
    int		th;

    result = new unsigned char[len];

    bytesperrow = 8 * 8 * 2 * 2 * tileperrow;

    th = len / bytesperrow;
    for (ty = 0; ty < th; ty++)
    {
	for (tx = 0; tx < 384 / 16; tx++)
	{
	    for (sy = 0; sy < 2; sy++)
	    {
		for (sx = 0; sx < 2; sx++)
		{
		    for (y = 0; y < 8; y++)
		    {
			for (x = 0; x < 8; x++)
			{
			    result[off++] = 
				palette_lut[flat[ ((th - ty - 1) * 16 + (1-sy) * 8 + (7-y)) * width + (tx * 16 + sx * 8 + x) ]];
			}
		    }
		}
	    }
	}
    }

    //memcpy(result, flat, off);
    //memset(result, 0, 8*8*2*2);

    return result;
}

void splicedata(unsigned char *dst, int offset, unsigned char *src, int len)
{
    int		i;

    for (i = 0; i < len; i++)
    {
	dst[offset+i] = src[i];
    }
}


int main(int argc, char* argv[])
{
    ifstream	    is;

    cout << "splicebmp: Version 002" << endl;

    is.open("powder.gba", ios_base::in | ios_base::binary);

    if (!is)
    {
	cerr << "Unable to open \"powder.gba\", aborting." << endl;
	return -1;
    }

    // Read in the original file.
    unsigned char *powdergba;
    int	        powderlen;

    is.seekg(0, ios_base::end);
    powderlen = is.tellg();

    powdergba = new unsigned char [powderlen];

    is.seekg(0, ios_base::beg);
    is.read((char *)powdergba, powderlen);

    is.close();

    int		powderoff;

    powderoff = findOffset(powdergba, powderlen, image_key, image_keylen);

    if (powderoff < 0)
    {
	cerr << "Unable to find old image in \"powder.gba\", aborting." << endl;
	return -2;
    }

    // Adjust the powder offset as it refers to the well icon
    powderoff -= 384 * 16;

    int		paletteoff;
    paletteoff = findOffset(powdergba, powderlen, palette_key, palette_keylen);
    if (paletteoff < 0)
    {
	cerr << "Unable to find old palette in \"powder.gba\", aborting." << endl;
	return -2;
    }

    unsigned char *bitmap, palette[512], *tiled;
    int   bitmaplen;

    bitmap = loadbitmap("dungeon16.bmp", palette, bitmaplen);
    if (!bitmap)
    {
	cerr << "Unable to load \"dungeon16.bmp\", aborting." << endl;
	return -3;
    }

    unsigned char palette_lut[256];
    buildpalettelut(palette_lut, palette, &powdergba[paletteoff]);

    tiled = tilify(bitmap, bitmaplen, palette_lut);

    splicedata(powdergba, powderoff, tiled, bitmaplen);
    splicedata(powdergba, paletteoff, palette, 512);

    // Save out result.
    ofstream	    os;

    os.open("powder_c.gba", ios_base::out | ios_base::trunc | ios_base::binary);

    if (!os)
    {
	cerr << "Cannot write to \"powder_c.gba\", aborting." << endl;
	return -4;
    }

    os.write((char *)powdergba, powderlen);

    os.close();

    cout << "Successfully spliced data!" << endl;
    
    return 0;
}

