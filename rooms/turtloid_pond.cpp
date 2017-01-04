// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from turtloid_pond.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_turtloid_pond_squarelist[] =
{
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_BLOCKEDDOOR,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_WATER,	
SQUARE_EMPTY,	SQUARE_WATER,	SQUARE_WATER,	SQUARE_WATER,	
SQUARE_WATER,	SQUARE_EMPTY,	SQUARE_BLOCKEDDOOR,	SQUARE_WATER,	
SQUARE_WATER,	SQUARE_WATER,	SQUARE_WATER,	SQUARE_WATER,	
SQUARE_WATER,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_WATER,	SQUARE_EMPTY,	
SQUARE_WATER,	SQUARE_WATER,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_WATER,	SQUARE_WATER,	
SQUARE_CORRIDOR,	SQUARE_WATER,	SQUARE_WATER,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_WATER,	SQUARE_WATER,	
SQUARE_EMPTY,	SQUARE_WATER,	SQUARE_WATER,	SQUARE_WATER,	
SQUARE_WATER,	SQUARE_BLOCKEDDOOR,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_WATER,	SQUARE_WATER,	SQUARE_WATER,	SQUARE_WATER,	
SQUARE_WATER,	SQUARE_WATER,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_BLOCKEDDOOR,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY
};

const PT2 glb_turtloid_pond_moblist[] =
{
	{ 7, 6, MOB_TURTLOID },
	{ 3, 6, MOB_TURTLOID },
	{ 5, 4, MOB_LARGETURTLOID },
	{ 4, 4, MOB_LARGETURTLOID },
	{ 3, 4, MOB_LARGETURTLOID },
	{ 5, 1, MOB_TURTLOID },
	{ 2, 1, MOB_TURTLOID },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_turtloid_pond_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_turtloid_pond_itemtypelist[] =
{
	{ 4, 4, ITEMTYPE_ARTIFACT },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_turtloid_pond_moblevellist[] =
{
	{ 4, 4, MOBLEVEL_UNIQUE },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_turtloid_pond_intrinsiclist[] =
{
	{ 4, 4, INTRINSIC_LEADER },
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_turtloid_pond_squareflaglist[] =
{
	{ 4, 7, SQUAREFLAG_LIT },
	{ 7, 6, SQUAREFLAG_LIT },
	{ 6, 6, SQUAREFLAG_LIT },
	{ 5, 6, SQUAREFLAG_LIT },
	{ 4, 6, SQUAREFLAG_LIT },
	{ 3, 6, SQUAREFLAG_LIT },
	{ 2, 6, SQUAREFLAG_LIT },
	{ 8, 5, SQUAREFLAG_LIT },
	{ 7, 5, SQUAREFLAG_LIT },
	{ 6, 5, SQUAREFLAG_LIT },
	{ 5, 5, SQUAREFLAG_LIT },
	{ 4, 5, SQUAREFLAG_LIT },
	{ 2, 5, SQUAREFLAG_LIT },
	{ 1, 5, SQUAREFLAG_LIT },
	{ 6, 4, SQUAREFLAG_LIT },
	{ 5, 4, SQUAREFLAG_LIT },
	{ 4, 4, SQUAREFLAG_LIT },
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 6, 3, SQUAREFLAG_LIT },
	{ 5, 3, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 6, 2, SQUAREFLAG_LIT },
	{ 5, 2, SQUAREFLAG_LIT },
	{ 4, 2, SQUAREFLAG_LIT },
	{ 3, 2, SQUAREFLAG_LIT },
	{ 2, 2, SQUAREFLAG_LIT },
	{ 1, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 7, 1, SQUAREFLAG_LIT },
	{ 6, 1, SQUAREFLAG_LIT },
	{ 5, 1, SQUAREFLAG_LIT },
	{ 4, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 6, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_turtloid_pond_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_turtloid_pond_roomdef =
{
	{ 9, 8, 0 },
	glb_turtloid_pond_squarelist,
	glb_turtloid_pond_squareflaglist,
	glb_turtloid_pond_itemlist,
	glb_turtloid_pond_moblist,
	glb_turtloid_pond_itemtypelist,
	glb_turtloid_pond_moblevellist,
	glb_turtloid_pond_intrinsiclist,
	glb_turtloid_pond_signpostlist,
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
	"turtloid_pond"
};
