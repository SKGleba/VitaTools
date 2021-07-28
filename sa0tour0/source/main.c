/*
	sa0tour0
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

/*
	This software contains code derived from gamecard-microsd.
	Included below is the license header from gamecard-microsd.
*/

/*
	gamecard-microsd
	Copyright 2017-2018, xyz

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

/*
	This software contains code derived from VitaShell.
	Included below is the license header from VitaShell.
*/

/*
	VitaShell
	Copyright (C) 2015-2017, TheFloW

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

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

#define SA0_MOUNT_POINT_ID 0xB00

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

typedef struct {
	const char *dev;
	const char *dev2;
	const char *blkdev;
	const char *blkdev2;
	int id;
} SceIoDevice;

typedef struct {
	int id;
	const char *dev_unix;
	int unk;
	int dev_major;
	int dev_minor;
	const char *dev_filesystem;
	int unk2;
	SceIoDevice *dev;
	int unk3;
	SceIoDevice *dev2;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
} SceIoMountPoint;

static SceIoDevice ur0_sa0_dev = { "sa0:", "exfatsa0", "sdstor0:int-lp-ina-user", "sdstor0:int-lp-ina-user", SA0_MOUNT_POINT_ID };

static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void* argp) {

	// Check if the default font is present in ur0
	int fd = ksceIoOpen("ur0:data/font/pvf/ltn0.pvf", SCE_O_RDONLY, 0);
	if (fd < 0)
		return SCE_KERNEL_START_FAILED;
	ksceIoClose(fd);

	// Get tai module info
	tai_module_info_t info;
	info.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &info) < 0)
		return -1;

	// Get important function
	switch (info.module_nid) {
	case 0x9642948C: // 3.60 retail
		module_get_offset(KERNEL_PID, info.modid, 0, 0x138C1, (uintptr_t*)&sceIoFindMountPoint);
		break;

	case 0xA96ACE9D: // 3.65 retail
	case 0x3347A95F: // 3.67 retail
	case 0x90DA33DE: // 3.68 retail
		module_get_offset(KERNEL_PID, info.modid, 0, 0x182F5, (uintptr_t*)&sceIoFindMountPoint);
		break;

	case 0xF16E72C7: // 3.69 retail
	case 0x81A49C2B: // 3.70 retail
	case 0xF2D59083: // 3.71 retail
	case 0x9C16D40A: // 3.72 retail
		module_get_offset(KERNEL_PID, info.modid, 0, 0x18735, (uintptr_t*)&sceIoFindMountPoint);
		break;

	default:
		return SCE_KERNEL_START_FAILED;
	}

	SceIoMountPoint* mount = sceIoFindMountPoint(SA0_MOUNT_POINT_ID);
	if (!mount)
		return -1;
	
	mount->dev = &ur0_sa0_dev;
	mount->dev2 = &ur0_sa0_dev;

	ksceIoUmount(SA0_MOUNT_POINT_ID, 0, 0, 0);
	ksceIoUmount(SA0_MOUNT_POINT_ID, 1, 0, 0);
	ksceIoMount(SA0_MOUNT_POINT_ID, NULL, 0, 0, 0, 0);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	return SCE_KERNEL_STOP_SUCCESS;
}
