#include <psp2/display.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <taihen.h>

#include "graphics.h"

#define printf psvDebugScreenPrintf

int main(int argc, char *argv[]) {

	psvDebugScreenInit();

	printf("Storage Format v1.0 by SKGleba\n\nAdding kernel patches...\n");
	tai_module_args_t argg;
	argg.size = sizeof(argg);
	argg.pid = KERNEL_PID;
	argg.args = 0;
	argg.argp = NULL;
	argg.flags = 0;
	SceUID mod_id = taiLoadStartKernelModuleForUser("ux0:app/SKGF0RM47/kernel.skprx", &argg); // add kernel patches
	
	if (mod_id >= 0) {
		printf("ok, launching the settings dlg...\n");
		sceAppMgrLaunchAppByUri(0xFFFFF, "settings_dlg:"); // in-app settings menu
		printf("removing patches in 2 seconds...\n");
		sceKernelDelayThread(2 * 1000 * 1000);
		printf("unload module: 0x%08X\n", taiStopUnloadKernelModuleForUser(mod_id, &argg, NULL, NULL)); // pointless but lol
		scePowerRequestColdReset(); // due to an unknown error in the patching system force reboot
	} else {
		printf("Error 0x%X\nexiting in 4 seconds...\n", mod_id);
		sceKernelDelayThread(4 * 1000 * 1000);
	}
	sceKernelExitProcess(0);
	return 0;
}
