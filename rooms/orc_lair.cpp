// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from orc_lair.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_orc_lair_squarelist[] =
{
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_BLOCKEDDOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_EMPTY,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_EMPTY
};

const PT2 glb_orc_lair_moblist[] =
{
	{ 2, 5, MOB_ORC },
	{ 3, 4, MOB_HILLORC },
	{ 5, 3, MOB_ORC },
	{ 3, 3, MOB_HILLORC },
	{ 1, 3, MOB_ORC },
	{ 3, 2, MOB_HILLORC },
	{ 4, 1, MOB_ORC },
	{ 2, 1, MOB_ORC },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_orc_lair_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_orc_lair_itemtypelist[] =
{
	{ 4, 4, ITEMTYPE_WEAPON },
	{ 2, 4, ITEMTYPE_ARMOUR },
	{ 3, 3, ITEMTYPE_ARTIFACT },
	{ 4, 2, ITEMTYPE_WEAPON },
	{ 2, 2, ITEMTYPE_ARMOUR },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_orc_lair_moblevellist[] =
{
	{ 3, 3, MOBLEVEL_UNIQUE },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_orc_lair_intrinsiclist[] =
{
	{ 3, 3, INTRINSIC_LEADER },
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_orc_lair_squareflaglist[] =
{
	{ 5, 6, SQUAREFLAG_LIT },
	{ 4, 6, SQUAREFLAG_LIT },
	{ 3, 6, SQUAREFLAG_LIT },
	{ 2, 6, SQUAREFLAG_LIT },
	{ 1, 6, SQUAREFLAG_LIT },
	{ 6, 5, SQUAREFLAG_LIT },
	{ 5, 5, SQUAREFLAG_LIT },
	{ 4, 5, SQUAREFLAG_LIT },
	{ 3, 5, SQUAREFLAG_LIT },
	{ 2, 5, SQUAREFLAG_LIT },
	{ 1, 5, SQUAREFLAG_LIT },
	{ 0, 5, SQUAREFLAG_LIT },
	{ 6, 4, SQUAREFLAG_LIT },
	{ 5, 4, SQUAREFLAG_LIT },
	{ 4, 4, SQUAREFLAG_LIT },
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 1, 4, SQUAREFLAG_LIT },
	{ 0, 4, SQUAREFLAG_LIT },
	{ 6, 3, SQUAREFLAG_LIT },
	{ 5, 3, SQUAREFLAG_LIT },
	{ 4, 3, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 1, 3, SQUAREFLAG_LIT },
	{ 0, 3, SQUAREFLAG_LIT },
	{ 6, 2, SQUAREFLAG_LIT },
	{ 5, 2, SQUAREFLAG_LIT },
	{ 4, 2, SQUAREFLAG_LIT },
	{ 3, 2, SQUAREFLAG_LIT },
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 6, 1, SQUAREFLAG_LIT },
	{ 5, 1, SQUAREFLAG_LIT },
	{ 4, 1, SQUAREFLAG_LIT },
	{ 3, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 1, 1, SQUAREFLAG_LIT },
	{ 5, 0, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_orc_lair_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_orc_lair_roomdef =
{
	{ 7, 7, 0 },
	glb_orc_lair_squarelist,
	glb_orc_lair_squareflaglist,
	glb_orc_lair_itemlist,
	glb_orc_lair_moblist,
	glb_orc_lair_itemtypelist,
	glb_orc_lair_moblevellist,
	glb_orc_lair_intrinsiclist,
	glb_orc_lair_signpostlist,
	10, -1,
	100,
	0, 0,
	0,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"orc_lair"
};
