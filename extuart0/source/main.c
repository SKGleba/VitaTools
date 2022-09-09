#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>

// re-acquire on every deep resume
static int sysevent_handler(int resume, int eventid, void* args, void* opt) {
	if (resume && eventid == 0x400000)
		ksceSysconSetMultiCnPort(1);
	return 0;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void* argp) {

	// cmd 0x190 - acquire uart0 to kermit
	ksceSysconSetMultiCnPort(1);

	// add a sehandler to re-enable on every deep resume
	ksceKernelRegisterSysEventHandler("extuart0_se", sysevent_handler, NULL);

	// enable uart logging from low
	uint32_t nvs_data = 0;
	int ret = ksceSysconNvsReadData(0x481, &nvs_data, 1);
	if (ret || nvs_data) {
		nvs_data = 0;
		ksceSysconNvsWriteData(0x481, &nvs_data, 1);
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	return SCE_KERNEL_STOP_SUCCESS;
}
