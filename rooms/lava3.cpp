// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from lava3.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_lava3_squarelist[] =
{
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_BLOCKEDDOOR,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_BLOCKEDDOOR,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_EMPTY,	
SQUARE_BLOCKEDDOOR,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_LAVA,	
SQUARE_EMPTY,	SQUARE_LAVA,	SQUARE_LAVA,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_BLOCKEDDOOR,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_BLOCKEDDOOR,	SQUARE_EMPTY
};

const PT2 glb_lava3_moblist[] =
{
	{ 5, 3, MOB_FIREELEMENTAL },
	{ 4, 3, MOB_FIREELEMENTAL },
	{ 3, 3, MOB_FIREELEMENTAL },
	{ 4, 2, MOB_FIREELEMENTAL },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_lava3_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_lava3_itemtypelist[] =
{
	{ 4, 3, ITEMTYPE_ARTIFACT },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_lava3_moblevellist[] =
{
	{ 4, 3, MOBLEVEL_UNIQUE },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_lava3_intrinsiclist[] =
{
	{ 4, 3, INTRINSIC_LEADER },
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_lava3_squareflaglist[] =
{
	{ 6, 6, SQUAREFLAG_LIT },
	{ 2, 6, SQUAREFLAG_LIT },
	{ 6, 5, SQUAREFLAG_LIT },
	{ 5, 5, SQUAREFLAG_LIT },
	{ 3, 5, SQUAREFLAG_LIT },
	{ 2, 5, SQUAREFLAG_LIT },
	{ 1, 5, SQUAREFLAG_LIT },
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
	{ 7, 2, SQUAREFLAG_LIT },
	{ 6, 2, SQUAREFLAG_LIT },
	{ 5, 2, SQUAREFLAG_LIT },
	{ 4, 2, SQUAREFLAG_LIT },
	{ 3, 2, SQUAREFLAG_LIT },
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 5, 1, SQUAREFLAG_LIT },
	{ 4, 1, SQUAREFLAG_LIT },
	{ 3, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_lava3_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_lava3_roomdef =
{
	{ 8, 7, 0 },
	glb_lava3_squarelist,
	glb_lava3_squareflaglist,
	glb_lava3_itemlist,
	glb_lava3_moblist,
	glb_lava3_itemtypelist,
	glb_lava3_moblevellist,
	glb_lava3_intrinsiclist,
	glb_lava3_signpostlist,
	20, -1,
	100,
	0, 0,
	0,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"lava3"
};
