#include "tai_compat.c"
#include "blit.h"

static int pos = 300;
static void *fb_addr = NULL;

// main loop
static void init_disp(void) {
	SceKernelAllocMemBlockKernelOpt optp;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	SceDisplayFrameBuf fb;
	fb.size        = sizeof(fb);		
	fb.pitch       = 960;
	fb.pixelformat = 0;
	fb.width       = 960;
	fb.height      = 544;
	ksceKernelGetMemBlockBase(ksceKernelAllocMemBlock("SceDisplay", 0x10208006, 0x200000, &optp), (void**)&fb_addr);
	fb.base = fb_addr;
	blit_set_frame_buf(&fb);
	ksceDisplaySetFrameBuf(&fb, 1);
	blit_set_color(0x00ffff00, 0x00000000);
	blit_stringf(0, 280, "logging enabled");
	blit_set_color(0x00ffffff, 0x00000000);
}

int KernelDebugPrintfCallback(int unk, const char *fmt, const va_list args){
	char string[512];
	vsnprintf(string, 511, fmt, args);
	if (*(uint32_t *)(fb_addr + 0x1FDFFC) != 0xCAFEBABE) {
		pos = 300;
		*(uint32_t *)(fb_addr + 0x1FDFFC) = 0xCAFEBABE;
	}
	blit_string(4, pos, string);
	pos-=-20;
	if (pos > 520) {
		pos = 300;
		memset(fb_addr + (300 * 960 * 4), 0, (240 * 960 * 4));
	}
	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	init_disp();
	tai_init();
	DACR_OFF(
		memset((ksceKernelGetSysrootBuffer() + 0x20), 0xFF, 0x10);
		cleainv_dcache((ksceKernelGetSysrootBuffer() + 0x20), 0x20);
	);
	int (* set_handlers)() = NULL, (* set_opmode)() = NULL;
	int sysmemid = ksceKernelSearchModuleByName("SceSysmem");
	module_get_offset(KERNEL_PID, sysmemid, 0, 0x1a13d, &set_handlers);
	module_get_offset(KERNEL_PID, sysmemid, 0, 0x1990d, &set_opmode);
	set_opmode(0);
	set_handlers(KernelDebugPrintfCallback, 0);
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
