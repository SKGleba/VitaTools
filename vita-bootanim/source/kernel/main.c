#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/display.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "vita-bootanim.h"

static SceUID anim_thread_id = 0;
static animation_header* header = NULL;
static void* animation = NULL, *camsram = NULL, * framebuffer[2] = { NULL, NULL };
static SceUID anim_block_id = 0, fb_block_id = 0, sram_block_id = 0;

unsigned int pa2va(unsigned int pa) {
	unsigned int va = 0, vaddr = 0, paddr = 0;
	for (int i = 0; i < 0x100000; i++) {
		vaddr = i << 12;
		__asm__("mcr p15,0,%1,c7,c8,0\n\t"
			"mrc p15,0,%0,c7,c4,0\n\t" : "=r" (paddr) : "r" (vaddr));
		if ((pa & 0xFFFFF000) == (paddr & 0xFFFFF000)) {
			va = vaddr + (pa & 0xFFF);
			break;
		}
	}
	return va;
}

int init_bootanim(const char *path) {
	SceIoStat stat;
	if (ksceIoGetstat(path, &stat) < 0)
		return -1;
	
	anim_block_id = ksceKernelAllocMemBlock("bootanim", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (stat.st_size + 0xffff) & 0xffff0000, NULL);
	ksceKernelGetMemBlockBase(anim_block_id, (void**)&animation);
	if (animation == NULL)
		return -2;
	
	int fd = ksceIoOpen(path, SCE_O_RDONLY, 0777);
	ksceIoRead(fd, animation, stat.st_size);
	ksceIoClose(fd);
	
	header = animation;
	if (header->magic != RCF_MAGIC || header->version != ANIM_VERSION)
		return -3;
	if (!header->anim_h || !header->anim_w)
		return -4;
	if (header->swap && header->sram)
		header->swap = 0;
	
	SceKernelAllocMemBlockKernelOpt optp;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	sram_block_id = ksceKernelAllocMemBlock("sramNT", (header->cache) ? 0x6020D006 : 0x60208006, 0x200000, &optp);
	ksceKernelGetMemBlockBase(sram_block_id, (void**)&camsram);
	if (camsram == NULL)
		return -5;
	
	if (header->sram)
		framebuffer[0] = camsram;
	else {
		if (header->fullres_frame) {
			ksceGzipDecompress(camsram, 960 * 544 * 4, (void*)(animation + sizeof(animation_header) + 4), NULL);
			ksceKernelCpuDcacheAndL2WritebackInvalidateRange(camsram, 960 * 544 * 4);
		}
		optp.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_PHYCONT | SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT;
		optp.alignment = 0x100000;
		fb_block_id = ksceKernelAllocMemBlock("phycont_anim", (header->cache) ? 0x6020D006 : 0x60208006, 0x400000, &optp);
		ksceKernelGetMemBlockBase(fb_block_id, (void**)&framebuffer[0]);
		if (framebuffer[0] == NULL)
			return -6;
		framebuffer[1] = (framebuffer[0] + 0x200000);
	}
	
	memset(framebuffer[0], 0, (header->sram) ? 0x200000 : 0x400000);
	
	return 0;
}

static int emergexit = 0, used_fb = 0;
static int play_bootanim(SceSize args, void* argp) {
	if (header->magic != RCF_MAGIC)
		goto exit_thread;
	
	SceDisplayFrameBuf fb;
	fb.size = sizeof(fb);
	fb.base = framebuffer[used_fb];
	fb.pixelformat = 0; // y no rgb F
	fb.pitch = header->anim_w;
	fb.height = header->anim_h;
	fb.width = (fb.pitch == 512) ? 480 : header->anim_w;
	
	if (ksceDisplaySetFrameBuf(&fb, 1) < 0)
		goto exit_thread;
	
	ksceDisplayWaitVblankStart();
	
	uint32_t cur_frame = 0;
	uint32_t cur_off = sizeof(animation_header);
	if (header->fullres_frame)
		cur_off -= -(*(uint32_t*)(animation + cur_off) + 4);
	while (!emergexit && ksceKernelSysrootGetShellPid() < 0) {
		
		if (cur_frame == header->frame_count) {
			if (header->loop) {
				cur_frame = 0;
				cur_off = sizeof(animation_header);
				if (header->fullres_frame)
					cur_off -= -(*(uint32_t*)(animation + cur_off) + 4);
			} else
				break;
		}
		
		if (header->vblank)
			ksceDisplayWaitVblankStart();

		if (header->swap)
			used_fb = !used_fb;

		if (ksceGzipDecompress(framebuffer[used_fb], fb.pitch * fb.height * 4, (void*)(animation + cur_off + 4), NULL) < 0)
			break;

		if (header->sweep)
			ksceKernelCpuDcacheAndL2WritebackInvalidateRange(framebuffer[used_fb], fb.pitch * fb.height * 4);

		if (header->swap) {
			fb.base = framebuffer[used_fb];
			ksceDisplaySetFrameBuf(&fb, 1);
		}

		cur_frame-=-1;
		cur_off-=-(*(uint32_t*)(animation + cur_off) + 4);
		
	}
	
	if (!header->sram) {
		fb.base = camsram;
		fb.pitch = 960;
		fb.height = 544;
		fb.width = 960;
		ksceDisplaySetFrameBuf(&fb, 1);
	}
		
exit_thread:
	if (anim_block_id > 0)
		ksceKernelFreeMemBlock(anim_block_id);
	if (fb_block_id > 0)
		ksceKernelFreeMemBlock(fb_block_id);
	if (sram_block_id > 0)
		ksceKernelFreeMemBlock(sram_block_id);
	ksceKernelExitDeleteThread(0);
	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	
	// exit if LT held
	if (~*(uint32_t*)(*(int*)(ksceSysrootGetSysbase() + 0x6c) + 0xCC) & 0x200)
		return SCE_KERNEL_START_SUCCESS;
	
	// speed up initial load before ScePower
	*(uint32_t*)pa2va(0xE3103000) = 0xF;
	*(uint32_t*)pa2va(0xE3103004) = 0;
	
	// alloc mem and read the bootanimation
	int ret = init_bootanim("ur0:tai/boot.rcf");
	if (ret < 0) {
		if (anim_block_id > 0)
			ksceKernelFreeMemBlock(anim_block_id);
		if (fb_block_id > 0)
			ksceKernelFreeMemBlock(fb_block_id);
		if (sram_block_id > 0)
			ksceKernelFreeMemBlock(sram_block_id);
		return SCE_KERNEL_START_FAILED;
	}
	
	// play the animation
	anim_thread_id = ksceKernelCreateThread("bootanim_thr", play_bootanim, 0x00, 0x1000, 0, 0, 0);
	ret = ksceKernelStartThread(anim_thread_id, 0, NULL);
	if (ret < 0) {
		if (anim_block_id > 0)
			ksceKernelFreeMemBlock(anim_block_id);
		if (fb_block_id > 0)
			ksceKernelFreeMemBlock(fb_block_id);
		if (sram_block_id > 0)
			ksceKernelFreeMemBlock(sram_block_id);
		return SCE_KERNEL_START_FAILED;
	}
	
	// TODO: multiple threads on diff cores?
	
	// set thread priority
	if (header->priority != 0xFF)
		ksceKernelChangeThreadPriority(anim_thread_id, header->priority);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	emergexit = 1;
	ksceKernelWaitThreadEnd(anim_thread_id, NULL, NULL);
	return SCE_KERNEL_STOP_SUCCESS;
}