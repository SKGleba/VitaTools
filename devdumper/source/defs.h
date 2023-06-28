#ifndef __DEFS_H__
#define __DEFS_H__

#define USE_TOOL_RAM

#define TEMP_MEMBLOCK_SIZE 0x1000000
#define TEMP_MEMBLOCK_SIZE_BLOCKS (TEMP_MEMBLOCK_SIZE / 0x200)

#define DUMP_ROOT_PATH "host0:"
//#define DUMP_ROOT_PATH "ux0:data/"

//#define DUMP_F00D_STUFF
#define KR_OUTPATH "kr.bin"
#define F00DXBAR_OUTPATH "xbar_f00d.bin"

//#define DUMP_NVS
//#define DUMP_NVS_NOF00D
#define NVS_OUTPATH "nvs.bin"

//#define DUMP_EMMC
#define EMMC_OUTPATH "emmc.img"

//#define DUMP_GC
#define GC_OUTPATH "gc.img"

//#define DUMP_GC_DRMBB
#define GC_DRMBB_KEY_OUTPATH "gc-drmbb-key.bin"

#define DUMP_MC
#define MC_OUTPATH "mc.img"

//#define DEV_SLOW_MODE // read device sector-by-sector, ignore read errors
//#define DEV_SLOW_MODE_RETRY_COUNT 3 // allowed retry count in slow mode

#endif