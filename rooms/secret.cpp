// Auto-generated .map file
// DO NOT HAND EDIT
// Generated from secret.map

#include "../map.h"
#include "../glbdef.h"

const SQUARE_NAMES glb_secret_squarelist[] =
{
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_DOOR,	
SQUARE_FLOOR,	SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_FLOOR,	
SQUARE_FLOOR,	SQUARE_SECRETDOOR,	SQUARE_FLOOR,	SQUARE_FLOOR,	
SQUARE_DOOR,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	
SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL,	SQUARE_WALL
};

const PT2 glb_secret_moblist[] =
{
	{ -1, -1, MOB_NONE }
};

const PT2 glb_secret_itemlist[] =
{
	{ -1, -1, ITEM_NONE }
};

const PT2 glb_secret_itemtypelist[] =
{
	{ -1, -1, ITEMTYPE_NONE }
};

const PT2 glb_secret_moblevellist[] =
{
	{ -1, -1, MOBLEVEL_NONE }
};

const PT2 glb_secret_intrinsiclist[] =
{
	{ -1, -1, INTRINSIC_NONE }
};

const PT2 glb_secret_squareflaglist[] =
{
	{ 6, 3, SQUAREFLAG_LIT },
	{ 5, 3, SQUAREFLAG_LIT },
	{ 4, 3, SQUAREFLAG_LIT },
	{ 3, 3, SQUAREFLAG_LIT },
	{ 2, 3, SQUAREFLAG_LIT },
	{ 1, 3, SQUAREFLAG_LIT },
	{ 0, 3, SQUAREFLAG_LIT },
	{ 6, 2, SQUAREFLAG_LIT },
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
	{ 1, 1, SQUAREFLAG_LIT },
	{ 0, 1, SQUAREFLAG_LIT },
	{ 6, 0, SQUAREFLAG_LIT },
	{ 5, 0, SQUAREFLAG_LIT },
	{ 4, 0, SQUAREFLAG_LIT },
	{ 3, 0, SQUAREFLAG_LIT },
	{ 2, 0, SQUAREFLAG_LIT },
	{ 1, 0, SQUAREFLAG_LIT },
	{ 0, 0, SQUAREFLAG_LIT },
	{ -1, -1, SQUAREFLAG_NONE }
};

const PT2 glb_secret_signpostlist[] =
{
	{ -1, -1, SIGNPOST_NONE }
};

const ROOM_DEF glb_secret_roomdef =
{
	{ 7, 4, 0 },
	glb_secret_squarelist,
	glb_secret_squareflaglist,
	glb_secret_itemlist,
	glb_secret_moblist,
	glb_secret_itemtypelist,
	glb_secret_moblevellist,
	glb_secret_intrinsiclist,
	glb_secret_signpostlist,
	-1, -1,
	100,
	0, 0,
	1,
	1,
	1,
	1,
	1,
	-1, -1, 
	-1, 
	"secret"
};
