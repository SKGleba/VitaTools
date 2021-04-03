#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <taihen.h>

#include "Archives.h"
#include "promote.c"
#include "incoming.c"
#include "bgvpk.h"
#include "offsets.c"

static SceUID hooks[6];
static tai_hook_ref_t ExportFileRef;
static tai_hook_ref_t GetFileTypeRef;
static const unsigned char nop32[4] = { 0xaf, 0xf3, 0x00, 0x80 };

static int ExportFilePatched(uint32_t* data) {
    
    int res = TAI_CONTINUE(int, ExportFileRef, data);

    if (res == 0x80101A09) {
        char download_path[1024];
        char bgdl_path[1024];
        char file_name[128];
        char dl_title[64];
        char flags[0xB];
        int cbg = 0;
        uint32_t num = *(uint32_t*)data[0];

        sceClibSnprintf(bgdl_path, sizeof(bgdl_path), "ux0:bgdl/t/%08x/d0.pdb", num);

        // Get bgdl title and file name
        uint16_t cur_off = 0xD3;
        SceUID fd = sceIoOpen(bgdl_path, SCE_O_RDONLY, 0);
        if (fd < 0)
            return fd;
        sceIoPread(fd, flags, 0xB, 0xD3); // read title flags
        sceIoPread(fd, dl_title, *(uint16_t*)(flags + 3), 0xDE); // read the title itself
        cur_off -= -(*(uint16_t*)(flags + 3) + 0xC);
        sceIoPread(fd, flags, 0xB, cur_off); // read flags for the next entry
        if (*(uint16_t*)(flags + 3) == 1) { // skip this entry, title is probably the URL and the next entry is the file
            cur_off -= -(*(uint16_t*)(flags + 3) + 0xC);
            sceIoPread(fd, flags, 0xB, cur_off);
            sceIoPread(fd, file_name, *(uint16_t*)(flags + 3), cur_off + 0xB);
        } else { // this entry is the target file from custom bgdl
            sceIoPread(fd, file_name, *(uint16_t*)(flags + 3), cur_off + 0xB);
            cbg = 1;
        }
        sceIoClose(fd);

        // Check if the file is a VPK
        int vpk = sceClibStrnlen(file_name, 128);
        if (vpk > 4)
            vpk = (file_name[vpk - 4] == '.' && file_name[vpk - 3] == 'v' && file_name[vpk - 2] == 'p' && file_name[vpk - 1] == 'k');
        else
            vpk = 0;
        
        sceClibSnprintf(bgdl_path, sizeof(bgdl_path), "ux0:bgdl/t/%08x/%s", num, file_name);
        
        // If custom bgdled or a VPK - install
        if (cbg || vpk) {
            // Get additional param (optional)
            int show_dlg = 1, dl_target = 1, custom_titleid = 0;
            sceClibSnprintf(download_path, sizeof(download_path), "ux0:bgdl/t/%08x/export_param.ini", num);
            bgvpk_export_param_struct export_params;
            export_params.magic = 0;
            fd = sceIoOpen(download_path, SCE_O_RDONLY, 0);
            if (fd >= 0) {
                sceIoRead(fd, &export_params, 16);
                sceIoClose(fd);
                if (export_params.magic == (BGVPK_MAGIC | BGVPK_CFG_VER)) {
                    show_dlg = export_params.show_dlg;
                    dl_target = export_params.target;
                    custom_titleid = export_params.titleid[0];
                }
            } else
                dl_target = vpk;
            
            // Ask the user if they want to install the app
            int reply = 1;
            if (show_dlg && dl_target) {
                char indlg_msg[0x100];
                dialog_init(0); // NOTE: this may error but still work (lol)
                sceClibSnprintf(indlg_msg, 0x100, "%s is ready for install", dl_title);
                if (dialog_show((custom_titleid) ? export_params.titleid : "VITASHELL", indlg_msg, "Save package", "Install") < 0)
                    goto save_file;
                reply = dialog_wait(); // Wait for user reply
                dialog_close();
                dialog_deinit();
            }
            
            // Install the app
            if (reply) {
                // Unzip the VPK to temp bgdl folder/X/ | Unzip the zip to ux0:data/
                sceClibSnprintf(download_path, sizeof(download_path), "ux0:bgdl/t/%08x/X", num);
                Zip* handle = ZipOpen(bgdl_path);
                res = ZipExtract(handle, NULL, (dl_target) ? download_path : "ux0:data");
                ZipClose(handle);
                if (res != 0)
                    return -1;
                if (!dl_target)
                    return 0;
                
                // Install/promote the app
                res = promoteApp(download_path);
                
                // TODO: Notif "installed"
                
                return res;
            }
        }
            
save_file: // Save the downloaded file to ux0:download/*
        sceClibSnprintf(download_path, sizeof(download_path), "ux0:download/%s", file_name);
        res = sceIoMkdir("ux0:download", 0006);
        if (res < 0 && res != 0x80010011)
            return res;
        sceIoRemove(download_path);
        return sceIoRename(bgdl_path, download_path);
    }

    return res;
}

static int GetFileTypePatched(int unk, int* type, char** filename, char** mime_type) {
    int res = TAI_CONTINUE(int, GetFileTypeRef, unk, type, filename, mime_type);

    if (res == 0x80103A21) {
        *type = 1; // Type photo
        return 0;
    }

    return res;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void* argp) {
    tai_module_info_t den_info;
    den_info.size = sizeof(den_info);
    if (taiGetModuleInfo("DownloadEnabler", &den_info) >= 0)
        sceKernelStopUnloadModule(den_info.modid, 0, NULL, 0, NULL, NULL);
    
    tai_module_info_t info;
    info.size = sizeof(info);
    uint32_t get_off, exp_off, rec_off, lock_off;
    if (taiGetModuleInfo("SceShell", &info) >= 0 && get_shell_offsets(info.module_nid, &get_off, &exp_off, &rec_off, &lock_off) >= 0) {
        hooks[0] = taiInjectData(info.modid, 0, get_off, "GET", 4);
        if (hooks[0] >= 0) { // we use the fact that there can be only one tai inject per offset to make sure we dont hook twice
            hooks[1] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, exp_off, 1, ExportFilePatched);
            hooks[2] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, rec_off, 1, GetFileTypePatched);
            hooks[3] = taiInjectData(info.modid, 0, lock_off, nop32, 4);
            hooks[4] = taiInjectData(info.modid, 0, lock_off + 8, nop32, 4);
            hooks[5] = taiInjectData(info.modid, 0, lock_off + 16, nop32, 4);
        }
    } else
        return SCE_KERNEL_START_FAILED;

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void* argp) {
    if (hooks[5] >= 0)
        taiInjectRelease(hooks[5]);
    if (hooks[4] >= 0)
        taiInjectRelease(hooks[4]);
    if (hooks[3] >= 0)
        taiInjectRelease(hooks[3]);
    if (hooks[2] >= 0)
        taiHookRelease(hooks[2], GetFileTypeRef);
    if (hooks[1] >= 0)
        taiHookRelease(hooks[1], ExportFileRef);
    if (hooks[0] >= 0)
        taiInjectRelease(hooks[0]);

    return SCE_KERNEL_STOP_SUCCESS;
}