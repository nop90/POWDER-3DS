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

#include "gfx/all_bitmaps.h"
#include "mygba.h"
#include "gfxengine.h"

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

void
shorttorgb(u16 cd, u8 &r, u8 &g, u8 &b)
{
    r = cd & 31;
    r <<= 3;
    cd >>= 5;
    g = cd & 31;
    g <<= 3;
    cd >>= 5;
    b = cd & 31;
}

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

    fp = hamfake_fopen(name, "rb");
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
	    red >>= 3;
	    green >>= 3;
	    blue >>= 3;

	    result[i] = blue;
	    result[i] <<= 5;
	    result[i] |= green;
	    result[i] <<= 5;
	    result[i] |= red;
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

#define NUM_ALPHA 5

void
floodColours(s16 *cd_to_pal)
{
    bool		found = true;
    int			i = 0, axis;
    u8			color[3];
    u16			cd;
    s16			*other;

    other = new s16[(1<<15)];
    
    int round = 0;

    while (found)
    {
	found = false;
	for (i = 0; i < (1<<15); i++)
	{
	    other[i] = cd_to_pal[i];
	    if (cd_to_pal[i] != -1)
	    {
		continue;
	    }
	    
	    shorttorgb(i, color[0], color[1], color[2]);

	    for (axis = 0; axis < 3; axis++)
	    {
		color[axis] >>= 3;
	    }
	    for (axis = 0; axis < 3; axis++)
	    {
		if (color[axis])
		{
		    color[axis]--;
		    cd = rgbtoshort(color[0]*8, color[1]*8, color[2]*8);
		    if (cd_to_pal[cd] != -1)
		    {
			other[i] = cd_to_pal[cd];
			found = true;
		    }
		    color[axis]++;
		}
		if (color[axis] < 31)
		{
		    color[axis]++;
		    cd = rgbtoshort(color[0]*8, color[1]*8, color[2]*8);
		    if (cd_to_pal[cd] != -1)
		    {
			other[i] = cd_to_pal[cd];
			found = true;
		    }
		    color[axis]--;
		}
	    }
	}
	if (found)
	{
	    memcpy(cd_to_pal, other, (1<<15) * sizeof(s16));
	}
    }

    delete [] other;
}

bool
bmp_loadExtraTileset()
{
    // First, load our alphabet, dungeon, and minimap.  We load them
    // as 15 bit images.
    u16		*a16[NUM_ALPHA], *d16, *m16, *mf16, *sprite16;
    int		 aw, ah, dw, dh, mw, mh, sw, sh;
    bool	 failed = false, alphafail = false;
    int		 anum;
    
    int		 tilewidth = 8;

    const char *aname[NUM_ALPHA] =
    {
	"alphabet_classic",
	"alphabet_brass",
	"alphabet_shadow",
	"alphabet_light",
	"alphabet_heavy",
    };
    BUF		buf;
    
    for (anum = 0; anum < NUM_ALPHA; anum++)
    {
	buf.sprintf("gfx/%s.bmp", aname[anum]);
	a16[anum] = bmp_load(buf.buffer(), aw, ah, true);
	if (!a16[anum])
	    alphafail = true;
    }
    d16 = bmp_load("gfx/dungeon16.bmp", dw, dh, true);
    m16 = bmp_load("gfx/mini16.bmp", mw, mh, true);
    mf16 = bmp_load("gfx/minif16.bmp", mw, mh, true);
    sprite16 = bmp_load("gfx/sprite16.bmp", sw, sh, true);

    // Check if any failed...
    if (alphafail || !d16 || !mf16 || !m16 || !sprite16)
    {
	// IF partial failure, spam.
	if (!alphafail || d16 || mf16 || m16 || sprite16)
	{
	    if (alphafail)
		printf("Failed to load gfx/alphabet_*.bmp\n");
	    if (!d16)
		printf("Failed to load gfx/dungeon16.bmp\n");
	    if (!m16)
		printf("Failed to load gfx/mini16.bmp\n");
	    if (!mf16)
		printf("Failed to load gfx/minif16.bmp\n");
	    if (!sprite16)
		printf("Failed to load gfx/sprite16.bmp\n");
	}

	failed = true;
    }

    // Calculate our tile width.  This is only legal in true SDL
    // modes.
#ifdef USING_SDL
    tilewidth = aw / 10;
#endif

    // Verify our sizes are accurate.
    if (!alphafail && (aw != 10*tilewidth || ah < 88))
    {
	printf("gfx/alphabet.bmp incorrect size, must be 80x88\n");
	failed = true;
    }
    if (d16 && (dw != 48*tilewidth || dh < 192))
    {
	printf("gfx/dungeon16.bmp incorrect size, must be 384x192\n");
	failed = true;
    }
    if (m16 && (mw != 48*tilewidth || mh < 64))
    {
	printf("gfx/mini16.bmp incorrect size, must be 384x64\n");
	failed = true;
    }
    if (sprite16 && (sw != 48*tilewidth || sh < 96))
    {
	printf("gfx/sprite16.bmp incorrect size, must be 384x96\n");
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

    for (i = 0; i < mw * mh; i++)
    {
	if (cd_to_pal[m16[i]] == -1)
	{
	    cd_to_pal[m16[i]] = numcol;
	    palette[numcol] = m16[i];
	    numcol++;
	    if (numcol > 255) { floodColours(cd_to_pal); numcol--; overflow++; }
	}
	if (cd_to_pal[mf16[i]] == -1)
	{
	    cd_to_pal[mf16[i]] = numcol;
	    palette[numcol] = mf16[i];
	    numcol++;
	    if (numcol > 255) { floodColours(cd_to_pal); numcol--; overflow++; }
	}
    }
    for (i = 0; i < dw * dh; i++)
    {
	if (cd_to_pal[d16[i]] == -1)
	{
	    cd_to_pal[d16[i]] = numcol;
	    palette[numcol] = d16[i];
	    numcol++;
	    if (numcol > 255) { floodColours(cd_to_pal); numcol--; overflow++; }
	}
    }
    for (anum = 0; anum < NUM_ALPHA; anum++)
    {
	for (i = 0; i < aw * ah; i++)
	{
	    if (cd_to_pal[a16[anum][i]] == -1)
	    {
		cd_to_pal[a16[anum][i]] = numcol;
		palette[numcol] = a16[anum][i];
		numcol++;
		if (numcol > 255) { floodColours(cd_to_pal); numcol--; overflow++; }
	    }
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

    // Set our tileset's palette.
    glb_tilesets[NUM_TILESETS-1].palette = palette;
    glb_tilesets[NUM_TILESETS-1].tilewidth = tilewidth;

    // Create tiled versions.
    for (anum = 0; anum < NUM_ALPHA; anum++)
    {
	glb_tilesets[NUM_TILESETS-1].alphabet[anum] = bmp_tilify(a16[anum], 
						    aw, ah, cd_to_pal, 
						    tilewidth, tilewidth, 1, 1);
    }
    glb_tilesets[NUM_TILESETS-1].dungeon = bmp_tilify(d16, dw, dh, cd_to_pal, 
						    tilewidth, tilewidth, 2, 2);
    glb_tilesets[NUM_TILESETS-1].mini = bmp_tilify(m16, mw, mh, cd_to_pal, 
						    tilewidth, tilewidth, 2, 2);
    glb_tilesets[NUM_TILESETS-1].minif = bmp_tilify(mf16, mw, mh, cd_to_pal, 
						    tilewidth, tilewidth, 2, 2);


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
	    if (numcol > 255) { floodColours(cd_to_pal); numcol--; overflow++; }
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

    glb_tilesets[NUM_TILESETS-1].spritepalette = palette;
    glb_tilesets[NUM_TILESETS-1].sprite = bmp_tilify(sprite16, sw, sh, cd_to_pal, 
						    tilewidth, tilewidth, 2, 2);
    // Free unused structures
    for (anum = 0; anum < NUM_ALPHA; anum++)
	delete [] a16[anum];
    delete [] d16;
    delete [] m16;
    delete [] mf16;
    delete [] sprite16;
    delete [] cd_to_pal;

    return true;
}
