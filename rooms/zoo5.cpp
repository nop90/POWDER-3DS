// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from zoo5.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_zoo5_squarelist[] =
{
SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	
SQUARE_FLOOR,	SQUARE_BLOCKEDDOOR,	SQUARE_WALL,	SQUARE_BLOCKEDDOOR,	
SQUARE_WALL
};

const PT2 glb_zoo5_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_zoo5_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_zoo5_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_zoo5_moblevellist[] =
{
	{ 1, 1, MOBLEVEL_OVERPOWERING },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_zoo5_intrinsiclist[] =
{
	{ 1, 1, INTRINSIC_ASLEEP },
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_zoo5_squareflaglist[] =
{
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 1, 1, SQUAREFLAG_LIT },
	{ 0, 1, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_zoo5_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_zoo5_roomdef =
{
	{ 3, 3, 0 },
	glb_zoo5_squarelist,
	glb_zoo5_squareflaglist,
	glb_zoo5_itemlist,
	glb_zoo5_moblist,
	glb_zoo5_itemtypelist,
	glb_zoo5_moblevellist,
	glb_zoo5_intrinsiclist,
	glb_zoo5_signpostlist,
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
	"zoo5"
};
