/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        hiscore.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Manages high score table
 */

#include <stdio.h>
#include "hiscore.h"
#include "sramstream.h"
#include "creature.h"
#include "gfxengine.h"
#include "map.h"
#include "victory.h"
#include "msg.h"

#define MAX_SCORES 15

#define FLAG_WON 2
#define FLAG_HAVEHEART 1
#define FLAG_NOONE 4

// #pragma pack(push, 1)

int glbVersion = 117;
int glbSaveCount = 0;
bool glbIsNewGame = false;
bool glbRNGValid = false;
int glbRNGSeed = 0;

extern u8 *getGlobalActionStrip();
extern void saveOptions(SRAMSTREAM &is);
extern void loadOptions(SRAMSTREAM &os);

void 
saveActionStrip(SRAMSTREAM &os)
{
// Save the memory on the GBA...
#ifdef HAS_STYLUS
    u8 *astrip = getGlobalActionStrip();

    os.writeRaw((char *) astrip, 50);
#endif
}

void 
loadActionStrip(SRAMSTREAM &is)
{
// Save the memory on the GBA...
#ifdef HAS_STYLUS
    u8 *astrip = getGlobalActionStrip();

    is.readRaw((char *) astrip, 50);
#endif
}

struct HISCORE_ENTRY
{
    // score of this character
    u16  score;
    // Number of turns taken.
    u16	 turns;
    // Up to 8 chars in name.
    char name[8];
    // Max depth, death depth
    u8   maxdlevel, deathdlevel;
    // Magic die & Hit die
    s16	 mp, hp;
    u8   mpdie, hitdie;
    // What version
    u8	 version;
    // Flags for winning, etc.
    u8	 deathflag;
};

// #pragma pack(pop)

HISCORE_ENTRY glbScoreList[MAX_SCORES];

int
hiscore_savecount()
{
    return glbSaveCount;
}

void
hiscore_setsavecount(int newval)
{
    glbSaveCount = newval;
}

void
hiscore_bumpsavecount()
{
    if (hiscore_savecount() < 100)
	hiscore_setsavecount(hiscore_savecount()+1);
}

void
hiscore_flagnewgame(bool isnewgame)
{
    glbIsNewGame = isnewgame;
}

bool
hiscore_isnewgame()
{
    return glbIsNewGame;
}

bool
hiscore_advancedmode()
{
    if (glbScoreList[MAX_SCORES-1].turns == 0)
	return false;
    else
	return true;
}

void
hiscore_clearEntry(HISCORE_ENTRY *entry)
{
    strcpy(entry->name, "No One");
    entry->maxdlevel = entry->deathdlevel = 0;
    entry->mp = entry->hp = 0;
    entry->mpdie = entry->hitdie = 0;
    entry->deathflag = FLAG_NOONE;
    entry->score = 0;
    entry->turns = 0;
    entry->version = glbVersion;
}

void
hiscore_saveEntry(SRAMSTREAM &os, HISCORE_ENTRY *entry)
{
    os.writeRaw((char *)entry, sizeof(HISCORE_ENTRY));
}

void
hiscore_loadEntry(SRAMSTREAM &is, HISCORE_ENTRY *entry)
{
    is.readRaw((char *)entry, sizeof(HISCORE_ENTRY));
}

void
hiscore_formatScore(char *line1, char *line2, 
		    HISCORE_ENTRY *entry, bool highlight)
{
    char		name[9];
    const char		*state;
    int			i;

    // Null terminate name.
    memcpy(name, entry->name, 8);
    name[8] = 0;
    // Add spaces to name.
    for (i = 0; i < 8; i++)
	if (!name[i])
	    break;
    for (; i < 8; i++)
	name[i] = ' ';

    // Format:
    // 123456789012345678901234567890
    // +Brask   Hhit/hdMmag/md Ldd/md 
    //    Heart $65536 t:83323 v061
    sprintf(line1, "%c%s%c%03d/%02d%c%03d/%02d %c%02d/%02d",
	    (highlight ? '+' : ' '),
	    name,
	    SYMBOL_HEART,
	    entry->hp, entry->hitdie,
	    SYMBOL_MAGIC,
	    entry->mp, entry->mpdie,
	    SYMBOL_DLEVEL,
	    entry->deathdlevel, entry->maxdlevel);

    if (entry->deathflag & FLAG_WON)
	state = "Won!";
    else if (entry->deathflag & FLAG_HAVEHEART)
	state = "Heart";
    else
	state = "Dead";
    sprintf(line2, "    %s $%d t:%d v%03d", state, entry->score, entry->turns, entry->version);
}

void
hiscore_init()
{
    SRAMSTREAM		is;
    // Load the hiscore, which triggers the appropriate rebuilding
    hiscore_load(is);
}

int
hiscore_getversion()
{
    return glbVersion;
}

void
hiscore_load(SRAMSTREAM &is)
{
    char		prefix[6];
    bool		clearsave = false, keepscore = true;
    int			i;
    u8			version;
    u8			data;
    
    is.readRaw(prefix, 6);
    if (memcmp(prefix, "POWDER", 6))
    {
	// This is an invalid memory chunk, rebuild!
	clearsave = true;
	keepscore = false;
	for (i = 0; i < MAX_SCORES; i++)
	{
	    hiscore_clearEntry(&glbScoreList[i]);
	}
    }
    else
    {
	// Check the version number.
	is.readRaw((char *) &version, 1);
	if (version != hiscore_getversion())
	{
	    // Invalid version, hiscore is cool, but wipe save.
	    clearsave = true;
	}
    }

    // Read save count.
    is.readRaw((char *) &data, 1);
    if (clearsave)
    {
	// Consider it invalid.
	glbSaveCount = 0;
    }
    else
    {
	glbSaveCount = data;
    }

    // Read the score entries.
    HISCORE_ENTRY		blank;
    for (i = 0; i < MAX_SCORES; i++)
    {
	if (keepscore)
	    hiscore_loadEntry(is, &glbScoreList[i]);
	else
	    hiscore_loadEntry(is, &blank);
    }

    // Read random seed and action bars settings.
    // These are reset with every new version since we don't want
    // to guarantee that ACTION_NAMEs or SPELL_NAMEs are consistent.
    glbRNGValid = false;
    if (!clearsave)
    {
	glbRNGValid = true;
	is.readRaw((char *) &glbRNGSeed, sizeof(glbRNGSeed));

	loadActionStrip(is);
	loadOptions(is);
    }

    // If the memory was entirely corrupt, rewrite with the valid stuff.
    if (!keepscore)
    {
	SRAMSTREAM	os;
	
	hiscore_save(os);
	hamfake_endWritingSession();
    }
}

void
hiscore_save(SRAMSTREAM &os)
{
    int			i;
    u8			data;

    os.writeRaw("POWDER", 6);
    data = hiscore_getversion();
    os.writeRaw((char *) &data, 1);

    data = glbSaveCount;
    os.writeRaw((char *) &data, 1);
    
    for (i = 0; i < MAX_SCORES; i++)
    {
	hiscore_saveEntry(os, &glbScoreList[i]);
    }
    // Write out the current RNG value.
    glbRNGSeed = rand_getseed();
    os.writeRaw((const char *) &glbRNGSeed, sizeof(glbRNGSeed));

    saveActionStrip(os);
    saveOptions(os);
}

void
hiscore_display(HISCORE_ENTRY *bonus, bool ischeater)
{
    int			 i;
    char		 line1[100], line2[100];
    bool		 bonuswon, curwon, hadbonus;
    int			 place = -1;

    hadbonus = false;
    if (bonus)
	hadbonus = true;
    bonuswon = false;
    if (bonus && (bonus->deathflag & FLAG_WON))
	bonuswon = true;

    for (i = 0; i < MAX_SCORES; i++)
    {
	// Check to see if we insert bonus.
	curwon = (glbScoreList[i].deathflag & FLAG_WON) ? true : false;

	// We only compare winners with other winners.  A winner always
	// scores higher than a loser.
	if ((!ischeater && bonus) && 
		((bonus->score >= glbScoreList[i].score && (curwon == bonuswon))
		 || (bonuswon && !curwon)
		) 
	   )
	{
	    hiscore_formatScore(line1, line2, bonus, true);
	    gfx_pager_addsingleline(line1);
	    gfx_pager_addsingleline(line2);
	    bonus = 0;
	    place = i;
	}

	// Ignore empty entries.
	if (glbScoreList[i].deathflag & FLAG_NOONE)
	    continue;

	hiscore_formatScore(line1, line2, &glbScoreList[i], false);
	gfx_pager_addsingleline(line1);
	gfx_pager_addsingleline(line2);
    }

    // If still no bonus, put at end.
    if (bonus)
    {
	gfx_pager_addsingleline("...                        ...");
	hiscore_formatScore(line1, line2, bonus, true);
	gfx_pager_addsingleline(line1);
	gfx_pager_addsingleline(line2);
    }

    // Inform user how they placed:
    if (hadbonus)
    {
	BUF	buf;

	if (place >= 0)
	{
	    BUF		placename = gram_createplace(place+1);
	    buf.sprintf("You came in %s place!  ", placename.buffer());
	}
	else if (ischeater)
	{
	    buf.reference("Your save scumming disqualified you from the list.  ");
	}
	else
	{
	    buf.reference("You did not make the high score list.  ");
	}

	msg_report(buf);
    }

    int		center;
    
    if (place < 0)
	center = MAX_SCORES * 2;
    else
	center = place * 2;

    if (!hadbonus)
	center = 0;

    gfx_pager_display(center);
}

bool
hiscore_addAndDisplayEntry(MOB *mob, bool haswon, bool ischeater)
{
    int		score;
    int		i, j;
    bool	hasheart;
    HISCORE_ENTRY	newentry;

    hiscore_clearEntry(&newentry);

    score = mob->calcScore(haswon);

    hasheart = mob->hasItem(ITEM_BLACKHEART);

    newentry.score = score;
    if (speed_gettime() >= 65535)
	newentry.turns = 65535;
    else
	newentry.turns = speed_gettime();

    // Compute the current and final depths.
    MAP		*map, *lastmap;
    int		 maxdepth, depth;

    depth = glbCurLevel->getDepth();
    lastmap = glbCurLevel;
    for (map = glbCurLevel; map; map = map->getMapDown(false))
    {
	lastmap = map;
    }
    
    maxdepth = lastmap->getDepth();
    // copy avatar name.
    memcpy(newentry.name, glbAvatarName, 8);

    newentry.deathdlevel = depth;
    newentry.maxdlevel = maxdepth;

    newentry.mp = mob->getMP();
    newentry.hp = mob->getHP();

    newentry.mpdie = mob->getMagicDie();
    newentry.hitdie = mob->getHitDie();

    newentry.version = hiscore_getversion();
    newentry.deathflag = 0;
    if (haswon)
	newentry.deathflag |= FLAG_WON;
    if (hasheart)
	newentry.deathflag |= FLAG_HAVEHEART;

    hiscore_display(&newentry, ischeater);

    // Add in the new score.
    for (i = 0; i < MAX_SCORES; i++)
    {
	bool		curwon;

	// Don't add cheaters
	if (ischeater)
	    break;

	curwon = (glbScoreList[i].deathflag & FLAG_WON) ? true : false;
	// We only compare winners with other winners.  A winner always
	// scores higher than a loser.
	if ( (newentry.score >= glbScoreList[i].score && (curwon == haswon))
		|| (haswon && !curwon)
	   )
	{
	    // Replace this entry.
	    for (j = MAX_SCORES-1; j > i; j--)
	    {
		memcpy(&glbScoreList[j], &glbScoreList[j-1],
			sizeof(HISCORE_ENTRY));
	    }

	    memcpy(&glbScoreList[i], &newentry,
		    sizeof(HISCORE_ENTRY));

	    SRAMSTREAM		os;
	    hiscore_save(os);

	    hamfake_endWritingSession();

	    return true;
	}
    }
    
    // Even if no highscore achieved, still resave the high score
    // table so options are saved.
    
    SRAMSTREAM		os;
    hiscore_save(os);

    hamfake_endWritingSession();

    return false;
}
