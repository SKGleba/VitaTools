/*
  fakegc
  Copyright 2021, skgleba

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>

#include <taihen.h>

static SceUID susHookID;
static tai_hook_ref_t susHookRef;

static int susPatched(int unk0, int unk1) {
  TAI_CONTINUE(int, susHookRef, unk0, unk1); // allow it to init all the buffers
  return 0;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  tai_module_info_t info;
  info.size = sizeof(info);
  if (taiGetModuleInfo("SceShell", &info) >= 0)
    susHookID = taiHookFunctionOffset(&susHookRef, info.modid, 0, (info.module_nid == 0x0552F692) ? 0x24f84 : 0x24fdc, 1, susPatched);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (susHookID >= 0)
    taiHookRelease(susHookID, susHookRef);
  return SCE_KERNEL_STOP_SUCCESS;
}
