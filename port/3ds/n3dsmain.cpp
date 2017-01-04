
#define APP_TITLE "Powder"

// 3DS

#include <sys/stat.h>
#include <stdio.h>
#include <3ds.h>
#include "sf2d.h"

extern int gba_main(void);


static bool task_quit;
static Handle task_pause_event;
static Handle task_suspend_event;
static aptHookCookie cookie;
static int muspaused=0;

static void task_apt_hook(APT_HookType hook, void* param) {
    switch(hook) {
        case APTHOOK_ONSUSPEND:
            svcClearEvent(task_suspend_event);
			CSND_SetPlayState(15, 0);//Stop music audio playback.
			csndExecCmds(0);
			muspaused=1;
            break;
        case APTHOOK_ONSLEEP:
            svcClearEvent(task_pause_event);
            break;
        default:
            break;
    }
}

void task_init() {
    task_quit = false;

    svcCreateEvent(&task_pause_event, RESET_STICKY);

    svcCreateEvent(&task_suspend_event, RESET_STICKY);

    svcSignalEvent(task_pause_event);
    svcSignalEvent(task_suspend_event);

    aptHook(&cookie, task_apt_hook, NULL);
}

void task_exit() {
    task_quit = true;

    aptUnhook(&cookie);

    if(task_pause_event != 0) {
        svcCloseHandle(task_pause_event);
        task_pause_event = 0;
    }

    if(task_suspend_event != 0) {
        svcCloseHandle(task_suspend_event);
        task_suspend_event = 0;
    }
}

int main()
{
	sf2d_init(); 
	sf2d_set_3D(false);
	sf2d_set_vblank_wait(true);

 	osSetSpeedupEnable(true);
	romfsInit();

	consoleInit(GFX_BOTTOM, NULL);
	printf("Console init...ok\n");

	task_init();

	mkdir("/3ds", 0777);
	mkdir("/3ds/Powder", 0777);

  // run
  gba_main();

//  N3ds_Quit();
  return 0;
}
