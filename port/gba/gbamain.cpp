#ifdef main
#undef main
#endif
#define main gba_main

#include "../../main.cpp"

#undef main

#include <gba.h>

int main()
{
    // Before we call our main and crash, we try a toy NDS example.

    irqInit();

    // Call our main.
    gba_main();

    return 0;
}

