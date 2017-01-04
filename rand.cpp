/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        rand.cpp ( POWDER Library, C++ )
 *
 * COMMENTS:
 *	A collection of utility functions to get random numbers.
 */

#include "mygba.h"

#include "assert.h"
#include "rand.h"
#include "dpdf_table.h"

// Bring in the Mersenne Twister
#include "mt19937ar.c"

#include <ctype.h>

class RAND_STATE
{
public:
    // Read from current RNG into this
    void	 read();
    // Write from this into RNG.
    void	 write();

    RAND_STATE	*myOlderState;
    unsigned long	myMT[N];
    unsigned int	myMTI;
};

void
RAND_STATE::read()
{
    memcpy(myMT, mt, sizeof(unsigned long) * N);
    myMTI = mti;
}

void
RAND_STATE::write()
{
    memcpy(mt, myMT, sizeof(unsigned long) * N);
    mti = myMTI;
}

RAND_STATE *glbOldRandState = 0;

void
rand_pushstate()
{
    RAND_STATE		*state;

    state = new RAND_STATE();
    state->myOlderState = glbOldRandState;
    state->read();
    glbOldRandState = state;
}

void
rand_popstate()
{
    UT_ASSERT(glbOldRandState != 0);
    if (!glbOldRandState)
	return;

    RAND_STATE	*state;
    state = glbOldRandState;
    state->write();
    glbOldRandState = state->myOlderState;
    delete state;
}

void
rand_init()
{
    if (!mt)
    {
	mt = new unsigned long[N];
    }
}

const u8 glb_percenttable[1024] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28,
    28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29,
    29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31,
    31, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 34, 34, 34, 34,
    34, 34, 34, 34, 34, 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
    36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 39,
    39, 39, 39, 39, 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 40, 40,
    40, 40, 40, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 43, 43, 43,
    43, 43, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 45, 45, 45, 45,
    45, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 46, 46, 46, 46,
    46, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48,
    48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 50,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51,
    51, 51, 51, 51, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 53, 53,
    53, 53, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 54, 54,
    54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55, 56, 56, 56,
    56, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 57, 57,
    57, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59,
    59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
    61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 62, 62, 62, 62, 62, 62,
    62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 65, 65, 65, 65, 65, 65, 65,
    65, 65, 65, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 67, 67,
    67, 67, 67, 67, 67, 67, 67, 67, 68, 68, 68, 68, 68, 68, 68, 68,
    68, 68, 69, 69, 69, 69, 69, 69, 69, 69, 69, 69, 70, 70, 70, 70,
    70, 70, 70, 70, 70, 70, 70, 71, 71, 71, 71, 71, 71, 71, 71, 71,
    71, 72, 72, 72, 72, 72, 72, 72, 72, 72, 72, 73, 73, 73, 73, 73,
    73, 73, 73, 73, 73, 74, 74, 74, 74, 74, 74, 74, 74, 74, 74, 75,
    75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 76, 76, 76, 76, 76, 76,
    76, 76, 76, 76, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 78, 78,
    78, 78, 78, 78, 78, 78, 78, 78, 79, 79, 79, 79, 79, 79, 79, 79,
    79, 79, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 81, 81, 81,
    81, 81, 81, 81, 81, 81, 81, 82, 82, 82, 82, 82, 82, 82, 82, 82,
    82, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 83, 84, 84, 84, 84,
    84, 84, 84, 84, 84, 84, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
    86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 87, 87, 87, 87, 87, 87,
    87, 87, 87, 87, 87, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 89,
    89, 89, 89, 89, 89, 89, 89, 89, 89, 90, 90, 90, 90, 90, 90, 90,
    90, 90, 90, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 92, 92,
    92, 92, 92, 92, 92, 92, 92, 92, 93, 93, 93, 93, 93, 93, 93, 93,
    93, 93, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 95, 95, 95, 95,
    95, 95, 95, 95, 95, 95, 95, 96, 96, 96, 96, 96, 96, 96, 96, 96,
    96, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98,
    98, 98, 98, 98, 98, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
};

#if 0
void
rand_print()
{
    // Build our percenttable.
    int		i;
    for (i = 0; i < 1023; i++)
    {
	float		val;
	val = ((float)i) / 1024;
	val *= 100;
	// Round down intentionally.
	glb_percenttable[i] = (u8) val;
    }

    for (i = 0; i < 1023; i++)
    {
	printf("%d,", glb_percenttable[i]);
	if (i && !(i & 15))
	    printf("\n\t");
	else
	    printf(" ");
    }
}
#endif

int
genrandom()
{
    return (int) genrand_int31(); 
}

long
rand_getseed()
{
    long		seed;

    // We lose one bit here as we only use 31 bits out of 32.
    // (Actually, we lose a heck of a lot more because we reduce
    // the internal state from 624*32 bits to 31 bits.)
    seed = genrandom();

    rand_setseed(seed);
    return seed;
}

void
rand_setseed(long seed)
{
    init_genrand(seed);
}

double
rand_double()
{
    return genrand_res53();
}

int
rand_range(int min, int max)
{
    int		v;

    if (min > max)
	return rand_range(max, min);
    
    v = rand_choice(max - min + 1);
    return v + min;
}

int
rand_choice(int num)
{
    int		v;

    // Choice of 0 or 1 is always 0.
    if (num < 2) return 0;

    // Handle simple cases...
    switch (num)
    {
	case 2:
	    return (genrandom() & 1);

	case 3:
	{
	    unsigned int		fullrandom;

	    fullrandom = genrandom();
	    // Guaranteed to terminate because we are unsigned.
	    // Slightly biases to 0.
	    while ((fullrandom & 3) == 3)
	    {
		fullrandom >>= 2;
	    }
	    return fullrandom & 3;
	}

	case 4:
	    return (genrandom() & 3);

	case 8:
	    // We have an affinity for 8 sided dice...
	    return (genrandom() & 7);

	case 100:
	{
	    // This is used by the percentile code is is more common
	    // than one might think
	    int				index;

	    index = genrandom() & 1023;
	    return glb_percenttable[index];
	}
    }

    v = abs(genrandom());
    v %= num;

    return v;
}

const char *
rand_string(const char **stringlist)
{
    int		n;
    
    // Find length of the string list.
    for (n = 0; stringlist[n]; n++);

    return stringlist[rand_choice(n)];
}

int
rand_roll(int num, int reroll)
{
    int		max = 0, val;

    // -1, 0, and 1 all evaluate to themselves always.
    if (num >= -1 && num <= 1)
	return num;

    if (num < 0)
    {
	// Negative numbers just invoke this and reverse the results.
	// Note we can't just negate the result, as we want higher rerolls
	// to move it closer to -1, not closer to num!
	val = rand_roll(-num, reroll);
	val -= num + 1;
	return val;
    }

    if (reroll < 0)
    {
	// Negative rerolls means we want to reroll but pick the
	// smallest result.  This is the same as inverting our normal
	// roll distribution, so thus...
	val = rand_roll(num, -reroll);
	val = num + 1 - val;
	return val;
    }
    
    // I wasn't even drunk when I made reroll of 0 mean roll
    // once, and thus necissating this change.
    reroll++;
    while (reroll--)
    {
	val = rand_choice(num) + 1;
	if (val > max)
	    max = val;
    }

    return max;
}

int
rand_rollMean(int sides, int reroll, int scale)
{
    if (sides == 0)
	return 0;

    if (sides < 0)
    {
	return -rand_rollMean(-sides, reroll, scale);
    }

    int		mean;
    
    // Simple case...
    if (!reroll)
    {
	mean = (sides + 1) * scale;
	mean = (mean + 1) / 2;

	return mean;
    }
    // Now, more complicated cases...
    if (reroll < 0)
    {
	// Invert distribution
	// Negative rerolls means we want to reroll but pick the
	// smallest result.  This is the same as inverting our normal
	// roll distribution, so thus...
	mean = rand_rollMean(sides, -reroll, scale);
	mean = (sides + 1)*scale - mean;
	return mean;
    }
    // Given M variates Ri[1..N], what is 
    // mean(max(Ri))
    // Very tempted to do bruteforce here.
    // We do brute force offline with the builddpdf program and
    // just do a table lookup here.  The GBA doesn't like floats
    // very much :>
    if (sides < 2)
    {
	// A one sided die is obvious
	// At least, obvious if you remember important things like 
	// including the scale factor!
	return scale;
    }

    // Make sure we are in the table, otherwise just clamp.
    if (sides > DPDF_MAXSIDES)
    {
	UT_ASSERT(!"Too many sides!");
	sides = DPDF_MAXSIDES;
    }
    if (reroll > DPDF_MAXROLLS)
    {
	UT_ASSERT(!"Too many rolls!");
	reroll = DPDF_MAXROLLS;
    }
    // To apply the user defined scale, we multiply by the users 
    // scale and then divide by our scale.
    mean = glb_dpdftable[reroll][sides-2];
    mean *= scale;
    mean += DPDF_SCALE / 2;
    mean /= DPDF_SCALE;

    return mean;
}

int
rand_dice(int numdie, int sides, int bonus)
{
    int		i, total = bonus;

    for (i = 0; i < numdie; i++)
    {
	total += rand_choice(sides) + 1;
    }
    return total;
}

bool
rand_chance(int percentage)
{
    int		percent;

    // We want to avoid any random number hijinks if we are invoking
    // with zero or 100.  This can occur due to rand_chance(glbdef.value)
    // where value is usually one or the other.
    if (percentage == 0)
	return false;
    if (percentage == 100)
	return true;

    percent = rand_choice(100);
    // We want strict less than so percentage 0 will never pass,
    // and percentage 99 will fail only one in 100.
    return percent < percentage;
}

int
rand_sign()
{
    return rand_choice(2) * 2 - 1;
}

void
rand_direction(int &dx, int &dy)
{
    if (rand_choice(2))
    {
	dx = rand_sign();
	dy = 0;
    }
    else
    {
	dx = 0;
	dy = rand_sign();
    }
}

void
rand_eightwaydirection(int &dx, int &dy)
{
    dx = rand_sign();
    dy = rand_sign();

    if (rand_choice(2))
    {
	// Go orthogonal...
	if (rand_choice(2))
	    dx = 0;
	else
	    dy = 0;
    }
}

int
rand_dice(const DICE &dice, int multiplier)
{
    return rand_dice(dice.myNumdie * multiplier, dice.mySides, 
		     dice.myBonus * multiplier);
}

int
rand_diceMean(const DICE &dice, int multiplier)
{
    int		i, total = dice.myBonus*256*multiplier;

    for (i = 0; i < dice.myNumdie * multiplier; i++)
    {
	total += rand_rollMean(dice.mySides, 0, 256);
    }
    // Round off and normalize
    total += 128;
    total >>= 8;
    return total;
}

void
rand_shuffle(u8 *set, int n)
{
    int		i, j;
    u8		tmp;

    for (i = n-1; i > 0; i--)
    {
	// Want to swap with anything earlier, including self!
	j = rand_choice(i+1);
	
	tmp = set[i];
	set[i] = set[j];
	set[j] = tmp;
    }
}

void
rand_shuffle(int *set, int n)
{
    int		i, j;
    int		tmp;

    for (i = n-1; i > 0; i--)
    {
	// Want to swap with anything earlier, including self!
	j = rand_choice(i+1);
	
	tmp = set[i];
	set[i] = set[j];
	set[j] = tmp;
    }
}

void
getDirection(int dir, int &dx, int &dy)
{
    dir &= 3;
    const static int		dxtable[4] = { 0, 1, 0, -1 };
    const static int		dytable[4] = { 1, 0, -1, 0 };
    dx = dxtable[dir];
    dy = dytable[dir];
}

void
rand_angletodir(int angle, int &dx, int &dy)
{
    angle &= 7;

    const static int		dxtable[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };
    const static int		dytable[8] = { 0, 1, 1, 1, 0,-1,-1,-1 };

    dx = dxtable[angle];
    dy = dytable[angle];
}

int
rand_dirtoangle(int dx, int dy)
{
    int		x, y, a;

    for (a = 0; a < 8; a++)
    {
	rand_angletodir(a, x, y);
	if (x == dx && y == dy)
	    return a;
    }

    // This is 0,0, so we just return any angle!
    return rand_range(0, 7);
}


unsigned int
rand_wanginthash(unsigned int key)
{
    key += ~(key << 16);
    key ^=  (key >> 5);
    key +=  (key << 3);
    key ^=  (key >> 13);
    key += ~(key << 9);
    key ^=  (key >> 17);
    return key;
}

unsigned int
rand_hashstring(const char *s)
{
    unsigned int	hash = 0;
    if (!s)
	return hash;
    while (*s)
    {
	hash *= 37;		// Stolen from Perl.
	hash += *s;
	s++;
    }
    
    return rand_wanginthash(hash);
}


// Written in a beautiful provincial park in a nice cool June day.
// If this were traditional manuscript, it would smell of woodsmoke,
// but the curse of digital is the destruction of all sidebands
// of history.  Which I guess makes comments like this all the
// more important.
void
rand_name(char *text, int len)
{
    // Very simple markov generator.
    // We repeat letters to make them more likely.
    const char *vowels = "aaaeeeiiiooouuyy'";
    const char *frictive = "rsfhvnmz";
    const char *plosive = "tpdgkbc";
    const char *weird = "qwjx";
    // State transitions..
    // v -> f, p, w, v'
    // v' -> f, p, w
    // f -> p', v
    // p -> v, f'
    // w, p', f' -> v

    int		syllables = 0;
    char	state;
    int		pos = 0;
    bool	prime = false;

    // Initial state choice
    if (rand_chance(30))
	state = 'v';
    else if (rand_chance(40))
	state = 'f';
    else if (rand_chance(70))
	state = 'p';
    else
	state = 'w';

    while (pos < len-1)
    {
	// Apply current state
	switch (state)
	{
	    case 'v':
		text[pos++] = vowels[rand_choice(strlen(vowels))];
		if (!prime)
		    syllables++;
		break;
	    case 'f':
		text[pos++] = frictive[rand_choice(strlen(frictive))];
		break;
	    case 'p':
		text[pos++] = plosive[rand_choice(strlen(plosive))];
		break;
	    case 'w':
		text[pos++] = weird[rand_choice(strlen(weird))];
		break;
	}

	// Chance to stop..
	if (syllables && pos >= 3)
	{
	    if (rand_chance(20+pos*4))
		break;
	}

	// Transition...
	switch (state)
	{
	    case 'v':
		if (!prime && rand_chance(10))
		{
		    state = 'v';
		    prime = true;
		    break;
		}
		else if (rand_chance(40))
		    state = 'f';
		else if (rand_chance(70))
		    state = 'p';
		else
		    state = 'w';
		prime = false;
		break;
	    case 'f':
		if (!prime && rand_chance(50))
		{
		    prime = true;
		    state = 'p';
		    break;
		}
		state = 'v';
		prime = false;
		break;
	    case 'p':
		if (!prime && rand_chance(10))
		{
		    prime = true;
		    state = 'f';
		    break;
		}
		state = 'v';
		prime = false;
		break;
	    case 'w':
		state = 'v';
		prime = false;
		break;
	}
    }
    text[0] = toupper(text[0]);
    text[pos++] = '\0';
}
