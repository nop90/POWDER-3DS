/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        bmp.cpp ( BMP Library, C++ )
 *
 * COMMENTS:
 */

#include "bmp.h"
#include <stdio.h>

#ifdef USING_DS
#include <nds.h>
#include <fat.h>

#define iprintf printf

#else
#endif

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

struct RGBQUAD
{
    u8 blue;
    u8 green;
    u8 red;
    u8 filler;
};

u16
rgbtoshort(u8 red, u8 green, u8 blue)
{
    u16		result;

    // Reduce to 5 bits.
    red >>= 3;
    green >>= 3;
    blue >>= 3;

    result = blue;
    result <<= 5;
    result |= green;
    result <<= 5;
    result |= red;

    return result;
}

static short
gfx_readportableshort(FILE *fp)
{
    u8		c[2];
    short	result;

    fread(c, 2, 1, fp);

    result = c[0];
    result |= c[1] << 8;

    return result;
}

static int
gfx_readportableint(FILE *fp)
{
    unsigned short	s1, s2;
    int			result;

    s1 = (unsigned short) gfx_readportableshort(fp);
    s2 = (unsigned short) gfx_readportableshort(fp);

    result = s1;
    result |= s2 << 16;

    return result;
}


unsigned short *
bmp_load(const char *name, int &w, int &h, bool quiet)
{
    // printf("Load bmp %s\n", name);

    FILE	*fp;
    BMPHEAD	 head;
    int		 i, x, y;

    fp = fopen(name, "rb");
    if (!fp)
    {
	if (!quiet)
	    printf("Failure to open %s\n", name);
	return 0;
    }

    fread(head.id, 2, 1, fp);
    
    head.size = gfx_readportableint(fp);
    head.reserved = gfx_readportableint(fp);
    head.headersize = gfx_readportableint(fp);
    head.infosize = gfx_readportableint(fp);
    head.width = gfx_readportableint(fp);
    head.depth = gfx_readportableint(fp);
    head.biplane = gfx_readportableshort(fp);
    head.bits = gfx_readportableshort(fp);
    head.comp = gfx_readportableint(fp);
    head.imagesize = gfx_readportableint(fp);
    head.xpelperm = gfx_readportableint(fp);
    head.ypelperm = gfx_readportableint(fp);
    head.clrused = gfx_readportableint(fp);
    head.clrimport = gfx_readportableint(fp);

    if (head.id[0] != 'B' && head.id[1] != 'M')
    {
	printf("%s does not look like a bitmap!\n", name);
	fclose(fp);
	return 0;
    }

    if (head.bits != 24 && head.bits != 8)
    {
	printf("Cannot support %d bit bmp.\n", head.bits);
	printf("Only 24 or 8 bit supported.\n");
	fclose(fp);
	return 0;
    }

    u16	*result;

    h = head.depth;
    w = head.width;

    // printf("Desired image is %d by %d\n", h, w);

    result = new u16 [h * w];

    RGBQUAD	palette[256];

    if (head.bits == 8)
    {
	int		numentries = 256;

	// Clear out palette to start.
	memset(palette, 0, 256*sizeof(RGBQUAD));

	// A non-zero clrused means we only have that number of
	// colours in our header.
	if (head.clrused)
	    numentries = head.clrused;
	fread(palette, sizeof(RGBQUAD), numentries, fp);
    }

    // BMPs are stored bottom up, so we reverse it here.
    for (y = head.depth - 1; y >= 0; y--)
    {
	for (x = 0; x < head.width; x++)
	{
	    u8		red, blue, green, idx;

	    if (head.bits == 24)
	    {
		// BMPs are stored BGR.
		fread(&blue, 1, 1, fp);
		fread(&green, 1, 1, fp);
		fread(&red, 1, 1, fp);
	    }
	    else
	    {
		fread(&idx, 1, 1, fp);
		blue = palette[idx].blue;
		green = palette[idx].green;
		red = palette[idx].red;
	    }
	
	    // Account for endian issues & repack
	    i = x + y * head.width;

	    // Reduce to 5 bits.
	    result[i] = rgbtoshort(red, green, blue);
	}
	// NOTE: We need to align our scan lines to 4 byte boundaries.
	if ((x * 3) & 3)
	{
	    int		extra;
	    u8		data;

	    extra = 4 - (x * 3 & 3);
	    while (extra--)
		fread(&data, 1, 1, fp);
	}
    }
    return result;
}

static u8 *
bmp_tilify(const u16 *flat, int w, int h, s16 *cd_to_pal,
	    int tilewidth, int tileheight, int numtilex, int numtiley)
{
    int	bytesperrow, tileperrow;
    int	tx, ty, sx, sy, x, y;
    u8 *result;
    int	off = 0;
    int th;
    int len;

    len = w * h;
    result = new u8 [len];
    
    tileperrow = w / (tilewidth * numtilex);
    bytesperrow = (tilewidth * tileheight * numtilex * numtiley) * tileperrow;

    th = len / bytesperrow;

    for (ty = 0; ty < th; ty++)
    {
	for (tx = 0; tx < w / (tilewidth * numtilex); tx++)
	{
	    for (sy = 0; sy < numtiley; sy++)
	    {
		for (sx = 0; sx < numtilex; sx++)
		{
		    for (y = 0; y < tileheight; y++)
		    {
			for (x = 0; x < tilewidth; x++)
			{
			    result[off++] = (u8) 
				cd_to_pal[flat[ 
    (ty * tileheight * numtiley + sy * tileheight + y) * w 
    + (tx * numtilex * tilewidth + sx * tilewidth + x) ]];
			}
		    }
		}
	    }
	}
    }

    return result;
}

void
saveToC(const char *fname, const char *dataname, u16 *data, int len)
{
    FILE		*fp;
    int			 column;

    fp = fopen(fname, "wt");
    if (!fp)
    {
	printf("Failure to write to %s\n", fname);
	return;
    }

    fprintf(fp, "const unsigned short %s[%d] = {\n", dataname, len);
    column = 0;
    while (len--)
    {
	fprintf(fp, "0x%04x", *data++);
	if (len)
	    fprintf(fp, ", ");
	column++;
	if (!(column & 7))
	    fprintf(fp, "\n");
    }
    if (column & 7)
	fprintf(fp, "\n");
    fprintf(fp, "};\n");

    fclose(fp);
}

void
saveToC(const char *fname, const char *dataname, u8 *data, int len)
{
    FILE		*fp;
    int			 column;

    fp = fopen(fname, "wt");
    if (!fp)
    {
	printf("Failure to write to %s\n", fname);
	return;
    }

    fprintf(fp, "const unsigned char %s[%d] = {\n", dataname, len);
    column = 0;
    while (len--)
    {
	fprintf(fp, "0x%02x", *data++);
	if (len)
	    fprintf(fp, ",");
	column++;
	if (!(column & 15))
	    fprintf(fp, "\n");
    }
    if (column & 15)
	fprintf(fp, "\n");
    fprintf(fp, "};\n");

    fclose(fp);
}

#define NUM_ALPHA 5

bool
bmp_convertTileset()
{
    // First, load our alphabet, dungeon, and minimap.  We load them
    // as 15 bit images.
    u16		*a16[NUM_ALPHA], *d16, *m16, *mf16, *sprite16;
    int		 aw, ah, dw, dh, mw, mh, sw, sh;
    bool	 failed = false, alphafailed = false;

    const char *aname[NUM_ALPHA] =
    {
	"alphabet_classic",
	"alphabet_brass",
	"alphabet_shadow",
	"alphabet_light",
	"alphabet_heavy",
    };
    char		buf[100], buf2[100];

    // Tilified versions of our data.
    // palettes stay 16 bit.
    u8		*a8[NUM_ALPHA], *d8, *m8, *mf8, *sprite8;
    u16		*mainpalette, *spritepalette;
    int		 anum;
    int		 tilewidth = -1;

    for (anum = 0; anum < NUM_ALPHA; anum++)
    {
	sprintf(buf, "%s.bmp", aname[anum]);
	a16[anum] = bmp_load(buf, aw, ah, true);
	if (!a16[anum])
	{
	    printf("Failed to load %s\n", buf);
	    alphafailed = true;
	}
	else
	{
	    // Recompute our tilewidth assuming 10 chars across
	    if (tilewidth < 0)
		tilewidth = aw / 10;
	    if ((aw != tilewidth * 10) || (ah < tilewidth * 11))
	    {
		printf("alphabet_*.bmp incorrect size, must be %dx%d\n", tilewidth*10, tilewidth * 11);
		failed = true;
	    }
	}
    }

    d16 = bmp_load("dungeon16.bmp", dw, dh, true);
    m16 = bmp_load("mini16.bmp", mw, mh, true);
    mf16 = bmp_load("minif16.bmp", mw, mh, true);
    sprite16 = bmp_load("sprite16.bmp", sw, sh, true);

    // Check if any failed...
    if (alphafailed || !d16 || !m16 || !mf16 || !sprite16)
    {
	if (!d16)
	    printf("Failed to load dungeon16.bmp\n");
	if (!m16)
	    printf("Failed to load mini16.bmp\n");
	if (!mf16)
	    printf("Failed to load minif16.bmp\n");
	if (!sprite16)
	    printf("Failed to load sprite16.bmp\n");

	failed = true;
    }

    // Verify our sizes are accurate.
    if (d16 && (dw != tilewidth*48 || dh < tilewidth*24))
    {
	printf("dungeon16.bmp incorrect size, must be %dx%d\n", tilewidth*48, tilewidth*24);
	failed = true;
    }
    if (m16 && (mw != tilewidth*48 || mh < tilewidth * 8))
    {
	printf("mini16.bmp incorrect size, must be %dx%d\n", tilewidth*48, tilewidth*8);
	failed = true;
    }
    if (sprite16 && (sw != tilewidth*48 || sh < tilewidth*4))
    {
	printf("sprite16.bmp incorrect size, must be %dx%d\n", tilewidth*48, tilewidth*4);
	failed = true;
    }
    
    if (failed)
    {
	for (anum = 0; anum < NUM_ALPHA; anum++)
	    delete [] a16[anum];
	delete [] d16;
	delete [] m16;
	delete [] mf16;
	delete [] sprite16;

	return false;
    }

    // We now want to build a consolidated palette.
    // We can be wasteful of memory here as this is startup and the DS
    // has 4Mb anyways.
    s16		*cd_to_pal;
    u16		*palette;
    
    // Set everything to unallocated.
    cd_to_pal = new s16[(1<<15)];
    memset(cd_to_pal, 0xff, sizeof(s16) * (1<<15));

    // Palette maps back from palette space to raw space
    palette = new u16[256];
    memset(palette, 0, sizeof(u16) * 256);

    int		numcol = 0;		// Total assigned colours.
    u16		cd;

    // We always want black to map to transparent, so assign it first.
    cd = 0;
    cd_to_pal[cd] = numcol;
    palette[numcol] = cd;
    numcol++;

    // Likewise, force a bright red, bright green, and bright yellow
    // for our selfish text purposes.
    cd = rgbtoshort(255, 128, 128);
    cd_to_pal[cd] = numcol;
    palette[numcol] = cd;
    numcol++;
    cd = rgbtoshort(255, 255, 128);
    cd_to_pal[cd] = numcol;
    palette[numcol] = cd;
    numcol++;
    cd = rgbtoshort(128, 255, 128);
    cd_to_pal[cd] = numcol;
    palette[numcol] = cd;
    numcol++;

    // Now, collect all colours.
    int		overflow = 0;		// Ignored mappings.
    int		i;

    for (anum = 0; anum < NUM_ALPHA; anum++)
    {
	for (i = 0; i < aw * ah; i++)
	{
	    if (cd_to_pal[a16[anum][i]] == -1)
	    {
		cd_to_pal[a16[anum][i]] = numcol;
		palette[numcol] = a16[anum][i];
		numcol++;
		if (numcol > 255) { numcol--; overflow++; }
	    }
	}
    }
    for (i = 0; i < mw * mh; i++)
    {
	if (cd_to_pal[m16[i]] == -1)
	{
	    cd_to_pal[m16[i]] = numcol;
	    palette[numcol] = m16[i];
	    numcol++;
	    if (numcol > 255) { numcol--; overflow++; }
	}
	if (cd_to_pal[mf16[i]] == -1)
	{
	    cd_to_pal[mf16[i]] = numcol;
	    palette[numcol] = mf16[i];
	    numcol++;
	    if (numcol > 255) { numcol--; overflow++; }
	}
    }
    for (i = 0; i < dw * dh; i++)
    {
	if (cd_to_pal[d16[i]] == -1)
	{
	    cd_to_pal[d16[i]] = numcol;
	    palette[numcol] = d16[i];
	    numcol++;
	    if (numcol > 255) { numcol--; overflow++; }
	}
    }

    // We have now built our look up table.  Report any errors in building.
    if (overflow)
    {
	printf("%d colours lost in truncation to 256 palette.\n", overflow);
    }

    // Convert to black our magic green.
    for (i = 0; i < numcol; i++)
    {
	if (palette[i] == 0x3e0)
	    palette[i] = 0;
    }

    // Create tiled versions.
    mainpalette = palette;
    for (anum = 0; anum < NUM_ALPHA; anum++)
	a8[anum] = bmp_tilify(a16[anum], aw, ah, cd_to_pal, tilewidth, tilewidth, 1, 1);
    d8 = bmp_tilify(d16, dw, dh, cd_to_pal, tilewidth, tilewidth, 2, 2);
    m8 = bmp_tilify(m16, mw, mh, cd_to_pal, tilewidth, tilewidth, 2, 2);
    mf8 = bmp_tilify(mf16, mw, mh, cd_to_pal, tilewidth, tilewidth, 2, 2);

    // Now repeat the process again, this time for our sprite bitmap.
    // Set everything to unallocated.
    memset(cd_to_pal, 0xff, sizeof(s16) * (1<<15));

    // Palette maps back from palette space to raw space
    palette = new u16[256];
    memset(palette, 0, sizeof(u16) * 256);

    // We always want black to map to transparent, so assign it first.
    cd_to_pal[0] = 0;
    palette[0] = 0;

    // Now, collect all colours.
    numcol = 1;		// Total assigned colours.
    overflow = 0;		// Ignored mappings.

    for (i = 0; i < sw * sh; i++)
    {
	if (cd_to_pal[sprite16[i]] == -1)
	{
	    cd_to_pal[sprite16[i]] = numcol;
	    palette[numcol] = sprite16[i];
	    numcol++;
	    if (numcol > 255) { numcol--; overflow++; }
	}
    }

    // We have now built our look up table.  Report any errors in building.
    if (overflow)
    {
	printf("%d colours lost in truncation to 256 palette in sprites.\n", overflow);
    }

    // Convert to black our magic green.
    for (i = 0; i < numcol; i++)
    {
	if (palette[i] == 0x3e0)
	    palette[i] = 0;
    }

    spritepalette = palette;
    sprite8 = bmp_tilify(sprite16, sw, sh, cd_to_pal, tilewidth, tilewidth, 2, 2);

    // Free unused structures
    for (anum = 0; anum < NUM_ALPHA; anum++)
	delete [] a16[anum];
    delete [] d16;
    delete [] m16;
    delete [] mf16;
    delete [] sprite16;
    delete [] cd_to_pal;

    // Save out our generated files.
    saveToC("master.pal.c", "master_Palette", mainpalette, 256);
    saveToC("dungeon16_16.c", "dungeon16_Tiles", d8, dw * dh);
    saveToC("mini16_16.c", "mini16_Tiles", m8, mw * mh);
    saveToC("minif16_16.c", "minif16_Tiles", mf8, mw * mh);
    for (anum = 0; anum < NUM_ALPHA; anum++)
    {
	sprintf(buf, "%s_8.c", aname[anum]);
	sprintf(buf2, "%s_Tiles", aname[anum]);
	saveToC(buf, buf2, a8[anum], aw * ah);
    }
    saveToC("sprite16_16.c", "sprite16_Tiles", sprite8, sw * sh);
    saveToC("sprite.pal.c", "sprite_Palette", spritepalette, 256);

    return true;
}
