#include <stdio.h>
#include <stdint.h>
#include <vitasdkkern.h>

#include "f00d.h"
#include "nvs.h"
#include "nmprunner.h"
#include "f00d/patch_ussm_nvsread/f00d_patch_nvs_read.h"
#include "f00d/dump_stuff/f00d_dump_kr_xbar.h"

int load_ussm(int* ctx_out) {
  memset(&NMPsmcomm_ctx, 0, sizeof(NMPsmcomm_ctx));
  memcpy(NMPsmcomm_ctx.data0, NMPctx_130_data, 0x90);
  NMPsmcomm_ctx.pathId = 2;
  NMPsmcomm_ctx.self_type = (NMPsmcomm_ctx.self_type & 0xFFFFFFF0) | 2;
  return ksceSblSmCommStartSmFromFile(0, "os0:sm/update_service_sm.self", 0, &NMPsmcomm_ctx, ctx_out);
}

/*
int load_ussm(int* ctx_out) {
  memset(&NMPsmcomm_ctx, 0, sizeof(NMPsmcomm_ctx));
  memcpy(NMPsmcomm_ctx.data0, NMPctx_130_data, 0x90);
  NMPsmcomm_ctx.pathId = 2;
  NMPsmcomm_ctx.self_type = (NMPsmcomm_ctx.self_type & 0xFFFFFFF0) | 2;
  return ksceSblSmCommStartSmFromFile_167(0, "os0:sm/update_service_sm.self", 0, 0, 0, 0, &NMPsmcomm_ctx, ctx_out);
}*/

int f00d_dump_nvs(const char* target_nvs_path, void* work_buf) {
  ksceDebugPrintf("allocating commem\n");
  NMPcorridor = NULL;
  NMPcorridor_paddr = 0x1F850000;
  NMPcorridor_size = 0x00010000;
  int ret = NMPreserve_commem(1, 1);
  ksceDebugPrintf("reserve_commem r %d\n", ret);
  if (ret || !NMPcorridor)
    return -1;

  ksceDebugPrintf("preparing ussm patches\n");

  uint32_t* ussm_patch_offset_offset = NMPcorridor + 0x0000FF00;
  int sysroot = ksceSysrootGetSysroot();
  uint32_t fw = *(uint32_t*)(*(int*)(sysroot + 0x6c) + 4);
  if (fw >= 0x03600000 && fw < 0x03710000)
    *ussm_patch_offset_offset = 0x0000C162;
  else if (fw >= 0x03710000 && fw < 0x03750000)
    *ussm_patch_offset_offset = 0x0000C1C2;
  else {
    ksceDebugPrintf("unsupported fw 0x%08X !\n", fw);
    return -8;
  }

  ksceDebugPrintf("preparing update_sm\n");

  NMPctx = -1;
  ret = NMPexploit_init(fw);
  ksceDebugPrintf("exploit_init r %d\n", ret);
  if (ret)
    return -2;
  ret = NMPconfigure_stage2(fw);
  ksceDebugPrintf("configure_stage2 r %d\n", ret);
  if (ret)
    return -3;
  ret = NMPcopy(NMPstage2_payload, 0, sizeof(NMPstage2_payload), 0);
  ksceDebugPrintf("copy_stage2 r %d\n", ret);
  if (ret)
    return -4;
  ret = NMPcopy(f00d_patch_nvs_read_nmp, 0x100, sizeof(f00d_patch_nvs_read_nmp), 0);
  ksceDebugPrintf("copy_main r %d\n", ret);
  if (ret)
    return -5;
  
  ksceDebugPrintf("will f00d_jump\n");
  ret = NMPf00d_jump((uint32_t)NMPcorridor_paddr, fw);
  ksceDebugPrintf("f00d_jump r %d\n", ret);
  if (ret)
    return -6;

  ret = NMPfree_commem(1);
  ksceDebugPrintf("free_commem r %d\n", ret);
  if (ret)
    return -7;

  ksceDebugPrintf("update_sm ready, dumping nvs\n");
  dump_nvs(target_nvs_path, work_buf, NMPctx);

  ksceDebugPrintf("unloading update_sm\n");
  uint32_t stop_res[2];
  stop_res[0] = 0;
  stop_res[1] = 0;
  ksceSblSmCommStopSm(NMPctx, &stop_res);

  return 0;
}

int f00d_dump_kr_xbar(const char* kr_dest, const char* xbar_dest) {
  ksceDebugPrintf("running the f00d dumper payload\n");
  NMPcorridor = NULL;
  NMPcorridor_paddr = 0x1C000000;
  NMPcorridor_size = 0x00100000;
  int ret = NMPrun_default(f00d_dump_kr_xbar_nmp, sizeof(f00d_dump_kr_xbar_nmp));
  ksceDebugPrintf("nmp_run_default r %d\n", ret);
  if (ret)
    return -1;

  ksceDebugPrintf("allocating commem\n");
  NMPcorridor = NULL;
  NMPcorridor_paddr = 0x1C100000;
  NMPcorridor_size = 0x00100000;
  ret = NMPreserve_commem(0, 1);
  ksceDebugPrintf("reserve_commem r %d\n", ret);
  if (ret || !NMPcorridor)
    return -2;

  ksceDebugPrintf("writing keyring dump to %s\n", kr_dest);
  ret = NMPfile_op(kr_dest, 0, 0x10000, 0);
  ksceDebugPrintf("file_op r %d\n", ret);
  if (ret)
    return -3;

  ksceDebugPrintf("writing f00d xbar dump to %s\n", xbar_dest);
  ret = NMPfile_op(xbar_dest, 0x10000, 0x10000, 0);
  ksceDebugPrintf("file_op r %d\n", ret);
  if (ret)
    return -4;

  ksceDebugPrintf("all done, freeing commem\n");
  ret = NMPfree_commem(1);
  ksceDebugPrintf("free_commem r %d\n", ret);
  if (ret)
    return -6;

  return 0;
}