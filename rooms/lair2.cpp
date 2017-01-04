// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from lair2.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_lair2_squarelist[] =
{
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_BLOCKEDDOOR,	
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL
};

const PT2 glb_lair2_moblist[] =
{
	{ 3, 3, MOB_GRIDBUG },
	{ 1, 3, MOB_GRIDBUG },
	{ 2, 2, MOB_GRIDBUG },
	{ 3, 1, MOB_GRIDBUG },
	{ 1, 1, MOB_GRIDBUG },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_lair2_itemlist[] =
{
	{ 2, 3, ITEM_BOULDER },
	{ 3, 2, ITEM_BOULDER },
	{ 1, 2, ITEM_BOULDER },
	{ 2, 1, ITEM_BOULDER },
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_lair2_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_lair2_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_lair2_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_lair2_squareflaglist[] =
{
	{ 4, 4, SQUAREFLAG_LIT },
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 1, 4, SQUAREFLAG_LIT },
	{ 0, 4, SQUAREFLAG_LIT },
	{ 4, 3, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 1, 3, SQUAREFLAG_LIT },
	{ 0, 3, SQUAREFLAG_LIT },
	{ 4, 2, SQUAREFLAG_LIT },
	{ 3, 2, SQUAREFLAG_LIT },
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 4, 1, SQUAREFLAG_LIT },
	{ 3, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 1, 1, SQUAREFLAG_LIT },
	{ 0, 1, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_lair2_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_lair2_roomdef =
{
	{ 5, 5, 0 },
	glb_lair2_squarelist,
	glb_lair2_squareflaglist,
	glb_lair2_itemlist,
	glb_lair2_moblist,
	glb_lair2_itemtypelist,
	glb_lair2_moblevellist,
	glb_lair2_intrinsiclist,
	glb_lair2_signpostlist,
	-1, -1,
	100,
	0, 0,
	0,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"lair2"
};
