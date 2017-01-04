// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from sokoban2.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_sokoban2_squarelist[] =
{
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_DOOR,	SQUARE_WALL,	
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_DOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_DOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_DOOR,	SQUARE_WALL,	
SQUARE_EMPTY
};

const PT2 glb_sokoban2_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_sokoban2_itemlist[] =
{
	{ 2, 2, ITEM_BOULDER },
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_sokoban2_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_sokoban2_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_sokoban2_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_sokoban2_squareflaglist[] =
{
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 1, 4, SQUAREFLAG_LIT },
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
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_sokoban2_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_sokoban2_roomdef =
{
	{ 5, 5, 0 },
	glb_sokoban2_squarelist,
	glb_sokoban2_squareflaglist,
	glb_sokoban2_itemlist,
	glb_sokoban2_moblist,
	glb_sokoban2_itemtypelist,
	glb_sokoban2_moblevellist,
	glb_sokoban2_intrinsiclist,
	glb_sokoban2_signpostlist,
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
	"sokoban2"
};
