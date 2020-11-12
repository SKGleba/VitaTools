#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <psp2/power.h>
#include <taihen.h>
#include <string.h>
#include "FatFormatProxy.h"

// custom system settings menu
extern unsigned char _binary_system_settings_xml_start;
extern unsigned char _binary_system_settings_xml_size;

// storage sdstor0 paths
static char *stor_str[] = {
	"sdstor0:ext-lp-ign-entire",
	"sdstor0:uma-pp-act-a",
	"sdstor0:uma-lp-act-entire",
	"sdstor0:int-lp-ign-userext",
	"sdstor0:xmc-lp-ign-userext"
};

static int storno = 0, target_fs = F_TYPE_EXFAT;
static SceUID g_hooks[7];

static tai_hook_ref_t g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int *value) {
  if (sceClibStrncmp(category, "/CONFIG/FRDR", 12) == 0) {
    if (value) {
      if (sceClibStrncmp(name, "target", 6) == 0)
		*value = storno;
	  else if (sceClibStrncmp(name, "fstype", 6) == 0)
	    *value = target_fs;
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static tai_hook_ref_t g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrSetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int value) {
  if (sceClibStrncmp(category, "/CONFIG/FRDR", 12) == 0) {
	if (sceClibStrncmp(name, "fstype", 6) == 0)
	  target_fs = value;
    else if (sceClibStrncmp(name, "target", 6) == 0)
      storno = value;
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static int (* g_OnButtonEventSettings_hook)(const char *id, int a2, void *a3);
static int OnButtonEventSettings_patched(const char *id, int a2, void *a3) {
  if (sceClibStrncmp(id, "id_frdr_format", 14) == 0)
    return formatBlkDev(stor_str[storno], target_fs, (storno < 3) ? 1 : 0);
  else if (sceClibStrncmp(id, "id_frdr_reboot", 14) == 0)
    return scePowerRequestColdReset();
  return g_OnButtonEventSettings_hook(id, a2, a3);
}

static tai_hook_ref_t g_scePafToplevelInitPluginFunctions_SceSettings_hook;
static int scePafToplevelInitPluginFunctions_SceSettings_patched(void *a1, int a2, uint32_t *funcs) {
  int res = TAI_CONTINUE(int, g_scePafToplevelInitPluginFunctions_SceSettings_hook, a1, a2, funcs);
  if (funcs[6] != (uint32_t)OnButtonEventSettings_patched) {
    g_OnButtonEventSettings_hook = (void *)funcs[6];
    funcs[6] = (uint32_t)OnButtonEventSettings_patched;
  }
  return res;
}

typedef struct {
  int size;
  const char *name;
  int type;
  int unk;
} SceRegMgrKeysInfo;

static tai_hook_ref_t g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched(const char *category, SceRegMgrKeysInfo *info, int unk) {
  if (sceClibStrncmp(category, "/CONFIG/FRDR", 12) == 0) {
    if (info) {
        info->type = 0x00040000; // type integer
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, category, info, unk);
}

static tai_hook_ref_t g_scePafMiscLoadXmlLayout_SceSettings_hook;
static int scePafMiscLoadXmlLayout_SceSettings_patched(int a1, void *xml_buf, int xml_size, int a4) {
  if ((82+22) < xml_size && sceClibStrncmp(xml_buf+82, "system_settings_plugin", 22) == 0) {
    xml_buf = (void *)&_binary_system_settings_xml_start;
    xml_size = (int)&_binary_system_settings_xml_size;
  }
  return TAI_CONTINUE(int, g_scePafMiscLoadXmlLayout_SceSettings_hook, a1, xml_buf, xml_size, a4);
}

static SceUID g_system_settings_core_modid = -1;
static tai_hook_ref_t g_sceKernelLoadStartModule_SceSettings_hook;
static SceUID sceKernelLoadStartModule_SceSettings_patched(char *path, SceSize args, void *argp, int flags, SceKernelLMOption *option, int *status) {
  SceUID ret = TAI_CONTINUE(SceUID, g_sceKernelLoadStartModule_SceSettings_hook, path, args, argp, flags, option, status);
  if (ret >= 0 && sceClibStrncmp(path, "vs0:app/NPXS10015/system_settings_core.suprx", 44) == 0) {
    g_system_settings_core_modid = ret;
    g_hooks[2] = taiHookFunctionImport(&g_scePafMiscLoadXmlLayout_SceSettings_hook, 
                                        "SceSettings", 
                                        0x3D643CE8, // ScePafMisc
                                        0x19FE55A8, 
                                        scePafMiscLoadXmlLayout_SceSettings_patched);
    g_hooks[3] = taiHookFunctionImport(&g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x16DDF3DC, 
                                        sceRegMgrGetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[4] = taiHookFunctionImport(&g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0xD72EA399, 
                                        sceRegMgrSetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[5] = taiHookFunctionImport(&g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x58421DD1, 
                                        sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched);
	g_hooks[6] = taiHookFunctionImport(&g_scePafToplevelInitPluginFunctions_SceSettings_hook, 
                                        "SceSettings", 
                                        0x4D9A9DD0, // ScePafToplevel
                                        0xF5354FEF, 
                                        scePafToplevelInitPluginFunctions_SceSettings_patched);
  }
  return ret;
}

static tai_hook_ref_t g_sceKernelStopUnloadModule_SceSettings_hook;
static int sceKernelStopUnloadModule_SceSettings_patched(SceUID modid, SceSize args, void *argp, int flags, SceKernelULMOption *option, int *status) {
  if (modid == g_system_settings_core_modid) {
    g_system_settings_core_modid = -1;
    if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
    if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
	if (g_hooks[6] >= 0) taiHookRelease(g_hooks[6], g_scePafToplevelInitPluginFunctions_SceSettings_hook);
  }
  return TAI_CONTINUE(int, g_sceKernelStopUnloadModule_SceSettings_hook, modid, args, argp, flags, option, status);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) { 
  g_hooks[0] = taiHookFunctionImport(&g_sceKernelLoadStartModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2DCC4AFA, 
                                      sceKernelLoadStartModule_SceSettings_patched);
  g_hooks[1] = taiHookFunctionImport(&g_sceKernelStopUnloadModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2415F8A4, 
                                      sceKernelStopUnloadModule_SceSettings_patched);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  // free hooks that didn't fail
  if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], g_sceKernelLoadStartModule_SceSettings_hook);
  if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], g_sceKernelStopUnloadModule_SceSettings_hook);
  if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
  if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
  if (g_hooks[6] >= 0) taiHookRelease(g_hooks[6], g_scePafToplevelInitPluginFunctions_SceSettings_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}
