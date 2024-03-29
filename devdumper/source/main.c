#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>

#include "defs.h"

#include "nvs.h"
#include "f00d.h"
#include "dev.h"

#define printf ksceDebugPrintf

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize argc, const void *args)
{
	ksceDebugPrintf("[MAIN] will alloc 16MiB\n");
	void* work_buffer = NULL;
#ifdef USE_TOOL_RAM
	int mbuid = ksceKernelAllocMemBlock("devdumper_buf", 0x10000000 | 0xF00000 | 0xD000 | 0x6, TEMP_MEMBLOCK_SIZE, NULL);
#else
	int mbuid = ksceKernelAllocMemBlock("devdumper_buf", 0x10000000 | 0xC00000 | 0xD000 | 0x6, TEMP_MEMBLOCK_SIZE, NULL);
#endif
	ksceKernelGetMemBlockBase(mbuid, (void**)&work_buffer);
	if (mbuid < 0 || !work_buffer) {
		ksceDebugPrintf("[MAIN] failed to alloc memblock : 0x%08X | 0x%08X\n", mbuid, work_buffer);
		return SCE_KERNEL_START_SUCCESS;
	}

	int ret = -1;

#ifdef DUMP_F00D_STUFF
	ksceDebugPrintf("[MAIN] will dump f00d stuff\n");
	ret = f00d_dump_kr_xbar(DUMP_ROOT_PATH KR_OUTPATH, DUMP_ROOT_PATH F00DXBAR_OUTPATH);
	ksceDebugPrintf("[MAIN] dump f00d stuff ret 0x%08X (%s)\n", ret, (ret < 0) ? "ERROR" : "OK");
#endif

#ifdef DUMP_NVS
	ksceDebugPrintf("[MAIN] will dump nvs\n");
	ret = f00d_dump_nvs(DUMP_ROOT_PATH NVS_OUTPATH, work_buffer);
	ksceDebugPrintf("[MAIN] dump nvs finished\n");
#endif

#ifdef DUMP_NVS_NOF00D
	ksceDebugPrintf("[MAIN] will dump nvs (no f00d)\n");
	dump_nvs(DUMP_ROOT_PATH NVS_OUTPATH, work_buffer, -1);
	ksceDebugPrintf("[MAIN] dump nvs (no f00d) finished\n");
#endif

#ifdef DUMP_EMMC
	ksceDebugPrintf("[MAIN] will dump emmc\n");
	ret = dump_sce_dev(ksceSdifGetSdContextPartValidateMmc(0), DUMP_ROOT_PATH EMMC_OUTPATH, work_buffer, TEMP_MEMBLOCK_SIZE_BLOCKS, 0);
	ksceDebugPrintf("[MAIN] dump emmc ret 0x%08X (%s)\n", ret, (ret < 0) ? "ERROR" : "OK");
#endif

#ifdef DUMP_GC
#ifdef DUMP_GC_DRMBB
	ksceDebugPrintf("[MAIN] will dump gc drm backbone auth key\n");
	ret = dump_gc_drm_bb(DUMP_ROOT_PATH GC_DRMBB_KEY_OUTPATH);
	ksceDebugPrintf("[MAIN] dump gc drmbb ret 0x%08X (%s)\n", ret, (ret < 0) ? "ERROR" : "OK");
#endif
	ksceDebugPrintf("[MAIN] will dump gc\n");
	ret = dump_sce_dev(ksceSdifGetSdContextPartValidateMmc(1), DUMP_ROOT_PATH GC_OUTPATH, work_buffer, TEMP_MEMBLOCK_SIZE_BLOCKS, 0);
	ksceDebugPrintf("[MAIN] dump gc ret 0x%08X (%s)\n", ret, (ret < 0) ? "ERROR" : "OK");
#endif

#ifdef DUMP_MC
	ksceDebugPrintf("[MAIN] will dump mc\n");
	ret = dump_sce_dev(-1, DUMP_ROOT_PATH MC_OUTPATH, work_buffer, TEMP_MEMBLOCK_SIZE_BLOCKS, 2);
	ksceDebugPrintf("[MAIN] dump mc ret 0x%08X (%s)\n", ret, (ret < 0) ? "ERROR" : "OK");
#endif

	ksceKernelFreeMemBlock(mbuid);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
