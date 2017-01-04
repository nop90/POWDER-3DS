// tile2c.cpp : Defines the entry point for the console application.
//

#include "bmp.h"

#include <stdio.h>


int 
main(int argc, char *argv[])
{
    // Ideally this would be generalized into a gfx2gba style program
    // with more options.  But I just want to get it working now.
    bmp_convertTileset();
    
    return 0;
}

