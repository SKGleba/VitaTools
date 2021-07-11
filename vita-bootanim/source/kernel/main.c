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

// ungzip optimized for rcf
int ungzip_frame(void *dst, void *src, uint32_t size, int clean_cache) {
	if (*(uint32_t*)src != 0x8088b1f)
		return -1;
	uint32_t data_mv = 23;
	if (*(uint8_t*)(src + 17) != 0x2E && (data_mv = 24, *(uint8_t*)(src + 18) != 0x2E) && (data_mv = 25, *(uint8_t*)(src + 18) != 0x2E))
		return -2;
	uint32_t out_size = *(uint32_t*)(src + size - 4);
	if (ksceDeflateDecompress(dst, out_size, src + data_mv, NULL) < 0)
		return -3;
	if (clean_cache)
		ksceKernelCpuDcacheAndL2WritebackRange(dst, out_size);
	return 0;
}

int init_bootanim(const char *path) {
	SceIoStat stat;
	if (ksceIoGetstat(path, &stat) < 0)
		return -1;
	
	SceKernelAllocMemBlockKernelOpt optp;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x20000000;
	anim_block_id = ksceKernelAllocMemBlock("bootanim", 0x6020D006, (stat.st_size + 0xffff) & 0xffff0000, &optp);
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
		if (header->fullres_frame)
			ungzip_frame(camsram, (void*)(animation + sizeof(animation_header) + 4), *(uint32_t*)(animation + sizeof(animation_header)), 1);
		optp.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_PHYCONT | SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT;
		optp.alignment = 0x100000;
		optp.paddr = 0;
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
	
	if (fb.pitch == 768) // correct res
		fb.width = 720;
	else if (fb.pitch == 512)
		fb.width = 480;
	else
		fb.width = fb.pitch;
	
	if (ksceDisplaySetFrameBuf(&fb, 1) < 0) // no idea why sync but its required(tm)
		goto exit_thread;
	
	ksceDisplayWaitVblankStart(); // maybe i should swap this with set fbuf lol
	
	uint32_t cur_frame = 0;
	uint32_t cur_off = sizeof(animation_header);
	if (header->fullres_frame) // skip frame0
		cur_off -= -(*(uint32_t*)(animation + cur_off) + 4);
	
	while (!emergexit && ksceKernelSysrootGetShellPid() < 0) {
		
		if (cur_frame == header->frame_count) { // loop if loop
			if (header->loop) {
				cur_frame = 0;
				cur_off = sizeof(animation_header);
				if (header->fullres_frame)
					cur_off -= -(*(uint32_t*)(animation + cur_off) + 4);
			} else
				break;
		}
		
		if (header->vblank && !header->swap) // draw @ vblank
			ksceDisplayWaitVblankStart();

		if (header->swap) // dual buffering (useless atm but lol)
			used_fb = !used_fb;
		
		if (ungzip_frame(framebuffer[used_fb], (void*)(animation + cur_off + 4), *(uint32_t*)(animation + cur_off), header->sweep) < 0) // draw
			break;

		if (header->swap) { // dual buffering2
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
	anim_thread_id = ksceKernelCreateThread("bootanim_thr", play_bootanim, header->priority, 0x1000, 0, 0, 0);
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

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	emergexit = 1;
	ksceKernelWaitThreadEnd(anim_thread_id, NULL, NULL);
	return SCE_KERNEL_STOP_SUCCESS;
}