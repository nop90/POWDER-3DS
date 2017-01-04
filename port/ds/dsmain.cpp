#ifdef main
#undef main
#endif
#define main gba_main

#include "../../main.cpp"

#undef main

#include <nds.h>

int main()
{
    // Before we call our main and crash, we try a toy NDS example.

    irqInit();
//    irqSet(IRQ_VBLANK, Vblank);
//   irqEnable(IRQ_VBLANK);
    videoSetMode(0);	//not using the main screen
    videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
    vramSetBankC(VRAM_C_SUB_BG); 
//    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankA(VRAM_A_LCD);

    SUB_BG0_CR = BG_MAP_BASE(31);

    int		i;
    for (i = 0; i < 255; i++)
    {
	BG_PALETTE_SUB[i] = RGB15(0,0,0);
    }
    
    BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255
    
    //consoleInit() is a lot more flexible but this gets you up and running quick
    consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

    // Push the cursor to the bottom so stuff seems to scroll out of
    // the top of the play window.
    for (int i = 0; i < 30; i++)
    {
	iprintf("\n");
    }

    iprintf("         POWDER\n");
    iprintf(" www.zincland.com/powder\n");

#if 0
    while(1) {
	    swiWaitForVBlank();
    }

#endif

    // We want the main window to be on the bottom as that is most useful
    // for all stylus actions.  It also ensures that text scrolls as expected.
    lcdMainOnBottom();
    
    // Call our main.
    gba_main();

    return 0;
}

