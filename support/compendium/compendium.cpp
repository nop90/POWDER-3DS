/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        compendium.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Extracts relevant info from game data into something
 *	parseable by the website.
 */

#include "mygba.h"
#include "../../glbdef.h"
#include "../../encyclopedia.h"
#include "../../buf.h"
#include "../../gfx/all_bitmaps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const encyclopedia_entry *
encyc_getentry(const char *bookname, int key)
{
    const encyclopedia_book	*book;
    int				 booknum;
    int				 entrynum;
    
    for (booknum = 0; booknum < NUM_ENCYCLOPEDIA_BOOKS; booknum++)
    {
	book = glb_encyclopedia[booknum];

	if (strcmp(book->bookname, bookname))
	    continue;
	
	for (entrynum = 0; entrynum < book->numentries; entrynum++)
	{
	    if (book->entries[entrynum]->key == key)
		return book->entries[entrynum];
	}
    }

    return 0;
}

static void
writeportableshort(FILE *fp, u16 v)
{
    u8		c[2];

    c[0] = v & 0xff;
    c[1] = v >> 8;

    fwrite(c, 2, 1, fp);
}

static void
writeportableint(FILE *fp, u32 v)
{
    writeportableshort(fp, v & 0xffff);
    writeportableshort(fp, v >> 16);
}

void
convertTo24bit(u16 clr, u8 &red, u8 &green, u8 &blue)
{
    red = clr & 31;
    red <<= 3;
    red += 4;
    clr >>= 5;
    green = clr & 31;
    green <<= 3;
    green += 4;
    clr >>= 5;
    blue = clr & 31;
    blue <<= 3;
    blue += 4;
}

// A 256 pixel image doesn't need a 256 colour palette :>
// Storing 24bit actually saves 512 byes, meaning png looks less tempting.
void
writeDataToBmp24bit(const char *fname, const u8 *data, const u16 *palette)
{
    FILE	*fp;
    int		 i, x, y;
    u8		 red, green, blue, index;

    fp = fopen(fname, "wb");
    if (!fp)
    {
	printf("Failed to open %s for writing\n", fname);
	return;
    }

    // Write the header.
    fwrite("BM", 2, 1, fp);
    writeportableint(fp, 16*16*3+54);	// size
    writeportableint(fp, 0);	// reserved
    writeportableint(fp, 54);	// Header size + palette
    writeportableint(fp, 40);	// Info size
    writeportableint(fp, 16);	// Width
    writeportableint(fp, 16);	// Height
    writeportableshort(fp, 1);	// Bit planes
    writeportableshort(fp, 24);	// Bits
    writeportableint(fp, 0);	// Compression
    writeportableint(fp, 0);	// image data size
    writeportableint(fp, 0);	// xpel/m
    writeportableint(fp, 0);	// ypel/m
    writeportableint(fp, 0);	// used colours
    writeportableint(fp, 0);	// important colours

    // Write out our data.  Our source data is tilified so we
    // need to invert the operation here
    // 16*3 divides four evenly so we have no craziness with our
    // scanline widths.
    for (y = 16; y --> 0;)  // the elusive --> operator.
    {
	for (x = 0; x < 16; x++)
	{
	    // Simple sub-tile index
	    i = x & 7;
	    i += (y & 7) * 8;

	    // Which tile
	    if (x & 8)
		i += 8 * 8;
	    if (y & 8)
		i += 2 * 8 * 8;

	    index = data[i];
	    convertTo24bit(palette[index], red, green, blue);

	    if (!index)
	    {
		// Colour zero gets hard coded to POWDER brown.
		red = 0xAF;
		green = 0xA2;
		blue = 0x90;
	    }

	    fwrite(&blue, 1, 1, fp);
	    fwrite(&green, 1, 1, fp);
	    fwrite(&red, 1, 1, fp);
	}
    }

    fclose(fp);
}

void
writeDataToBmp8bit(const char *fname, const u8 *data, const u16 *palette)
{
    FILE		*fp;
    int		 i, x, y;
    u8		 red, green, blue, zero = 0;

    fp = fopen(fname, "wb");
    if (!fp)
    {
	printf("Failed to open %s for writing\n", fname);
	return;
    }

    // Write the header.
    fwrite("BM", 2, 1, fp);
    writeportableint(fp, 16*16+1078);	// size
    writeportableint(fp, 0);	// reserved
    writeportableint(fp, 1078);	// Header size + palette
    writeportableint(fp, 40);	// Info size
    writeportableint(fp, 16);	// Width
    writeportableint(fp, 16);	// Height
    writeportableshort(fp, 1);	// Bit planes
    writeportableshort(fp, 8);	// Bits
    writeportableint(fp, 0);	// Compression
    writeportableint(fp, 0);	// image data size
    writeportableint(fp, 0);	// xpel/m
    writeportableint(fp, 0);	// ypel/m
    writeportableint(fp, 0);	// used colours
    writeportableint(fp, 0);	// important colours

    // Write out our palette...
    // Format is BGRA, where A is unused.
    for (i = 0; i < 256; i++)
    {
	convertTo24bit(palette[i], red, green, blue);

	if (!i)
	{
	    // Colour zero gets hard coded to POWDER brown.
	    red = 0xAF;
	    green = 0xA2;
	    blue = 0x90;
	}

	fwrite(&blue, 1, 1, fp);
	fwrite(&green, 1, 1, fp);
	fwrite(&red, 1, 1, fp);
	fwrite(&zero, 1, 1, fp);
    }

    // Write out our data.  Our source data is tilified so we
    // need to invert the operation here
    // 16 divides four evenly so we have no craziness with our
    // scanline widths.
    for (y = 16; y --> 0;)  // the elusive --> operator.
    {
	for (x = 0; x < 16; x++)
	{
	    // Simple sub-tile index
	    i = x & 7;
	    i += (y & 7) * 8;

	    // Which tile
	    if (x & 8)
		i += 8 * 8;
	    if (y & 8)
		i += 2 * 8 * 8;

	    fwrite(&data[i], 1, 1, fp);
	}
    }

    fclose(fp);
}


void
writeTileToBmp(BUF fname, int tile)
{
    // Find address of data...
    const u8		*data;

    data = &glb_tilesets[4].dungeon[tile * 4 * 8 * 8];
    
    writeDataToBmp24bit(fname.buffer(), data, glb_tilesets[4].palette);
}

void
writeSpriteToBmp(BUF fname, int tile)
{
    // Find address of data...
    const u8		*data;

    data = &glb_tilesets[4].sprite[tile * 4 * 8 * 8];
    
    writeDataToBmp24bit(fname.buffer(), data, glb_tilesets[4].spritepalette);
}

// Does this char mark the end of a sentence?
bool
gram_isendsentence(char c)
{
    switch (c)
    {
	case '.':
	case '!':
	case '?':
	    return true;
    }
    return false;
}

BUF
gram_capitalize(BUF buf)
{
    bool		hard = false;
    char		*s;
    bool		docaps = true;	// Start of sentence, caps.
    BUF			result;
    const char		*str = buf.buffer();

    result = buf;
    
    // THis cast to char * is safe as we only assign to s after a harden.
    for (s = (char *) str; *s; s++)
    {
	if (!isspace(*s))
	{
	    // Note we do not eat up caps if we get a non-alpha, ie:
	    // "foo bar" will become "Foo bar" (ignoring the quote)
	    // Note that numerical keys are ignored, so
	    // "+1 mace" becomes "+1 Mace"
	    if (docaps && isalpha(*s))
	    {
		if (islower(*s))
		{
		    if (!hard)
		    {
			result.uniquify();
			s = result.evildata() + (s - str);
			hard = true;
		    }
		    *s = toupper(*s);
		}
		// Eat the caps.
		docaps = false;
	    }

	    // Determine if this is end-sentence, if so, the next
	    // char should be capped.
	    if (gram_isendsentence(*s))
		docaps = true;
	}
    }
    
    return result;
}

void
writeFile(FILE *fplist, const char *rawname, const encyclopedia_entry *entry,
	bool istile, int tile)
{
    BUF		 fname, bmpname, imgname, name;
    FILE	*fp;
    int		 i;

    fname.sprintf("%s.txt", rawname);
    fname.makeFileSafe();
    bmpname.sprintf("%s.bmp", rawname);
    bmpname.makeFileSafe();
    imgname.sprintf("%s", rawname);
    imgname.makeFileSafe();

    // We create bmps, but link to pngs as we expect
    // later people to convert....
    // Rather than harcoding ourselves, our icon has no
    // extesion so it is trivial to add it in the template.
    fp = fopen(fname.buffer(), "wt");

    // Header info
    fprintf(fp, "Icon: %s\n", imgname.buffer());
    name.strcpy(rawname);
    name = gram_capitalize(name);
    fprintf(fp, "Name: %s\n", name.buffer());

    fprintf(fp, "~~~~\n");

    // Contents.  Lines aren't wrapped, but we don't care.
    for (i = 0; i < entry->numlines; i++)
    {
	fprintf(fp, "%s\n", entry->lines[i]);
    }
    // All done!
    fclose(fp);

    // Output the relevant .bmp
    if (istile)
	writeTileToBmp(bmpname, tile);
    else
	writeSpriteToBmp(bmpname, tile);

    fprintf(fplist, "%s\n", fname.buffer());
}

int
main()
{
    MOB_NAMES			 mob;
    SPELL_NAMES			 spell;
    const encyclopedia_entry	*entry;
    FILE			*fplist;
    encyclopedia_entry		 e;

    fplist = fopen("moblist.txt", "wt");
    
    // Bestiary entry....
    e.numlines = 1;
    e.lines = new const char*[1];
    e.lines[0] = "Piecing together the tales of those few that have escaped the dungeon depths, we have assembled this bestiary to help forewarn you of the dangers you face.  Be warned it is incomplete: we've heard tale of many fearsome foes not on this paltry list!";
    writeFile(fplist, "bestiary", &e, true, TILE_VOID);
    
    FOREACH_MOB(mob)
    {
	if (!glb_mobdefs[mob].publish)
	    continue;
	// Write out relevant data...
	entry = encyc_getentry("MOB", mob);
	if (entry)
	{
	    writeFile(fplist, glb_mobdefs[mob].name, entry,
			true, glb_mobdefs[mob].tile);
	}
    }
    fclose(fplist);

    fplist = fopen("spelllist.txt", "wt");

    e.lines[0] = "Wizards jealously guard their secrets, so any attempt to assemble all magic in one location is doomed to be incomplete.  Consider this a mere primer for the wide range of spells that you may encounter on your journey.";
    writeFile(fplist, "spelltome", &e, true, TILE_BLUESPELLBOOK);
    FOREACH_SPELL(spell)
    {
	if (!glb_spelldefs[spell].publish)
	    continue;

	entry = encyc_getentry("SPELL", spell);

	if (entry)
	{
	    writeFile(fplist, glb_spelldefs[spell].name, entry,
			false, glb_spelldefs[spell].tile);
	}
    }
    fclose(fplist);

    // We also want to write out the spellbook icon of choice.
    BUF buf;
    buf.reference("spellbook.bmp");
    writeTileToBmp(buf, TILE_BLUESPELLBOOK);

    return 0;
}
