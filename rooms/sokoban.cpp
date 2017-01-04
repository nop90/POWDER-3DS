// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from sokoban.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_sokoban_squarelist[] =
{
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_DOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_DOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_EMPTY
};

const PT2 glb_sokoban_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_sokoban_itemlist[] =
{
	{ 7, 2, ITEM_BOULDER },
	{ 1, 2, ITEM_BOULDER },
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_sokoban_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_sokoban_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_sokoban_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_sokoban_squareflaglist[] =
{
	{ 7, 4, SQUAREFLAG_LIT },
	{ 6, 4, SQUAREFLAG_LIT },
	{ 5, 4, SQUAREFLAG_LIT },
	{ 4, 4, SQUAREFLAG_LIT },
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
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
	{ 6, 0, SQUAREFLAG_LIT },
	{ 5, 0, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_sokoban_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_sokoban_roomdef =
{
	{ 9, 5, 0 },
	glb_sokoban_squarelist,
	glb_sokoban_squareflaglist,
	glb_sokoban_itemlist,
	glb_sokoban_moblist,
	glb_sokoban_itemtypelist,
	glb_sokoban_moblevellist,
	glb_sokoban_intrinsiclist,
	glb_sokoban_signpostlist,
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
	"sokoban"
};
