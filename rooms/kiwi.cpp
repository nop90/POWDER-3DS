// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from kiwi.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_kiwi_squarelist[] =
{
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_DOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_SECRETDOOR,	
SQUARE_CORRIDOR,	SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_CORRIDOR,	
SQUARE_EMPTY,	SQUARE_WALL,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_CORRIDOR,	SQUARE_EMPTY,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_DOOR,	
SQUARE_WALL,	SQUARE_EMPTY,	SQUARE_EMPTY
};

const PT2 glb_kiwi_moblist[] =
{
	{ 3, 3, MOB_KIWI },
	{ 1, 3, MOB_KIWI },
	{ 2, 1, MOB_KIWI },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_kiwi_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_kiwi_itemtypelist[] =
{
	{ 5, 3, ITEMTYPE_ANY },
	{ 5, 2, ITEMTYPE_ANY },
	{ 5, 1, ITEMTYPE_ANY },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_kiwi_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_kiwi_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_kiwi_squareflaglist[] =
{
	{ 4, 4, SQUAREFLAG_LIT },
	{ 3, 4, SQUAREFLAG_LIT },
	{ 2, 4, SQUAREFLAG_LIT },
	{ 1, 4, SQUAREFLAG_LIT },
	{ 0, 4, SQUAREFLAG_LIT },
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
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_kiwi_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_kiwi_roomdef =
{
	{ 7, 5, 0 },
	glb_kiwi_squarelist,
	glb_kiwi_squareflaglist,
	glb_kiwi_itemlist,
	glb_kiwi_moblist,
	glb_kiwi_itemtypelist,
	glb_kiwi_moblevellist,
	glb_kiwi_intrinsiclist,
	glb_kiwi_signpostlist,
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
	"kiwi"
};
