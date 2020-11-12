/*
	( PoC ) Hook SceSettings to redirect memory card formats to YAMT's format func
	- To redirect the mc format menu's options load this plugin for NPXS10015
	- To redirect @boot mc format popup load this plugin for NPXS10016
	Most apps/plugins like shell just call "reset::format_mc_for_promote" under NPXS10016
*/

#include <psp2/kernel/modulemgr.h>
#include <taihen.h>

#include "FatFormatProxy.h"

int format_gen_uid = 0;

static tai_hook_ref_t format_gen_ref;
int format_gen_patched(int master, int slave) {
	return (master == 8) ? formatBlkDev("sdstor0:ext-lp-ign-entire", F_TYPE_EXFAT, 1) : TAI_CONTINUE(int, format_gen_ref, master, slave);
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp){

	tai_module_info_t info;
	info.size = sizeof(info);

	if(taiGetModuleInfo("SceSettings", &info) >= 0) {
		uint32_t fgen_off = 0;
		module_get_offset(info.modid, 0, 0x15b62b, &fgen_off);
		if (*(uint16_t *)fgen_off == 0xf0e9)
			format_gen_uid = taiHookFunctionOffset(&format_gen_ref, info.modid, 0, 0x15b62a, 1, format_gen_patched);
		else
			format_gen_uid = taiHookFunctionOffset(&format_gen_ref, info.modid, 0, 0x15b626, 1, format_gen_patched);
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	taiHookRelease(format_gen_uid, format_gen_ref);
	return SCE_KERNEL_STOP_SUCCESS;
}
