/*
 * PROPRIETARY INFORMATION.  This software is proprietary to POWDER
 * Development, and is not to be reproduced, transmitted, or disclosed
 * in any way without written permission.
 *
 * Produced by:	Jeff Lait
 *
 *      	POWDER Development
 *
 * NAME:        rand.h ( POWDER Library, C++ )
 *
 * COMMENTS:
 * 	All the good old random number routines that form the
 * 	backbone of any Roguelike.
 */

#ifndef __rand_h__
#define __rand_h__

struct DICE
{
    u16		myNumdie, mySides;
    s16		myBonus;
};

// Used to generate our percentile table.
#if 0
void rand_print();
#endif

void rand_pushstate();
void rand_popstate();

// A safe way to push/pop the state inside a function.
class RAND_STATEPUSH
{
public:
    RAND_STATEPUSH() { rand_pushstate(); }
    ~RAND_STATEPUSH() { rand_popstate(); }
};

// Needs to be called once.
void rand_init();

// Random in inclusive range.
int rand_range(int min, int max);

// Random from 0..num-1
int rand_choice(int num);

// Giving a null terminated list of strings, pick one of the strings
// at random
const char *rand_string(const char **stringlist);

// Rolls the specific number sided die.  The reroll count
// is how many rerolls you get.  Largest is returned.
// Result is between 1..num, unless num is 0, in which case it is 0,
// or negative, in which case -1..-num.
// Negative reroll means you get rerolls but the LEAST is returned.
int rand_roll(int sides, int reroll = 0);

// Returns the expected value of the rand_roll function, multiplied
// by the given scale, rounded to nearest.
int rand_rollMean(int sides, int reroll, int scale);

// Roll numdieDsides + bonus
int rand_dice(int numdie, int sides, int bonus);
int rand_dice(const DICE &dice, int multiplier = 1);
// Returns the expected result from rolling the dice.
int rand_diceMean(const DICE &dice, int multiplier = 1);

// Returns the current seed.
long rand_getseed();
// Sets the seed...
void rand_setseed(long seed);

// Returns true percentage % of the time.
bool rand_chance(int percentage);

// Returns -1 or 1.
int  rand_sign();

// Initializes dx & dy, only one of which is non-zero.
void	rand_direction(int &dx, int &dy);

// Initializes dx & dy, both of which are not zero.
void	rand_eightwaydirection(int &dx, int &dy);

// Given a direction from 0..3, returns the dx,dy pair for that direction.
void	getDirection(int dir, int &dx, int &dy);

// Given an angle 0..7, returns dx, dy pair.
void	rand_angletodir(int angle, int &dx, int &dy);
// Given a dx & dy, returns the angle.
int	rand_dirtoangle(int dx, int dy);

// Shuffles the given set of numbers...
void	rand_shuffle(u8 *set, int n);
void	rand_shuffle(int *set, int n);

unsigned int	rand_wanginthash(unsigned int key);
unsigned int	rand_hashstring(const char *s);

// Never use this as floats are attrociously slow on GBA
double rand_double();

// Builds a random player name
void		rand_name(char *text, int len);

// Some standard methods...
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define BOUND(val, low, high) MAX(MIN(val, high), low)
#define SIGN(a) ((a) < 0 ? -1 : (a) > 0 ? 1 : 0)
#define ABS(a) ((a) < 0 ? -(a) : (a))

// Useful to iterate over all directions.
#define FORALL_4DIR(dx, dy) \
    for (int lcl_angle = 0; getDirection(lcl_angle, dx, dy), lcl_angle < 4; lcl_angle++)

#define FORALL_8DIR(dx, dy) \
    for (int lcl_angle = 0; rand_angletodir(lcl_angle, dx, dy), lcl_angle < 8; lcl_angle++)

#endif
