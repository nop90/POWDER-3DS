// map2c.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <ctype.h>

// Portability crap
#ifndef WIN32
	#define stricmp strcasecmp
#endif

using namespace std;

struct MAP_LUT
{
    char letter;
    char *entry;
    MAP_LUT *next;
};

struct OUT_ENTRY
{
    const char *name;
    int		x, y;
    OUT_ENTRY  *next;
};

MAP_LUT *glbLUTRoot = 0;
char **glbMap = 0;
int glbWidth = 0, glbHeight = 0;

int glb_minlevel = -1, glb_maxlevel = -1;
int glb_rarity = 100;
int glb_mandatory = 0, glb_onlyonce = 0;
int glb_allowstairs = 1;
int glb_allowgeneration = 1;
int glb_allowdig = 1;
int glb_allowitems = 1;
int glb_allowtele = 1;
int glb_x = -1, glb_y = -1;
char *glb_quest = "-1";

void
writeList(ostream &os, const char *prefix, const char *listtype, OUT_ENTRY *list)
{
    OUT_ENTRY		*entry;
    int			 i;

    os << "const PT2 glb_" << prefix << "_" << listtype << "list[] =" << endl;
    os << "{" << endl;
    i = 0;
    for (entry = list; entry; entry = entry->next)
    {
	if (i)
	    os << "," << endl;
	os << "\t{ " << entry->x << ", " << entry->y;
	os << ", " << entry->name << " }";
	i++;
    }
    os << endl;
    os << "};" << endl;
    os << endl;
}

void
readMap(istream &is, const char *firstline)
{
    // Determine width from the line.
    int		i;
    char	line[1024];

    for (i = 1; firstline[i] == '-'; i++);

    if (firstline[i] != '+')
    {
	cerr << "No terminating + on first map line!" << endl;
	return;
    }

    if (glbMap)
    {
	cerr << "Multiple maps defined!" << endl;
	return;
    }

    glbWidth = i - 1;
    glbMap = new char *[100];

    glbHeight = 0;
    while (is.getline(line, 1000))
    {
	if (line[0] == '+')
	{
	    // Hit the end!
	    break;
	}

	if (line[0] != '|')
	{
	    cerr << "Missing opening | in map!" << endl;
	    return;
	}

	if (strlen(line) != glbWidth + 2)
	{
	    cerr << "Line: \"" << line << "\" is wrong length" << endl;
	    return;
	}
	glbMap[glbHeight] = new char [glbWidth+3];
	strcpy(glbMap[glbHeight], &line[1]);
	glbMap[glbHeight][glbWidth] = '\0';
	glbHeight++;
	if (glbHeight >= 100)
	{
	    cerr << "Too many lines in the map!" << endl;
	    return;
	}
    }
}

void
readLUT(char *line)
{
    char	 c;
    MAP_LUT	*lut;
    char	*word;

    c = line[0];

    line += 2;
    word = strtok(line, ", \t\n");
    while (word)
    {
	lut = new MAP_LUT;
	lut->next = glbLUTRoot;
	lut->letter = c;
	lut->entry = strdup(word);
	glbLUTRoot = lut;
	word = strtok(0, ", \t\n");
    }
}

int
check_boolean(char *word)
{
    if (!stricmp(word, "true") || !stricmp(word, "on") ||
	!stricmp(word, "yes"))
	return 1;
    if (!stricmp(word, "false") || !stricmp(word, "off") ||
	!stricmp(word, "no"))
	return 0;

    return atoi(word);
}

void
readVarAssign(char *line)
{
    char		*var;
    char		*val;

    var = strtok(line, " =\t\n");
    val = strtok(0, " =\t\n");
    if (!var || !val)
    {
	cerr << "Illformed var assign ignored." << endl;
    }

    // Test known variables:
    if (!stricmp(var, "minlevel"))
	glb_minlevel = atoi(val);
    else if (!stricmp(var, "maxlevel"))
	glb_maxlevel = atoi(val);
    else if (!stricmp(var, "rarity"))
	glb_rarity = atoi(val);
    else if (!stricmp(var, "mandatory"))
	glb_mandatory = check_boolean(val);
    else if (!stricmp(var, "onlyonce"))
	glb_onlyonce = check_boolean(val);
    else if (!stricmp(var, "allowstairs"))
	glb_allowstairs = check_boolean(val);
    else if (!stricmp(var, "allowgeneration"))
	glb_allowgeneration = check_boolean(val);
    else if (!stricmp(var, "allowdig"))
	glb_allowdig = check_boolean(val);
    else if (!stricmp(var, "allowtele"))
	glb_allowtele = check_boolean(val);
    else if (!stricmp(var, "allowitems"))
	glb_allowitems = check_boolean(val);
    else if (!stricmp(var, "quest"))
	glb_quest = strdup(val);
    else if (!stricmp(var, "pos"))
    {
	glb_x = atoi(val);
	val = strtok(0, " ,=\t\n");
	if (val)
	    glb_y = atoi(val);
	else
	    cerr << "pos command has no y component!" << endl;
    }
    else
    {
	cerr << "Unknown variable " << var << endl;
    }
}

int 
main(int argc, char* argv[])
{
    if (argc < 3)
    {
	cerr << "map2c input.map output.cpp" << endl;
	return -1;
    }

    char	*prefix;
    int		 i;

    ifstream is(argv[1]);

    if (!is)
    {
	cerr << "Could not open to read " << argv[1] << endl;
	return -1;
    }

    char line[1024];
    
    while (is.getline(line, 1000))
    {
	// Skip blank lines.
	if (!line[0] || !line[1])
	    continue;

	// Skip comments.
	if (line[0] == '#' && isspace(line[1]))
	    continue;

	// If we are +-, we are the start of a map.
	if (line[0] == '+' && line[1] == '-')
	{
	    readMap(is, line);
	    continue;
	}

	// If : is second character, it is a definition.
	if (line[1] == ':')
	{
	    readLUT(line);
	    continue;
	}

	if (strchr(line, '='))
	{
	    readVarAssign(line);
	    continue;
	}

	cerr << "Ignoring unknown line:" << endl;
	cerr << "\t\"" << line << "\"" << endl;
    }

    if (!glbLUTRoot)
    {
	cerr << "No look ups found, aborting" << endl;
	return -1;
    }

    if (!glbMap)
    {
	cerr << "No map found, aborting" << endl;
	return -1;
    }

    ofstream os(argv[2]);
    if (!os)
    {
	cerr << "Could not open for write " << argv[2] << endl;
	return -1;
    }

    prefix = strdup(argv[2]);
    if (strchr(prefix, '.'))
	*strchr(prefix, '.') = '\0';

    // Output our lists....
    os << "// Auto-generated .map file" << endl;
    os << "// DO NOT HAND EDIT" << endl;
    os << "// Generated from " << argv[1] << endl;
    os << endl;
    os << "#include \"../map.h\"" << endl;
    os << "#include \"../glbdef.h\"" << endl;
    os << endl;

    // Output out our lookups..
    int	    x, y;
    MAP_LUT *lut;
    bool    foundany;

    OUT_ENTRY *moblist = 0;
    OUT_ENTRY *moblevellist = 0;
    OUT_ENTRY *itemlist = 0;
    OUT_ENTRY *itemtypelist = 0;
    OUT_ENTRY *intrinsiclist = 0;
    OUT_ENTRY *squareflaglist = 0;
    OUT_ENTRY *signpostlist = 0;
    OUT_ENTRY *entry;

    // We must null terminate our lists.
    moblist = new OUT_ENTRY;
    moblist->next = 0;
    moblist->name = "MOB_NONE";
    moblist->x = -1;
    moblist->y = -1;

    itemlist = new OUT_ENTRY;
    itemlist->next = 0;
    itemlist->name = "ITEM_NONE";
    itemlist->x = -1;
    itemlist->y = -1;

    moblevellist = new OUT_ENTRY;
    moblevellist->next = 0;
    moblevellist->name = "MOBLEVEL_NONE";
    moblevellist->x = -1;
    moblevellist->y = -1;

    itemtypelist = new OUT_ENTRY;
    itemtypelist->next = 0;
    itemtypelist->name = "ITEMTYPE_NONE";
    itemtypelist->x = -1;
    itemtypelist->y = -1;

    intrinsiclist = new OUT_ENTRY;
    intrinsiclist->next = 0;
    intrinsiclist->name = "INTRINSIC_NONE";
    intrinsiclist->x = -1;
    intrinsiclist->y = -1;

    squareflaglist = new OUT_ENTRY;
    squareflaglist->next = 0;
    squareflaglist->name = "SQUAREFLAG_NONE";
    squareflaglist->x = -1;
    squareflaglist->y = -1;

    signpostlist = new OUT_ENTRY;
    signpostlist->next = 0;
    signpostlist->name = "SIGNPOST_NONE";
    signpostlist->x = -1;
    signpostlist->y = -1;

    os << "const SQUARE_NAMES glb_" << prefix << "_squarelist[] =" << endl;
    os << "{" << endl;
	
    i = 0;
    for (y = 0; y < glbHeight; y++)
    {
	for (x = 0; x < glbWidth; x++)
	{
	    if (i)
		os << ",\t";
	    if (i && !(i & 3))
		os << endl;

	    foundany = false;
	    for (lut = glbLUTRoot; lut; lut = lut->next)
	    {
		if (lut->letter == glbMap[y][x])
		{
		    // Found a valid legend.
		    foundany = true;
		    
		    if (!strncmp(lut->entry, "MOBLEVEL", 8))
		    {
			entry = new OUT_ENTRY;
			entry->next = moblevellist;
			moblevellist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else if (!strncmp(lut->entry, "MOB", 3))
		    {
			entry = new OUT_ENTRY;
			entry->next = moblist;
			moblist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else if (!strncmp(lut->entry, "SQUAREFLAG", 10))
		    {
			entry = new OUT_ENTRY;
			entry->next = squareflaglist;
			squareflaglist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else if (!strncmp(lut->entry, "INTRINSIC", 9))
		    {
			entry = new OUT_ENTRY;
			entry->next = intrinsiclist;
			intrinsiclist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else if (!strncmp(lut->entry, "SQUARE", 6))
		    {
			os << lut->entry;
			i++;
		    }
		    else if (!strncmp(lut->entry, "ITEMTYPE", 8))
		    {
			entry = new OUT_ENTRY;
			entry->next = itemtypelist;
			itemtypelist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else if (!strncmp(lut->entry, "ITEM", 4))
		    {
			entry = new OUT_ENTRY;
			entry->next = itemlist;
			itemlist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else if (!strncmp(lut->entry, "SIGNPOST", 8))
		    {
			entry = new OUT_ENTRY;
			entry->next = signpostlist;
			signpostlist = entry;
			entry->name = lut->entry;
			entry->x = x;
			entry->y = y;
		    }
		    else
		    {
			cerr << "Unknown square type " << lut->entry << ", ignoring" << endl;
		    }
		}
	    }

	    if (!foundany)
	    {
		fprintf(stderr, "Could not find symbol %c in Legend!\r\n",
				glbMap[y][x]);
	    }
	}
    }
    os << endl;
    os << "};" << endl;

    os << endl;

    // Output our moblist & itemlist.
    writeList(os, prefix, "mob", moblist);
    writeList(os, prefix, "item", itemlist);
    writeList(os, prefix, "itemtype", itemtypelist);
    writeList(os, prefix, "moblevel", moblevellist);
    writeList(os, prefix, "intrinsic", intrinsiclist);
    writeList(os, prefix, "squareflag", squareflaglist);
    writeList(os, prefix, "signpost", signpostlist);

    // Finally, the stuff to link it together.
    os << "const ROOM_DEF glb_" << prefix << "_roomdef =" << endl;
    os << "{" << endl;
    os << "\t{ " << glbWidth << ", " << glbHeight << ", 0 }," << endl;
    os << "\tglb_" << prefix << "_squarelist," << endl;
    os << "\tglb_" << prefix << "_squareflaglist," << endl;
    os << "\tglb_" << prefix << "_itemlist," << endl;
    os << "\tglb_" << prefix << "_moblist," << endl;
    os << "\tglb_" << prefix << "_itemtypelist," << endl;
    os << "\tglb_" << prefix << "_moblevellist," << endl;
    os << "\tglb_" << prefix << "_intrinsiclist," << endl;
    os << "\tglb_" << prefix << "_signpostlist," << endl;

    // Global values:
    os << "\t" << glb_minlevel << ", " << glb_maxlevel << "," << endl;
    os << "\t" << glb_rarity << "," << endl;
    os << "\t" << glb_mandatory << ", " << glb_onlyonce << "," << endl;
    os << "\t" << glb_allowstairs << "," << endl;
    os << "\t" << glb_allowgeneration << "," << endl;
    os << "\t" << glb_allowdig << "," << endl;
    os << "\t" << glb_allowitems << "," << endl;
    os << "\t" << glb_allowtele << "," << endl;
    os << "\t" << glb_x << ", " << glb_y << ", " << endl;
    os << "\t" << glb_quest << ", " << endl;
    // Insert new globals here so you get the comma proper!

    os << "\t" << "\"" << prefix << "\"" << endl;
    
    os << "};" << endl;

    return 0;
}

