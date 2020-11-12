#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <taihen.h>

static int starthook = 0;

// load our user module asap
static tai_hook_ref_t g_start_preloaded_modules_hook;
static int start_preloaded_modules_patched(SceUID pid) {
	int ret = TAI_CONTINUE(int, g_start_preloaded_modules_hook, pid);
	ksceKernelLoadStartModuleForPid(pid, "ux0:app/SKGF0RM47/user.suprx", 0, NULL, 0, NULL, NULL);
	taiHookReleaseForKernel(starthook, g_start_preloaded_modules_hook); // pRo CoDe
	return ret;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	
	// allow GC-SD
	char movsr01[2] = {0x01, 0x20};
	int sysmem_modid = ksceKernelSearchModuleByName("SceSysmem");
	if (taiInjectDataForKernel(KERNEL_PID, sysmem_modid, 0, 0x21610, movsr01, sizeof(movsr01)) < 0)
		return SCE_KERNEL_START_FAILED;
	
	// patch start_loaded_modules_for_pid to loadstart our umodule
	starthook = taiHookFunctionExportForKernel(KERNEL_PID, &g_start_preloaded_modules_hook, "SceKernelModulemgr", 0xC445FA63, 0x432DCC7A, start_preloaded_modules_patched);
	if (starthook < 0) { // probs 3.65
		starthook = taiHookFunctionExportForKernel(KERNEL_PID, &g_start_preloaded_modules_hook, "SceKernelModulemgr", 0x92C9FFC2, 0x998C7AE9, start_preloaded_modules_patched);
		if (starthook < 0) // :shrug:
			return SCE_KERNEL_START_FAILED;
	}
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	if (starthook >= 0)
		taiHookReleaseForKernel(starthook, g_start_preloaded_modules_hook);
	return SCE_KERNEL_STOP_SUCCESS;
}
