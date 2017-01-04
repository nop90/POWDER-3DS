/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        main.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	Good old main.
 *
 *	This is where the main game loop resides.  It is also where
 *	a lot of the UI interaction has ended up.  Code here, almost
 *	by definition, should be refactored somewhere else.
 */

#include "mygba.h"
#include <stdio.h>
#include <ctype.h>

#include "gfxengine.h"
#include "gfx/all_bitmaps.h"
#include "artifact.h"
#include "encyc_support.h"
#include "control.h"
#include "glbdef.h"
#include "creature.h"
#include "map.h"
#include "assert.h"
#include "msg.h"
#include "item.h"
#include "itemstack.h"
#include "input.h"
#include "sramstream.h"
#include "victory.h"
#include "mobref.h"
#include "speed.h"
#include "piety.h"
#include "hiscore.h"
#include "stylus.h"
#include "rooms/allrooms.h"

#ifdef iPOWDER
#include "thread.h"
#endif

#if defined(USING_SDL) || defined(USING_DS) || defined(_3DS)
#include <time.h>
#endif

// Defines whether we are in map testing mode.
//#define MAPTEST

// MULTIBOOT

// The following comment is preserved to provide sufficinet irony
// to avoid the danger of code anemia.
/*
----------------------------------
This is a simple mapping application...
----------------------------------
*/

#ifdef USING_SDL
#define TILEWIDTH (gfx_gettilewidth())
#define TILEHEIGHT (gfx_gettileheight())
#else
#define TILEWIDTH 8
#define TILEHEIGHT 8
#endif

//
// No one likes autoprompt.  Boohoo!
// But I have used it for a year now, so screw the user!
// In the end, it turned out to be a keyboard centric issue.  Those
// lacking keyboards love autoprompt, those with hate.  Thankfully
// this is a compile time issue.
//
#ifdef HAS_KEYBOARD
bool		glbAutoPrompt = false;
#else
bool		glbAutoPrompt = true;
#endif
bool		glbSuppressAutoClimb = false;

// Do we allow the user to tap on the screen to move?
bool		glbScreenTap = true;

#ifdef iPOWDER
bool		glbActionBar = false;
#else
bool		glbActionBar = true;
#endif
bool		glbColouredFont = true;
bool		glbFinishedASave = false;
bool		glbSafeWalk = false;
bool		glbToggleSafeWalk = false;
bool		glbSwap = false;
bool		glbToggleSwap = false;
bool		glbJump = false;
bool		glbToggleJump = false;
int		glbAutoRunDX = 0;
int		glbAutoRunDY = 0;
bool		glbAutoRunEnabled = false;
bool		glbAutoRunOpenSpace = false;
bool		glbHasJustSearched = false;
int		glbSearchCount = 0;

extern int glbMapCount;
extern int glbMobCount;
extern int glbItemCount;

ACTION_NAMES	glb_actionbuttons[NUM_BUTTONS];

#ifdef iPOWDER
// Note the default of 0 actually is rather sensible here
int glb_spellgreylist[NUM_SPELLS];
#endif

#define STRIPLENGTH 50

// Specifies the default.
const ACTION_NAMES	glb_globaldefaultactionstrip[STRIPLENGTH] =
{
    ACTION_HELP,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_OPTIONS,
    // Bottom row.
    ACTION_LOOK,
    ACTION_PICKUP,
    ACTION_CLIMB,
    ACTION_ZAP,
    ACTION_FIRE,
    ACTION_EAT,
    ACTION_CLOSE,
    ACTION_PRAY,
    ACTION_SWAP,
    ACTION_NAME,
    ACTION_MINIMAP,
    ACTION_JUMP,
    ACTION_COMMAND,
    ACTION_INVENTORY,
    ACTION_VERBLIST,
    // Left
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    // Right
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
};


u8 glb_globalactionstrip[STRIPLENGTH];

u8 *
getGlobalActionStrip()
{
    return glb_globalactionstrip;
}

// This is not remappable so is fixed.
const ACTION_NAMES	glb_inventoryactionstrip[STRIPLENGTH] =
{
    // Top
    ACTION_FAVORITE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    // Bottom
    ACTION_EQUIP,
    ACTION_DEQUIP,
    ACTION_DIP,
    ACTION_QUAFF,
    ACTION_READ,
    ACTION_ZAP,
    ACTION_EAT,
    ACTION_DROP,
    ACTION_EXAMINE,
    ACTION_QUIVER,
    ACTION_THROW,
    ACTION_SPLITSTACK,
    ACTION_NAME,
    ACTION_INVENTORY,
    ACTION_SORT,
    // Left
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    // Right
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
    ACTION_NONE,
};

void
attemptunlock()
{
#ifdef iPOWDER
    gfx_pager_addtext(
	    "POWDER is not like many similar looking games.  It "
	    "is entirely reasonable for you to spend the next "
	    "year actively playing it and still not reach "
	    "dungeon level 15."
	    );
    gfx_pager_newline(); gfx_pager_newline();
    gfx_pager_addtext(
	    "This is not because of an arduous grind!  You could win a game "
	    "of POWDER in three hours of play time.  It is because "
	    "POWDER is hard, unforgiving, and a heck of a lot of fun."
	    );
    gfx_pager_newline(); gfx_pager_newline();
    gfx_pager_addtext(
	    "To explore deeper than dungeon level 15, however, you "
	    "must unlock POWDER.  To help you reach those levels, as an "
	    "added bonus, when unlocked, you gain access to the "
	    "powerful Wish command that allows you to summon any item or "
	    "creature.  Finally, unlocking will enable the stress "
	    "test."
	    );
    gfx_pager_newline(); 
    gfx_pager_display();

    if (!hamfake_allowunlocks())
    {
	gfx_pager_addtext(
	    "You have disabled in-app purchases.  Please re-enable "
	    "and try again."
	    );
	gfx_pager_newline();
	gfx_pager_display();

	return;
    }

    if (!gfx_yesnomenu("Unlock POWDER now?"))
    {
	return;
    }

    hamfake_buttonreq(6);

    // This may take a while, let the user know it is asynchronous.
    gfx_pager_addtext(
	    "Accessing the store..."
	    );
    gfx_pager_newline(); gfx_pager_newline();
    gfx_pager_addtext(
	    "This may take a while.  System pop-ups will take you through "
	    "the rest of the process.  In the meantime, we return you to "
	    "your regularly scheduled game."
	    );
    gfx_pager_newline(); 
    gfx_pager_display();
#endif
}

ACTION_NAMES
getInventoryActionStrip(int button)
{
    return glb_inventoryactionstrip[button];
}

void
resetDefaultActionStrip()
{
    int		i;

    // Initialize the button strip.
    for (i = 0; i < STRIPLENGTH; i++)
    {
	glb_globalactionstrip[i] = action_packStripButton(glb_globaldefaultactionstrip[i]);
    }
}


const BUTTONS	glb_mapbuttons[] =
{
    BUTTON_A,
    BUTTON_B,
    BUTTON_R,
    BUTTON_L,
#ifdef HAS_XYBUTTON
    BUTTON_X,
    BUTTON_Y,
#endif
    NUM_BUTTONS
};

char
getActionChar(ACTION_NAMES action)
{
    return glb_actiondefs[action].hint[0];
}

void
hideSideActionBar()
{
#ifdef HAS_STYLUS
    int		x, sx, sy;
    if (glbActionBar)
    {
	for (x = 0; x < STRIPLENGTH; x++)
	{
	    action_indexToOverlayPos(x, sx, sy);

	    if (sx == -1 || sx == 29)
	    {
		hamfake_enablesprite(x+1, false);
	    }
	}
    }
#endif
}

bool 
getMask(u8 val, int mask)
{
    if (val & mask)
	return true;
    return false;
}

u8
setMask(bool state, int mask)
{
    if (state)
	return mask;
    return 0;
}

void
loadOptions(SRAMSTREAM &is)
{
    u8		val;
    int		i;

    // Load our action key mappings.
    for (i = 0; glb_mapbuttons[i] != NUM_BUTTONS; i++)
    {
	is.readRaw((char *) &val, 1);
	glb_actionbuttons[glb_mapbuttons[i]] = 
				    (ACTION_NAMES) val;
    }

    is.readRaw((char *) &val, 1);
	
    glbAutoPrompt = getMask(val, 1);
    glbActionBar = getMask(val, 2);
    glbOpaqueTiles = getMask(val, 4);
    glbColouredFont = getMask(val, 8);
    glbGender = getMask(val, 16);
    glbSafeWalk = getMask(val, 32);
    // We save the opposite sense so people get the expected for
    // versions that didn't save it.
    glbScreenTap = !getMask(val, 64);

    is.readRaw((char *) &val, 1);
    int		pset, sset;
    pset = val & 0xf;
    sset = val >> 4;
    // Verify the tileset is legal.
    if (pset >= NUM_TILESETS - !hamfake_extratileset())
	pset = 0;
    if (sset >= NUM_TILESETS - !hamfake_extratileset())
	sset = 0;
    gfx_settilesetmode(0, pset);
    gfx_settilesetmode(1, sset);

    // Load our current font
    is.readRaw((char *) &val, 1);
    if (val >= NUM_FONTS)
	val = 0;
    gfx_switchfonts(val);

    is.readRaw(glbAvatarName, 23);
    glbAvatarName[23] = 0;
}

void
saveOptions(SRAMSTREAM &os)
{
    u8 		val;
    int		i;

    // Save our current action key mappings.
    for (i = 0; glb_mapbuttons[i] != NUM_BUTTONS; i++)
    {
	val = glb_actionbuttons[glb_mapbuttons[i]];
	os.writeRaw((const char *) &val, 1);
    }

    val = 0;

    val |= setMask(glbAutoPrompt, 1);
    val |= setMask(glbActionBar, 2);
    val |= setMask(glbOpaqueTiles, 4);
    val |= setMask(glbColouredFont, 8);
    val |= setMask(glbGender, 16);
    val |= setMask(glbSafeWalk, 32);
    val |= setMask(!glbScreenTap, 64);
    glbSuppressAutoClimb = false;

    os.writeRaw((const char *) &val, 1);

    // Save our current tileset
    val = gfx_gettilesetmode(0) | (gfx_gettilesetmode(1) << 4);
    os.writeRaw((const char *) &val, 1);
    val = gfx_getfont();
    os.writeRaw((const char *) &val, 1);

    // Not necessarily null terminated!
    os.writeRaw(glbAvatarName, 23);
}

bool
makeAWish(MOB *avatar)
{
    bool		didsomething = false;

    const char *topmenu[] =
    {
	"Item by Type",
	"Magic Item",
	"Monster",
	"Spell",
	"Skill",
	"Experience",
	"Befriend Gods",
	"Level Teleport",
	"Create Room",
	"Shape Shift",
	0
    };

    static int		topchoice = 0;
    int			select, aorb;

    select = gfx_selectmenu(5, 3, topmenu, aorb, topchoice);
    if (select < 0 || aorb)
    {
	for (int y = 3; y < 19; y++)
	    gfx_cleartextline(y);
	return false;
    }
    topchoice = select;
	

    switch (topchoice)
    {
	case 0: // Item by type...
	{
	    const char *typelist[NUM_ITEMTYPES+1];
	    ITEMTYPE_NAMES	itype;
	    static int 		lasttype = 0;
	    
	    FOREACH_ITEMTYPE(itype)
	    {
		typelist[itype] = glb_itemtypedefs[itype].name;
	    }
	    typelist[NUM_ITEMTYPES] = 0;
	    select = gfx_selectmenu(5, 3, typelist, aorb, lasttype);
	    if (select < 0 || aorb)
		break;
	    itype = (ITEMTYPE_NAMES) select;
	    lasttype = select;
	    
	    // Build our list of matching items.
	    const char *itemlist[NUM_ITEMS+1];
	    ITEM_NAMES  itemname[NUM_ITEMS+1], item;
	    int		i = 0;
	    
	    FOREACH_ITEM(item)
	    {
		if (itype == ITEMTYPE_ANY 
		    || itype == ITEMTYPE_ARTIFACT 
		    || glb_itemdefs[item].itemtype == itype)
		{
		    itemlist[i] = glb_itemdefs[item].name;
		    itemname[i] = item;
		    i++;
		}
	    }
	    itemlist[i] = 0;

	    // Empty list means no selection possible
	    if (!i)
		break;

	    static int lastitemchoice = 0;
	    select = gfx_selectmenu(5, 3, itemlist, aorb, lastitemchoice);
	    if (select < 0 || aorb)
		break;
	    lastitemchoice = select;

	    {
		// Acquire the item...
		ITEM *wish;
		wish = ITEM::create((ITEM_NAMES) itemname[select]);
		if (itype == ITEMTYPE_ARTIFACT)
		{
		    wish->makeArtifact();
		}
		// Some items need built in charges
		if (itemname[select] == ITEM_BONES ||
		    itemname[select] == ITEM_CORPSE)
		{
		    wish->addCharges(20);
		}
		avatar->formatAndReport("%U <wish> for %IU.", wish);
		if (!avatar->acquireItem(wish))
		{
		    avatar->formatAndReport("%IU <I:fall> at %r feet.", wish);
		    glbCurLevel->acquireItem(wish, avatar->getX(), avatar->getY(), avatar);
		}
	    }
	    
	    didsomething = true;
	    break;
	}

	case 1:	// Magic Item
	{
	    const char *typelist[NUM_MAGICTYPES+1];
	    MAGICTYPE_NAMES	mtype;
	    static int 		lasttype = 0;

	    FOREACH_MAGICTYPE(mtype)
	    {
		typelist[mtype] = glb_magictypedefs[mtype].name;
	    }
	    typelist[NUM_MAGICTYPES] = 0;
	    select = gfx_selectmenu(5, 3, typelist, aorb, lasttype);
	    if (select < 0 || aorb)
		break;
	    mtype = (MAGICTYPE_NAMES) select;
	    lasttype = select;

	    int numtype[NUM_MAGICTYPES] =
	    {
		0,
		NUM_POTIONS,
		NUM_SCROLLS,
		NUM_RINGS,
		NUM_HELMS,
		NUM_WANDS,
		NUM_AMULETS,
		NUM_SPELLBOOKS,
		NUM_BOOTSS,
		NUM_STAFFS
	    };

	    const char *itemlist[NUM_ITEMS+1];
	    int		i = 0;
	    for (i = 0; i < numtype[mtype]; i++)
	    {
		switch (mtype)
		{
		    case MAGICTYPE_NONE:
		    case NUM_MAGICTYPES:
			UT_ASSERT(!"Illegal magic type");
			itemlist[i] = "illegal";
			break;
		    case MAGICTYPE_POTION:
			itemlist[i] = glb_potiondefs[i].name;
			break;
		    case MAGICTYPE_SCROLL:
			itemlist[i] = glb_scrolldefs[i].name;
			break;
		    case MAGICTYPE_RING:
			itemlist[i] = glb_ringdefs[i].name;
			break;
		    case MAGICTYPE_HELM:
			itemlist[i] = glb_helmdefs[i].name;
			break;
		    case MAGICTYPE_WAND:
			itemlist[i] = glb_wanddefs[i].name;
			break;
		    case MAGICTYPE_AMULET:
			itemlist[i] = glb_amuletdefs[i].name;
			break;
		    case MAGICTYPE_SPELLBOOK:
			itemlist[i] = glb_spellbookdefs[i].name;
			break;
		    case MAGICTYPE_BOOTS:
			itemlist[i] = glb_bootsdefs[i].name;
			break;
		    case MAGICTYPE_STAFF:
			itemlist[i] = glb_staffdefs[i].name;
			break;
		}
	    }
	    itemlist[i] = 0;

	    // Nothing in the list, nothing to do.
	    if (!i)
		break;
	    
	    static int lastitemchoice = 0;
	    select = gfx_selectmenu(5, 3, itemlist, aorb, lastitemchoice);
	    if (select < 0 || aorb)
		break;
	    lastitemchoice = select;

	    {
		// Acquire the item...
		ITEM *wish;
		wish = ITEM::createMagicItem(mtype, select);

		avatar->formatAndReport("%U <wish> for %IU.", wish);
		if (!avatar->acquireItem(wish))
		{
		    avatar->formatAndReport("%IU <I:fall> at %r feet.", wish);
		    glbCurLevel->acquireItem(wish, avatar->getX(), avatar->getY(), avatar);
		}
	    }
	    
	    didsomething = true;
	    break;
	}

	case 2:		// Monster...
	{
	    const char *moblist[NUM_MOBS+1];
	    MOB_NAMES	mobname;
	    static int	lastmob = 0;
	    int		tx, ty;

	    FOREACH_MOB(mobname)
	    {
		moblist[mobname] = glb_mobdefs[mobname].name;
	    }
	    moblist[NUM_MOBS] = 0;

	    select = gfx_selectmenu(5, 3, moblist, aorb, lastmob);
	    if (select < 0 || aorb)
		break;
	    mobname = (MOB_NAMES) select;
	    lastmob = select;

	    for (int y = 3; y < 19; y++)
		gfx_cleartextline(y);
    
	    // Ask for creation location.
	    avatar->formatAndReport("Create where?");
	    tx = avatar->getX();
	    ty = avatar->getY();
	    if (!gfx_selecttile(tx, ty))
		break;

	    MOB *mob = MOB::create(mobname);

	    mob->move(tx, ty);
	    glbCurLevel->registerMob(mob);

	    avatar->formatAndReport("%U <wish> for %MU.", mob);

	    didsomething = true;
	    break;
	}

	case 3:		// Spell...
	{
	    const char *spelllist[NUM_SPELLS+1];
	    SPELL_NAMES	spellname;
	    static int	lastspell = 0;

	    FOREACH_SPELL(spellname)
	    {
		spelllist[spellname] = glb_spelldefs[spellname].name;
	    }
	    spelllist[NUM_SPELLS] = 0;

	    select = gfx_selectmenu(5, 3, spelllist, aorb, lastspell);
	    if (select < 0 || aorb)
		break;
	    spellname = (SPELL_NAMES) select;
	    lastspell = select;

	    for (int y = 3; y < 19; y++)
		gfx_cleartextline(y);

	    avatar->formatAndReport("%U <wish> for %B1.", spelllist[spellname]);

	    if (!avatar->canCastSpell(spellname))
	    {
		// Make sure this is a "free" spell and we don't run
		// out of spell points..
		avatar->incrementMagicDie(1);
		// Likewise, verify we have sufficient mana for it.
		avatar->incrementMaxMP(glb_spelldefs[spellname].mpcost);
		avatar->learnSpell(spellname);
		didsomething = true;
	    }
	    else
	    {
		avatar->formatAndReport("%U already know %B1.", spelllist[spellname]);
	    }
	    break;
	}
	case 4:		// Skill...
	{
	    const char *skilllist[NUM_SKILLS+1];
	    SKILL_NAMES	skillname;
	    static int	lastskill = 0;

	    FOREACH_SKILL(skillname)
	    {
		skilllist[skillname] = glb_skilldefs[skillname].name;
	    }
	    skilllist[NUM_SKILLS] = 0;

	    select = gfx_selectmenu(5, 3, skilllist, aorb, lastskill);
	    if (select < 0 || aorb)
		break;
	    skillname = (SKILL_NAMES) select;
	    lastskill = select;

	    for (int y = 3; y < 19; y++)
		gfx_cleartextline(y);

	    avatar->formatAndReport("%U <wish> for %B1.", skilllist[skillname]);

	    if (!avatar->hasSkill(skillname))
	    {
		avatar->incrementHitDie(1);
		avatar->learnSkill(skillname);
		didsomething = true;
	    }
	    else
	    {
		avatar->formatAndReport("%U already know %B1.", skilllist[skillname]);
	    }
	    break;
	}
	case 5:		// Experience
	{
	    const char *xpmenu[] =
	    {
		"250 XP",
		"1 Level",
		"10 Levels",
		"Ascension Kit",
		0
	    };
	    int		xpval[] =
	    {
		250,
		1000,
		10000,
		-1
	    };

	    select = gfx_selectmenu(5, 3, xpmenu, aorb);
	    if (select < 0 || aorb)
		break;

	    // GAining exp triggers level up warnings
	    for (int y = 3; y < 19; y++)
		gfx_cleartextline(y);
	    
	    avatar->formatAndReport("You gain experience...");

	    if (xpval[select] < 0)
		avatar->buildAscensionKit();
	    else
		avatar->receiveExp(xpval[select]);
	    
	    didsomething = true;
	    break;
	}

	case 6:		// Befriend gods
	{
	    const char *godlist[NUM_GODS+1];
	    GOD_NAMES	godname;

	    FOREACH_GOD(godname)
	    {
		godlist[godname] = glb_goddefs[godname].name;
	    }
	    godlist[NUM_GODS] = 0;
	    select = gfx_selectmenu(5, 3, godlist, aorb);
	    if (select < 0 || aorb)
		break;
	    godname = (GOD_NAMES) select;

	    avatar->formatAndReport("%U <win> the favour of %B1.", godlist[godname]);
	    avatar->pietyGrant(godname, 100);

	    didsomething = true;
	    break;
	}

	case 7:		// Level teleport
	{
	    char buf[50];
	    int	 lvl;

	    {
		for (int y = 3; y < 19; y++)
		    gfx_cleartextline(y);
	    }
	    gfx_printtext(0, 4, "Level? ");
	    input_getline(buf, 3, 7, 4, 23, 8);
	    {
		int		y;
		for (y = 0; y < 20; y++)
		    gfx_cleartextline(y);
	    }

	    if (!isdigit(buf[0]))
		break;

	    // It is suspected that there exist platforms where
	    // atoi does not work.  Sigh.
	    // What is more depressing that this has taken me
	    // three tries to get right.
	    lvl = 0;
	    int		off = 0;
	    while (isdigit(buf[off]))
	    {
		lvl *= 10;
		lvl += buf[off] - '0';
		off++;
	    }

	    // Our depth is a s8 so can't support greater than 127,
	    // we just cap at 100 (anyways, the game proper stops at 25
	    // so not much point for the higher options)
	    // This avoids infinite loops if someone asks for a really deep
	    // level.
	    if (lvl > 100)
		lvl = 100;

	    // Move to level lvl!
	    MAP		*newlevel;
	    glbCurLevel->unregisterMob(avatar);
	    newlevel = glbCurLevel;
	    while (newlevel && newlevel->getDepth() != lvl)
	    {
		if (lvl < newlevel->getDepth())
		    newlevel = newlevel->getMapUp();
		else
		    newlevel = newlevel->getMapDown();
	    }

	    if (newlevel)
	    {
		MAP::changeCurrentLevel(newlevel);
	    }

	    int		x, y;
	    if (!glbCurLevel->findTile(SQUARE_LADDERUP, x, y))
	    {
		if (!glbCurLevel->findTile(SQUARE_LADDERDOWN, x, y))
		{
		    if (!glbCurLevel->findTile(SQUARE_DIMDOOR, x, y))
		    {
			// Default paranoid position.
			x = y = 15;
		    }
		}
	    }
	    // Find a tile where we don't sit on an existing mob.
	    // If walk/swim fail, just go on top of existing mob.
	    if (!glbCurLevel->findCloseTile(x, y, MOVE_WALK))
		glbCurLevel->findCloseTile(x, y, MOVE_WALK);

	    avatar->move(x, y, true);
	    glbCurLevel->registerMob(avatar);
	    didsomething = true;

	    break;
	}

	case 8:		// Create a room
	{
	    const char *roomlist[NUM_ALLROOMDEFS+1];
	    int		rnum;
	    int		tx, ty;
	    static int	lastroom = 0;

	    for (rnum = 0; rnum < NUM_ALLROOMDEFS; rnum++)
	    {
		roomlist[rnum] = glb_allroomdefs[rnum]->name;
	    }
	    roomlist[rnum] = 0;

	    select = gfx_selectmenu(5, 3, roomlist, aorb, lastroom);
	    if (select < 0 || aorb)
		break;

	    lastroom = select;
	    rnum = select;
	    
	    for (int y = 3; y < 19; y++)
		gfx_cleartextline(y);

	    avatar->formatAndReport("Create where?");
	    tx = avatar->getX();
	    ty = avatar->getY();
	    if (!gfx_selecttile(tx, ty))
		break;

	    avatar->formatAndReport("%U <wish> for %B1.", roomlist[rnum]);

	    ROOM		room;

	    glbCurLevel->buildRandomRoomFromDefinition(room, glb_allroomdefs[rnum]);
	    // Center on desired tx/ty
	    int		dx, dy;
	    dx = tx - (room.l.x + room.h.x)/2;
	    if (room.l.x + dx < 1)
		dx = 1 - room.l.x;
	    if (room.h.x + dx > MAP_WIDTH - 2)
		dx = MAP_WIDTH-2 - room.h.x;
	    dy = ty - (room.l.y + room.h.y)/2;
	    if (room.l.y + dy < 1)
		dy = 1 - room.l.y;
	    if (room.h.y + dy > MAP_HEIGHT - 2)
		dy = MAP_HEIGHT-2 - room.h.y;

	    room.l.x += dx;
	    room.h.x += dx;
	    room.l.y += dy;
	    room.h.y += dy;

	    glbCurLevel->drawRoom(room, false, false, false);
	    glbCurLevel->populateRoom(room);
	    didsomething = true;
	    break;
	}

	case 9:		// Shape Shift
	{
	    const char *moblist[NUM_MOBS+1];
	    MOB_NAMES	mobname;
	    static int	lastmob = 0;
	    int		tx, ty;

	    FOREACH_MOB(mobname)
	    {
		moblist[mobname] = glb_mobdefs[mobname].name;
	    }
	    moblist[NUM_MOBS] = 0;

	    select = gfx_selectmenu(5, 3, moblist, aorb, lastmob);
	    if (select < 0 || aorb)
		break;
	    mobname = (MOB_NAMES) select;
	    lastmob = select;

	    for (int y = 3; y < 19; y++)
		gfx_cleartextline(y);
    
	    // Determine if we should reset our stats.
	    if (gfx_yesnomenu("Match monster stats?"))
		avatar->changeDefinition(mobname, true);
	    else
		avatar->changeDefinition(mobname, false);

	    didsomething = true;
	    break;
	}
    }

    for (int y = 3; y < 19; y++)
	gfx_cleartextline(y);
    
    return didsomething;
}

void
writeYesNoBar()
{
#ifdef HAS_STYLUS
    if (!glbActionBar)
	return;

    int		x, sx, sy;
    for (x = 0; x < STRIPLENGTH; x++)
    {
	if (x == 18)
	{
	    gfx_spritetile(x+1, SPRITE_YESWAY);
	}
	else if (x == 26)
	{
	    gfx_spritetile(x+1, SPRITE_NOWAY);
	}
	else
	{
	    hamfake_enablesprite(x+1, false);
	    continue;
	}
	action_indexToOverlayPos(x, sx, sy);
	hamfake_movesprite(x+1, sx*TILEWIDTH, sy*TILEHEIGHT);
	hamfake_enablesprite(x+1, true);
    }
#endif
}

void
writeGlobalActionBar(bool useemptyslot)
{
    // Set the absolute overlay tiles we need?
#ifdef HAS_STYLUS
    int			x, sx, sy, endx;
    hamfake_setinventorymode(false);

#ifdef iPOWDER
    // Post the current grey values for all spells
    SPELL_NAMES		spell;
    FOREACH_SPELL(spell)
    {
	int		grey;
	action_spriteFromStripButton(action_packStripButton(spell), &grey);

	glb_spellgreylist[spell] = grey;
    }
#endif

    endx = STRIPLENGTH;
    if (!glbActionBar)
    {
	// Only draw if in drag mode, then only top bar.
#ifdef HAS_KEYBOARD
	if (useemptyslot)
	    endx = 15;
	else
	    endx = 0;
#else
	// If we lack a keyboard, we don't want the F shortcuts put up
	// there to confuse people.
	endx = 0;
#endif
    }
    
    for (x = 0; x < endx; x++)
    {
	if (!glb_globalactionstrip[x])
	{
	    if (!useemptyslot)
	    {
		hamfake_enablesprite(x+1, false);
		continue;
	    }
	    else
	    {
		gfx_spritetile(x+1, SPRITE_EMPTYSLOT);
	    }
	}
	else
	{
	    SPRITE_NAMES		sprite, overlay = SPRITE_INVALID;
	    int				grey;
	    sprite = action_spriteFromStripButton(glb_globalactionstrip[x], &grey);
	    if (grey == 256)
		overlay = SPRITE_SPELLREADY;
	    gfx_spritetile_grey(x+1, sprite, grey, overlay);
	}
	action_indexToOverlayPos(x, sx, sy);
	hamfake_movesprite(x+1, sx*TILEWIDTH, sy*TILEHEIGHT);
	hamfake_enablesprite(x+1, true);
    }
    // Disable any remaining sprites.
    for (; x < STRIPLENGTH; x++)
    {
	hamfake_enablesprite(x+1, false);
    }
#endif
}

COLOUR_NAMES
getColourFromRatio(int cur, int max)
{
    if (cur < 5 || (cur < max / 6))
	return COLOUR_LIGHTRED;
    if (cur < max / 2)
	return COLOUR_LIGHTYELLOW;
    return COLOUR_LIGHTGREEN;
}

void
writeStatusLine()
{
    BUF			 text;
    MOB			*avatar;
    HUNGER_NAMES	 hunger;
    u8			 dsymbol;

    if (glbCurLevel)
	dsymbol = glb_branchdefs[glbCurLevel->branchName()].symbol;
    else
	dsymbol = SYMBOL_DLEVEL;

    avatar = MOB::getAvatar();
    if (avatar)
    {
#ifdef HAS_KEYBOARD
	text.sprintf( 
		"%c%2d%c%03d/%02d%c%03d/%02d%c%03d%c%02d Zinc ",
		    SYMBOL_AC, avatar->getAC(),
		    SYMBOL_HEART, avatar->getHP(),avatar->getHitDie(),
		    SYMBOL_MAGIC, avatar->getMP(),avatar->getMagicDie(),
		    SYMBOL_EXP, avatar->getExp(),
		    dsymbol, glbCurLevel->getDepth());
#else
#ifdef HAS_XYBUTTON
	text.sprintf( 
		"%c%2d%c%03d/%02d%c%03d/%02d%c%03d%c%02d%c%c%c%c%c%c",
		    SYMBOL_AC, avatar->getAC(),
		    SYMBOL_HEART, avatar->getHP(),avatar->getHitDie(),
		    SYMBOL_MAGIC, avatar->getMP(),avatar->getMagicDie(),
		    SYMBOL_EXP, avatar->getExp(),
		    dsymbol, glbCurLevel->getDepth(),
		    // As the A is on the RHS, we do this B then A.
		    // Likewise for Y then X.
		    getActionChar(glb_actionbuttons[BUTTON_L]),
		    getActionChar(glb_actionbuttons[BUTTON_R]),
		    getActionChar(glb_actionbuttons[BUTTON_Y]),
		    getActionChar(glb_actionbuttons[BUTTON_X]),
		    getActionChar(glb_actionbuttons[BUTTON_B]),
		    getActionChar(glb_actionbuttons[BUTTON_A]));
#else
	text.sprintf( 
		"%c%2d%c%03d/%02d%c%03d/%02d%c%03d%c%02d %c%c %c%c",
		    SYMBOL_AC, avatar->getAC(),
		    SYMBOL_HEART, avatar->getHP(),avatar->getHitDie(),
		    SYMBOL_MAGIC, avatar->getMP(),avatar->getMagicDie(),
		    SYMBOL_EXP, avatar->getExp(),
		    dsymbol, glbCurLevel->getDepth(),
		    // As the A is on the RHS, we do this B then A.
		    getActionChar(glb_actionbuttons[BUTTON_L]),
		    getActionChar(glb_actionbuttons[BUTTON_R]),
		    getActionChar(glb_actionbuttons[BUTTON_B]),
		    getActionChar(glb_actionbuttons[BUTTON_A]));
#endif
#endif
	// text.sprintf( "Time: %d     ", speed_gettime());
	gfx_printtext(0, 19, text);

	COLOUR_NAMES		colour;
	int				x;

	// Rewrite our health in coloured text...
	if (glbColouredFont && avatar->getHP() < avatar->getMaxHP())
	{
	    colour = getColourFromRatio(avatar->getHP(), avatar->getMaxHP());
	    for (x = 0; x < 3; x++)
		gfx_printcolourchar(4+x, 19, text.buffer()[4+x], colour);
	}
	// And the magic...
	if (glbColouredFont && avatar->getMP() < avatar->getMaxMP())
	{
	    colour = getColourFromRatio(avatar->getMP(), avatar->getMaxMP());
	    for (x = 0; x < 3; x++)
		gfx_printcolourchar(11+x, 19, text.buffer()[11+x], colour);
	}

	// Build the status line...  Check the stats we think are cool.
	text.clear();

	hunger = avatar->getHungerLevel();
	if (glb_hungerdefs[hunger].name)
	{
	    text.strcat(glb_hungerdefs[hunger].name);
	    text.strcat(" ");
	}

	if (avatar->hasIntrinsic(INTRINSIC_STONING))
	    text.strcat("Hardening ");
	if (avatar->hasIntrinsic(INTRINSIC_AFLAME))
	    text.strcat("Aflame ");
	if (avatar->hasIntrinsic(INTRINSIC_STRANGLE))
	    text.strcat("Strangled ");
	if (avatar->hasIntrinsic(INTRINSIC_PARALYSED))
	    text.strcat("Paralysed ");
	if (avatar->hasIntrinsic(INTRINSIC_ASLEEP))
	    text.strcat("Asleep ");
	if (avatar->hasIntrinsic(INTRINSIC_CONFUSED))
	    text.strcat("Confused ");
	if (avatar->hasIntrinsic(INTRINSIC_POISON_DEADLY))
	    text.strcat("Deadly Poisoned ");
	if (avatar->hasIntrinsic(INTRINSIC_POISON_HARSH))
	    text.strcat("Harshly Poisoned ");
	if (avatar->hasIntrinsic(INTRINSIC_POISON_STRONG))
	    text.strcat("Strongly Poisoned ");
	if (avatar->hasIntrinsic(INTRINSIC_POISON_NORMAL))
	    text.strcat("Poisoned ");
	if (avatar->hasIntrinsic(INTRINSIC_POISON_MILD))
	    text.strcat("Mildly Poisoned ");
	if (avatar->hasIntrinsic(INTRINSIC_BLEED))
	    text.strcat("Bleeding ");
	if (avatar->hasIntrinsic(INTRINSIC_BLIND))
	    text.strcat("Blind ");
	if (avatar->hasIntrinsic(INTRINSIC_DEAF))
	    text.strcat("Deaf ");
	if (avatar->hasIntrinsic(INTRINSIC_TIRED))
	    text.strcat("Tired ");
	if (avatar->hasIntrinsic(INTRINSIC_SUBMERGED))
	{
	    // Switch on tile type...
	    int			tile;

	    tile = glbCurLevel->getTile(avatar->getX(), avatar->getY());
	    if (glb_squaredefs[tile].submergeattack == ATTACK_DROWN)
		text.strcat("Submerged ");
	    else
		text.strcat("Buried ");
	}
	if (avatar->hasIntrinsic(INTRINSIC_INPIT))
	    text.strcat("In Pit ");
	if (avatar->hasIntrinsic(INTRINSIC_INTREE))
	    text.strcat("In Tree ");
	if (avatar->hasIntrinsic(INTRINSIC_SLOW))
	    text.strcat("Slow ");
	if (avatar->hasIntrinsic(INTRINSIC_AMNESIA))
	    text.strcat("Amnesic ");
	if (avatar->hasIntrinsic(INTRINSIC_OFFBALANCE))
	    text.strcat("Off Balance ");
	if (glbHasJustSearched)
	{
	    if (avatar->hasIntrinsic(INTRINSIC_SEARCH))
		text.strcat("Searching ");
	    else if (glbSearchCount == 1)
		text.strcat("Searched ");
	    else
		text.appendSprintf("Searched x%d ", glbSearchCount);
	}

	gfx_cleartextline(18);
	gfx_printtext(0, 18, text);
    }
    else
    {
	gfx_printtext(0, 19, SYMBOLSTRING_STRENGTH "0"
			     SYMBOLSTRING_SMARTS "0"
			     SYMBOLSTRING_AC "00"
			     SYMBOLSTRING_HEART "000/00"
			     SYMBOLSTRING_MAGIC "000/00"
			     SYMBOLSTRING_EXP "000"
			     SYMBOLSTRING_DLEVEL "00"
			     "  ");

	gfx_cleartextline(18);
	if (glbHasAlreadyWon)
	    gfx_printtext(0, 18, "Celebarating");
	else
	    gfx_printtext(0, 18, "Dead");
    }

    // Write out our buttons...
    writeGlobalActionBar(false);
}

// This menu should be freed with delete []
const char **
buildMenu(ACTION_NAMES *menu, u8 **stripvalue = 0)
{
    int		num;
    int		i;
    const char **result;

    // Count number of actions...
    for (i = 0; menu[i] != ACTION_NONE; i++);
    num = i;

    result = new const char *[num+1];
    if (stripvalue)
	*stripvalue = new u8 [num+1];

    for (i = 0; menu[i] != ACTION_NONE; i++)
    {
	result[i] = glb_actiondefs[menu[i]].name;
	if (stripvalue)
	    (*stripvalue)[i] = action_packStripButton(menu[i]);
    }
    // Terminating null...
    result[i] = 0;
    if (stripvalue)
	(*stripvalue)[i] = action_packStripButton(ACTION_NONE);

    return result;
}

ACTION_NAMES
selectMenu(ACTION_NAMES *menu)
{
    const char **m;
    int		 select, aorb, def;
    static ACTION_NAMES		lastaction = ACTION_NONE;

    m = buildMenu(menu);

    // Find our default...
    {
	ACTION_NAMES		*iter;

	def = 0;
	for (iter = menu; *iter != ACTION_NONE; iter++)
	{
	    if (*iter == lastaction)
		def = iter - menu;
	}
    }

    select = gfx_selectmenu(30-10, 3, m, aorb, def);

    if (aorb != 0)
	select = -1;
    
    delete [] m;

    if (select == -1)
	return ACTION_NONE;
    else
    {
	lastaction = menu[select];
	return menu[select];
    }
}

// aorb = 0, a, 1, then b
ACTION_NAMES
selectAssignment(ACTION_NAMES *menu, int &button)
{
    const char	**m;
    static int	lastsel = 0;
    int		select;
    u8		*stripvalue;

    m = buildMenu(menu, &stripvalue);
    select = gfx_selectmenu(30-12, 3, m, button, lastsel, true, true,
			stripvalue,
			glb_globalactionstrip);
    if (select >= 0)
	lastsel = select;
    delete [] m;
    delete [] stripvalue;

    if (select == -1)
	return ACTION_NONE;
    else
	return menu[select];
}

//
// Prebuilt action menus
//

// These are generic actions that can be assigned to a or b.
ACTION_NAMES	menu_generic[] =
{
    ACTION_CLIMB,
    ACTION_SEARCH,
    ACTION_ZAP,
    ACTION_FIRE,
    ACTION_JUMP,
    ACTION_LOOK,
    ACTION_WAIT,
    ACTION_PICKUP,
    ACTION_EAT,
    ACTION_OPEN,
    ACTION_CLOSE,
    // ACTION_LOCK, // Does nothing.  Should be item activated anyways.
    ACTION_BREATHE,	// Breathes in a selected direction.
    ACTION_WISH,	// Wishes that you knew the wizard password.		
    ACTION_RELEASE,
    ACTION_COMMAND,
    ACTION_NAME,
    ACTION_SWAP,
    ACTION_SLEEP,
    ACTION_FORGET,
    ACTION_MOVE,
    ACTION_SAFEWALK,
    ACTION_RUN,
    ACTION_MINIMAP, // Message queue + minimap.  Only back queues on L.
    ACTION_HISTORY, // Brings up message history display.
    ACTION_PRAY,
    ACTION_USE,
    ACTION_VERBLIST,	// Needed so you can bind to this list itself
    ACTION_INVENTORY,	// Likewise required for binding targets.
    ACTION_OPTIONS,
    ACTION_NONE
};

void
buildActionMenu(ITEM *item, ACTION_NAMES *menu)
{
    bool	equipped = false;

    if (!item)
    {
	// Trivially true...
	*menu++ = ACTION_SORT;
	*menu++ = ACTION_NONE;
	return;
    }
    
    if (!item->getX())
	equipped = true;

    // Standard actions..
    if (item->getMagicType() == MAGICTYPE_POTION ||
	item->getDefinition() == ITEM_WATER)
	*menu++ = ACTION_QUAFF;
    if (item->getMagicType() == MAGICTYPE_SCROLL ||
	item->getMagicType() == MAGICTYPE_SPELLBOOK)
	*menu++ = ACTION_READ;
    if (item->getMagicType() == MAGICTYPE_WAND)
	*menu++ = ACTION_ZAP;
    if (item->getDefinition() == ITEM_LIGHTNINGRAPIER)
	*menu++ = ACTION_ZAP;

    if (equipped)
    {
	// A smaller set of actions is valid for equipped items.
	*menu++ = ACTION_DEQUIP;
	*menu++ = ACTION_DIP;
	*menu++ = ACTION_NAME;
	*menu++ = ACTION_FAVORITE;
	*menu++ = ACTION_EXAMINE;
	*menu++ = ACTION_SORT;
    }
    else
    {
	// Standard actions..
	*menu++ = ACTION_EQUIP;
	*menu++ = ACTION_EAT;
	*menu++ = ACTION_DIP;
	*menu++ = ACTION_DROP;
	*menu++ = ACTION_SORT;

	// If there is more than one item in a stack, you can split
	// the stack.
	if (item->getStackCount() > 1)
	    *menu++ = ACTION_SPLITSTACK;
	
	*menu++ = ACTION_NAME;
	*menu++ = ACTION_FAVORITE;
	*menu++ = ACTION_EXAMINE;
	*menu++ = ACTION_QUIVER;
	*menu++ = ACTION_THROW;
    }

    // End the sequence.
    *menu++ = ACTION_NONE;
}

void
writeItemActionBar(ITEM *item, bool onlyexamine = false)
{
    ACTION_NAMES	menu[NUM_ACTIONS];

    hamfake_setinventorymode(true);

    if (onlyexamine)
    {
	ACTION_NAMES	*m = menu;
	*m++ = ACTION_INVENTORY;
	if (item)
	    *m++ = ACTION_EXAMINE;
	*m++ = ACTION_NONE;
    }
    else
	buildActionMenu(item, menu);

    // Set the absolute overlay tiles we need?
#ifdef HAS_STYLUS
    ACTION_NAMES	action;
    int			x, sx, sy;
    
    if (!glbActionBar)
    {
	for (x = 0; x < STRIPLENGTH; x++)
	{
	    hamfake_enablesprite(x+1, false);
	}
	return;
    }
    for (x = 0; x < STRIPLENGTH; x++)
    {
	bool		enable = true;

	action = glb_inventoryactionstrip[x];

	action_indexToOverlayPos(x, sx, sy);
	hamfake_movesprite(x+1, sx*TILEWIDTH, sy*TILEHEIGHT);
	// First parse always succeed/always fail.
	// Then look at our action menu for insight.
	if (action == ACTION_NONE)
	    enable = false;
	else if (action == ACTION_INVENTORY)
	    enable = true;
	else
	{
	    int			j;
	    // See if we can find it in our menu.
	    for (j = 0; menu[j] != ACTION_NONE; j++)
	    {
		if (menu[j] == action)
		    break;
	    }
	    if (menu[j] == ACTION_NONE)
		enable = false;
	    else
		enable = true;
	}
	// Display if enabled.
	hamfake_enablesprite(x+1, enable);
	if (enable)
	{
	    gfx_spritetile(x+1, (SPRITE_NAMES) glb_actiondefs[action].tile);
	}
    }
#endif
}

void
introMessage()
{
    // Display the intro message.
#ifndef MAPTEST
    if (hamfake_isunlocked())
	encyc_viewentry("HELP", HELP_POWDER);
    else
	encyc_viewentry("HELP", HELP_POWDER_LOCKED);
#endif
}

// Paranoid much?
/* Original: "By Jeff Lait " */
static unsigned char    glb_author[] = {
    'B'+34, 'y'+13, ' '+22, 'J'+94, 'e'+22, 'f'+13, 'f'+52, ' '+54,
    'L'+72, 'a'+45, 'i'+64, 't'+90, ' '+32, 0
};

static const unsigned char    glb_author_off[] = {
    34,13,22,94,22,13,52,54,72,45,64,90,32,
};

static const char *
get_glb_author() {
    int i;
    if (glb_author[0] != 66)
    {
	for (i=0; i<13; i++)
	    glb_author[i] -= glb_author_off[i];
    }
    return (const char *)glb_author;
}

// This makes me feel like it is 1988.
void processOptions();

bool
intro_screen()
{
    MAINMENU_NAMES	selection;
    static int		def = 0;

    {
	int		y;
	for (y = 2; y < 19; y++)
	    gfx_cleartextline(y);
    }
    glbAutoRunEnabled = false;

    while (!hamfake_forceQuit())
    {
	for (int y = 2; y < 19; y++)
	    gfx_cleartextline(y);
	
	gfx_printtext(10, 1, "POWDER 117"); 

	gfx_printtext(30 - strlen(get_glb_author()), 19,
		    get_glb_author());

#ifdef iPOWDER
	// So what *can* I badge this?
	//gfx_printtext(0, 18, "Built with *redacted*");
	if (hamfake_isunlocked())
	    gfx_printtext(0, 18, "Full Version");
#else
#ifdef USING_SDL
	gfx_printtext(0, 18, "Built with SDL 1.2");
#else
#ifdef USING_DS
	gfx_printtext(0, 18, "Built with DevkitPro 1.4.4 r21");
#else
#ifdef _3DS
	gfx_printtext(0, 18, "Built with DevkitArm");
#else
	gfx_printtext(0, 18, "Built with HAM 2.8");
#endif
#endif
#endif
#endif

	// Ask the user what they want to do.
	{
	    int			aorb;
	    const char		*textmenu[NUM_MAINMENUS];
	    int			i;

	    const MAINMENU_NAMES		menu[] =
	    {
		MAINMENU_NEWGAME,
		MAINMENU_TUTORIAL,
		MAINMENU_VIEWSCORES,
		MAINMENU_OPTIONS,
#if defined(USING_SDL) && !defined(SYS_PSP)
#ifndef iPOWDER
		MAINMENU_QUIT,
#endif
#ifdef iPOWDER
		MAINMENU_STRESSTEST,
		MAINMENU_UNLOCK,
#endif
#endif
		MAINMENU_NONE
	    };

	    const MAINMENU_NAMES		menu_load[] =
	    {
		MAINMENU_LOAD,
		MAINMENU_NEWGAME,
		MAINMENU_TUTORIAL,
		MAINMENU_VIEWSCORES,
		MAINMENU_OPTIONS,
#if defined(USING_SDL) && !defined(SYS_PSP)
#ifndef iPOWDER
		MAINMENU_QUIT,
#endif
#ifdef iPOWDER
		MAINMENU_STRESSTEST,
		MAINMENU_UNLOCK,
#endif
#endif
		MAINMENU_NONE
	    };

	    const MAINMENU_NAMES		menu_scum[] =
	    {
		MAINMENU_NEWGAME,
		MAINMENU_TUTORIAL,
		MAINMENU_SAVESCUM,
		MAINMENU_VIEWSCORES,
		MAINMENU_OPTIONS,
#if defined(USING_SDL) && !defined(SYS_PSP)
#ifndef iPOWDER
		MAINMENU_QUIT,
#endif
#ifdef iPOWDER
		MAINMENU_STRESSTEST,
		MAINMENU_UNLOCK,
#endif
#endif
		MAINMENU_NONE
	    };

	    const MAINMENU_NAMES *usemenu;
	    
	    switch (hiscore_savecount())
	    {
		case 0:
		    usemenu = menu;
		    break;
		case 1:
		    usemenu = menu_load;
		    break;
		default:
		    usemenu = menu_scum;
		    break;
	    }

	    for (i = 0; usemenu[i] != MAINMENU_NONE; i++)
	    {
#ifdef iPOWDER
		// Disable the unlock if unlocked.
		if (usemenu[i] == MAINMENU_UNLOCK &&
		    hamfake_isunlocked())
		    break;
#endif
		textmenu[i] = glb_mainmenudefs[usemenu[i]].name;
	    }
	    textmenu[i] = 0;

	    int			rawchoice;

	    rawchoice = gfx_selectmenu(2, 6, textmenu, aorb, def);

	    // Ignore bad selection.
	    if (rawchoice < 0)
		continue;

	    def = rawchoice;

	    // Ignore b.
	    if (aorb)
		continue;

	    selection = usemenu[rawchoice];
	}

	// Selection is greater than 0.

	// Special case: Quit
	if (selection == MAINMENU_QUIT)
	{
	    hamfake_softReset();
	    // No return from the valley of the dead!
	}

	// Special case: View scores.
	if (selection == MAINMENU_VIEWSCORES)
	{
	    hiscore_display(0, false);

	    {
		int		y;
		for (y = 2; y < 19; y++)
		    gfx_cleartextline(y);
	    }

	    // Continue
	    continue;
	}

	// Special case: Options.
	if (selection == MAINMENU_OPTIONS)
	{
	    int		y;
	    for (y = 2; y < 19; y++)
		gfx_cleartextline(y);
	    processOptions();
	    hamfake_clearKeyboardBuffer();
	    for (y = 2; y < 19; y++)
		gfx_cleartextline(y);

	    // Do not start a new game :>
	    continue;
	}

	if (selection == MAINMENU_STRESSTEST)
	{
#ifdef iPOWDER
	    if (!hamfake_isunlocked())
	    {
		// Let them know they have to unlock!
		for (int y = 2; y < 19; y++)
		    gfx_cleartextline(y);

		gfx_pager_addtext(
			"The stress test will automatically play short "
			"games of POWDER, picking random levels and "
			"random actions to perform.  It is "
			"surprisingly soothing to watch the computer "
			"lose as often as you do.");
		gfx_pager_newline(); gfx_pager_newline();
		gfx_pager_addtext(
			"Access to the stress test requires your copy "
			"of POWDER to be unlocked, however."
			);
		gfx_pager_display();
		continue;
	    }
#endif
	}

	if (selection == MAINMENU_UNLOCK)
	{
	    attemptunlock();

	    // We can't test for success here as we return immediately
	    // from posting even though the user is still mucking
	    // about trying to decide if they want to pay the paltry
	    // unlock fee or not.
	    // (After XX years I can call it paltry compared to may
	    // own contribution to the cause)
	    def = 0;
	    continue;
	}

	if (selection == MAINMENU_NEWGAME)
	{
	    // Double check if they have an active game.
	    // It doesn't always overwrite, but likely will thanks
	    // to Quit & Save being the default everywhere.
	    if (hiscore_savecount() == 1)
	    {
		int		y;
		for (y = 2; y < 19; y++)
		    gfx_cleartextline(y);
		if (!gfx_yesnomenu("Replace your active game?"))
		    continue;
	    }
	}

	// Fall out.
	break;
    }
    
    {
	int		y;
	for (y = 2; y < 19; y++)
	    gfx_cleartextline(y);
    }
    
    glbTutorial = false;

    if (selection == MAINMENU_NEWGAME)
    {
	// Request a deity to worship.  Since this involves extra decisions
	// violates the principle of POWDER that one can just start
	// playing without thinking.  However, many advanced players
	// would want to try a specific god so end up start-scumming
	// until they got equipment good for that god.  I'm strongly
	// opposed to start scumming, so thus the decision to allow
	// advanced players to select a starting god, and thus
	// implied equipment.  Grats to Derek S. Ray for writing
	// a patch for this and allowing me to wash hands of any
	// hypocrisy for going against my official policy.

	if (hiscore_advancedmode())
	{
	    GOD_NAMES		god;
	    const char		*menu[NUM_GODS+1];

	    FOREACH_GOD(god)
	    {
		menu[god] = glb_goddefs[god].name;
	    }
	    menu[NUM_GODS] = 0;

	    static int	startgod = 0;
	    int		choice;
	    while (1)
	    {
		if (hamfake_forceQuit())
		    hamfake_softReset();

		int		aorb, y;
		gfx_printtext(5,3, "Initial God?");
		choice = gfx_selectmenu(30 - 20, 4, menu, aorb, startgod);
		for (y = 3; y < 19; y++)
		    gfx_cleartextline(y);
		if (choice >= 0)
		    break;
	    }
	    startgod = choice;
	    god = (GOD_NAMES) choice;

	    piety_setgod(god);
	}
	else
	{
	    // Default to no one god, the classic value.
	    piety_setgod(GOD_AGNOSTIC);
	}
	
	// Request the gender.  This is before name so those who pick
	// randomly can pick an appropriate name.

	// Default to a random gender choice on boot up, after that
	// keep the users last preference.
	if (glbGender == -1)
	    glbGender = rand_choice(2);

	const char *gendermenu[] =
	{
	    "Male",
	    "Female",
	    0
	};
	int		aorb;

	gfx_printtext(0, 6, "Gender? ");
	glbGender = gfx_selectmenu(5, 7, gendermenu, aorb, glbGender);

	// Secret code: [Start] to pick random gender.
	if (glbGender == -1)
	    glbGender = rand_choice(2);

	{
	    int		y;
	    for (y = 6; y < 10; y++)
		gfx_cleartextline(y);
	}

	// Request the name.
	BUF	welcome;
	welcome.sprintf("You are a %s %s.",
		glbGender ? "female" : "male",
		glb_goddefs[piety_chosengod()].classname);
	gfx_printtext(0, 3, welcome);

	// Now get their name.
	gfx_printtext(0, 4, "Name? ");

	input_getline(glbAvatarName, 23, 6, 4, 23, 8, glbAvatarName);

	// See if it is time to invoke the stress test.
#ifndef iPOWDER
	if (rand_hashstring(glbAvatarName) == 0x7a51ad9b)
	    glbStressTest = true;
	else
#endif
	    glbStressTest = false;

	// We don't want to save the game if the user just
	// hit home on the name screen!
	if (hamfake_forceQuit())
	    hamfake_softReset();
    }

    if (selection == MAINMENU_TUTORIAL)
    {
	// Tutorial has no name
	strcpy(glbAvatarName, "George");
	glbTutorial = true;
    }

    if (selection == MAINMENU_STRESSTEST)
    {
	// Getting here implies we are unlocked.
	strcpy(glbAvatarName, "Defiant");
	glbStressTest = true;
    }

    {
	int		y;
	for (y = 0; y < 20; y++)
	    gfx_cleartextline(y);
    }

    if (hamfake_forceQuit())
	hamfake_softReset();

    // Return whether one should load or not.
    return (selection == MAINMENU_LOAD || selection == MAINMENU_SAVESCUM);
}

void
loadGame()
{
    // For testing, we can verify the new map...
    SRAMSTREAM		is;
    int			val;

    glbAutoRunEnabled = false;

    // Verify we can load...
    if (!hiscore_savecount())
    {
	msg_report("No saved game!  Aborting load!");
	return;
    }

    // Flag that this is now longer a new game.
    hiscore_flagnewgame(false);

    // If we are a save scummer...
    if (hiscore_savecount() > 1)
    {
	BUF		buf;
	buf.sprintf("You have save scummed %d times.  ",
		    hiscore_savecount() - 1);
	msg_report(buf);
    }

    msg_report("Loading...");

    ai_reset();
    if (glbCurLevel)
	glbCurLevel->deleteAllMaps();

    artifact_init();

    // Load hiscore details.
    hiscore_load(is);

    // Load our name.
    is.readString(glbAvatarName, 30);

    is.read(val, 8);
    glbWizard = val ? true : false;

    is.read(val, 8);
    glbTutorial = val ? true : false;

    is.read(val, 8);
    glbHasAlreadyWon = val ? true : false;

    int			totaltime;
    is.read(totaltime, 32);
    // Push our time back in time to account for already
    // used time.
    glbGameStartFrame = gfx_getframecount() - totaltime;

    // Get the number of moves.
    speed_load(is);

    // Load our name table.
    name_load(is);
    
    ITEM::loadGlobal(is);
    MOB::loadGlobal(is);
    MAP::loadGlobal(is);
    piety_load(is);

    MAP::changeCurrentLevel(new MAP(0, BRANCH_NONE));
    glbCurLevel->loadBranch(is);

    // At the very end, we load the avatar number.
    // This way we ensure the avatar has been 
    // loaded before we try and load its refid.
    MOB::loadAvatar(is);

    // Rebuild FOV & Light tables.
    // They were wiped out by the save to
    // save space.
    gfx_forceLockTile(TILE_AVATAR, 2);
    if (MOB::getAvatar())
    {
	gfx_scrollcenter(MOB::getAvatar()->getX(), MOB::getAvatar()->getY());
	MOB::getAvatar()->rebuildAppearance();
	glbCurLevel->buildFOV(MOB::getAvatar()->getX(), 
			      MOB::getAvatar()->getY(),
			      7, 5);
    }
    glbCurLevel->updateLights();
    glbCurLevel->refresh();

    writeStatusLine();
    msg_report("Done!");

    // Successfully loaded.  Any reload is thus a save scum.
    hiscore_bumpsavecount();

    // Save the new hiscore with the new save count!
    {
	SRAMSTREAM	os;
	hiscore_save(os);

	// Trigger non-GBA sessions to complete the save.
	hamfake_endWritingSession();
    }

    // Report whim of xom
    if (MOB::getAvatar())
	MOB::getAvatar()->pietyAnnounceWhimOfXom();

#ifdef STRESS_TEST
    // Finished loading our game, verify our mob structure.
    bool		valid;

    valid = glbCurLevel->verifyMob();
    if (!valid)
	msg_report("MOBREF Corrupt.  ");
    else
	msg_report("MOBREF Intact.  ");
#endif
}

void
rebuild_all()
{
    // Ensure there is no avatar currently!
    MOB::setAvatar(0);

    // Ensure we don't get any embarassing level changes.
    MAP::delayedLevelChange(0);

    ai_reset();
    if (glbCurLevel)
	glbCurLevel->deleteAllMaps();

    // Everything should be gone now!
    // printf("Map: %d, Mob: %d, Item: %d\n", glbMapCount, glbMobCount, glbItemCount);
		    
    glbWizard = false;
    glbHasAlreadyWon = false;

    MOB::init();
    ITEM::init();
    piety_init();
    artifact_init();

    MAP		*map;
    
    map = new MAP(1, BRANCH_MAIN);

    map->build(false);
    map->populate();
    
    MAP::changeCurrentLevel(map);
    map->setWindDirection(0, 0, 0);

    // Dude's position.
    int		x, y;
    MOB		*avatar;

    bool	found = false;

    // Find a valid x & y...
    // Ideally we want to always start on the up stair case.
    found = glbCurLevel->findTile(SQUARE_LADDERUP, x, y);
    if (found)
	found = glbCurLevel->findCloseTile(x, y, MOVE_WALK);

    if (!found)
    {
	found = glbCurLevel->findTile(SQUARE_DIMDOOR, x, y);
	if (found)
	    found = glbCurLevel->findCloseTile(x, y, MOVE_WALK);

	if (!found)
	{
	    found = glbCurLevel->findRandomLoc(x, y, MOVE_WALK, false,
				false, false, false, false, true);
	    if (!found)
	    {
		// This could be hell and nothing but lava.
		found = glbCurLevel->findRandomLoc(x, y, MOVE_STD_SWIM, false,
				false, false, false, false, true);

		if (!found)
		{
		    // Very bad!
		    UT_ASSERT(!"No valid starting location!");
		    x = y = 15;
		}
	    }
	}
    }

    avatar = MOB::create(MOB_AVATAR);
    avatar->move(x, y);
    glbCurLevel->registerMob(avatar);

    glbCurLevel->buildFOV(x, y, 7, 5);

    MOB::setAvatar(avatar);

    // ID all items...
    ITEM		*id;

    for (y = 0; y < MOBINV_HEIGHT; y++)
	for (x = 0; x < MOBINV_WIDTH; x++)
	{
	    id = avatar->getItem(x, y);
#if 1
	    if (id)
	    {
		id->markIdentified(true);
		id->markMapped();
	    }
#endif
	}

#ifndef MAPTEST
    gfx_scrollcenter(avatar->getX(), avatar->getY());
#endif
    glbCurLevel->doHeartbeat();
    glbCurLevel->updateLights();
    // Only mark mapped with full range if we are not blind...
    if (avatar->hasIntrinsic(INTRINSIC_BLIND))
	glbCurLevel->markMapped(avatar->getX(), avatar->getY(), 0, 0);
    else
	glbCurLevel->markMapped(avatar->getX(), avatar->getY(), 7, 5);
    glbCurLevel->refresh();

    gfx_forceLockTile(TILE_AVATAR, 2);
    avatar->rebuildAppearance();

    glbGameStartFrame = gfx_getframecount();

    speed_reset();

    msg_clear();
    writeStatusLine();
    
    if (!glbStressTest)
    {
	hamfake_rebuildScreen();

	introMessage();

	avatar->reportMessage("Welcome to POWDER!");

	// Describe where the avatar is standing, in case there
	// is some item.
	glbCurLevel->describeSquare(avatar->getX(), avatar->getY(), 
				    avatar->hasIntrinsic(INTRINSIC_BLIND),
				    false);

	avatar->pietyAnnounceWhimOfXom();
	if (glbCurLevel->isSquareInLockedRoom(avatar->getX(), avatar->getY()))
	{
	    avatar->reportMessage("There is a secret door in the walls of this room.  Keep Searching!");
	}
    }

    // Second write is to restore the action bar.
    writeStatusLine();
}

void
processWorld()
{
    // We check for avatar map consistency both at the start and the
    // end of this method.
    MAP::checkForAvatarMapChange();
    
    // Update our smoke.
    // This is an annoying time to do this - you get insta-flood and
    // insta-smoke move.  We want the newly created pit to last one
    // turn before it floods.
    // I have no idea how to do this.  We sort of want the update
    // after the user chooses, but before the user acts?
    glbCurLevel->moveSmoke();
    glbCurLevel->moveFluids();

    if (MOB::getAvatar())
    {
	gfx_scrollcenter(MOB::getAvatar()->getX(), 
			 MOB::getAvatar()->getY());
	glbCurLevel->buildFOV(MOB::getAvatar()->getX(), 
			      MOB::getAvatar()->getY(),
			      7, 5);

    }
    // This needs to be done so rays hit where you are, not where
    // you will be.
    // This is too expensive, however.  Our solution is to update
    // the display whenever the status line scrolls.  This way you'll
    // always get the partial effects.
    // glbCurLevel->refresh();

    // Move the NPCs...
    glbCurLevel->moveNPCs();

    // Give the gods a chance to meddle.
    if (speed_isheartbeatphase())
	if (MOB::getAvatar())
	    MOB::getAvatar()->pietyRungods();

    speed_ticktime();
    
    writeStatusLine();

    // Rebuild our map...
    glbCurLevel->doHeartbeat();
    glbCurLevel->updateLights();
    if (MOB::getAvatar())
    {
	if (MOB::getAvatar()->hasIntrinsic(INTRINSIC_BLIND))
	    glbCurLevel->markMapped(MOB::getAvatar()->getX(), MOB::getAvatar()->getY(), 0, 0);
	else
	    glbCurLevel->markMapped(MOB::getAvatar()->getX(), MOB::getAvatar()->getY(), 7, 5);
    }
    //if (ctrl_rawpressed(BUTTON_R))

    MAP::checkForAvatarMapChange();

    // If the avatar exists, we only refresh on valid move phases.
    // Note this is tested AFTER speed_ticktime!  This actually means
    // that we refresh just before a turn in which the avatar will
    // be able to move. If we used the pretick value, we'd refresh on
    // processWorlds that occur after the avatar move, but not update
    // for any succeeding non-avatar moves.
    if (!MOB::getAvatar() || MOB::getAvatar()->validMovePhase(speed_getphase()))
	glbCurLevel->refresh();
}

bool
performAutoPickup()
{
    MOB		*avatar = MOB::getAvatar();
    ITEMSTACK	 stack;
    ITEM	*item;
    bool	 submerged, didpickup;
    int		 i, n;

    if (!avatar)
	return false;

    if (avatar->hasIntrinsic(INTRINSIC_INTREE))
	return false;

    submerged = avatar->hasIntrinsic(INTRINSIC_INPIT) ||
		avatar->hasIntrinsic(INTRINSIC_SUBMERGED);
    glbCurLevel->getItemStack(stack, avatar->getX(), avatar->getY(),
			      submerged);

    didpickup = false;
    n = stack.entries();
    for (i = 0; i < n; i++)
    {
	if (stack(i)->isQuivered())
	{
	    item = stack(i);
	    didpickup = avatar->actionPickUp(item);
	    if (!didpickup)
	    {
		// We failed to pickup!  Better not try any more items.
		return false;
	    }
	}
    }

    // Return if we found anything to pickup.
    return didpickup;
}

bool
performRandomAction(MOB *avatar)
{
    ITEM		*item;
    int			 ix, iy;
    int			 dx, dy, dz;
    bool		 doitem;
    
    if (!avatar)
	return false;
    
    doitem = avatar->findItemSlot(ix, iy);

    switch (rand_choice(1 + doitem * 4))
    {
	case 0:		// spell
	{
	    SPELL_NAMES		spell;

	    // Cast a random spell
	    spell = (SPELL_NAMES) rand_choice(NUM_SPELLS);
	    if (spell == SPELL_NONE)
		break;

	    // Ensure we have sufficient xp,hp,mp
	    avatar->incrementMaxMP(glb_spelldefs[spell].mpcost);
	    avatar->incrementMaxHP(glb_spelldefs[spell].hpcost);
	    if (avatar->getExp() < glb_spelldefs[spell].xpcost)
	    {
		avatar->receiveExp(glb_spelldefs[spell].xpcost);
	    }
	    avatar->setIntrinsic((INTRINSIC_NAMES)glb_spelldefs[spell].intrinsic);
	    
	    dx = dy = dz = 0;
	    if (glb_spelldefs[spell].needsdir)
	    {
		dx = rand_range(-1, 1);
		dy = rand_range(-1, 1);
		if (!dx && !dy)
		{
		    dz = rand_range(-1, 1);
		}
	    }
	    if (glb_spelldefs[spell].needstarget)
	    {
		// Really should try to target someone...
		dx = rand_range(-3, 3);
		dy = rand_range(-3, 3);
	    }

	    return avatar->actionCast(spell, dx, dy, dz);
	}

	case 1:		// quaff
	    item = ITEM::createRandomType(ITEMTYPE_POTION);
	    avatar->acquireItem(item, ix, iy);

	    return avatar->actionEat(ix, iy);

	case 2:		// zap
	    item = ITEM::createRandomType(ITEMTYPE_WAND);
	    avatar->acquireItem(item, ix, iy);

	    dx = rand_range(-1, 1);
	    dy = rand_range(-1, 1);
	    dz = 0;
	    if (!dx && !dy)
	    {
		dz = rand_range(-1, 1);
	    }

	    return avatar->actionZap(ix, iy, dx, dy, dz);

	case 3:		// read
	    item = ITEM::createRandomType(ITEMTYPE_SCROLL);
	    avatar->acquireItem(item, ix, iy);

	    return avatar->actionRead(ix, iy);

	case 4:		// dip
	{
	    ITEM		*dipper;

	    item = ITEM::createRandomType(ITEMTYPE_POTION);
	    avatar->acquireItem(item, ix, iy);

	    dipper = avatar->randomItem();
	    if (dipper)
	    {
		return avatar->actionDip(ix, iy, dipper->getX(), dipper->getY());
	    }
	    break;
	}

	default:
	    UT_ASSERT(!"Invalid choice for random action");
	    break;
    }
    
    return false;
}

void
performAutoRead()
{
    MOB		*avatar = MOB::getAvatar();
    SIGNPOST_NAMES	signpost;

    if (!avatar)
	return;

    // Check to see if we are on a signpost, in which case we want
    // to spam the proper message
    signpost = glbCurLevel->getSignPost(avatar->getX(), avatar->getY());
    if (signpost != SIGNPOST_NONE)
    {
#ifdef iPOWDER
	encyc_viewentry("SIGNPOST_iPOWDER", signpost);
#else
#ifdef HAS_KEYBOARD
	encyc_viewentry("SIGNPOST_SDL", signpost);
#else
	encyc_viewentry("SIGNPOST_GBA", signpost);
#endif
#endif
    }
}

void
performAutoPrompt()
{
    MOB		*avatar = MOB::getAvatar();
    ITEMSTACK	 stack, eatstack;
    bool	 submerged;
    bool	 hasladder = false, didclimb = false;
    bool	 consumed;
    int		 i, j, n, aorb, select, laddernum, cancelnum;
    SQUARE_NAMES square;

    if (!avatar)
	return;

    if (avatar->hasIntrinsic(INTRINSIC_INTREE))
	return;

    submerged = avatar->hasIntrinsic(INTRINSIC_INPIT) ||
		avatar->hasIntrinsic(INTRINSIC_SUBMERGED);
    glbCurLevel->getItemStack(stack, avatar->getX(), avatar->getY(),
			      submerged);

    // Determine if we are on a ladder that the user might want to climb.
    square = glbCurLevel->getTile(avatar->getX(), avatar->getY());

    hasladder = glb_squaredefs[square].promptclimb;

    // Whenever we climb we disable the next autoprompt to avoid
    // having the user get a double-prompt if they manually climb.
    if (glbSuppressAutoClimb)
	hasladder = false;
    glbSuppressAutoClimb = false;

    // Repeatedly prompt the user until they either accept or run out
    // of things to do.
    while (stack.entries() || hasladder)
    {
	// Build our accept/reject string.
	char		**list;

	n = stack.entries();
	list = new char *[stack.entries()*2 + 2 + hasladder];

	stack.reverse();
	i = 0;
	for (j = 0; j < n; j++)
	{
	    list[i++] = stack(j)->getName(false).strdup();
	}
	// Add the option of eating any corpses here
	eatstack.clear();
	for (j = 0; j < n; j++)
	{
	    if (avatar->canDigest(stack(j)))
	    {
		BUF		buf;
		buf.strcpy("eat ");
		buf.strcat(stack(j)->getName());
		list[i++] = strdup(buf.buffer());
		// Track our edible items
		eatstack.append(stack(j));
	    }
	}
	laddernum = i;
	if (hasladder)
	{
	    if (square == SQUARE_DIMDOOR)
		list[i++] = strdup("Enter Portal");
	    else
		list[i++] = strdup("Climb Ladder");
	}
	cancelnum = i;
	list[i++] = strdup("cancel");
	list[i] = 0;

	// Determine what the user wants.
	gfx_printtext(1, 3, "Action?");
	select = gfx_selectmenu(3, 4, (const char **) list, aorb);
	// Handle cancels.
	if (select == cancelnum)
	    select = -1;

	// Empty the screen:
	for (i = 3; i < 18; i++)
	{
	    gfx_cleartextline(i);
	}
	
	if (select < 0 || aorb)
	{
	    // The user has cancelled, quit!
	    break;
	}

	// Check if the use climbed or picked up.
	if (hasladder && (select == laddernum))
	{
	    // Climb request
	    consumed = avatar->actionClimb();
	    didclimb = true;
	}
	else if (select >= n)
	{
	    // Eat request...
	    j = select - n;
	    if (j >= eatstack.entries())
	    {
		// Cancel request.
		break;
	    }
	    // Perform the eat
	    ITEM		*newitem = 0;

	    consumed = avatar->actionPickUp(eatstack(j), &newitem);
	    if (newitem && newitem->isInventory())
	    {
		consumed |= avatar->actionEat(newitem->getX(), newitem->getY());
		didclimb = true;
	    }
	}
	else
	{
	    // Pick up request.
	    consumed = avatar->actionPickUp(stack(select));
	}

	// Destroy the list.
	for (i = 0; list[i]; i++)
	{
	    free(list[i]);
	}
	delete [] list;

	// If, and only if, we consumed, pass time.
	// We really need to abstract this!
	while (consumed)
	{
	    if (MOB::getAvatar())
		MOB::getAvatar()->doHeartbeat();
	    processWorld();

	    // The mob may be paralysed or what not.  If a save
	    // request occurs during this time slice, we want to
	    // ensure we quit *this* turn!
	    if (hamfake_forceQuit())
	    {
		return;
	    }

	    // Run the move prequel code.  If this return false,
	    // it means we should skip the move - ie: process heartbeat
	    // and world and continue.
	    // Thus, if it passes, we want to break out and
	    // go to the normal control selection.
	    if (!MOB::getAvatar() ||
		MOB::getAvatar()->doMovePrequel())
	    {
		break;
	    }
	}

	// Recover if we have no avatar anymore.
	avatar = MOB::getAvatar();
	if (!avatar)
	    break;
	
	// Update the item stack
	stack.clear();

	submerged = avatar->hasIntrinsic(INTRINSIC_INPIT) ||
		    avatar->hasIntrinsic(INTRINSIC_SUBMERGED);
	glbCurLevel->getItemStack(stack, avatar->getX(), avatar->getY(),
				  submerged);

	// If we climbed, we don't want to continue.
	if (didclimb)
	    break;
    }
}

bool
useInventory(MOB *avatar, bool quickselect)
{
    // Something will happen, clear the message queue.
    msg_clear();

    bool		done = false;
    bool		consumed = true;
    bool		forceconsume = false;
    int			aorb = 0;
    STYLUSLOCK		styluslock(REGION_BOTTOMBUTTON);
    
    // If we are doing quickselect, determine which key will cause
    // the quick select.
    if (quickselect)
    {
	if (ctrl_rawpressed(BUTTON_A))
	    aorb = BUTTON_A;
	else if (ctrl_rawpressed(BUTTON_B))
	    aorb = BUTTON_B;
	else if (ctrl_rawpressed(BUTTON_R))
	    aorb = BUTTON_R;
	else if (ctrl_rawpressed(BUTTON_L))
	    aorb = BUTTON_L;
	else if (ctrl_rawpressed(BUTTON_X))
	    aorb = BUTTON_X;
	else if (ctrl_rawpressed(BUTTON_Y))
	    aorb = BUTTON_Y;
	else
	{
	    // They let go of quick select before we got here!
	    return false;
	}
    }
    
    gfx_showinventory(avatar);
    hamfake_enableexternalactions(true);

    {
	int		cx, cy;
	ITEM		*item;
	gfx_getinvcursor(cx, cy);
	item = avatar->getItem(cx, cy);
	writeItemActionBar(item);
    }
    // Wait for a key...
    while (!done)
    {
	if (!gfx_isnewframe())
	    continue;

	if (hamfake_forceQuit())
	    break;

	ITEM		*item;
	int		 dx = 0, dy = 0, cx, cy;
	bool		 apress, bpress, dirty;
	int		 sx, sy, ex, ey;
	ACTION_NAMES	 action = ACTION_NONE;

	// Select cancels...
	if (ctrl_hit(BUTTON_SELECT))
	{
	    consumed = forceconsume;
	    break;
	}
	
	if (styluslock.performInventoryDrag(sx, sy, ex, ey, avatar, dirty))
	{
	    // No mater what, we
	    // Update our cursor location.
	    gfx_setinvcursor(ex, ey, false);

	    if (!sx)
	    {
		// If they drag to another equip slot, no-op
		// In any case, our final slot location should be the
		// staring slot, not the ending slot, as the ending
		// slot isn't where the item ends up.
		// (We could try and figure that out, but it is very
		// difficult due to stack merges, etc)
		gfx_setinvcursor(sx, sy, false);
		if (ex)
		{
		    // Item is in equip slots, dequip.
		    // TODO: move it to the requested location?
		    gfx_hideinventory();
		    consumed = avatar->actionDequip((ITEMSLOT_NAMES) sy);
		    done = true;
		    break;
		}
	    }
	    else
	    {
		// Dragging from inventory.  If it goes onto an equipment
		// slot it is an equip request.
		if (!ex)
		{
		    gfx_hideinventory();
		    consumed = avatar->actionEquip(sx, sy,
					    (ITEMSLOT_NAMES) ey);
		    done = true;
		    break;
		}
		else
		{
		    // Swap of items, this is safe since we use
		    // linked lists right now.
		    ITEM		*i1, *i2;
		    i1 = avatar->getItem(sx, sy);
		    i2 = avatar->getItem(ex, ey);
		    if (i1)
			i1->setPos(ex, ey);
		    if (i2)
			i2->setPos(sx, sy);
		}
	    }
	}
		
	if (dirty)
	{
	    ITEM		*item;
	    gfx_getinvcursor(cx, cy);
	    // Redisplay the inventory as we'll have erased the void tile.
	    gfx_hideinventory();
	    item = avatar->getItem(cx, cy);
	    writeItemActionBar(item);
	    gfx_showinventory(avatar);
	}
	
	ctrl_getdir(dx, dy);

	if (quickselect)
	{
	    apress = false;
	    if (!ctrl_rawpressed(aorb))
	    {
		apress = true;
	    }
	    bpress = false;
	}
	else
	{
	    apress = ctrl_hit(BUTTON_A);
	    bpress = ctrl_hit(BUTTON_B);
	    // Allow more generous cancelling so one can get out
	    // the way one got in.
	    bpress |= ctrl_hit(BUTTON_X);
	    bpress |= ctrl_hit(BUTTON_Y);
	}

	// Abort with b.
	if (bpress)
	{
	    consumed = forceconsume;
	    break;
	}
	
	// Check to see if we are hovering an inventory item
	if (stylus_queryinventoryitem(sx, sy))
	{
	    gfx_getinvcursor(cx, cy);
	    dx = sx - cx;
	    dy = sy - cy;
	}

	if (dx || dy)
	{
	    // A direction key is hit, update the cursor pos.
	    gfx_getinvcursor(cx, cy);
	    cx += dx;
	    cy += dy;
	    gfx_setinvcursor(cx, cy, false);
	    // No more tests...

	    // Refetch the cursor to handle wrapping...
	    gfx_getinvcursor(cx, cy);
	    item = avatar->getItem(cx, cy);
	    writeItemActionBar(item);
	    continue;
	}

	// Determine if we have key press, so thus a fake action.
	if (!apress)
	{
	    int		keypress;

	    keypress = hamfake_getKeyPress(false);

	    switch (keypress)
	    {
		case 'E':
		case 'w':
		    action = ACTION_EQUIP;
		    apress = true;
		    break;

		case 'u':
		case 'T':
		    action = ACTION_DEQUIP;
		    apress = true;
		    break;

		case 'd':
		    action = ACTION_DROP;
		    apress = true;
		    break;

		case 'e':
		    action = ACTION_EAT;
		    apress = true;
		    break;

		case 'r':
		    action = ACTION_READ;
		    apress = true;
		    break;

		case '!':
		case 'D':
		    action = ACTION_DIP;
		    apress = true;
		    break;

		case 'z':
		    action = ACTION_ZAP;
		    apress = true;
		    break;

		case 't':
		    action = ACTION_THROW;
		    apress = true;
		    break;

		case 'f':
		    action = ACTION_FAVORITE;
		    apress = true;
		    break;

		case 'q':
		    action = ACTION_QUIVER;
		    apress = true;
		    break;

		case 'n':
		    action = ACTION_NAME;
		    apress = true;
		    break;

		case 's':
		    action = ACTION_SORT;
		    apress = true;
		    break;

		case 'S':
		    action = ACTION_SPLITSTACK;
		    apress = true;
		    break;

		case 'x':
		    action = ACTION_EXAMINE;
		    apress = true;
		    break;
	    }
	}

	if (!apress)
	{
	    int		button;

	    // Check to see if the action menu was used.
	    if (glbActionBar && styluslock.getbottombutton(button))
	    {
		action = glb_inventoryactionstrip[button];
		if (action != ACTION_NONE)
		{
		    apress = true;
		}
	    }
	}

	if (!apress)
	{
	    int		iact, ispell;
	    hamfake_externalaction(iact, ispell);
	    if (iact != ACTION_NONE)
	    {
		action = (ACTION_NAMES) iact;
		apress = true;
	    }
	}

	if (apress)
	{
	    // User has selected something, process the
	    // request...
	    gfx_getinvcursor(cx, cy);
	    item = avatar->getItem(cx, cy);

	    if (!item)
	    {
		if (quickselect)
		{
		    consumed = false;
		    break;
		}
		if (action != ACTION_INVENTORY &&
		    action != ACTION_SORT)
		    continue;		// Ignore
	    }

	    // Build the menu...
	    ACTION_NAMES	menu[NUM_ACTIONS];

	    buildActionMenu(item, menu);

	    // If the user selected quickly from keyboard, we don't
	    // need an action here.
	    if (action == ACTION_NONE)
		action = selectMenu(menu);

	    // Clear out the selection menu.
	    gfx_showinventory(avatar);
	    switch (action)
	    {
		case ACTION_NONE:
		    // Do nothing...
		    break;
		case ACTION_QUAFF:	// Drink is same as eat
		case ACTION_EAT:
		    gfx_hideinventory();
		    consumed = avatar->actionEat(cx, cy);
		    done = true;
		    break;
		case ACTION_QUIVER:
		    gfx_hideinventory();
		    consumed = avatar->actionQuiver(cx, cy);
		    // Since quiver is free, we let people chain
		    // quivers.
		    done = false;
		    gfx_showinventory(avatar);
		    break;
		case ACTION_FAVORITE:
		    gfx_hideinventory();
		    item->markFavorite(!item->isFavorite());
		    // Since <pedantic>favorite</pedantic> is free, we let
		    // people chain <pedantic>favorites</pedantic>.
		    done = false;
		    gfx_showinventory(avatar);
		    break;
		case ACTION_READ:
		    gfx_hideinventory();
		    consumed = avatar->actionRead(cx, cy);
		    // If someone aborts a read, they should get
		    // back to menu.
		    done = consumed;
		    // Clear out the description viewer.
		    if (!done)
			gfx_showinventory(avatar);
		    break;
		case ACTION_EXAMINE:
		    gfx_hideinventory();

		    item->viewDescription();
		    
		    consumed = false;
		    // Do not return to the main screen as they likely
		    // are examinig a chain of items.
		    // This is the sort of impressive change I make while
		    // waiting in the Signet Ana lounge for my first class
		    // seat.
		    // done = true;

		    // Clear out the description viewer.
		    gfx_showinventory(avatar);
		    break;
		case ACTION_NAME:
		{
		    char		buf[100];
		    int			nametype;
		    
		    gfx_hideinventory();
		    
		    msg_clear();
		    gfx_printtext(0, 0, "Specific or type? ");

		    {
			const char	*nametype_menu[] =
			{
			    "Specific",
			    "Type",
			    0
			};
			static int	 namedef = 1;
			int		 aorb;
			
			nametype = gfx_selectmenu(30-10, 3, nametype_menu, aorb, namedef);
			// B button cancels.
			if (aorb)
			    nametype = -1;
			if (nametype >= 0)
			    namedef = nametype;
		    }

		    {
			int		y;
			for (y = 0; y < 18; y++)
			    gfx_cleartextline(y);
		    }
		    if (nametype < 0)
		    {
			consumed = false;
			done = true;
			break;
		    }
		    // Prohibit renaming artifacts.
		    if (item->isArtifact() && !nametype)
		    {
			msg_report("You cannot rename an artifact!  ");
			consumed = false;
			done = true;
			break;
		    }
		    
		    gfx_printtext(0, 0, "New Name? ");
		    input_getline(buf, 19, 10, 0, 23, 8);
		    
		    {
			int		y;
			for (y = 0; y < 18; y++)
			    gfx_cleartextline(y);
		    }

		    if (nametype)
			item->renameType(buf);
		    else
			item->rename(buf);

		    consumed = false;
		    done = true;
		    break;
		}
		case ACTION_THROW:
		{
		    // Find the zap direction...
		    BUF			buf;
		    int			tx, ty;
		    int			dx, dy, dz;

		    gfx_hideinventory();
		    done = true;
		    avatar->formatAndReport("Throw %I1U where?", item);
		    
		    tx = avatar->getX();
		    ty = avatar->getY();
		    if (gfx_selectdirection(tx, ty, dx, dy, dz))
		    {
			msg_clear();

			consumed = avatar->actionThrow(
					cx, cy, dx, dy, dz);
		    }
		    else
		    {
			// Cancelled...
			msg_clear();
			consumed = false;
		    }
		    break;
		}
		case ACTION_DIP:
		{
		    // Request a second item, dip this into
		    // that item.  Check if we are on a well
		    // in which case we want to dip into
		    // the well.
		    int		selectx, selecty;
		    
		    selectx = cx;
		    selecty = cy;

		    avatar->formatAndReport("Dip %IU in what?", item);

		    if (gfx_selectinventory(selectx, selecty))
		    {
			// Attempt to dip the two
			// selected items together...
			gfx_getinvcursor(selectx, selecty);
			// Restore cursor to start item.
			gfx_setinvcursor(cx, cy, false);
			gfx_hideinventory();

			consumed = avatar->actionDip(
					selectx, selecty,
					cx, cy);

			done = true;
		    }
		    else
		    {
			// Wipe our dip prompt.
			gfx_setinvcursor(cx, cy, false);
			msg_clear();
		    }
		    
		    break;
		}
		case ACTION_DROP:
		    gfx_hideinventory();
		    consumed = avatar->actionDrop(cx, cy);

		    // We allow an arbitrary number of drops at once.
		    done = false;
		    // We want to cost a turn.
		    forceconsume = true;
		    gfx_showinventory(avatar);
		    break;
		case ACTION_EQUIP:    
		{
		    bool	madechoice = false;
		    int		slotx, sloty;
		    BUF		buf;

		    slotx = 0;
		    sloty = ITEMSLOT_RHAND;

		    avatar->formatAndReport("Equip %I1U where?", item);

		    // We want to make it easy for the user, so default
		    // to the most logical slot.
		    if (item->isHelmet())
			sloty = ITEMSLOT_HEAD;
		    if (item->isBoots())
			sloty = ITEMSLOT_FEET;
		    if (item->isShield())
			sloty = ITEMSLOT_LHAND;
		    if (item->isJacket())
			sloty = ITEMSLOT_BODY;
		    if (item->isAmulet())
			sloty = ITEMSLOT_AMULET;
		    if (item->isRing())
		    {
			sloty = ITEMSLOT_RRING;
			if (avatar->getEquippedItem(ITEMSLOT_RRING)
			    && !avatar->getEquippedItem(ITEMSLOT_LRING))
			    sloty = ITEMSLOT_LRING;
		    }
		    // If the item is a bow, default to the left hand.
		    if (item->getDefinition() == ITEM_BOW)
		    {
			sloty = ITEMSLOT_LHAND;
		    }

		    // Item is in main inventory, equip.
		    // Get the user to choose a inventory slot...
		    gfx_setinvcursor(slotx, sloty, true);

		    STYLUSLOCK	styluslock(REGION_SLOTS);
		    while (1)
		    {
			if (!gfx_isnewframe())
			    continue;
			if (hamfake_forceQuit())
			    break;
			
			ctrl_getdir(dx, dy);
			apress = ctrl_hit(BUTTON_A);
			bpress = ctrl_hit(BUTTON_B);

			if (bpress)
			{
			    // cancel.
			    madechoice = false;
			    break;
			}
			if (apress)
			{
			    // Made a choice.
			    madechoice = true;
			    break;
			}

			int		sslot;
			if (styluslock.selectinventoryslot(sslot))
			{
			    if (sslot == -1)
				madechoice = false;
			    else
			    {
				madechoice = true;
				sloty = sslot;
				gfx_setinvcursor(slotx, sloty, true);
			    }
			    break;
			}

			// Consume invalid keys.
			hamfake_getKeyPress(false);

			// Move in dy direction...
			gfx_getinvcursor(slotx, sloty);
			sloty += dy;
			gfx_setinvcursor(slotx, sloty, true);
		    }

		    // If aborted, reset cursor to cx/cy
		    if (!madechoice)
		    {
			gfx_setinvcursor(cx, cy, false);
			msg_clear();
		    }
		    else
		    {
			// Try to do the action...
			gfx_getinvcursor(slotx, sloty);
			// Even if we accepted, leave the cursor
			// where it started as this seems to be
			// what I expect.  (And I am all who
			// counts here, right???)
			gfx_setinvcursor(cx, cy, false);
			gfx_hideinventory();
			
			msg_clear();

			consumed = avatar->actionEquip(cx, cy, 
					(ITEMSLOT_NAMES) sloty);
			done = true;
		    }
		    break;
		}
		case ACTION_DEQUIP:
		{
		    if (cx)
		    {
			avatar->formatAndReport("%U <be> not wearing %IU.", item);
			consumed = false;
		    }
		    else
		    {
			// Item is in equip slots, dequip.
			gfx_hideinventory();
			consumed = avatar->actionDequip((ITEMSLOT_NAMES) cy);
			done = true;
		    }
		    break;
		}
		case ACTION_ZAP:
		{
		    // Find the zap direction...
		    BUF			buf;
		    int			tx, ty;
		    int			dx, dy, dz;

		    gfx_hideinventory();
		    done = true;
		    avatar->formatAndReport("Zap %IU where?", item);
		    
		    tx = avatar->getX();
		    ty = avatar->getY();
		    if (gfx_selectdirection(tx, ty, dx, dy, dz))
		    {
			msg_clear();

			consumed = avatar->actionZap(
					cx, cy, dx, dy, dz);
		    }
		    else
		    {
			// Cancelled...
			msg_clear();
			consumed = false;
		    }
		    break;
		}
		case ACTION_SORT:
		{
		    // Re arrange backpack
		    gfx_hideinventory();
		    consumed = avatar->actionSort();
		    done = false;
		    // Clear out the description viewer.
		    gfx_showinventory(avatar);
		    break;
		}
		case ACTION_SPLITSTACK:
		{
		    // This doesn't take a turn or cost time.
		    done = false;
		    consumed = false;

		    // Take one item from this stack and add back
		    // to our inventory.
		    if (item->getStackCount() <= 1)
		    {
			// Illegal split.
			avatar->formatAndReport("%U can't split %IU in two.", item);
			break;
		    }
		    ITEM		*split;
		    int			 sx, sy;

		    if (avatar->findItemSlot(sx, sy))
		    {
			// Have to update the inventory so they
			// can see that something happened.
			gfx_hideinventory();
			split = item->splitStack();
			avatar->formatAndReport("%U <separate> %IU.", split);
			avatar->acquireItem(split, sx, sy);
			gfx_showinventory(avatar);
		    }
		    else
		    {
			avatar->formatAndReport("%U <lack> space to split the stack.");
			break;
		    }
		    break;
		}
		case ACTION_INVENTORY:
		    // We just want to quit back to the game.
		    gfx_hideinventory();

		    // We don't want to waste a turn doing this!
		    consumed = forceconsume;
		    done = true;
		    break;
		default:
		{
		    UT_ASSERT(!"Unhandled ACTION!");
		    break;
		}
	    }

	    // If the avatar is not done yet, we want to rebuild the lost
	    // action bar.
	    if (!done)
	    {
		gfx_getinvcursor(cx, cy);
		item = avatar->getItem(cx, cy);
		writeItemActionBar(item);
	    }

	    if (quickselect)
	    {
		// Even if the user didn't make a valid choice,
		// we still exit the inventory screen.
		consumed = false;
		break;
	    }
	}
    }

    if (!done)
	gfx_hideinventory();

    return consumed;
}

void
processHelp()
{
    while (1)
    {
	HELP_NAMES		helplist[NUM_HELPS];
	const char		*helptype[NUM_HELPS];

	int			helpidx = 0;

	if (hamfake_isunlocked())
	    helplist[helpidx++] = HELP_POWDER;
	else
	    helplist[helpidx++] = HELP_POWDER_LOCKED;

#ifdef HAS_KEYBOARD
	helplist[helpidx++] = HELP_KEYBOARD;
#endif
#ifdef HAS_STYLUS
#ifdef iPOWDER
	helplist[helpidx++] = HELP_TOUCH;
#else
	helplist[helpidx++] = HELP_STYLUS;
#endif
#endif
#ifndef HAS_KEYBOARD
#ifndef iPOWDER
	helplist[helpidx++] = HELP_GAMEBOY;
#endif
#endif
	helplist[helpidx++] = HELP_STATUSLINE;
	helplist[helpidx++] = HELP_SPELL;
	helplist[helpidx++] = HELP_SKILL;
	helplist[helpidx++] = HELP_GOD;

	for (int i = 0; i < helpidx; i++)
	{
	    helptype[i] = glb_helpdefs[helplist[i]].menu;
	}
	helptype[helpidx] = 0;

	int			 selection, aorb;
	static int		 helpdef = 0;

	selection = gfx_selectmenu(30 - 15, 3, helptype, aorb, helpdef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}
	helpdef = selection;

	selection = helplist[selection];

	switch (selection)
	{
	    case HELP_GOD:
	    {
		// Secondary menu.
		const char  *menu[NUM_GODS+1];
		int	     god;

		for (god = 0; god < NUM_GODS; god++)
		{
		    menu[god] = glb_goddefs[god].name;
		}
		menu[god] = 0;

		static int goddef = 0;
		
		while (1)
		{
		    god = gfx_selectmenu(30 - 15, 3, menu, aorb, goddef);
		    if (aorb || god < 0)
		    {
			break;
		    }
		    goddef = god;

		    encyc_viewentry("GOD", god);
		}
		break;
	    }
	    case HELP_SPELL:
	    {
		// Secondary menu.
		const char  *menu[NUM_SPELLS+1];
		int	     spell;

		// Skip first as it is no-spell.
		for (spell = 1; spell < NUM_SPELLS; spell++)
		{
		    menu[spell-1] = glb_spelldefs[spell].name;
		}
		menu[spell-1] = 0;

		static int spelldef = 0;
		
		while (1)
		{
		    spell = gfx_selectmenu(30 - 15, 3, menu, aorb, spelldef);
		    if (aorb || spell < 0)
		    {
			break;
		    }
		    spelldef = spell;

		    // Increment by one for no-spell
		    encyc_viewSpellDescription((SPELL_NAMES)(spell+1));
		}
		break;
	    }

	    case HELP_SKILL:
	    {
		// Secondary menu.
		const char	*menu[NUM_SKILLS+1];
		int		 skill;

		// Skip first as it is no-skill.
		for (skill = 1; skill < NUM_SKILLS; skill++)
		{
		    menu[skill-1] = glb_skilldefs[skill].name;
		}
		menu[skill-1] = 0;

		static int skilldef = 0;
		
		while (1)
		{
		    skill = gfx_selectmenu(30 - 15, 3, menu, aorb, skilldef);
		    if (aorb || skill < 0)
		    {
			break;
		    }
		    skilldef = skill;

		    // Increment by one for no-skill
		    encyc_viewSkillDescription((SKILL_NAMES)(skill+1));
		}
		break;
	    }

	    default:
		encyc_viewentry("HELP", selection);
		break;
	}
    }
}

void
saveGame()
{
    SRAMSTREAM		os;

    msg_report("Saving...");

    // If the save count is 0,
    // 	- we are writing a new game for the first time.
    // If the save count is 1,
    //	- we are overwriting a valid game.
    // If the save count is 2,
    //	- We are resaving a game we loaded.
    // Else, we have cheated!
    // If we have flagged as a new game, we know we should reset.
    if (hiscore_isnewgame() || hiscore_savecount() <= 2)
	hiscore_setsavecount(1);
    else
	hiscore_setsavecount(hiscore_savecount()-1);
    
    // In any case, it is no longer a new game.
    hiscore_flagnewgame(false);
    
    hiscore_save(os);

    // Save our name.
    os.writeString(glbAvatarName, 30);

    os.write(glbWizard, 8);
    os.write(glbTutorial, 8);
    os.write(glbHasAlreadyWon, 8);

    // Save our current total time so far.
    int			totaltime;

    totaltime = gfx_getframecount() - glbGameStartFrame;

    os.write(totaltime, 32);

    // Save total moves so far.
    speed_save(os);

    // Save our speed table.
    name_save(os);

    ITEM::saveGlobal(os);
    MOB::saveGlobal(os);
    MAP::saveGlobal(os);
    piety_save(os);
    
    glbCurLevel->saveBranch(os);

    // Now save our avatar...
    MOB::saveAvatar(os);

    os.flush();

    // Trigger non-GBA sessions to complete the save.
    hamfake_endWritingSession();
    
    // The saving process may generate temp levels
    // that will refresh...
    glbCurLevel->refresh();
    msg_report("Done!");

#ifdef STRESS_TEST
    // Verify the mobref structure is good.
    bool		valid;

    valid = glbCurLevel->verifyMob();
    if (!valid)
	msg_report("MOBREF Corrupt.  ");
    else
	msg_report("MOBREF Intact.  ");
#endif

    // Having saved, we now axe the avatar, forcing a reload.
    MOB		*avatar;
    avatar = MOB::getAvatar();
    MOB::setAvatar(0);
    // Kill the avatar.
    if (avatar)
    {
	glbCurLevel->unregisterMob(avatar);
	delete avatar;
    }

    hamfake_clearKeyboardBuffer();
}

void
processOptions()
{
    int		selection;
    OPTION_NAMES	option;
    int		aorb;
    static int	def = 0, defnoavatar = 0;
    
    const char	*menu[] =
    {
	"Help",
	"Save&Quit",
#ifdef iPOWDER
	"Buttons",
#endif
	"Tiles",
	"Opacity",
	"Fonts",
	"ColorText",
	"AutoPrompt",
	"SafeWalk",
	"ActionBar",
	"CharDump",
	"KnownItems",
	"DebugInfo",
	"Collapse",
	"Quit",
	"Defaults",
	0,		// Full Screen.
	0
    };

    OPTION_NAMES menuopt[] =
    {
	OPTION_HELP,
	OPTION_SAVE,
#ifdef iPOWDER
	OPTION_BUTTONS,
#endif
	OPTION_TILES,
	OPTION_OPACITY,
	OPTION_FONTS,
	OPTION_COLOUREDFONTS,
	OPTION_AUTOPROMPT,
	OPTION_SAFEWALK,
	OPTION_ACTIONBAR,
	OPTION_CHARDUMP,
	OPTION_KNOWNITEMS,
	OPTION_DEBUGINFO,
	OPTION_COLLAPSE,
	OPTION_QUIT,
	OPTION_DEFAULTS,
	OPTION_FULLSCREEN
    };

    const char	*menunoavatar[] =
    {
	"Help",
	"Tiles",
	"Opacity",
	"Fonts",
	"ColorText",
	"AutoPrompt",
	"SafeWalk",
	"ActionBar",
	"Defaults",
#ifdef iPOWDER
	"Unlocking",
#endif
	0,		// Full Screen.
	0
    };
    
    OPTION_NAMES menunoavataropt[] =
    {
	OPTION_HELP,
	OPTION_TILES,
	OPTION_OPACITY,
	OPTION_FONTS,
	OPTION_COLOUREDFONTS,
	OPTION_AUTOPROMPT,
	OPTION_SAFEWALK,
	OPTION_ACTIONBAR,
	OPTION_DEFAULTS,
#ifdef iPOWDER
	OPTION_UNLOCKING,
#endif
	OPTION_FULLSCREEN
    };

#if defined(USING_SDL) && !defined(iPOWDER)
    int			i;

    // Toggle the Full Screen option depending on our actual full
    // screen settings.
    for (i = 0; menu[i]; i++);
    if (hamfake_isFullScreen())
	menu[i] = "Windowed";
    else
	menu[i] = "FullScreen";
    for (i = 0; menunoavatar[i]; i++);
    if (hamfake_isFullScreen())
	menunoavatar[i] = "Windowed";
    else
	menunoavatar[i] = "FullScreen";
#endif

    option = OPTION_NONE;
    if (MOB::getAvatar())
    {
	selection = gfx_selectmenu(30-11, 3, menu, aorb, def);
	if (selection >= 0 && !aorb)
	{
	    def = selection;
	    option = menuopt[selection];
	}
    }
    else
    {
	selection = gfx_selectmenu(30-11, 3, menunoavatar, aorb, defnoavatar);
	if (selection >= 0 && !aorb)
	{
	    defnoavatar = selection;
	    option = menunoavataropt[selection];
	}
    }

    if (option == OPTION_NONE)
	return;

    // Cancel on b press
    if (aorb)
	return;

    if (option == OPTION_SAVE)
    {
	saveGame();
	glbFinishedASave = true;
	return;
    }

    if (option == OPTION_QUIT)
    {
	if (gfx_yesnomenu("Really quit?", false))
	{
	    // Suicide the avatar.
	    MOB		*avatar;
	    avatar = MOB::getAvatar();
	    MOB::setAvatar(0);
	    // Kill the avatar.
	    if (avatar)
	    {
		glbCurLevel->unregisterMob(avatar);
		delete avatar;
	    }
	    glbFinishedASave = true;
	    return;
	}
    }

    if (option == OPTION_LOAD)
    {
	// Check the current time.  If we are too late
	// we don't want to axe the current game!
	if (speed_gettime() > 100)
	{
	    BUF			buf;

	    buf.sprintf("%s cries: I still have a chance!  "
			 "Let me continue!  ",
			 glbAvatarName[0] ? glbAvatarName
				    : "Lazy Player");
	    msg_report(buf);
	}
	else
	{
	    loadGame();
	}

	return;
    }

    if (option == OPTION_CHARDUMP)
    {
	if (MOB::getAvatar())
	    MOB::getAvatar()->characterDump(false, false, true);

	return;
    }

    if (option == OPTION_KNOWNITEMS)
    {
	ITEM::viewDiscoveries();

	return;
    }

    if (option == OPTION_DEBUGINFO)
    {
	BUF		buf;
	bool		valid;

	// Perform mob verification.
	if (glbCurLevel)
	{
	    valid = glbCurLevel->verifyMob();
	    if (valid)
		gfx_pager_addtext("MOBREF chain valid.  ");
	    else
		gfx_pager_addtext("MOBREF chain broken!  ");
	    gfx_pager_newline();
	}

	buf.sprintf("%d mobs, %d tables, %d buffers",
		mobref_getnumrefs(),
		mobref_getnumtables(),
		buf_numbufs());
	gfx_pager_addtext(buf); gfx_pager_newline();

	buf.sprintf("%d items, %d mobs, %d maps.",
		glbItemCount, glbMobCount, glbMapCount);
	gfx_pager_addtext(buf); gfx_pager_newline();

	gfx_pager_addtext("Meaningless Stats:");
	gfx_pager_newline();

	buf.sprintf("%d Mob Types", NUM_MOBS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Item Types", NUM_ITEMS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Wand Types", NUM_WANDS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Book Types", NUM_SPELLBOOKS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Amulet Types", NUM_AMULETS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Helms Types", NUM_HELMS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Ring Types", NUM_RINGS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Scroll Types", NUM_SCROLLS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Potion Types", NUM_POTIONS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Attack Types", NUM_ATTACKS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Element Types", NUM_ELEMENTS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Spells", NUM_SPELLS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Skills", NUM_SKILLS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Intrinsics", NUM_INTRINSICS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Actions", NUM_ACTIONS);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Sprites", NUM_SPRITES);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Tiles", NUM_TILES);
	gfx_pager_addtext(buf); gfx_pager_newline();
	buf.sprintf("%d Paperdoll Tiles", NUM_MINIS);
	gfx_pager_addtext(buf); gfx_pager_newline();

	gfx_pager_display();
	return;
    }

    if (option == OPTION_COLLAPSE)
    {
	const char		*actionbar[] =
	{
	    "Collapse the Dungeon",
	    "Cancel",
	    0
	};

	// Always default to 1.
	selection = gfx_selectmenu(30 - 20, 3, actionbar, aorb, 1);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	if (!selection)
	{
	    MOB::getAvatar()->formatAndReport("%U <hear> a distant rumble.  The world feels more empty.");
	    glbCurLevel->collapseDungeon();
	}
	return;
    }

    if (option == OPTION_FULLSCREEN)
    {
	hamfake_setFullScreen(!hamfake_isFullScreen());
	return;
    }

    if (option == OPTION_DEFAULTS)
    {
	const char		*actionbar[] =
	{
	    "Revert to Defaults",
	    "Cancel",
	    0
	};

	selection = gfx_selectmenu(30 - 20, 3, actionbar, aorb);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	if (!selection)
	{
	    resetDefaultActionStrip();
	    writeGlobalActionBar(false);
	    hamfake_buttonreq(4);
	}
	return;
    }

    if (option == OPTION_BUTTONS)
    {
	const char		*buttons[] =
	{
	    "Move Buttons",
	    "Create Action",
	    "Create Spell",
	    "Delete Button",
	    "Screen Tap",
	    "Cancel",
	    0
	};
	MOB		*avatar;
	avatar = MOB::getAvatar();

	static int		def = 0;

	selection = gfx_selectmenu(30 - 20, 3, buttons, aorb, def);
	if (aorb || selection < 0 || selection == 5)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}
	// Remember last choice.
	// Damn newb mistake, but just made it.
	def = selection;

	if (selection == 0)	// move
	{
	    hamfake_buttonreq(0);
	}
	else if (selection == 1)	// action
	{
	    const char		*m[NUM_ACTIONS + 3];
	    ACTION_NAMES	 a[NUM_ACTIONS+3];
	    int			 i, j, choice, aorb;
	    static int		 lastchoice = 0;
	    
	    for (i = 0; menu_generic[i] != ACTION_NONE; i++)
	    {
		m[i] = glb_actiondefs[menu_generic[i]].name;
		a[i] = menu_generic[i];
	    }
	    a[i] = ACTION_NONE;
	    m[i] = "cancel";
	    m[i+1] = 0;

	    for (int y = 3; y <= 19; y++)
		gfx_cleartextline(y);
	    gfx_printtext(0, 3, "Which action?");
	    choice = gfx_selectmenu(5, 4, m, aorb, lastchoice);
	    if (aorb || (choice != -1 && a[choice] == ACTION_NONE))
		choice = -1;

	    if (choice != -1)
	    {
		// Create the button
		hamfake_buttonreq(1, a[choice]);
	    }
	}
	else if (selection == 2)	// spell
	{
	    const char		*m[NUM_SPELLS + 3];
	    SPELL_NAMES		 s[NUM_SPELLS + 3];
	    SPELL_NAMES		 spell;
	    int			 i, j, choice, aorb;
	    static int		 lastchoice = 0;
	    
	    // Select a magic spell
	    j = 0;
	    FOREACH_SPELL(spell)
	    {
		if (avatar->hasSpell(spell))
		{
		    s[j] = spell;
		    m[j] = glb_spelldefs[spell].name;
		    j++;
		}
	    }

	    // Include cancel option to cast nothing.
	    s[j] = SPELL_NONE;
	    m[j] = "cancel";
	    j++;
            m[j] = 0;

	    if (j <= 1)
	    {
		avatar->reportMessage("You know no spells!");
		return;
	    }
	    for (int y = 3; y <= 19; y++)
		gfx_cleartextline(y);
	    gfx_printtext(0, 3, "Which spell?");
	    choice = gfx_selectmenu(5, 4, m, aorb, lastchoice);
	    if (aorb || (choice != -1 && s[choice] == SPELL_NONE))
		choice = -1;

	    if (choice != -1)
	    {
		// Create the button
		hamfake_buttonreq(2, s[choice]);
	    }
	}
	else if (selection == 3)	// delete
	{
	    hamfake_buttonreq(3);
	}
	else if (selection == 4)	// Toggle screen tapping
	{
	    const char		*screentap[] =
	    {
		"Disable Screen Tap",
		"Enable Screen Tap",
		0
	    };

	    for (int y = 3; y <= 19; y++)
		gfx_cleartextline(y);
	    int			screentapdef;

	    screentapdef = glbScreenTap;
	    selection = gfx_selectmenu(30 - 20, 3, screentap, aorb, screentapdef);
	    if (aorb || selection < 0)
	    {
		// TODO: Return to start menu rather than cancelling all!
		return;
	    }

	    glbScreenTap = selection ? true : false;
	    return;
	}
	return;
    }

#ifdef iPOWDER
    if (option == OPTION_UNLOCKING)
    {
	const char		*actionbar[] =
	{
	    "Restore Unlock",
	    "Erase Unlock",
	    "Cancel",
	    0
	};

	selection = gfx_selectmenu(30 - 20, 3, actionbar, aorb);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	if (selection == 0)
	{
	    if (hamfake_isunlocked())
	    {
		gfx_pager_addtext(
	"Your copy of POWDER is already unlocked, so there is nothing "
	"to restore."
		    );
		gfx_pager_newline(); 
		gfx_pager_display();
	    }
	    else
	    {
		hamfake_buttonreq(6, 1);

		gfx_pager_addtext(
			"Accessing the store..."
			);
		gfx_pager_newline(); gfx_pager_newline();
		gfx_pager_addtext(
	"This may take a while.  System pop-ups will take you through "
	"the rest of the process.  In the meantime, we return you to "
	"your regularly scheduled game."
		    );
		gfx_pager_newline(); 
		gfx_pager_display();
	    }
	}
	else if (selection == 1)
	{
	    if (!hamfake_isunlocked())
	    {
		gfx_pager_addtext(
			"Your copy of POWDER isn't unlocked yet, so there "
			"is no status to revoke."
			);
		gfx_pager_newline(); 
		gfx_pager_display();
	    }
	    else
	    {
		if (gfx_yesnomenu("Lose your unlock status?"))
		{
		    hamfake_setunlocked(false);
		    FILE		*fp;

		    fp = hamfake_fopen("powder.unlockcode", "w");
		    if (fp)
		    {
			fputs("This is not the unlock code.", fp);
			fclose(fp);
		    }
		}
		else
		{
		    gfx_pager_addtext(
			"A wise decision."
			);
		    gfx_pager_newline(); 
		    gfx_pager_display();
		}
	    }
	}
	return;
    }
#endif
    if (option == OPTION_ACTIONBAR)
    {
	const char		*actionbar[] =
	{
	    "Disable Action Bar",
	    "Enable Action Bar",
	    0
	};

	int			actionbardef;

	actionbardef = glbActionBar;
	selection = gfx_selectmenu(30 - 20, 3, actionbar, aorb, actionbardef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	glbActionBar = selection ? true : false;
	return;
    }

    if (option == OPTION_AUTOPROMPT)
    {
	const char		*autoprompt[] =
	{
	    "Disable Prompt",
	    "Enable Prompt",
	    0
	};

	int			autopromptdef;

	autopromptdef = glbAutoPrompt;
	selection = gfx_selectmenu(30 - 15, 3, autoprompt, aorb, autopromptdef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	glbAutoPrompt = selection ? true : false;
	glbSuppressAutoClimb = false;
	return;
    }
    
    if (option == OPTION_SAFEWALK)
    {
	const char		*autoprompt[] =
	{
	    "Attack by Default",
	    "Walk by Default",
	    0
	};

	int			autopromptdef;

	autopromptdef = glbSafeWalk;
	selection = gfx_selectmenu(30 - 20, 3, autoprompt, aorb, autopromptdef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	glbSafeWalk = selection ? true : false;
	return;
    }
    
    if (option == OPTION_COLOUREDFONTS)
    {
	const char		*autoprompt[] =
	{
	    "No Colored Text",
	    "Colored Text",
	    0
	};

	int			autopromptdef;

	autopromptdef = glbColouredFont;
	selection = gfx_selectmenu(30 - 15, 3, autoprompt, aorb, autopromptdef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	glbColouredFont = selection ? true : false;
	return;
    }
    
    if (option == OPTION_OPACITY)
    {
	const char		*autoprompt[] =
	{
	    "Transparent",
	    "Opaque",
	    0
	};

	int			opaquepromptdef;

	opaquepromptdef = glbOpaqueTiles;
	selection = gfx_selectmenu(30 - 15, 3, autoprompt, aorb, opaquepromptdef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}

	glbOpaqueTiles = selection ? true : false;
	return;
    }

    if (option == OPTION_TILES)
    {
	const char 	*tiles[NUM_TILESETS + 2];
	int		 i;

	for (i = 0; i < NUM_TILESETS; i++)
	{
	    tiles[i] = glb_tilesets[i].name;
	}
	tiles[NUM_TILESETS] = 0;

	// Clear out from disk if requested.
	if (!hamfake_extratileset())
	    tiles[NUM_TILESETS-1] = 0;

	int		 tiledef = gfx_gettileset();

	selection = gfx_selectmenu(30 - 15, 3, 
			tiles, 
			aorb, tiledef);

	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}
	
	// New tile default.
	tiledef = selection;

	// Switch to the new tiles.
	gfx_switchtilesets(selection);
	return;
    }

    if (option == OPTION_FONTS)
    {
	const char		*fonts[] =
	{
	    "Classic",
	    "Brass",
	    "Shadow",
	    "Heavy",
	    "Light",
	    0
	};

	int		 fontdef = gfx_getfont();

	selection = gfx_selectmenu(30 - 15, 3, 
			fonts, 
			aorb, fontdef);
	if (aorb || selection < 0)
	{
	    // TODO: Return to start menu rather than cancelling all!
	    return;
	}
	
	// New tile default.
	fontdef = selection;

	// Switch to the new tiles.
	gfx_switchfonts(selection);
	return;
    }

    if (option == OPTION_HELP)
    {
	processHelp();
	return;
    }
}

ACTION_NAMES
processVerbList()
{
    // Nothing will happen as we will merely build an
    // assignment here...
    int			button;
    int			y;
    ACTION_NAMES	action;
    
    action = selectAssignment(menu_generic, button);

    // Wait for the action to go high so we don't get
    // another trigger for USE...
    while (ctrl_rawpressed(button));

    switch (button)
    {
	case BUTTON_SELECT:
	{
	    bool	found = false;
	    int 	test = 0;
	    BUF		buf;

	    if (action == ACTION_NONE)
		break;

	    // Perform a remap of A or B.
	    for (y = 3; y <= 19; y++)
		gfx_cleartextline(y);
	    writeStatusLine();
	    buf.sprintf("Bind %s to?  ", glb_actiondefs[action].name);
	    msg_report(buf);

	    while (!found)
	    {
		for (test = BUTTON_A; test <= BUTTON_Y; test++)
		{
		    if (ctrl_hit(test))
		    {
			found = true;
			break;
		    }
		}
	    }
	    // We bind it to test.
	    if (action != ACTION_NONE)
		glb_actionbuttons[test] = action;

	    // Report appropriate message.
	    if (test == BUTTON_START ||
		test == BUTTON_SELECT)
	    {
		// Cancel.
		msg_report("Aborted.  ");
	    }
	    else
	    {
		msg_report("Bound.  ");
	    }

	    // But don't process...
	    action = ACTION_NONE;
	    
	    break;
	}

	case BUTTON_START:
	case BUTTON_B:
	// Quick bind of X & Y works poorly with the default of one
	// of them being the verb list - you want to hit it again to
	// make the verb list go bye-bye but that then rebinds!
	case BUTTON_X:
	case BUTTON_Y:
	    // Abort operation.
	    action = ACTION_NONE;
	    break;

	case BUTTON_A:
	    // Force the action
	    break;

	case BUTTON_L:
	case BUTTON_R:
	    // Immediate remap.
	    if (action != ACTION_NONE)
		glb_actionbuttons[button] = action;
	    action = ACTION_NONE;
	    break;
    }

    // Now, rebuild our scene & status line...
    for (y = 3; y <= 19; y++)
	gfx_cleartextline(y);
    writeStatusLine();

    return action;
}

bool
processAction(ACTION_NAMES action, SPELL_NAMES forcespell, MOB *avatar)
{
    int			dx, dy;

    bool		done = false, success;
    
    switch (action)
    {
	case ACTION_NONE:
	    break;

	case ACTION_EQUIP:
	    UT_ASSERT(0);
	    break;

	case ACTION_DEQUIP:
	    UT_ASSERT(0);
	    break;

	case ACTION_DROP:
	    UT_ASSERT(0);
	    break;

	case ACTION_INVENTORY:
	    done = useInventory(avatar, false);
	    break;

	case ACTION_VERBLIST:
	    action = processVerbList();
	    if (action != ACTION_NONE && action != ACTION_VERBLIST)
		done = processAction(action, SPELL_NONE, avatar);
	    break;

	// Special case: ACTION_OPTIONS...
	case ACTION_OPTIONS:
	case ACTION_HELP:
	{
	    if (action == ACTION_OPTIONS)
		processOptions();
	    else
		processHelp();

	    // Now, rebuild our scene & status line...
	    int		y;

	    for (y = 3; y <= 19; y++)
		gfx_cleartextline(y);
	    writeStatusLine();
	    done = false;
	    break;
	}

	case ACTION_OPEN:
	    success = msg_askdir("Open in what direction?", dx, dy, avatar->canMoveDiabolically());

	    if (success)
	    {
		done = avatar->actionOpen(dx, dy);
	    }
	    break;

	case ACTION_CLOSE:
	    // Prompt direction, close that door.
	    success = msg_askdir("Close in what direction?", dx, dy, avatar->canMoveDiabolically());

	    if (success)
	    {
		done = avatar->actionClose(dx, dy);
	    }
	    break;

	case ACTION_WISH:
#ifdef iPOWDER
	    if (!glbWizard)
	    {
		if (hamfake_isunlocked())
		{
		    if (gfx_yesnomenu("Enable Wishing?"))
		    {
			glbWizard = true;
		    }
		}
		else
		{
		    if (gfx_yesnomenu("Unlock Wishing?"))
		    {
			attemptunlock();
			// We can't set wizard here as we don't know
			// when/if they succeed in the unlock.
		    }
		}
	    }
#endif

	    if (glbStressTest || !glbWizard)
	    {
		// A couple of friendly rumours to justify this 
		// option to those who do not know the super sekrit
		// password.
		const char *wizlist[] =
		{
		    "%U <wish> %p knew the wizard password.",
		    "If wishes were slugs, the slime pits would be full.",
		    "%U <wish> %p had some more pickled slugs.  Yum!",
		    "%U <wish> very, very hard.  Nothing happens.",
		    "If wishes were kiwis, we'd all be dead by now.",
		    "If wishes were granted, this game would be easier.",
		    "%U <wish> %p knew the purpose of the Y-rune.",
		    "A flock of bats, a quarry of cockatrices.  %U <wish> %p were at home.",
		    "%U <wish> for advice.  %U <hear> a fighter's voice: \"It's the mushroom fume making us weary.  We need lighter armour to keep in shape.\"",
		    "%U <wish> for advice.  %U <hear> a priest's voice: \"It's no use going down there.  The blue dragons are everywhere.  I haven't seen them, but I *know* it.\"",
		    "%U <wish> for advice.  %U <hear> a wizard's voice: \"The shallowest part of the dungeon has some wicked magical energies running in it.  I swear I didn't fumble and cast it myself!\"",
		    "%U <wish> for advice.  %U <hear> a thief's voice: \"They come in packs of three and sparkle so loud you can hear them through four meters of thick gravel!\"",
		    "%U <wish> for advice.  %U <hear> a thief's voice: \"Invisible == Invincible?  I wonder...\"",
		    "%U <wish> for advice.  %U <hear> an adventurer's voice: \"If the water of the lake freezes: Run!\"",
		    "%U <wish> for advice.  %U <hear> a necromancer's voice: \"Be content with the zombies, disturb not the middle room!\"",
		    "%U <wish> for advice.  %U <hear> a thief's voice: \"The kiwi's are hiding something!\"",
		    
		    0
		};
		avatar->formatAndReport(rand_string(wizlist));
		
		// Yes, this uses a turn.  I am cruel that way.
		done = true;
	    }
	    else
	    {
		done = makeAWish(avatar);
	    }
	    break;

	case ACTION_FIRE:
	{
	    // Find the fire direction...
	    int			tx, ty;
	    int			dx, dy, dz;

	    done = true;
	    avatar->reportMessage("Fire where?");
	    
	    tx = avatar->getX();
	    ty = avatar->getY();
	    if (gfx_selectdirection(tx, ty, dx, dy, dz))
	    {
		msg_clear();

		done = avatar->actionFire(dx, dy, dz);
	    }
	    else
	    {
		// Cancelled...
		msg_clear();
		done = false;
	    }
	    break;
	}

	case ACTION_MOVE:
	{
	    // Find the movement direction...
	    int			dx, dy, dz, tx, ty;

	    done = true;
	    avatar->reportMessage("Move where?");
	    
	    tx = avatar->getX();
	    ty = avatar->getY();
	    if (gfx_selectdirection(tx, ty, dx, dy, dz))
	    {
		msg_clear();

		if (dz)
		{
		    if (dz > 0)
			done = avatar->actionClimbUp();
		    else
			done = avatar->actionClimbDown();
		}
		else
		{
		    if (dx && dy && !avatar->canMoveDiabolically())
		    {
			msg_clear();
			msg_report("You cannot move in that fashion.");
		    }
		    else
			done = avatar->actionBump(dx, dy);
		}
	    }
	    else
	    {
		// Cancelled...
		msg_clear();
		done = false;
	    }
	    break;
	}

	case ACTION_BREATHE:
	{
	    // Find the breath direction...
	    int			tx, ty;
	    int			dx, dy, dz;

	    done = true;
	    avatar->reportMessage("Breathe where?");
	    
	    tx = avatar->getX();
	    ty = avatar->getY();
	    if (gfx_selectdirection(tx, ty, dx, dy, dz))
	    {
		msg_clear();

		done = avatar->actionBreathe(dx, dy);
	    }
	    else
	    {
		// Cancelled...
		msg_clear();
		done = false;
	    }
	    break;
	}

	case ACTION_LOCK:
	    // Prompt dir, lock.
	    break;

	case ACTION_CLIMB:
	    // Try to climb the square we are on.
	    done = avatar->actionClimb();
	    break;

	case ACTION_PICKUP:
	{
	    ITEMSTACK		 stack;
	    bool		 submerged;
	    ITEM		*item;

	    submerged = avatar->hasIntrinsic(INTRINSIC_INPIT) ||
			avatar->hasIntrinsic(INTRINSIC_SUBMERGED);
	    
	    // Pick up from our square.
	    glbCurLevel->getItemStack(stack, avatar->getX(), avatar->getY(),
					submerged);

	    stack.reverse();

	    if (stack.entries() == 0)
		item = 0;
	    else if (stack.entries() == 1)
		item = stack(0);
	    else
	    {
		// Ask the user what to pick up.
		avatar->reportMessage("Pick up what?");
		
		char			**list;
		int			 i, n, aorb, select;

		n = stack.entries();
		list = new char *[n+1];
		for (i = 0; i < n; i++)
		{
		    list[i] = stack(i)->getName(false).strdup();
		}
		list[i] = 0;

		// Select the item to pickup
		select = gfx_selectmenu(3, 3, (const char **) list, aorb);
		if (select < 0 || aorb)
		    item = 0;
		else
		    item = stack(select);

		// Free the list.
		for (i = 0; i < n; i++)
		    free(list[i]);
		delete [] list;

		// And clear it...
		for (i = 3; i < 18; i++)
		{
		    gfx_cleartextline(i);
		}

		if (!item)
		{
		    avatar->reportMessage("You decide against it.");
		    break;
		}
	    }
	    
	    done = avatar->actionPickUp(item);
	    break;
	}

	case ACTION_QUAFF:
	case ACTION_EAT:
	{
	    ITEMSTACK		 stack;
	    bool		 submerged;
	    ITEM		*item, *newitem;

	    submerged = avatar->hasIntrinsic(INTRINSIC_INPIT) ||
			avatar->hasIntrinsic(INTRINSIC_SUBMERGED);
	    
	    // Eat from our square.
	    glbCurLevel->getItemStack(stack, avatar->getX(), avatar->getY(),
					submerged);

	    stack.reverse();

	    if (stack.entries() == 0)
		item = 0;
	    else if (stack.entries() == 1)
		item = stack(0);
	    else
	    {
		// Ask the user what to pick up.
		avatar->reportMessage("Eat what?");
		
		char			**list;
		int			 i, n, aorb, select;

		n = stack.entries();
		list = new char *[n+1];
		for (i = 0; i < n; i++)
		{
		    list[i] = stack(i)->getName(false).strdup();
		}
		list[i] = 0;

		// Select the item to pickup
		select = gfx_selectmenu(3, 3, (const char **) list, aorb);
		if (select < 0 || aorb)
		    item = 0;
		else
		    item = stack(select);

		// Free the list.
		for (i = 0; i < n; i++)
		    free(list[i]);
		delete [] list;

		// And clear it...
		for (i = 3; i < 18; i++)
		{
		    gfx_cleartextline(i);
		}

		if (!item)
		{
		    avatar->reportMessage("You decide against it.");
		    break;
		}
	    }

	    if (!item)
	    {
		avatar->reportMessage("There is nothing here to eat!");
		break;
	    }

	    // First, pick the item up...
	    newitem = 0;
	    done = avatar->actionPickUp(item, &newitem);

	    // Determine if we picked it up.
	    if (newitem && newitem->isInventory())
	    {
		// Attempt to eat...
		done |= avatar->actionEat(newitem->getX(), newitem->getY());
	    }
	    break;
	}

	case ACTION_SWAP:    
	    success = msg_askdir("Swap places in what direction?", dx, dy, avatar->canMoveDiabolically());

	    if (success)
	    {
		done = avatar->actionSwap(dx, dy);
	    }
	    break;

	case ACTION_RELEASE:
	    done = avatar->actionReleasePossession();
	    break;

	case ACTION_SLEEP:
	    done = avatar->actionSleep();
	    break;

	case ACTION_FORGET:
	{
	    SKILL_NAMES		*skill_list;
	    SPELL_NAMES		*spell_list;
	    const char		**options;
	    int			 i, n, choice, aorb;
	    SPELL_NAMES		 spell = SPELL_NONE;
	    SKILL_NAMES		 skill = SKILL_NONE;

	    n = 0;
	    for (i = 0; i < NUM_SKILLS; i++)
	    {
		if (avatar->hasSkill((SKILL_NAMES)i))
		    n++;
	    }

	    for (i = 0; i < NUM_SPELLS; i++)
	    {
		if (avatar->hasSpell((SPELL_NAMES)i))
		    n++;
	    }

	    if (!n)
	    {
		avatar->reportMessage("You have nothing to forget.");
		break;
	    }

	    avatar->reportMessage("Forget what?");
	    
	    skill_list = new SKILL_NAMES[n];
	    spell_list = new SPELL_NAMES[n];
	    options = new const char *[n+1];
	    
	    n = 0;
	    for (i = 0; i < NUM_SKILLS; i++)
	    {
		if (avatar->hasSkill((SKILL_NAMES)i))
		{
		    skill_list[n] = (SKILL_NAMES) i;
		    spell_list[n] = SPELL_NONE;
		    options[n] = glb_skilldefs[i].name;
		    n++;
		}
	    }

	    for (i = 0; i < NUM_SPELLS; i++)
	    {
		if (avatar->hasSpell((SPELL_NAMES)i))
		{
		    spell_list[n] = (SPELL_NAMES) i;
		    skill_list[n] = SKILL_NONE;
		    options[n] = glb_spelldefs[i].name;
		    n++;
		}
	    }
	    options[n] = 0;

	    // Get the forget choice.
	    static int lastforget = 0;
	    
	    choice = gfx_selectmenu(12, 4, options, aorb, lastforget);

	    if (aorb)
		choice = -1;

	    msg_clear();

	    {
		int		y;
		for (y = 3; y < 19; y++)
		    gfx_cleartextline(y);
	    }
	    if (choice >= 0)
	    {
		lastforget = choice;
		skill = skill_list[choice];
		spell = spell_list[choice];

		// Verify the player wants to forget this
		BUF		forgetprompt;
		if (skill != SKILL_NONE)
		    forgetprompt.sprintf("Forget %s?", glb_skilldefs[skill].name);
		else
		    forgetprompt.sprintf("Forget %s?", glb_spelldefs[spell].name);

		if (!gfx_yesnomenu(forgetprompt))
		    choice = -1;
	    }

	    if (choice >= 0)
	    {
		done = avatar->actionForget(spell, skill);
	    }

	    delete [] skill_list;
	    delete [] spell_list;
	    delete [] options;

	    break;
	}

	case ACTION_SEARCH:
	    // Look all around us...
	    done = avatar->actionSearch();
	    break;

	case ACTION_JUMP:
	    success = msg_askdir("Jump where?", dx, dy, avatar->canMoveDiabolically());

	    if (success)
	    {
		done = avatar->actionJump(dx, dy);
	    }
	    break;

	case ACTION_WAIT:
	    // Do nothing for a turn...
	    done = true;
	    break;

	case ACTION_RUN:
	    if (avatar->hasIntrinsic(INTRINSIC_BLIND))
	    {
		avatar->formatAndReport("%U <decide> against running around blindly.");
		break;
	    }
	    // Note we prohibit diagonal running as our run code
	    // doesn't like it.  Er, that sounds suspiciously like
	    // laziness!  I mean, because moving diabolically requires
	    // utmost concentration so cannot be done in a haphazard
	    // manner!
	    success = msg_askdir("Run where?", dx, dy, false);

	    if (success && (dx || dy))
	    {
		glbAutoRunDX = dx;
		glbAutoRunDY = dy;
		glbAutoRunEnabled = true;
		glbAutoRunOpenSpace = true;
		// Note we do not consume a turn here, but instead
		// let the main loop pick up those global variables.
		// We do wait for the direct to go high as we
		// don't want our direction key to interrupt the run.
#ifndef HAS_KEYBOARD
		while (ctrl_anyrawpressed())
		    hamfake_awaitEvent();
#endif
		
	    }
	    break;

	case ACTION_LOOK:
	{
	    int			tx, ty;
	    int			oldoverlay;
	    int			aorb = 0;

	    // This needs to be cleaned up, as it can be of sync
	    // with how map determines the invisible avatar.
	    // Too bad I'm relaxing in the ANA Signet Lounge drinking free
	    // beer to care.
	    // The code that used to be here has been elided as I have
	    // since realized it is wiser just to read the overlay
	    // from the screen itself.

	    avatar->reportMessage("Where do you want to look?");
	    
	    tx = avatar->getX();
	    ty = avatar->getY();
	    
	    // Rather than trying to guess our overlay tile,
	    // just read it directly from the screen.
	    oldoverlay = gfx_getoverlay(tx, ty);
	    while (1)
	    {
		bool		stylus, select;
		int		ox, oy;

		ox = tx;
		oy = ty;

		select = gfx_selecttile(tx, ty, true, &oldoverlay, &aorb, &stylus);
		// Complete selection on a stylus press.
		if (select && !stylus)
		    break;
		// Finish stylus selction if they select themselves
		if (select && stylus && (tx == ox) && (ty == oy))
		    break;

		msg_clear();
		// Describe that square...
		if (!glbCurLevel->describeSquare(tx, ty, 
			    avatar->hasIntrinsic(INTRINSIC_BLIND),
			    true))
		{
		    // Nothing to report, so say so..
		    avatar->reportMessage("There is nothing of interest there.");
		}
	    }
	    msg_clear();
    
	    // Give extended description of mobs.
	    MOB		*mob;

	    mob = glbCurLevel->getMob(tx, ty);
	    // We only do full describe if [A] is hit.
	    if (!aorb && mob)
	    {
		if (mob->isAvatar() || avatar->getSenseType(mob) == SENSE_SIGHT)
		{
		    // We can see the avatar, we can see what they are wearing.
		    // Build the long description text.
		    mob->viewDescription();
		}
	    }
	    
	    break;
	}

	case ACTION_NAME:
	{
	    int			tx, ty;

	    avatar->reportMessage("Who do you want to name?");
	    
	    tx = avatar->getX();
	    ty = avatar->getY();
	    if (gfx_selecttile(tx, ty))
	    {
		MOB		*mob;

		mob = glbCurLevel->getMob(tx, ty);

		// You can't name what isn't there.  You shouldn't
		// find out about invisible mobs by naming.
		if (!mob || !avatar->canSense(mob))
		{
		    avatar->reportMessage("There is noone there to name.");
		}
		else if (mob->hasIntrinsic(INTRINSIC_UNIQUE))
		{
		    avatar->reportMessage("You cannot rename uniques.");
		}
		else
		{
		    char		buf[50];

		    msg_clear();

		    gfx_printtext(0, 0, "New Name? ");
		    input_getline(buf, 19, 10, 0, 23, 8);
		    
		    {
			int		y;
			for (y = 0; y < 18; y++)
			    gfx_cleartextline(y);
		    }

		    // Special case: renaming the mob will rename
		    // the glbAvatarName.
		    if (mob->isAvatar())
		    {
			strcpy(glbAvatarName, buf);
		    }
		    else
			mob->christen(buf);
		}
	    }
	    else
	    {
		avatar->reportMessage("You decide against it.");
	    }
	    break;
	}

	case ACTION_COMMAND:
	{
	    MOBSTACK		pets;

	    glbCurLevel->getPets(pets, avatar, true);

	    if (!pets.entries())
	    {
		// Failed as no pets.
		avatar->reportMessage("You have no visible pets to command.");
		break;
	    }

	    char		**menu;
	    static int		 lastchoice = 0;
	    int			 i, choice, aorb;
	    MOB			*target = 0;
	    BUF			 buf;

	    menu = new char *[pets.entries() + 2];

	    for (i = 0; i < pets.entries(); i++)
	    {
		menu[i] = pets(i)->getName(false, false, true, true).strdup();
	    }

	    // Add in all.
	    menu[i++] = "All Pets";
	    menu[i++] = 0;

	    // Let the user pick.
	    gfx_printtext(0, 3, "Command Which Pet?");
	    choice = gfx_selectmenu(5, 4, (const char **) menu, 
				    aorb, lastchoice);

	    if (choice != -1)
		lastchoice = choice;

	    // Erase...
	    for (i = 3; i < 19; i++)
		gfx_cleartextline(i);

	    // Free the menu
	    for (i = 0; i < pets.entries(); i++)
	    {
		free(menu[i]);
	    }
	    delete [] menu;

	    if (aorb || choice == -1)
	    {
		// B is hit, so cancel...
		break;
	    }

	    const char	*commandlist[] =
	    {
		"Kill...",
		"Guard Me",
		"Stay",
		0
	    };
	    static int lastcommand = 0;
	    int	command;
	    gfx_printtext(0, 3, "Command?");
	    command = gfx_selectmenu(5, 4, commandlist, aorb, lastcommand);

	    if (command != -1)
		lastcommand = command;

	    // Erase...
	    for (i = 3; i < 19; i++)
		gfx_cleartextline(i);

	    // Setup the target.
	    switch (command)
	    {
		case 0:		// Kill...
		{
		    int			tx, ty;

		    avatar->reportMessage("Kill who?");

		    tx = avatar->getX();
		    ty = avatar->getY();
		    if (!gfx_selecttile(tx, ty))
		    {
			msg_clear();
			break;
		    }

		    target = glbCurLevel->getMob(tx, ty);

		    // Can only target people we know!
		    if (!avatar->canSense(target))
			target = 0;

		    msg_clear();
		    break;
		}
		case 1:		// Guard Me
		case 2:		// Stay
		    break;
	    }

	    // If we are to kill and have no target, abort.
	    if (command == 0 && !target)
	    {
		avatar->reportMessage("You decide against violence.");
		break;
	    }

	    // Apply the command to all critters.
	    for (i = 0; i < pets.entries(); i++)
	    {
		// Skip unless it is the right creature, or
		// the user picked all.  God this is disgusting code.
		if (choice != pets.entries() &&
		    choice != i)
		    continue;

		BUF	petname = pets(i)->getName(false, false, true, true);
		switch (command)
		{
		    case 0:		// Kill!
			buf.sprintf("%s kill!", petname.buffer());
			avatar->reportMessage(buf);
			pets(i)->setAIFSM(AI_FSM_ATTACK);
			pets(i)->setAITarget(target);
			break;

		    case 1:		// Guard me!
			buf.sprintf("%s guard me!", petname.buffer());
			avatar->reportMessage(buf);
			pets(i)->setAIFSM(AI_FSM_GUARD);
			pets(i)->clearAITarget();
			break;

		    case 2:		// Stay!
			buf.sprintf("%s stay!", petname.buffer());
			avatar->reportMessage(buf);
			pets(i)->setAIFSM(AI_FSM_STAY);
			pets(i)->clearAITarget();
			break;
		}
	    }
	    done = true;
	    
	    break;
	}

	case ACTION_ZAP:
	{
	    const char		*m[NUM_SPELLS + 3];
	    u8			 stripbutton[NUM_SPELLS+3];
	    SPELL_NAMES		 s[NUM_SPELLS + 3];
	    SPELL_NAMES		 spell;
	    int			 i, j, choice, aorb;
	    int			 dx, dy, dz;
	    static int		 lastchoice = 0;
	    BUF			 buf;
	    
	    // Cast a magic spell
	    j = 0;
	    FOREACH_SPELL(spell)
	    {
		if (avatar->hasSpell(spell))
		{
		    s[j] = spell;
		    m[j] = glb_spelldefs[spell].name;
		    if (!avatar->canCastSpell(spell))
		    {
			char *mi = new char[strlen(m[j])+10];
			sprintf(mi, "%s (!)", m[j]);
			m[j] = mi;
		    }
		    stripbutton[j] = action_packStripButton(spell);
		    j++;
		}
	    }

	    // Include cancel option to cast nothing.
	    s[j] = SPELL_NONE;
	    m[j] = "cancel";
	    stripbutton[j] = action_packStripButton(ACTION_NONE);
	    j++;

	    m[j] = 0;

	    if (j <= 1)
	    {
		avatar->reportMessage("You know no spells!");
		break;
	    }

	    spell = forcespell;
	    
	    choice = -1;
	    while (spell == SPELL_NONE)
	    {
		gfx_printtext(0, 3, "Which spell?");

		choice = gfx_selectmenu(5, 4, m, aorb, lastchoice, true,
				false, stripbutton, glb_globalactionstrip);

		// Add cancel option.
		if (choice != -1 && s[choice] == SPELL_NONE)
		    choice = -1;

		if (choice != -1)
		{
		    lastchoice = choice;
		}

		if (aorb == BUTTON_SELECT && choice != -1)
		{
		    // Give long description..
		    encyc_viewSpellDescription((SPELL_NAMES) s[choice]);
		}

		if (aorb == BUTTON_B || choice == -1)
		{
		    // B is hit, so cancel...
		    choice = -1;
		    break;
		}
		// Check for selection
		if (aorb == BUTTON_A)
		    break;
	    }
	    // Erase...
	    for (i = 3; i < 19; i++)
		gfx_cleartextline(i);

	    for (i = 0; i < j; i++)
	    {
		if (s[i] != SPELL_NONE &&
		    m[i] != glb_spelldefs[s[i]].name)
		{
		    delete [] m[i];
		}
	    }

	    if (spell == SPELL_NONE && choice == -1)
		break;

	    if (spell == SPELL_NONE)
		spell = s[choice];

	    // This may seem silly, but you may be using
	    // quick keys
	    if (!avatar->hasSpell(spell))
	    {
		avatar->formatAndReport("%U <do> not know that spell.");
		break;
	    }

	    // Now, cast the spell...  First, get a direction...
	    dx = dy = dz = 0;
	    if (glb_spelldefs[spell].needsdir)
	    {
		int			tx, ty;

		avatar->formatAndReport("Cast %B1 in direction?",
			    glb_spelldefs[spell].name);
	    
		tx = avatar->getX();
		ty = avatar->getY();
		if (!gfx_selectdirection(tx, ty, dx, dy, dz))
		{
		    msg_clear();
		    break;
		}

		// Chose direction, clear message...
		msg_clear();
	    }
	    else if (glb_spelldefs[spell].needstarget)
	    {
		int			tx, ty;

		avatar->formatAndReport("Cast %B1 at?",
				    glb_spelldefs[spell].name);

		tx = avatar->getX();
		ty = avatar->getY();
		if (!gfx_selecttile(tx, ty))
		{
		    msg_clear();
		    break;
		}
			
		// Choose direction, clear message...
		dx = tx - avatar->getX();
		dy = ty - avatar->getY();
		msg_clear();
	    }
	    
	    // A hit, so cast the spell.  Check if we have the MP.
	    if (avatar->getMP() < glb_spelldefs[spell].mpcost)
	    {
		buf.sprintf("You need %d magic points!",
			glb_spelldefs[spell].mpcost);
		avatar->reportMessage(buf);
		break;
	    }

	    // Check for HP...
	    if (avatar->getHP() < glb_spelldefs[spell].hpcost)
	    {
		buf.sprintf("You need %d health points!",
			glb_spelldefs[spell].hpcost);
		avatar->reportMessage(buf);
		break;
	    }

	    // Check for XP...
	    if (avatar->getExp() < glb_spelldefs[spell].xpcost)
	    {
		buf.sprintf("You need %d free experience points!",
			glb_spelldefs[spell].xpcost);
		avatar->reportMessage(buf);
		break;
	    }
	    
	    // NOw, cast the spell...
	    done = avatar->actionCast(spell, dx, dy, dz);
	    
	    break;
	}

	case ACTION_USE:
	{
	    done = useInventory(avatar, true);
	    break;
	}

	case ACTION_PRAY:
	{
	    done = avatar->actionPray();
	    break;
	}

	case ACTION_HISTORY:
	{
	    // History doesn't consume time (duh)
	    done = false;

	    msg_displayhistory();
	    break;
	}

	case ACTION_SAFEWALK:
	{
	    // Selecting this will toggle the current mode
	    // permamently (as opposed to binding which lets
	    // you just enter and leave)
	    done = false;
	    glbSafeWalk = !glbSafeWalk;
	    if (glbSafeWalk)
		avatar->formatAndReport("%U <tread> carefully through the corridors.");
	    else
		avatar->formatAndReport("%U <prepare> for battle at a moments notice.");
	    break;
	}

	case ACTION_MINIMAP:
	{
	    // Minimap doesn't consume time.
	    done = false;

	    // Display, wait for keypress, return.
	    gfx_displayminimap(glbCurLevel);

	    msg_awaitaccept();

	    gfx_hideminimap();

	    break;
	}

	default:
	    UT_ASSERT(0);
	    break;
    }

    return done;
}

bool
processDirectionStylus(int &dx, int &dy, bool &walkonly, bool &xmajor, bool allowdiag)
{
    int		x, y;

    walkonly = true;
    xmajor = false;

    // If the user has turned off screen tapping, we don't move
    // when the screen is hit.
    if (!glbScreenTap)
	return false;

    hamfake_getstyluspos(x, y);
    x -= 15*TILEWIDTH;
    y -= 10*TILEHEIGHT;
    // We want a very large active area!
    // Indeed, argueably there should be no limit!
    // The 8 pixel border ensures the DS has a 16 pixel border
    // which can be used for the side buttons.
    // But, if we have the action bar disabled, this dead zone is of
    // no use to us!
    if (glbActionBar)
	if (ABS(x) > 14*TILEWIDTH || ABS(y) > 9*TILEHEIGHT)
	    return false;

    dx = dy = 0;

    // This means search.
    if (ABS(x) < TILEWIDTH && ABS(y) < TILEHEIGHT)
	return true;

    // Check if we click on one of the 8 squares around us.  This means
    // bump.
    if (ABS(x) <= 3*TILEWIDTH  && ABS(y) < TILEHEIGHT)
    {
	walkonly = false;
    }
    else if (ABS(x) < TILEWIDTH && ABS(y) <= 3*TILEHEIGHT)
    {
	walkonly = false;
    }
    else if (allowdiag && ABS(x) <= 3*TILEWIDTH && ABS(y) < 3*TILEHEIGHT)
    {
	walkonly = false;
    }

    if (ABS(x) > ABS(y))
    {
	xmajor = true;
	dx = SIGN(x);
	// We want values less than 8 to be zero
	y /= TILEHEIGHT;
	dy = SIGN(y);
    }
    else
    {
	xmajor = false;
	x /= TILEWIDTH;
	dx = SIGN(x);
	dy = SIGN(y);
    }

    return true;
}

void
checkMiniMapButtons()
{
    int		i;

    // Check for new map request...
    for (i = 0; glb_mapbuttons[i] != NUM_BUTTONS; i++)
    {
	if (glb_actionbuttons[glb_mapbuttons[i]] == ACTION_MINIMAP &&
	    ctrl_rawpressed(glb_mapbuttons[i]))
	{
	    bool		minimap = false;

	    // L button goes back, all other ones go forward.
	    if (!msg_restore(glb_mapbuttons[i] == BUTTON_L))
	    {
		minimap = true;
		gfx_displayminimap(glbCurLevel);
	    }

	    // Wait for it to be released...
	    while (ctrl_rawpressed(glb_mapbuttons[i]));

	    if (minimap)
		gfx_hideminimap();
	}
    }
}


void
checkModifierButtons()
{
    // Update our safewalk status.
    glbToggleSafeWalk = false;
    if (hamfake_getKeyModifiers() & GFX_KEYMODCTRL)
    {
	glbToggleSafeWalk = true;
    }

    glbToggleSwap = false;
    if (hamfake_getKeyModifiers() & GFX_KEYMODSHIFT)
    {
	glbToggleSwap = true;
    }

    glbToggleJump = false;
    if (hamfake_getKeyModifiers() & GFX_KEYMODALT)
    {
	glbToggleJump = true;
    }

    int		i;

    // Check for new map request...
    for (i = 0; glb_mapbuttons[i] != NUM_BUTTONS; i++)
    {
	if (glb_actionbuttons[glb_mapbuttons[i]] == ACTION_SAFEWALK &&
	    ctrl_rawpressed(glb_mapbuttons[i]))
	{
	    glbToggleSafeWalk = true;
	}
    }
}

void
processAllInput(ACTION_NAMES &action, SPELL_NAMES &spell, int &dx, int &dy)
{
    STYLUSLOCK	styluslock(REGION_BOTTOMBUTTON | REGION_SIDEBUTTON);
    
    // Check for forced actions.
    if (glbAutoRunEnabled)
    {
	if (!MOB::getAvatar())
	    glbAutoRunEnabled = false;

	if (hamfake_peekKeyPress())
	    glbAutoRunEnabled = false;

#ifndef HAS_KEYBOARD
	if (ctrl_anyrawpressed())
	    glbAutoRunEnabled = false;
#endif

	if (!glbAutoRunEnabled)
	    msg_report("Running interrupted.");
    }

    if (glbAutoRunEnabled)
    {
	MOB		*avatar = MOB::getAvatar(), *mob;
	int		 dx, dy, ax, ay, cx, cy;

	ax = avatar->getX();
	ay = avatar->getY();

	// Verify there are no non-friendly creatures around us.
	FORALL_4DIR(dx, dy)
	{
	    mob = glbCurLevel->getMob(ax+dx, ay+dy);

	    if (mob && avatar->getAttitude(mob) != ATTITUDE_FRIENDLY)
	    {
		if (glbAutoRunEnabled)
		    avatar->formatAndReport("Seeing a foe, %U <stop> running.");
		glbAutoRunEnabled = false;
	    }
	}

	dx = glbAutoRunDX;
	dy = glbAutoRunDY;
	cx = !dx;
	cy = !dy;
	
	// I am being *extraordinarily* kind here and not letting
	// people run into acid pools.  I doubt players will recognize
	// it as an act of kindness, though.
	if (avatar->canMoveDelta(dx, dy, true, false, true))
	{
	    // We can move forward.  It is required that we cannot
	    // move to either side or we have a branch and must abort.
	    if (avatar->canMoveDelta(cx, cy, true, false, true) ||
		avatar->canMoveDelta(-cx, -cy, true, false, true))
	    {
		if (!glbAutoRunOpenSpace)
		{
		    if (glbAutoRunEnabled)
			avatar->formatAndReport("%U <stop> at an open area.");
		    glbAutoRunEnabled = false;    
		}
	    }
	    else
	    {
		// We have entered a corridor, so next time we are in
		// open space we'll abort.
		glbAutoRunOpenSpace = false;
	    }
	}
	else
	{
	    // We can't move forward.  If we can move in only one
	    // of the cross directions, yay, that is our new direction
	    // Otherwise we abort confused.
	    // If we have not yet left open space we do not turn!
	    // Otherwise in a closed room we'd happily run in circles,
	    // which is cute, but not what we really want.
	    if (glbAutoRunOpenSpace)
	    {
		if (glbAutoRunEnabled)
		    avatar->formatAndReport("%U <stop> to take %R bearings.");
		glbAutoRunEnabled = false;
	    }
		
	    if (avatar->canMoveDelta(cx, cy, true, false, true))
	    {
		if (!avatar->canMoveDelta(-cx, -cy, true, false, true))
		{
		    glbAutoRunDX = cx;
		    glbAutoRunDY = cy;
		}
		else
		{
		    if (glbAutoRunEnabled)
			avatar->formatAndReport("%U <stop> at a crossroads.");
		    glbAutoRunEnabled = false;
		}
	    }
	    else if (avatar->canMoveDelta(-cx, -cy, true, false, true))
	    {
		glbAutoRunDX = -cx;
		glbAutoRunDY = -cy;
	    }
	    else
	    {
		if (glbAutoRunEnabled)
		    avatar->formatAndReport("%U <stop> at a dead end.");
		glbAutoRunEnabled = false;
	    }
	}
    }

    // If we still have auto run, force us to bump in the current direction.
    if (glbAutoRunEnabled)
    {
	action = ACTION_NONE;
	spell = SPELL_NONE;
	dx = glbAutoRunDX;
	dy = glbAutoRunDY;
	return;
    }

    hamfake_enableexternalactions(true);
    while (1)
    {
	// Wait for a new frame...
	while (!gfx_isnewframe());

	if (hamfake_forceQuit())
	    break;

	bool		forceaction = false;
	bool		walkonly = false;
	bool		xmajor = false;
	MOB		*avatar = MOB::getAvatar();

	action = ACTION_NONE;
	dx = dy = 0;

	// We only work when the avatar lives.
	if (!avatar)
	{
	    return;
	}

	// Check for an inventory request.
	if (ctrl_hit(BUTTON_SELECT))
	{
	    action = ACTION_INVENTORY;
	    return;
	}
	if (ctrl_hit(BUTTON_START))
	{
	    action = ACTION_VERBLIST;
	    return;
	}

	checkMiniMapButtons();

	int		 actionbutton = NUM_BUTTONS;

	// Move our dude if the button is down and it is possible.
	ctrl_getdir(dx, dy, 0, avatar->canMoveDiabolically());
	// We do not want to clear our keyboard buffer here
	// because we want direction keys to repeat!!
	// hamfake_clearKeyboardBuffer();

	checkModifierButtons();
	if (dx || dy)
	{
	    // Trigger a walk mode..
	    if (glbToggleSafeWalk != glbSafeWalk)
	    {
		// Walking has no real sense of a major direction
		// so we just ensure we are sane for ortho cases
		if (dx)
		    xmajor = true;
		else
		    xmajor = false;
		forceaction = true;
		action = ACTION_WALK;
	    }
	    if (glbToggleSwap != glbSwap)
	    {
		if (dx)
		    xmajor = true;
		else
		    xmajor = false;
		forceaction = true;
		action = ACTION_SWAP;
	    }
	    if (glbToggleJump != glbJump)
	    {
		if (dx)
		    xmajor = true;
		else
		    xmajor = false;
		forceaction = true;
		action = ACTION_JUMP;
	    }
	}

#ifdef HAS_KEYBOARD
	// Get the keypress here.  Note that we will always consume
	// the keypress!  Our hope is that the previous tests, which
	// occur after the awaitEvent, will help us avoid losing
	// good keypresses here.
	int			keypress;

	keypress = hamfake_getKeyPress(false);
	if (forceaction)
	    keypress = 0;
	else
	    action = ACTION_NONE;

	switch (keypress)
	{
	    case GFX_KEYF1:
	    case GFX_KEYF2:
	    case GFX_KEYF3:
	    case GFX_KEYF4:
	    case GFX_KEYF5:
	    case GFX_KEYF6:
	    case GFX_KEYF7:
	    case GFX_KEYF8:
	    case GFX_KEYF9:
	    case GFX_KEYF10:
	    case GFX_KEYF11:
	    case GFX_KEYF12:
	    case GFX_KEYF13:
	    case GFX_KEYF14:
	    case GFX_KEYF15:
		// Use the mapped key.
		action_unpackStripButton(
			glb_globalactionstrip[keypress - GFX_KEYF1],
			action, spell);
		break;
	    case 'o':
		action = ACTION_OPEN;
		break;

	    case '&':
		action = ACTION_WISH;
		break;

	    case 'c':
		action = ACTION_CLOSE;
		break;

	    case 'e':
		action = ACTION_EAT;
		break;

	    case 'B':
		action = ACTION_BREATHE;
		break;

	    case '<':
	    case '>':
		action = ACTION_CLIMB;
		break;

	    case 'S':
		action = ACTION_SWAP;
		break;

	    case 'R':
		action = ACTION_SLEEP;
		break;

	    case 'q':
		action = ACTION_RELEASE;
		break;

	    case 'F':
		action = ACTION_FORGET;
		break;

	    case 'f':
		action = ACTION_FIRE;
		break;

	    case 'W':
		action = ACTION_MOVE;
		break;

	    case 's':
		action = ACTION_SEARCH;
		break;

	    case '.':
	    case '5':		// Num pad option.
	    case 'w':
		action = ACTION_WAIT;
		break;

	    case 'r':
		action = ACTION_RUN;
		break;

	    case 'x':
		action = ACTION_LOOK;
		break;

	    case 'z':
		action = ACTION_ZAP;
		break;

	    case 'J':
		action = ACTION_JUMP;
		break;

	    case 'm':
	    case 'X':
		action = ACTION_MINIMAP;
		break;

	    case 'C':
		action = ACTION_COMMAND;
		break;

	    case 'N':
		action = ACTION_NAME;
		break;

	    case '_':
		action = ACTION_PRAY;
		break;

	    case '?':
		action = ACTION_HELP;
		break;
	    case 'O':
		action = ACTION_OPTIONS;
		break;

	    case 'v':
		action = ACTION_HISTORY;
		break;

	    case 'V':
		action = ACTION_VERBLIST;
		break;

	    case ',':
	    case 'g':
		action = ACTION_PICKUP;
		break;

	    case 'p':
	    case '\x10':		// ^p
		msg_restore(true);
		break;

	    case GFX_KEYLMB:
		if (processDirectionStylus(dx, dy, walkonly, xmajor,
			    avatar->canMoveDiabolically()))
		{
		    if (!dx && !dy)
			action = ACTION_SEARCH;
		    else if (walkonly)
			action = ACTION_WALK;
		}
		break;
	}
#else
	if (!forceaction)
	{
	    for (int i = 0; glb_mapbuttons[i] != NUM_BUTTONS; i++)
	    {
		if (ctrl_hit(glb_mapbuttons[i]))
		    actionbutton = glb_mapbuttons[i];
	    }
	    if (actionbutton != NUM_BUTTONS)
		action = glb_actionbuttons[actionbutton];
	    else
		action = ACTION_NONE;

	    // We do not accept minimap actions at this juncture - they
	    // are handled outside this loop so we can use the press-and-hold
	    // paradigm like cool people.
	    if (action == ACTION_MINIMAP)
		action = ACTION_NONE;
	    // Likewise, safewalk is a toggle
	    if (action == ACTION_SAFEWALK)
		action = ACTION_NONE;

	    if (ctrl_hit(BUTTON_TOUCH))
	    {
		if (processDirectionStylus(dx, dy, walkonly, xmajor,
			    avatar->canMoveDiabolically()))
		{
		    if (!dx && !dy)
			action = ACTION_SEARCH;
		    else if (walkonly)
			action = ACTION_WALK;
		}
	    }
	}
#endif

	// If we are set to ACTION_WALK we want to clear out the dx/dy
	// appropriately if there is something for us to bump into.
	// Monsters do *not* trigger a collision as if they are hostile
	// we do want to come to a stop so the user can attack and if
	// they are friendly we will swap.
	if (action == ACTION_WALK)
	{
	    bool	blockdir = true;
	    if (dx && dy && avatar->canMoveDiabolically())
	    {
		// If we are able to move on diagonal, keep it!
		if (avatar->canMoveDelta(dx, dy,
				    false, false, false))
		{
		    blockdir = false;
		}
	    }
	    if (!blockdir)
	    {
	    }
	    else if (xmajor)
	    {
		// See if blocked in x.  IF so, zero x so we bump in
		// dy.  If not, zero dy.
		if (avatar->canMove(avatar->getX() + dx, avatar->getY(),
				    false, false, false))
		{
		    dy = 0;
		}
		else
		{
		    dx = 0;
		}
	    }
	    else
	    {
		// See if blocked in y.  IF so, zero y so we bump in
		// dx.  If not, zero dx.
		if (avatar->canMove(avatar->getX(), avatar->getY() + dy,
				    false, false, false))
		{
		    dx = 0;
		}
		else
		{
		    dy = 0;
		}
	    }

	    // If our direction is zeroed, clear the action
	    if (!dx && !dy)
		action = ACTION_NONE;
	}

	int buttonsel;
	if (action == ACTION_NONE && glbActionBar && styluslock.getbottombutton(buttonsel))
	{
	    action_unpackStripButton(glb_globalactionstrip[buttonsel],
				action, spell);
	}
	if (action == ACTION_NONE && spell == SPELL_NONE)
	{
	    int		iact, ispell;
	    hamfake_externalaction(iact, ispell);
	    action = (ACTION_NAMES) iact;
	    spell = (SPELL_NAMES) ispell;
	}

	// If we now have a valid action or dx/dy, return it!
	if (action != ACTION_NONE || dx || dy || spell != SPELL_NONE)
	{
	    return;
	}
    }
}

#ifdef iPOWDER

int fake_main();

void *
fakemainCB(void *)
{
    fake_main();

    return 0;
}

void
ipowder_main(const char *path)
{
    static THREAD		*powderthread;

    // Already started, don't restart!
    if (powderthread)
	return;

    hamfake_setdatapath(path);

    powderthread = THREAD::alloc();

    powderthread->start(fakemainCB, 0);
}

#define main fake_main

#endif

#ifdef MAPSTATS
void
buildMapStats()
{
    glbStressTest = true;
    glbMapStats = true;
    int			maxiter = 10000;
    int			level = 3;
    MAP			*map;
    FILE 		*fp;

    fp = fopen("mapstats.csv", "wt");

    MAP::printStatsHeader(fp);
    
    int			i;
    for (i = 0; i < maxiter; i++)
    {
	rebuild_all();
	MAP		*newlevel;
	newlevel = glbCurLevel;
	while (newlevel && newlevel->getDepth() != level)
	{
	    if (level < newlevel->getDepth())
		newlevel = newlevel->getMapUp();
	    else
		newlevel = newlevel->getMapDown();
	}
	if (newlevel->printStats(fp))
	    break;
    }

    fclose(fp);

    if (i == maxiter)
	exit(0);
}
#endif


int 
#ifndef _3DS
main(void)
#else
gba_main(void)
#endif
{
    int			lastturn = -1;
    int			howlongdead = 0;

    // Initialize button maps:
#ifdef iPOWDER
    glb_actionbuttons[BUTTON_A] = ACTION_NONE;
    glb_actionbuttons[BUTTON_B] = ACTION_NONE;
#else
    glb_actionbuttons[BUTTON_A] = ACTION_FIRE;
    glb_actionbuttons[BUTTON_B] = ACTION_ZAP;
#endif
    glb_actionbuttons[BUTTON_R] = ACTION_MINIMAP;
    glb_actionbuttons[BUTTON_L] = ACTION_MINIMAP;
    glb_actionbuttons[BUTTON_X] = ACTION_VERBLIST;
    glb_actionbuttons[BUTTON_Y] = ACTION_INVENTORY;

    resetDefaultActionStrip();

    rand_init();
    gfx_init();
    ctrl_init();
    msg_init();
    item_init();
    mob_init();
    mobref_init();
    speed_init();
    ai_init();
    name_init();
    artifact_init();

    // Ensure we are happy with our original tileset
    // We do this before hiscore_init as it will invoke loadOptions
    // and thus set the users desired tileset.
#ifndef _3DS
#ifdef iPOWDER
    gfx_settilesetmode(0, 4);	// 10 pixel
    gfx_settilesetmode(1, 3);	// 12 pixel
#else
    gfx_settilesetmode(0, 4);	// Akoi Meexx
#endif
#else
    gfx_settilesetmode(0, 0);	
#endif

    hiscore_init();

    // Reset our RNG if requested
    if (glbRNGValid)
    {
	// Mix in any other entropy we might have.
#if defined(USING_SDL) || defined(USING_DS) || defined(_3DS)
	glbRNGSeed ^= time(0);
#elif defined(_WIN32_WCE)
	glbRNGSeed ^= GetTickCount();
#endif
	
	rand_setseed(glbRNGSeed);
    }

#ifdef MAPSTATS
    gfx_setmode(0);
    buildMapStats();
    glbMapStats = false;
    glbStressTest = false;
    glbWizard = true;
#else

    // Run the intro screen.
    bool 		doload = false;
    
    // Start in graphics mode:

    gfx_setmode(3);

#ifdef blahblah
    glbStressTest = true;
#else
    doload = intro_screen();
#endif

    // Switch to tiled mode.
    gfx_setmode(0);

    {
	int		y;
	for (y = 0; y < 20; y++)
	    gfx_cleartextline(y);
    }

    if (doload)
	loadGame();
    else
    {
	hiscore_flagnewgame(true);
	{
	    SRAMSTREAM		os;
	    hiscore_save(os);
	}

	rebuild_all();
    }
#endif


    while(1)
    {
	if (gfx_isnewframe())
	{
	    ACTION_NAMES	action = ACTION_NONE;
	    SPELL_NAMES		spell = SPELL_NONE; // Quick cast a spell!
	    int			dx, dy;

	    if (glbStressTest)
	    {
		static int 		phase = 100;
		static int		totalmoves = 0;
		totalmoves++;

		if (hamfake_forceQuit())
		{
		    // quit request, run away!
		    break;
		}

		
		if (phase <= 0)
		{
		    // printf("Total moves: %d\n", totalmoves);
		    rebuild_all();

		    //MOB::getAvatar()->receiveDamage(ATTACK_SPELL_LIGHTNING, 0, 0);
		    // Allow a little bit longer for the avatar to explore
		    phase = rand_range(100,1000);
		    continue;
		}
		phase--;

		// A chance of the avatar doing a zany action
		if (MOB::getAvatar() && rand_chance(1))
		{
		    UT_ASSERT(!MOB::getAvatar()->hasIntrinsic(INTRINSIC_DEAD));
		    performRandomAction(MOB::getAvatar());
		}
		
		// Always run the processWorld without avatar movement.
		if (MOB::getAvatar())
		{
		    UT_ASSERT(!MOB::getAvatar()->hasIntrinsic(INTRINSIC_DEAD));
		    if (MOB::getAvatar()->doHeartbeat())
		    {
			// If you lose possesssion during heartbeat
			// the avatar flag may be set to null.
			if (MOB::getAvatar())
			{
			    UT_ASSERT(!MOB::getAvatar()->hasIntrinsic(INTRINSIC_DEAD));
			}
			if (MOB::getAvatar() && MOB::getAvatar()->doMovePrequel())
			{
			    UT_ASSERT(!MOB::getAvatar()->hasIntrinsic(INTRINSIC_DEAD));
			    MOB::getAvatar()->doAI();
			}
		    }
		    processWorld();
		    continue;
		}
		else
		{
		    // Don't waste more than 50 moves after the avatar
		    // has died
		    if (phase > 50)
			phase = 50;
		    processWorld();
		    continue;
		}
	    }

	    if (hamfake_forceQuit())
	    {
		// Only save when we are not in tutorial!
		if (MOB::getAvatar() && !glbTutorial)
		    saveGame();
		break;
	    }
	    
	    // Run the move prequel code.  If this return false,
	    // it means we should skip the move - ie: process heartbeat
	    // and world and continue.
	    // We only want to run the move prequel on net new turns,
	    // otherwise probabilistic failures will be tied to world
	    // clock not game clock.
	    if (MOB::getAvatar() &&
		lastturn != speed_gettime() &&
		!MOB::getAvatar()->doMovePrequel())
	    {
		lastturn = speed_gettime();
		MOB::getAvatar()->doHeartbeat();
		processWorld();
		continue;
	    }
	    lastturn = speed_gettime();

	    // For each button which is assigned to the minimap action,
	    // check to see if it is pressed.

	    checkMiniMapButtons();

	    // Handle free auto-pickup.
	    if (MOB::isAvatarOnFreshSquare())
	    {
		bool		anychange = false;
		anychange |= performAutoPickup();
		// Rebuild our map. There may have been light changes
		// if we picked up torches, for example.
		if (anychange)
		{
		    glbCurLevel->updateLights();
		    glbCurLevel->refresh();
		}

		// Prompt the user for any auto pickup.
		performAutoRead();
		if (glbAutoPrompt)
		{
		    performAutoPrompt();
		}
		MOB::setAvatarOnFreshSquare(false);
	    }

	    // Check to see if the avatar is dead, and if the start
	    // is hit.  Then restart.
	    if (!MOB::getAvatar())
	    {
		bool		restart = glbFinishedASave;
#ifdef HAS_KEYBOARD
		int		key;
		// Blocking wait.  At least the mouse properly triggers
		// keypresses.
		key = hamfake_getKeyPress(false);
		if (!key)
		    continue;

		if (key == '\n' || key == '\r')
		    restart = true;
		if (key == GFX_KEYLMB)
		{
		    // We want to support pure stylus devices so these
		    // count as hitting top of screen.
		    // Literal minded people can HIT the Enter text :>
		    int		sx, sy;
		    hamfake_getstyluspos(sx, sy);
		    if (sy < 2*TILEHEIGHT)
			restart = true;
		}
		char 		*msg = "Hit [Enter] to play again.  ";
#else
		bool		hit = false;
		int		i;
		for (i = BUTTON_UP; i < NUM_BUTTONS; i++)
		{
		    if (ctrl_hit((BUTTONS) i))
		    {
			hit = true;
			if (i == BUTTON_START)
			    restart = true;
			if (i == BUTTON_TOUCH)
			{
			    int		sx, sy;
			    hamfake_getstyluspos(sx, sy);
			    if (sy < 2*TILEHEIGHT)
				restart = true;
			}
		    }
		}
		// Do nothing if no button hit.
		if (!hit)
		    continue;
#ifdef iPOWDER
		const char	*msg = "Tap [Here] to play again.  ";
#else
		const char	*msg = "Hit [Start] to play again.  ";
#endif
#endif
		// Wait for a key if start wasn't hit.
		if (!restart)
		{
		    if (!(howlongdead & 63))
		    {
			msg_clear();
			msg_report(msg);
		    }
		    howlongdead++;
		    processWorld();
		    continue;
		}

		howlongdead = 0;

		// Getting here means we have a restart request!
		bool		doload;
		glbFinishedASave = false;

		{
		    int		y;
		    for (y = 0; y < 20; y++)
			gfx_cleartextline(y);
		}
		hamfake_clearKeyboardBuffer();
#ifndef HAS_KEYBOARD
		while (ctrl_anyrawpressed() && !hamfake_forceQuit())
		{
		    if (gfx_isnewframe())
			continue;
		}
#endif

		// Reset our inventory position so we start the
		// new game with a reasonable location
		gfx_setinvcursor(1, 0, false);

		doload = intro_screen();
		
		{
		    int		y;
		    for (y = 0; y < 20; y++)
			gfx_cleartextline(y);
		}

		glbCurLevel->deleteAllMaps();

		if (doload)
		    loadGame();
		else
		{
		    hiscore_flagnewgame(true);
		    {
			SRAMSTREAM		os;
			hiscore_save(os);
		    }

		    rebuild_all();
		}
		hamfake_clearKeyboardBuffer();
#ifndef HAS_KEYBOARD
		while (ctrl_anyrawpressed())
		{
		    if (gfx_isnewframe() && !hamfake_forceQuit())
			continue;
		}
#endif
	    }

	    if (glbHasJustSearched)
		glbSearchCount++;
	    else
		glbSearchCount = 1;

	    glbHasJustSearched = false;

	    // All interesting things can be so easily summarized.
	    processAllInput(action, spell, dx, dy);

	    // No dir key?  No go...
	    if (!dx && !dy && action == ACTION_NONE && spell == SPELL_NONE)
		continue;

	    // Something will happen, clear the message queue.
	    msg_clear();

	    // Check if the avatar is still alive...
	    if (MOB::getAvatar())
	    {
		bool		done = false;
		
		MOB::getAvatar()->flagLastMoveAsOld();
		if (action == ACTION_WALK)
		{
		    done = MOB::getAvatar()->actionWalk(dx, dy);
		}
		else if ((action == ACTION_SWAP) && (dx || dy))
		{
		    done = MOB::getAvatar()->actionSwap(dx, dy);
		}
		else if ((action == ACTION_JUMP) && (dx || dy))
		{
		    done = MOB::getAvatar()->actionJump(dx, dy);
		}
		else if (action == ACTION_NONE)
		{
		    // If it is a cast request?
		    if (spell != SPELL_NONE)
		    {
			done = processAction(ACTION_ZAP, spell, MOB::getAvatar());
		    }
		    else
		    {
			// Default to bump.
			done = MOB::getAvatar()->actionBump(dx, dy);
		    }
		}
		else
		{
		    done = processAction(action, 
					SPELL_NONE,
					 MOB::getAvatar());
		}
		if (!done)
		{
		    writeStatusLine();
		    continue;		// Aborted the action, nothing done.
		}
		if (MOB::getAvatar())
		{
		    // Actually did a move so we complete the reset
		    // of the last move direction.  This will only
		    // clear the dx/dy if it is flagged as old.
		    MOB::getAvatar()->resetLastMoveDirection();
		    MOB::getAvatar()->doHeartbeat();
		}

#if 0
		// Give the Avatar some extra experience...
		if (MOB::getAvatar())
		    MOB::getAvatar()->receiveExp(100);
#endif

		processWorld();

	    }
	    else
	    {
		msg_report("Being dead, you do nothing.  ");

		processWorld();
	    }
	}
    }    
    hamfake_postShutdown();

    return 0;
}

