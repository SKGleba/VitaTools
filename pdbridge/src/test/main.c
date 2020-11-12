#include <stdio.h>
#include <taihen.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include "debugScreen.h"

#define printf(...) psvDebugScreenPrintf(__VA_ARGS__)

void wait_key_press(int mode)
{
	SceCtrlData pad;

	printf("Press %s.\n", (mode) ? "START" : "SELECT to exit");

	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if ((mode) && (pad.buttons & SCE_CTRL_START))
			break;
		else if ((!mode) && (pad.buttons & SCE_CTRL_SELECT))
			break;
		sceKernelDelayThread(200 * 1000);
	}
}

void do_test() {
	printf("unk_set_colorfuckery: 0x%X\n", ScePdDisplayOled_BFA0B350_some_dark_mode(0xFA));
	char buf[0x100];
	ScePdDisplayOled_2C4D117E_get_buf(3, buf);
	printf("POWER_MODE: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(4, buf);
	printf("ADDR_MODE: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(5, buf);
	printf("PIXEL_FORMAT: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(6, buf);
	printf("DISP_MODE: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(7, buf);
	printf("DIAG_RESULT: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(0xB, buf);
	printf("DDB_START: %02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	ScePdDisplayOled_2C4D117E_get_buf(0xC, buf);
	printf("GLOBAL_PARAM: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(0xD, buf);
	printf("ELVSS_CON: %02X%02X%02X\n", buf[0], buf[1], buf[2]);
	ScePdDisplayOled_2C4D117E_get_buf(0xE, buf);
	printf("TEMP_SWIRE: %02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[4]);
	ScePdDisplayOled_2C4D117E_get_buf(0xF, buf);
	printf("SCR_CONTROL: %02X\n", buf[0]);
	ScePdDisplayOled_2C4D117E_get_buf(0x1E, buf);
	printf("PASSWD1: %02X%02X\n", buf[0], buf[1]);
	ScePdDisplayOled_2C4D117E_get_buf(0x1F, buf);
	printf("PASSWD2: %02X%02X\n", buf[0], buf[1]);
	ScePdDisplayOled_2C4D117E_get_buf(0x20, buf);
	printf("PWR_SEQ: %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
	//printf("set: 0x%X\n", ScePdDisplayOled_9218ED1B_set_buf(buf));
}

int main(int argc, char *argv[])
{
	int ret = diagBridgeLoadStartModule(5);
	if (ret > 0)
		sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);
		
	psvDebugScreenInit();
	wait_key_press(1);
	do_test();
	wait_key_press(0);
	
	diagBridgeStopUnloadModule(5);

	return 0;
}
