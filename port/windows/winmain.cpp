/* -- Include the precompiled libraries -- */
#ifdef WIN32
#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#endif

#include "SDL.h"

#ifdef _WIN32_WCE
#include <windows.h>

char *strdup(const char *s)
{
    return _strdup(s);
}
#endif

#ifdef main
#undef main
#endif
#define main gba_main

#include "../../main.cpp"

#undef main

#include <windows.h>

// We suppress our console window in this fashion.
#ifdef WIN32
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int 
main(int argc, char **argv)
#endif
{
    // Call our main.
    gba_main();
    
    SDL_Quit( );

    return 0;
}
