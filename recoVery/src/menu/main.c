#include "tai_compat.c"
#include "blit.h"

#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define DBG ksceDebugPrintf

#define SWAP_MPATH(newpath) \
do {\
	ksceIoSync(mpath, 0); \
	vstor_stop(); \
	mpath = newpath; \
	vstor_setimgp(mpath); \
	vstor_start(0); \
} while (0)
	
typedef struct {
	uint32_t off;
	uint32_t sz;
	uint8_t code;
	uint8_t type;
	uint8_t active;
	uint32_t flags;
	uint16_t unk;
} __attribute__((packed)) partition_t;

typedef struct {
	char magic[0x20];
	uint32_t version;
	uint32_t device_size;
	char unk1[0x28];
	partition_t partitions[0x10];
	char unk2[0x5e];
	char unk3[0x10 * 4];
	uint16_t sig;
} __attribute__((packed)) master_block_t;

static char *mpath = NULL; // currently mounted path
static int (*vstor_gpx)(void) = NULL;
static uint8_t npo[2] = {0x35, 0xe0};
static int (*vstor_stop)(void) = NULL;
static int (*vstor_start)(int mode) = NULL;
static char custom_mpath[64], custom_cmd[4]; // user configs
int select = 1, menusize = 0, g_rwl = SCE_O_RDWR, dispOff = 0;
static int (*vstor_setimgp)(const char *img) = NULL;
static uint8_t vstor_ret_2[] = {0x02, 0x20, 0xf3, 0xe7};
static void *fsp_buf = NULL, *vstor_params_table = NULL;
static int (*vstor_setdescr)(const char *name0, const char *name1) = NULL;
const char menu[9][20] = {"Mount GC-SD", "Mount user", "Mount os", "Mount vsh", "Mount custom", "Other command", "Load supply.skprx", "Exit recoVery", ""};

// fixed storage params gen with exfat & small partitions support
int custom_vstor_gpx(void) {
	DBG("vstor_get_sparams\n");
	int ret = vstor_gpx();
	if (ret != 2)
		return ret;
	DBG("c_gpx open %s\n", mpath);
	int permz = ksceKernelSetPermission(0x40);
	int fd = ksceIoOpen(mpath, g_rwl, 0777);
	if (fd < 0)
		goto ded;
	*(uint32_t *)(vstor_params_table + 0xc) = fd;
	DBG("c_gpx verify mbr\n");
	char mbr[0x200];
	memset(mbr, 0, 0x200);
	ksceIoRead(fd, mbr, 0x200);
	if (*(uint16_t *)(mbr + 0x1fe) != 0xaa55)
		goto ded;
	// set sector and device size
	*(uint32_t *)(vstor_params_table + 0x50) = (uint16_t)0x200;
	if (memcmp(mbr + 0x3, "EXFAT", 5) == 0) { // ExFAT device
		DBG("c_gpx set mode exfat\n");
		*(uint32_t*)(vstor_params_table + 0x10) = *(uint32_t*)(mbr + 0x48);
	} else if (memcmp(mbr, "Sony Computer Entertainment Inc.", 0x20) == 0) {
		DBG("c_gpx set mode sce\n");
		*(uint32_t*)(vstor_params_table + 0x10) = *(uint32_t*)(mbr + 0x24);
    } else {
		DBG("c_gpx set mode fat16\n");
		if (*(uint32_t *)(mbr + 0x20) > 0) // Use big sectors
			*(uint32_t *)(vstor_params_table + 0x10) = *(uint32_t *)(mbr + 0x20);
		else // Use small sectors
			*(uint32_t *)(vstor_params_table + 0x10) = (uint32_t)*(uint16_t *)(mbr + 0x13);
	}
	DBG("c_gpx done set evf\n");
	if (ksceIoLseek(fd, 0, SCE_SEEK_END) < 0)
		goto ded;
	ksceKernelSetPermission(permz);
	ksceKernelSetEventFlag(*(uint32_t *)(vstor_params_table + 0x38), 0x200);
	return 0;
ded:
	DBG("c_gpx died\n");
	if (fd > 0)
		ksceIoClose(fd);
	*(uint32_t *)(vstor_params_table + 0xc) = -1;
	ksceKernelSetPermission(permz);
	return -0x7fdbbeee;
}

// prepare the vstor module & get funcs
static void patch_vstor(void) {
	// get module's load n0
	int vstor_ln = ksceKernelSearchModuleByName("SceUsbstorVStorDriver");
	// skip buggy image checks
	INJECT_NOGET(vstor_ln, 0x1738, npo, 2);
	// redirect getParam for exfat & small support
	INJECT_NOGET(vstor_ln, 0x20c, vstor_ret_2, 4);
	uintptr_t gpx_ptr;					
	module_get_offset(KERNEL_PID, vstor_ln, 1, 0x164, &gpx_ptr); 
	DACR_OFF(*(uint32_t *)gpx_ptr = custom_vstor_gpx;);
	cleainv_dcache(gpx_ptr, 4);
	// get vstor offs
	module_get_offset(KERNEL_PID, vstor_ln, 0, 0x1711, &vstor_start);
	module_get_offset(KERNEL_PID, vstor_ln, 0, 0x16b9, &vstor_setdescr);
	module_get_offset(KERNEL_PID, vstor_ln, 0, 0x16d9, &vstor_setimgp);
	module_get_offset(KERNEL_PID, vstor_ln, 0, 0x1859, &vstor_stop);
	module_get_offset(KERNEL_PID, vstor_ln, 0, 0x11d, &vstor_gpx);
	module_get_offset(KERNEL_PID, vstor_ln, 1, 0, &vstor_params_table);
}

// get custom block mountpoint
static int read_custom() {
	memset(custom_mpath, 0, 64);
	int fd = ksceIoOpen("sd0:cmp.recovery", SCE_O_RDONLY, 0);
	if (fd > 0) {
		ksceIoRead(fd, custom_mpath, 64);
		ksceIoClose(fd);
		return 1;
	}
	return 0;
}

// read/write emmc
static void work_emmc(int mode, uint8_t active, uint8_t part_id) {
	DBG("work_emmc %d(%d) [%s]\n", part_id, active, (mode) ? "WRITE" : "READ");
	if (part_id < 3 || active > 1 || fsp_buf == NULL) // make sure that its not touching boot stuff & no null derefs
		return;
	char mbr[0x200];
	int mmc = ksceSdifGetSdContextPartValidateMmc(0);
	if (ksceSdifReadSectorMmc(mmc, 0, &mbr, 1) < 0)
		return;
	uint32_t copied = 0, size = 0, off = 0;
	master_block_t *master = (master_block_t *)mbr;
	for (size_t i = 0; i < ARRAYSIZE(master->partitions); ++i) {
		partition_t *p = &master->partitions[i];
		if (p->active == active && p->code == part_id) {
			size = p->sz;
			off = p->off;
		}
	}
	if (off < 0x8000 || size == 0)
		return;
	int fd = (mode) ? ksceIoOpen("sd0:toflash.img", SCE_O_RDONLY, 0) : ksceIoOpen("sd0:dumped.img", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
	if (fd < 0)
		return;
	DBG("work_emmc op start\n");
	if (mode) {
		while((copied + 0x8000) <= size) { // first copy with full buffer
			ksceIoRead(fd, fsp_buf, 0x8000 * 0x200);
			ksceSdifWriteSectorMmc(mmc, off + copied, fsp_buf, 0x8000);
			copied = copied + 0x8000;
		}
		if (copied < size && (size - copied) <= 0x8000) { // then copy the remaining data
			ksceIoRead(fd, fsp_buf, (size - copied) * 0x200);
			ksceSdifWriteSectorMmc(mmc, off + copied, fsp_buf, (size - copied));
			copied = size;
		}
	} else {
		while((copied + 0x8000) <= size) { // first copy with full buffer
			ksceSdifReadSectorMmc(mmc, off + copied, fsp_buf, 0x8000);
			ksceIoWrite(fd, fsp_buf, 0x8000 * 0x200);
			copied = copied + 0x8000;
		}
		if (copied < size && (size - copied) <= 0x8000) { // then copy the remaining data
			ksceSdifReadSectorMmc(mmc, off + copied, fsp_buf, (size - copied));
			ksceIoWrite(fd, fsp_buf, (size - copied) * 0x200);
			copied = size;
		}
	}
	DBG("work_emmc op stop\n");
	ksceIoClose(fd);
}

// dump the emmc MBR
static void dump_mbr(void) {
	char mbr[0x200];
	ksceSdifReadSectorMmc(ksceSdifGetSdContextPartValidateMmc(0), 0, &mbr, 1);
	int fd = ksceIoOpen("sd0:mbr.recovery", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
	ksceIoWrite(fd, mbr, 0x200);
	ksceIoClose(fd);
}

// custom user commands handler
static void handle_ccmd() {
	memset(custom_cmd, 0, 4);
	int fd = ksceIoOpen("sd0:cmd.recovery", SCE_O_RDONLY, 0);
	if (fd < 0)
		return;
	ksceIoRead(fd, custom_cmd, 4);
	ksceIoClose(fd);
	DBG("ccmd_handler: got %s\n", custom_cmd);
	switch(custom_cmd[0]) {
		case 'F':
			if ((custom_cmd[2] > 0x2f) && (custom_cmd[3] > 0x2f))
				work_emmc(1, custom_cmd[1] - 0x30, (uint8_t)(((custom_cmd[2] - 0x30) * 10) + (custom_cmd[3] - 0x30)));
			break;
		case 'D':
			if ((custom_cmd[2] > 0x2f) && (custom_cmd[3] > 0x2f))
				work_emmc(0, custom_cmd[1] - 0x30, (uint8_t)(((custom_cmd[2] - 0x30) * 10) + (custom_cmd[3] - 0x30)));
			break;
		case 'M':
			if (custom_cmd[1] == 'U')
				ksceIoUmount((((custom_cmd[2] - 0x30) * 10) + (custom_cmd[3] - 0x30)) * 0x100, 1, 0, 0);
			else
				ksceIoMount((((custom_cmd[2] - 0x30) * 10) + (custom_cmd[3] - 0x30)) * 0x100, NULL, 0, 0, 0, 0);
			break;
		default:
			dump_mbr();
			break;
	}
	return;
}

// hard reset
static void reset(int bic) {
	DBG("requested death...\n");
	int *(* sc_call)() = NULL;
	int syscon_ln = ksceKernelSearchModuleByName("SceSyscon");
	module_get_offset(KERNEL_PID, syscon_ln, 0, (bic) ? 0x30C9 : 1, (uintptr_t *)&sc_call);
	if (bic) {
		sc_call(0, 0x888, 2);
		sc_call(0, 0x989, 1);
	} else
		sc_call(2, 0, 0);
}

// commands handler
int do_work(int cmd, int rw) {
	DBG("cmd_handler got req 0x%X(%d)\n", cmd, rw);
	g_rwl = (rw) ? SCE_O_RDWR : SCE_O_RDONLY;
	ksceSdifReadSectorMmc((void *)0xCAFEBABE, -1, NULL, 0);
	ksceSdifWriteSectorMmc((void *)0xCAFEBABE, -1, NULL, 0);
	int exit = 0;
	switch(cmd) {
		case 1:
			SWAP_MPATH("sdstor0:ext-lp-ign-entire");
			break;
		case 2:
			SWAP_MPATH("sdstor0:int-lp-ign-user");
			break;
		case 3:
			SWAP_MPATH("sdstor0:int-lp-act-os");
			break;
		case 4:
			SWAP_MPATH("sdstor0:int-lp-ign-vsh");
			break;
		case 5:
			ksceIoSync(mpath, 0);
			if (read_custom())
				SWAP_MPATH(custom_mpath);
			break;
		case 6:
			ksceIoSync(mpath, 0);
			vstor_stop();
			handle_ccmd();
			vstor_start(0);
			break;
		case 7:
			ksceIoSync(mpath, 0);
			ksceKernelLoadStartModule("sd0:supply.skprx", 0, NULL, 0, NULL, NULL);
			break;
		case 8:
			ksceIoSync(mpath, 0);
			vstor_stop();
			if (rw)
				reset(0);
			else
				reset(1);
			exit = 1;
			break;
		case 9:
			ksceSdifReadSectorMmc((void*)0xCAFEBABE, -1, NULL, 1);
			if (rw == 2) {
				ksceSdifWriteSectorMmc((void*)0xCAFEBABE, -1, NULL, 1);
				DBG("WARNING: FULL RW PERMS\n");
			}
			SWAP_MPATH("sdstor0:int-lp-act-entire");
			break;
		default:
			exit = 1;
			break;
	}
	return exit;	
}

// update ptr pos
void drawScreen() {
	blit_set_color(0x00000000, 0x00000000);
	for	(int i = 1; i <= menusize; i++)	
		blit_stringf((strlen(menu[i - 1]) + 2) * 16, i * 20 + 80, (i == 9) ? ">Mount EMMC" : "<");
	blit_stringf(36 * 16, 50, " ########### ");
	blit_stringf(36 * 16, 70, "  #       #  ");
	blit_stringf(36 * 16, 90, "   #     #   ");
	blit_stringf(36 * 16, 110, "    #   #    ");
	blit_stringf(36 * 16, 130, "     # #     ");
	blit_stringf(36 * 16, 150, "      #    WARNING:");
	blit_stringf(36 * 16, 170, "             ");
	blit_stringf(36 * 16, 190, "      #      ");
	blit_stringf(36 * 16, 210, "     # #     ");
	blit_stringf(36 * 16, 230, "      #      ");
	blit_stringf((36 * 16) + (12 * 16), 170, " WRITABLE");
	blit_set_color(0x0000ffff, 0x00000000);
	blit_stringf((strlen(menu[select - 1]) + 2) * 16, select * 20 + 80, (select == 9) ? ">Mount EMMC" : "<");
	if (g_rwl == SCE_O_RDWR)
		blit_stringf((36 * 16) + (12 * 16), 170, " WRITABLE");
	blit_set_color(0x00ff00ff, 0x00000000);
	blit_stringf(20, 280, "mp: %s             ", mpath);
	if (g_rwl == SCE_O_RDWR) {
		blit_set_color(0x000000ff, 0x00000000);
		blit_stringf(36 * 16, 50, " ########### ");
		blit_stringf(36 * 16, 70, "  #       #  ");
		blit_stringf(36 * 16, 90, "   #     #   ");
		blit_stringf(36 * 16, 110, "    #   #    ");
		blit_stringf(36 * 16, 130, "     # #     ");
		blit_stringf(36 * 16, 150, "      #    WARNING:");
		blit_stringf(36 * 16, 170, "             ");
		blit_stringf(36 * 16, 190, "      #      ");
		blit_stringf(36 * 16, 210, "     # #     ");
		blit_stringf(36 * 16, 230, "      #      ");
	}
	blit_set_color(0x00ffffff, 0x00000000);
}

// main loop
static void lookdog(void) {
	void *fb_addr = NULL;
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
	memset(fb_addr, 0, 0x1FE000);
	fb.base = fb_addr;
	blit_set_frame_buf(&fb);
	ksceDisplaySetFrameBuf(&fb, 1);
	blit_set_color(0x00ffff00, 0x00000000);
	blit_stringf(20, 20, "recoVery v1.02 by SKGleba");
	blit_stringf(20, 40, "|||||||||||||||||||||||||");
	blit_stringf(20, 60, "VVVVVVVVVVVVVVVVVVVVVVVVV");
	blit_set_color(0x00ffffff, 0x00000000);
	menusize = sizeof(menu) / sizeof(menu[0]);
	for (int i = 0; i < menusize; i++)
		blit_stringf(20, ((i + 1) * 20) + 80, menu[i]);
	drawScreen();
	SceCtrlData ctrl_peek, ctrl_press;
	while(1) {
		ctrl_press = ctrl_peek;
		ksceCtrlPeekBufferPositive(0, &ctrl_peek, 1);
		ctrl_press.buttons = ctrl_peek.buttons & ~ctrl_press.buttons;
		if (ctrl_press.buttons == SCE_CTRL_UP) {
			select = select - 1;
			if (select < 1)
				select = menusize - 1;
			drawScreen();
		} else if (ctrl_press.buttons == SCE_CTRL_DOWN) {
			select = select + 1;
			if (select > menusize)
				select = 1;
			drawScreen();
		} else if (ctrl_press.buttons == SCE_CTRL_CROSS) {
			if (do_work(select, 1))
				break;
			drawScreen();
		} else if (ctrl_press.buttons == SCE_CTRL_CIRCLE) {
			if (do_work(select, 0))
				break;
			drawScreen();
		} else if (ctrl_press.buttons == SCE_CTRL_TRIANGLE) {
			if (do_work(select, 2))
				break;
			drawScreen();
		} else if (ctrl_press.buttons == SCE_CTRL_PSBUTTON) {
			kscePowerRequestDisplayOff();
			drawScreen(); // bypass system hang
		}
		ksceKernelDelayThread(100 * 1000);
	}
	drawScreen();
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	DBG("starting recovery... 0x%X\n", tai_init()); // initialize the patch funcs
	ksceKernelGetMemBlockBase( // Prepare a buffer for FS ops
		ksceKernelAllocMemBlock("workfs_buf", 0x1020D006, 0x1000000, NULL), 
		(void**)&fsp_buf
	);
	ksceIoMount(0x100, NULL, 0, 0, 0, 0); // Fix for GC-SD not mounting at boot
	patch_vstor(); // prepare the vstor module
	ksceUdcdStopCurrentInternal(2); // make sure USB2 (main) is not used
	vstor_stop(); // this should error out after the prev func
	vstor_setdescr("\"PS Vita\" rV", "1.02"); // set our fancy device descriptor
	mpath = "sdstor0:ext-lp-ign-entire"; // first usbmount the GC-SD
	vstor_setimgp(mpath);
	vstor_start(0); // start share
	lookdog(); // main loop
	ksceIoSync(mpath, 0); // we are not threading but make sure that there are no remaining ops
	vstor_stop(); // make sure all handlers are done
	while(1) { // signal the user that we are dead
		DBG("dead\n");
		ksceGpioPortReset(0, 7);
		ksceKernelDelayThread(2000 * 1000);
		ksceGpioPortSet(0, 7);
		ksceKernelDelayThread(4000 * 1000);
	}
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
