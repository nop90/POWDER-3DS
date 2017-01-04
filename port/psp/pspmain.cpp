
#include <SDL.h>
//#include "hamfake.h"
#define APP_TITLE "Powder"

// PSP

#define SDL_main main
#include <pspsdk.h>
#include <psppower.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

PSP_MODULE_INFO(APP_TITLE, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
//PSP_HEAP_SIZE_KB(20480);

#define printf pspDebugScreenPrintf

int ExitCallback(int arg1, int arg2, void *common)
{
  sceKernelExitGame();
  return 0;
}

int CallbackThread(SceSize args, void *argp)
{
  int cbid;
  cbid = sceKernelCreateCallback("exit_callback", ExitCallback, NULL);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB();
  return 0;
}

int SystemSetup(void)
{
  scePowerSetClockFrequency(333, 333, 166);
  sceDisplaySetMode(0, 480, 272);
  pspDebugScreenInit();

  int thid = 0;
  thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
  if (thid >= 0)
    sceKernelStartThread(thid, 0, 0);

  return thid;
}

void SystemShutdown()
{
  sceKernelSleepThread();
}


#ifdef main
#undef main
#endif
#define main gba_main

#include "../../main.cpp"

#undef main

int main(int argc, char *argv[])
{
  SystemSetup();

  // run
  gba_main();

  SDL_Quit();
  SystemShutdown();
  return 0;
}
