// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from crypt5.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_crypt5_squarelist[] =
{
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_DOOR,	SQUARE_WALL,	SQUARE_DOOR,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_EMPTY,	
SQUARE_FLOOR,	SQUARE_EMPTY,	SQUARE_FLOOR,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_EMPTY,	SQUARE_EMPTY,	SQUARE_EMPTY,	
SQUARE_FLOOR,	SQUARE_DOOR,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL
};

const PT2 glb_crypt5_moblist[] =
{
	{ 2, 3, MOB_LICH },
	{ -1, -1, MOB_NONE }
};

const PT2 glb_crypt5_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_crypt5_itemtypelist[] =
{
	{ 2, 3, ITEMTYPE_WEAPON },
	{ 2, 3, ITEMTYPE_ARTIFACT },
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_crypt5_moblevellist[] =
{
	{ 2, 3, MOBLEVEL_UNIQUE },
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_crypt5_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_crypt5_squareflaglist[] =
{
	{ 5, 5, SQUAREFLAG_LIT },
	{ 4, 5, SQUAREFLAG_LIT },
	{ 3, 5, SQUAREFLAG_LIT },
	{ 2, 5, SQUAREFLAG_LIT },
	{ 1, 5, SQUAREFLAG_LIT },
	{ 0, 5, SQUAREFLAG_LIT },
	{ 5, 4, SQUAREFLAG_LIT },
	{ 4, 4, SQUAREFLAG_LIT },
	{ 0, 4, SQUAREFLAG_LIT },
	{ 5, 3, SQUAREFLAG_LIT },
	{ 4, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 0, 3, SQUAREFLAG_LIT },
	{ 5, 2, SQUAREFLAG_LIT },
	{ 4, 2, SQUAREFLAG_LIT },
	{ 0, 2, SQUAREFLAG_LIT },
	{ 5, 1, SQUAREFLAG_LIT },
	{ 4, 1, SQUAREFLAG_LIT },
	{ 3, 1, SQUAREFLAG_LIT },
	{ 2, 1, SQUAREFLAG_LIT },
	{ 1, 1, SQUAREFLAG_LIT },
	{ 0, 1, SQUAREFLAG_LIT },
	{ 5, 0, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_crypt5_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_crypt5_roomdef =
{
	{ 6, 6, 0 },
	glb_crypt5_squarelist,
	glb_crypt5_squareflaglist,
	glb_crypt5_itemlist,
	glb_crypt5_moblist,
	glb_crypt5_itemtypelist,
	glb_crypt5_moblevellist,
	glb_crypt5_intrinsiclist,
	glb_crypt5_signpostlist,
	2, -1,
	100,
	0, 0,
	0,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"crypt5"
};
