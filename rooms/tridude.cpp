// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from tridude.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_tridude_squarelist[] =
{
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WATER,	
SQUARE_WATER,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WATER,	SQUARE_WATER,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL
};

const PT2 glb_tridude_moblist[] =
{
	{ 5, 4, MOB_TRIDUDE },
	{ 4, 4, MOB_TRIDUDE },
	{ 3, 4, MOB_TRIDUDE },
	{ 4, 3, MOB_TRIDUDE },
	{ 3, 2, MOB_TRIDUDE },
	{ 2, 2, MOB_TRIDUDE },
	{ 1, 2, MOB_TRIDUDE },
	{ 2, 1, MOB_TRIDUDE },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_tridude_itemlist[] =
{
	{ 5, 3, ITEM_BOULDER },
	{ 3, 3, ITEM_BOULDER },
	{ 4, 2, ITEM_BOULDER },
	{ 1, 1, ITEM_BOULDER },
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_tridude_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_tridude_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_tridude_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_tridude_squareflaglist[] =
{
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
	{ 0, 1, SQUAREFLAG_LIT },
	{ 6, 0, SQUAREFLAG_LIT },
	{ 5, 0, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_tridude_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_tridude_roomdef =
{
	{ 7, 6, 0 },
	glb_tridude_squarelist,
	glb_tridude_squareflaglist,
	glb_tridude_itemlist,
	glb_tridude_moblist,
	glb_tridude_itemtypelist,
	glb_tridude_moblevellist,
	glb_tridude_intrinsiclist,
	glb_tridude_signpostlist,
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
	"tridude"
};
