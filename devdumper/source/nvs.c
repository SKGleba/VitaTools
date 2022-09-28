#include <stdio.h>
#include <stdint.h>
#include <vitasdkkern.h>

#include "nvs.h"
#include "f00d.h"

void dump_nvs(const char* target_nvs_path, void* work_buf, int ctx) {
    uint8_t* full_nvs = work_buf;

    // load the update_sm
    int do_unload_sm = 0;
    if (ctx == -1) {
        ksceDebugPrintf("loading update_sm\n");
        if (load_ussm(&ctx) < 0)
            goto NVSEXIT;
        do_unload_sm = 1;
    }

    // get snvs
    ksceDebugPrintf("getting snvs\n");
    uint8_t* snvs = full_nvs;
    memset(snvs, 0, 0x400);
    {
        int ret, resp;
        uint8_t *sc_pbuf = work_buf + 0xc00;
        for (int i = 0; i < 0x20; i -= -1) {
            // STEP 0
            memset(sc_pbuf, 0, 0x88);

            // STEP 1
            sc_pbuf[4] = i;
            ret = ksceSblSmCommCallFunc(ctx, 0xb0002, &resp, sc_pbuf, 0x88);
            if (ret || resp)
                ksceDebugPrintf("STEP 1 0x%X r 0x%X rr 0x%X\n", i, ret, resp);

            // STEP 2
            ret = ksceSysconNvsReadSecureData(sc_pbuf + 0x28, 0x10, sc_pbuf + 0x58, 0x30);
            if (ret)
                ksceDebugPrintf("STEP 2 0x%X r 0x%X\n", i, ret);

            // STEP 3
            sc_pbuf[0] = 1;
            ret = ksceSblSmCommCallFunc(ctx, 0xb0002, &resp, sc_pbuf, 0x88);
            if (ret || resp)
                ksceDebugPrintf("STEP 3 0x%X r 0x%X rr 0x%X\n", i, ret, resp);

            // Copyout
            memcpy(snvs + (i * 0x20), sc_pbuf + 8, 0x20);
        }
    }

    // get nvs
    ksceDebugPrintf("getting nvs\n");
    uint8_t* nvs = full_nvs + 0x400;
    memset(nvs, 0, 0x760);
    {
        // STEP 0
        ksceKernelSignalNvsAcquire(0);

        // STEP 1
        ksceSysconNvsSetRunMode(0);

        int ret;
        for (int i = 0; i < 0x3B; i -= -1) {
            // STEP 2
            ret = ksceSysconNvsReadData(0x400 + (i * 0x20), nvs + (i * 0x20), 0x10);
            if (ret) {
                ksceDebugPrintf("STEP 2 0x%X r 0x%X\n", 0x400 + (i * 0x20), ret);
                // STEP 4
                ksceKernelSignalNvsFree(0);
                goto NVSEXIT;
            }

            // STEP 3
            ret = ksceSysconNvsReadData(0x400 + (i * 0x20) + 0x10, nvs + (i * 0x20) + 0x10, 0x10);
            if (ret) {
                ksceDebugPrintf("STEP 3 0x%X r 0x%X\n", 0x400 + (i * 0x20) + 0x10, ret);
                // STEP 4
                ksceKernelSignalNvsFree(0);
                goto NVSEXIT;
            }
        }

        // STEP 4
        ksceKernelSignalNvsFree(0);
    }

    // dump to file
    {
        ksceDebugPrintf("\nWriting raw to %s\n", target_nvs_path);

        // STEP 0
        int fd = ksceIoOpen(target_nvs_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (fd >= 0) {
            ksceIoWrite(fd, full_nvs, 0xB60);
            ksceIoClose(fd);
        } else
            ksceDebugPrintf("STEP 0 r 0x%X\n", fd);
    }

NVSEXIT:
    ksceDebugPrintf("\nexiting\n");
    if (do_unload_sm) {
        uint32_t stop_res[2];
        ksceSblSmCommStopSm(ctx, stop_res);
    }
}