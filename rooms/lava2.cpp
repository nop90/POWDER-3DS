// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from lava2.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_lava2_squarelist[] =
{
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_DOOR,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_DOOR,	SQUARE_DOOR,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_DOOR,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY
};

const PT2 glb_lava2_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_lava2_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_lava2_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_lava2_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_lava2_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_lava2_squareflaglist[] =
{
	{ 2, 4, SQUAREFLAG_LIT },
	{ 4, 3, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 1, 3, SQUAREFLAG_LIT },
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
	{ 3, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_lava2_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_lava2_roomdef =
{
	{ 7, 5, 0 },
	glb_lava2_squarelist,
	glb_lava2_squareflaglist,
	glb_lava2_itemlist,
	glb_lava2_moblist,
	glb_lava2_itemtypelist,
	glb_lava2_moblevellist,
	glb_lava2_intrinsiclist,
	glb_lava2_signpostlist,
	5, -1,
	100,
	0, 0,
	1,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"lava2"
};
