#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <psp2/power.h>
#include <taihen.h>
#include <string.h>

#include "../kernel/vita-bootanim.h"

#define ANIM_PATH "ur0:tai/boot.rcf"

static animation_header header;

extern unsigned char _binary_theme_settings_xml_start;
extern unsigned char _binary_theme_settings_xml_size;

static SceUID g_hooks[6];

static void save_config_user(void) {
  SceUID fd;
  fd = sceIoOpen(ANIM_PATH, SCE_O_WRONLY, 6);
  if (fd >= 0) {
    sceIoWrite(fd, &header, sizeof(animation_header));
    sceIoClose(fd);
  }
}

static void load_config_user(void) {
  SceUID fd;
  fd = sceIoOpen(ANIM_PATH, SCE_O_RDONLY, 0);
  if (fd >= 0) {
    sceIoRead(fd, &header, sizeof(animation_header));
    sceIoClose(fd);
  }
}

static tai_hook_ref_t g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int *value) {
  if (sceClibStrncmp(category, "/CONFIG/VBANIM", 14) == 0) {
    if (value) {
      if (sceClibStrncmp(name, "cache", 5) == 0)
        *value = header.cache;
      else if (sceClibStrncmp(name, "prirt", 5) == 0)
        *value = (header.priority > 2 && header.priority != 4 && header.priority != 16) ? 2 : header.priority;
      else if (sceClibStrncmp(name, "sweep", 5) == 0)
        *value = header.sweep;
      else if (sceClibStrncmp(name, "blank", 5) == 0)
        *value = header.vblank;
      else if (sceClibStrncmp(name, "swpfb", 5) == 0)
        *value = header.swap;
      else if (sceClibStrncmp(name, "rsram", 5) == 0)
        *value = header.sram;
      else if (sceClibStrncmp(name, "loopa", 5) == 0)
        *value = header.loop;
      else if (sceClibStrncmp(name, "enbld", 5) == 0)
        *value = (header.magic == RCF_MAGIC);
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static tai_hook_ref_t g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrSetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int value) {
  if (sceClibStrncmp(category, "/CONFIG/VBANIM", 14) == 0) {
    if (sceClibStrncmp(name, "cache", 5) == 0)
      header.cache = value;
    else if (sceClibStrncmp(name, "prirt", 5) == 0)
      header.priority = value;
    else if (sceClibStrncmp(name, "sweep", 5) == 0)
      header.sweep = value;
    else if (sceClibStrncmp(name, "blank", 5) == 0)
      header.vblank = value;
    else if (sceClibStrncmp(name, "swpfb", 5) == 0)
      header.swap = value;
    else if (sceClibStrncmp(name, "rsram", 5) == 0)
      header.sram = value;
    else if (sceClibStrncmp(name, "loopa", 5) == 0)
      header.loop = value;
    else if (sceClibStrncmp(name, "enbld", 5) == 0)
      header.magic = (value) ? RCF_MAGIC : 'DEAD';
    save_config_user();
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

typedef struct {
  int size;
  const char *name;
  int type;
  int unk;
} SceRegMgrKeysInfo;

static tai_hook_ref_t g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched(const char *category, SceRegMgrKeysInfo *info, int unk) {
  if (sceClibStrncmp(category, "/CONFIG/VBANIM", 14) == 0) {
    if (info)
        info->type = 0x00040000; // type integer
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, category, info, unk);
}

static tai_hook_ref_t g_scePafMiscLoadXmlLayout_SceSettings_hook;
static int scePafMiscLoadXmlLayout_SceSettings_patched(int a1, void *xml_buf, int xml_size, int a4) {
  if (sceClibStrncmp(xml_buf+82, "theme_settings_plugin", 21) == 0) {
    xml_buf = (void *)&_binary_theme_settings_xml_start;
    xml_size = (int)&_binary_theme_settings_xml_size;
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
  }
  return TAI_CONTINUE(int, g_sceKernelStopUnloadModule_SceSettings_hook, modid, args, argp, flags, option, status);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  sceClibMemset(&header, 0, sizeof(animation_header));
  load_config_user();
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
  return SCE_KERNEL_STOP_SUCCESS;
}
