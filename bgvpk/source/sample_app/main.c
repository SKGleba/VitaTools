#include <stdio.h>
#include <sys/syslimits.h>
#include <stdlib.h>
#include <string.h>
#include <vitasdk.h>
#include <taihen.h>

#include "graphics.h"
#include "bgdl.c"
#include "../bgvpk.h"

#define APP_PATH "ux0:app/SKGSMPL3E/"

// Load bgvpk.suprx to shell, dont worry about double-load, it has checks
int load_bgvpk(void) {
	tai_module_args_t args;
	int pid = 0;
	if (sceAppMgrGetIdByName(&pid, "NPXS19999") >= 0 && pid != 0) {
		args.size = sizeof(args);
		args.pid = pid;
		args.args = 0;
		args.argp = NULL;
		args.flags = 0;
		return taiLoadStartModuleForPidForUser(APP_PATH "bgvpk.suprx", &args);
	} else
		return -1;
}

/*
 Create a param file for bgvpk's export of [bgdlid]
 - [show_dialog]: set to ask user before installing the VPK
 - [vpk]: if set to 0 it will unzip the contents to ux0:data/
 - [caller_id]: titleid of the app which will be the dialogMastah xd
*/
int add_bgvpk_params(int bgdlid, int show_dialog, int vpk, const char *caller_id) {
	char export_cfg_path[128];
	bgvpk_export_param_struct export_params;
	export_params.magic = (BGVPK_MAGIC | BGVPK_CFG_VER);
	export_params.show_dlg = show_dialog;
	export_params.target = vpk;
	if (caller_id)
		sceClibStrncpy(export_params.titleid, caller_id, 11);
	sceClibSnprintf(export_cfg_path, sizeof(export_cfg_path), "ux0:bgdl/t/%08x/export_param.ini", bgdlid);
	int fd = sceIoOpen(export_cfg_path, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
	if (fd < 0)
		return fd;
	int res = sceIoWrite(fd, &export_params, 16);
	sceIoClose(fd);
	if (res < 0)
		return res;
	return bgdlid;
}

int main(int argc, char **argv){

	psvDebugScreenInit();

	// load bgvpk
	// NOTE: it will fail if unknown shell or other error, it will NOT fail if already loaded
	int res = load_bgvpk();
	psvDebugScreenPrintf("load_bgvpk: %x\n", res);
	
	// init the bgdl service
	res = bgdl_init();
	psvDebugScreenPrintf("bgdl_init: %x\n", res);
	
	// download VPK and ask user to install or save
	res = bgdl_queue("Vita Save Manager", "http://hen.13375384.xyz/vpk/savemgr.vpk");
	psvDebugScreenPrintf("0: start download with ID %X\n", res);
	
	// download VPK and install in the BG
	res = bgdl_queue("Registry Editor", "http://hen.13375384.xyz/vpk/reg.vpk");
	if (res >= 0)
		res = add_bgvpk_params(res, 0, 1, NULL);
	psvDebugScreenPrintf("1: start download with ID %X\n", res);
	
	// download zip and extract to ux0:data
	res = bgdl_queue("Vanilla taiHEN configuration", "http://hen.13375384.xyz/tai/v_tai.zip");
	if (res >= 0)
		res = add_bgvpk_params(res, 0, 0, NULL);
	psvDebugScreenPrintf("2: start download with ID %X\n", res);
	
	// download VPK and ask user to install or save (custom callerID - Content Manager)
	res = bgdl_queue("batteryFixer", "http://hen.13375384.xyz/vpk/battery.vpk");
	if (res >= 0)
		res = add_bgvpk_params(res, 1, 1, "NPXS10026");
	psvDebugScreenPrintf("3: start download with ID %X\n", res);
	
	// while(1){};
	
	return 0;
}
