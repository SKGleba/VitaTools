#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>

static char *module_str[] = {
	"os0:kd/scePdAudio.skprx",
	"os0:kd/scePdCamera.skprx",
	"os0:kd/scePdCtrl.skprx",
	"os0:kd/scePdDisplay.skprx",
	"os0:kd/scePdDisplayLcd.skprx",
	"os0:kd/scePdDisplayOled.skprx",
	"os0:kd/scePdDs3UsbOnly.skprx",
	"os0:kd/scePdEther.skprx",
	"os0:kd/scePdHdmi.skprx",
	"os0:kd/scePdIdStorage.skprx",
	"os0:kd/scePdLed.skprx",
	"os0:kd/scePdMotion.skprx",
	"os0:kd/scePdMsif.skprx",
	"os0:kd/scePdPower.skprx",
	"os0:kd/scePdSdif.skprx",
	"os0:kd/scePdSyscon.skprx",
	"os0:kd/scePdTouch.skprx",
	"os0:kd/scePdUart.skprx",
	"os0:kd/scePdUdcdEval.skprx",
	"os0:kd/scePdUdcdMotion.skprx",
	"os0:kd/scePdWlanBtLab.skprx",
};

static int modids[21];

int diagBridgeLoadStartModule(int index) {
	int state = 0, opret = -1;
	if (index > 20 || modids[index] != 0)
		goto lerr;
	ENTER_SYSCALL(state);
	opret = ksceKernelLoadStartModule(module_str[index], 0, NULL, 0, NULL, NULL);
	if (opret >= 0)
		modids[index] = opret;
	EXIT_SYSCALL(state);
lerr:
	if (opret < 0)
		ksceDebugPrintf("[DIAG_BRIDGE] error loading pd[%d]: 0x%X\n", index, opret);
	else
		ksceDebugPrintf("[DIAG_BRIDGE] %s [%d] loaded: 0x%X\n", module_str[index], index, modids[index]);
	return opret;
}

int diagBridgeStopUnloadModule(int index) {
	int state = 0, opret = -1;
	if (index > 20 || modids[index] == 0)
		goto uerr;
	ENTER_SYSCALL(state);
	opret = ksceKernelStopUnloadModule(modids[index], 0, NULL, 0, NULL, NULL);
	if (opret >= 0)
		modids[index] = 0;
	EXIT_SYSCALL(state);
uerr:
	if (opret < 0)
		ksceDebugPrintf("[DIAG_BRIDGE] error stopping pd[%d]: 0x%X\n", index, opret);
	else
		ksceDebugPrintf("[DIAG_BRIDGE] %s [%d] stopped: 0x%X\n", module_str[index], index, modids[index]);
	return opret;
}

int diagBridgeGetLoadStatus(int index) {
	if (index > 20)
		return -1;
	ksceDebugPrintf("[DIAG_BRIDGE] status for %s [%d]: 0x%X\n", module_str[index], index, modids[index]);
	return (modids[index] == 0) ? 0 : 1;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	ksceDebugPrintf("[DIAG_BRIDGE] started\n");
	
	// set some useful dipsw
	ksceKernelClearDipsw(129);
	ksceKernelSetDipsw(159);
	ksceKernelSetDipsw(184);
	ksceKernelSetDipsw(185);
	ksceKernelSetDipsw(187);
	ksceKernelSetDipsw(197);
	ksceKernelSetDipsw(199);
	ksceKernelSetDipsw(224);
	ksceKernelSetDipsw(236);
	
	ksceDebugPrintf("[DIAG_BRIDGE] extra dipsw set\n");
	
	// PD modules are for 3.65
	if (*(uint32_t *)(*(int *)(ksceKernelGetSysbase() + 0x6c) + 4) != 0x03650000)
		return SCE_KERNEL_START_FAILED;
	
	memset(modids, 0, 21 * 4);
	
	ksceDebugPrintf("[DIAG_BRIDGE] ready\n");
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

