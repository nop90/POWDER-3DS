#include <SDL.h>

#ifdef main
#undef main
#endif
#define main gba_main

#include "../../main.cpp"

#undef main

int main(int argc, char **argv)
{
    // Call our main.
    gba_main();
    
    SDL_Quit( );

    return 0;
}
