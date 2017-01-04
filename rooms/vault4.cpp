// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from vault4.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_vault4_squarelist[] =
{
SQUARE_WALL,	SQUARE_WALL,	SQUARE_DOOR,	SQUARE_WALL,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_DOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_SECRETDOOR,	
SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_CORRIDOR,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_DOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY
};

const PT2 glb_vault4_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_vault4_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_vault4_itemtypelist[] =
{
	{ 6, 5, ITEMTYPE_ANY },
	{ 5, 5, ITEMTYPE_ANY },
	{ 6, 4, ITEMTYPE_ANY },
	{ 4, 4, ITEMTYPE_ANY },
	{ 6, 2, ITEMTYPE_ANY },
	{ 5, 2, ITEMTYPE_ARTIFACT },
	{ 6, 1, ITEMTYPE_ANY },
	{ 5, 1, ITEMTYPE_ANY },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_vault4_moblevellist[] =
{
	{ 5, 5, MOBLEVEL_STRONG },
	{ 6, 4, MOBLEVEL_STRONG },
	{ 4, 4, MOBLEVEL_STRONG },
	{ 5, 2, MOBLEVEL_UNIQUE },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_vault4_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_vault4_squareflaglist[] =
{
	{ 3, 6, SQUAREFLAG_LIT },
	{ 2, 6, SQUAREFLAG_LIT },
	{ 1, 6, SQUAREFLAG_LIT },
	{ 0, 6, SQUAREFLAG_LIT },
	{ 3, 5, SQUAREFLAG_LIT },
	{ 2, 5, SQUAREFLAG_LIT },
	{ 1, 5, SQUAREFLAG_LIT },
	{ 0, 5, SQUAREFLAG_LIT },
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

const PT2 glb_vault4_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_vault4_roomdef =
{
	{ 8, 7, 0 },
	glb_vault4_squarelist,
	glb_vault4_squareflaglist,
	glb_vault4_itemlist,
	glb_vault4_moblist,
	glb_vault4_itemtypelist,
	glb_vault4_moblevellist,
	glb_vault4_intrinsiclist,
	glb_vault4_signpostlist,
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
	"vault4"
};
