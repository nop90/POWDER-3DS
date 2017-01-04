// bmp2c.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <fstream>

using namespace std;

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
    int	    ypelperm;
    int	    clrused;
    int	    clrimport;
};

#pragma pack(pop)

static short
gfx_readportableshort(istream &is)
{
    unsigned char	c[2];
    short	result;

    // This is one of my top-ten stupid conversions.
    // If MSVC weren't such crap, I could just use char * everywhere.
    is.read((char *) c, 2);

    // By reading a byte at a time we can re-assemble the short
    // without worrying about the native format as arithimatic is the 
    // same.
    result = c[0];
    result |= c[1] << 8;

    return result;
}

static int
gfx_readportableint(istream &is)
{
    unsigned short	s1, s2;
    int			result;

    s1 = (unsigned short) gfx_readportableshort(is);
    s2 = (unsigned short) gfx_readportableshort(is);

    result = s1;
    result |= s2 << 16;

    return result;
}

unsigned short *
loadbitmap(const char *name, int &size)
{
    ifstream	    is;
    BMPHEAD	    head;
    int		    i, x, y;

    is.open(name, ios_base::in | ios_base::binary);

    if (!is)
    {
	cerr << "Failure to open: " << name << endl;
	return 0;
    }

    is.read(head.id, 2);

    head.size = gfx_readportableint(is);
    head.reserved = gfx_readportableint(is);
    head.headersize = gfx_readportableint(is);
    head.infosize = gfx_readportableint(is);
    head.width = gfx_readportableint(is);
    head.depth = gfx_readportableint(is);
    head.biplane = gfx_readportableshort(is);
    head.bits = gfx_readportableshort(is);
    head.comp = gfx_readportableint(is);
    head.imagesize = gfx_readportableint(is);
    head.xpelperm = gfx_readportableint(is);
    head.ypelperm = gfx_readportableint(is);
    head.clrused = gfx_readportableint(is);
    head.clrimport = gfx_readportableint(is);

    if (head.bits != 24)
    {
	cerr << "Head.bits is " << head.bits << endl;
	cerr << "Can only support 24 bit colour bitmaps." << endl;
	cerr << "If this is 6144, maybe endianness problem?" << endl;
	return 0;
    }

    unsigned short	*result;

    size = head.imagesize / 3;
    result = new unsigned short [size];

    // BMPs are stored bottom up, so we reverse it here.
    for (y = head.depth - 1; y >= 0; y--)
    {
	for (x = 0; x < head.width; x++)
	{
	    unsigned char		red, blue, green;

	    // BMPs are stored BGR.
	    is.read((char *) &blue, 1);
	    is.read((char *) &green, 1);
	    is.read((char *) &red, 1);
	
	    // Convert to 15 bit BGR.

	    // Convert all to 5 bits from 8 bits.
	    red >>= 3;
	    green >>= 3;
	    blue >>= 3;

	    // Account for endian issues & repack
	    i = x + y * head.width;
	    //result[i] = (((blue << 2) | (green >> 3)) << 8) |
	    //		(((green << 5) | red));
	    result[i] = blue;
	    result[i] <<= 5;
	    result[i] |= green;
	    result[i] <<= 5;
	    result[i] |= red;

	    // Swap endianness.
	    // Because we save this as 16 bit text we don't
	    // have to correct for the machine's endianess.  I hope.
	    //result[i] = (result[i] << 8) | (result[i] >> 8);
	}
    }

    return result;
}

int main(int argc, char *argv[])
{
    cout << "bmp2c: Version 001" << endl;

    if (argc < 2)
    {
	cerr << "No given source file, aborting." << endl;
	return -1;
    }

    unsigned short	*result;
    int			 size;
    int			 i;
    
    result = loadbitmap(argv[1], size);

    if (!result)
	return -1;

    char		outputname[500];
    char		varname[500];

    sprintf(outputname, "%s.c", argv[1]);

    ofstream	os(outputname);

    if (!os)
    {
	cerr << "Unable to write to " << outputname << ", aborting." << endl;
	return -1;
    }

    strcpy(varname, argv[1]);
    // Terminate at .
    if (strchr(varname, '.'))
	*strchr(varname, '.') = '\0';

    os << "// Auto-generated .bmp file" << endl;
    os << "// DO NOT HAND EDIT" << endl;
    os << "// Generated from " << argv[1] << endl;
    os << endl;
    os << "const unsigned short bmp_" << varname << "[" << size << "] = {" << endl;

    for (i = 0; i < size; i++)
    {
	char		buf[100];

	sprintf(buf, "0x%04x", (unsigned int) result[i]);
	if (i < size-1)
	    strcat(buf, ", ");
	os << buf;
	if ((i & 7) == 7)
	    os << endl;
    }
    if (i & 7)
	os << endl;
    
    os << "};" << endl;
    
    return 0;
}
