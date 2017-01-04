// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from troll.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_troll_squarelist[] =
{
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_BLOCKEDDOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL
};

const PT2 glb_troll_moblist[] =
{
	{ 5, 4, MOB_TROLL },
	{ 3, 4, MOB_TROLL },
	{ 7, 2, MOB_TROLL },
	{ 5, 2, MOB_CAVETROLL },
	{ 3, 2, MOB_CAVETROLL },
	{ 1, 2, MOB_TROLL },
	{ 4, 1, MOB_CAVETROLL },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_troll_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_troll_itemtypelist[] =
{
	{ 5, 1, ITEMTYPE_ARMOUR },
	{ 4, 1, ITEMTYPE_ARTIFACT },
	{ 3, 1, ITEMTYPE_WEAPON },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_troll_moblevellist[] =
{
	{ 4, 1, MOBLEVEL_UNIQUE },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_troll_intrinsiclist[] =
{
	{ 4, 1, INTRINSIC_LEADER },
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_troll_squareflaglist[] =
{
	{ 8, 5, SQUAREFLAG_LIT },
	{ 7, 5, SQUAREFLAG_LIT },
	{ 6, 5, SQUAREFLAG_LIT },
	{ 5, 5, SQUAREFLAG_LIT },
	{ 4, 5, SQUAREFLAG_LIT },
	{ 3, 5, SQUAREFLAG_LIT },
	{ 2, 5, SQUAREFLAG_LIT },
	{ 1, 5, SQUAREFLAG_LIT },
	{ 0, 5, SQUAREFLAG_LIT },
	{ 8, 4, SQUAREFLAG_LIT },
	{ 7, 4, SQUAREFLAG_LIT },
	{ 6, 4, SQUAREFLAG_LIT },
	{ 5, 4, SQUAREFLAG_LIT },
	{ 4, 4, SQUAREFLAG_LIT },
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 1, 4, SQUAREFLAG_LIT },
	{ 0, 4, SQUAREFLAG_LIT },
	{ 8, 3, SQUAREFLAG_LIT },
	{ 7, 3, SQUAREFLAG_LIT },
	{ 6, 3, SQUAREFLAG_LIT },
	{ 5, 3, SQUAREFLAG_LIT },
	{ 4, 3, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 1, 3, SQUAREFLAG_LIT },
	{ 0, 3, SQUAREFLAG_LIT },
	{ 8, 2, SQUAREFLAG_LIT },
	{ 7, 2, SQUAREFLAG_LIT },
	{ 6, 2, SQUAREFLAG_LIT },
	{ 5, 2, SQUAREFLAG_LIT },
	{ 4, 2, SQUAREFLAG_LIT },
	{ 3, 2, SQUAREFLAG_LIT },
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 8, 1, SQUAREFLAG_LIT },
	{ 7, 1, SQUAREFLAG_LIT },
	{ 6, 1, SQUAREFLAG_LIT },
	{ 5, 1, SQUAREFLAG_LIT },
	{ 4, 1, SQUAREFLAG_LIT },
	{ 3, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 1, 1, SQUAREFLAG_LIT },
	{ 0, 1, SQUAREFLAG_LIT },
	{ 8, 0, SQUAREFLAG_LIT },
	{ 7, 0, SQUAREFLAG_LIT },
	{ 6, 0, SQUAREFLAG_LIT },
	{ 5, 0, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_troll_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_troll_roomdef =
{
	{ 9, 6, 0 },
	glb_troll_squarelist,
	glb_troll_squareflaglist,
	glb_troll_itemlist,
	glb_troll_moblist,
	glb_troll_itemtypelist,
	glb_troll_moblevellist,
	glb_troll_intrinsiclist,
	glb_troll_signpostlist,
	5, -1,
	100,
	0, 0,
	0,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"troll"
};
