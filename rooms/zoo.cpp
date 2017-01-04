// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from zoo.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_zoo_squarelist[] =
{
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_BLOCKEDDOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_BLOCKEDDOOR,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL
};

const PT2 glb_zoo_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_zoo_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_zoo_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_zoo_moblevellist[] =
{
	{ 2, 3, MOBLEVEL_NORMAL },
	{ 1, 3, MOBLEVEL_NORMAL },
	{ 2, 2, MOBLEVEL_NORMAL },
	{ 1, 2, MOBLEVEL_NORMAL },
	{ 2, 1, MOBLEVEL_NORMAL },
	{ 1, 1, MOBLEVEL_NORMAL },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_zoo_intrinsiclist[] =
{
	{ 2, 3, INTRINSIC_ASLEEP },
	{ 1, 3, INTRINSIC_ASLEEP },
	{ 2, 2, INTRINSIC_ASLEEP },
	{ 1, 2, INTRINSIC_ASLEEP },
	{ 2, 1, INTRINSIC_ASLEEP },
	{ 1, 1, INTRINSIC_ASLEEP },
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_zoo_squareflaglist[] =
{
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 1, 4, SQUAREFLAG_LIT },
	{ 0, 4, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 1, 3, SQUAREFLAG_LIT },
	{ 0, 3, SQUAREFLAG_LIT },
	{ 3, 2, SQUAREFLAG_LIT },
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 3, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 1, 1, SQUAREFLAG_LIT },
	{ 0, 1, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_zoo_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_zoo_roomdef =
{
	{ 4, 5, 0 },
	glb_zoo_squarelist,
	glb_zoo_squareflaglist,
	glb_zoo_itemlist,
	glb_zoo_moblist,
	glb_zoo_itemtypelist,
	glb_zoo_moblevellist,
	glb_zoo_intrinsiclist,
	glb_zoo_signpostlist,
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
	"zoo"
};
