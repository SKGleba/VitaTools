#include <stdio.h>
#include <stdint.h>
#include <vitasdkkern.h>

#include "defs.h"

#include "dev.h"

#ifdef DEV_SLOW_MODE
static void slow_read_dev(int device, uint32_t off, void* dst, uint32_t count, int type) {
    int ret = 0, retry_count = 0;
    for (uint32_t i = 0; i < count; i++) {
        retry_count = DEV_SLOW_MODE_RETRY_COUNT;
SLOW_RETRY:
        if (!type)
            ret = ksceSdifReadSectorMmc(device, off + i, (dst + (0x200 * i)), 1);
        else if (type == 1)
            ret = ksceSdifReadSectorSd(device, off + i, (dst + (0x200 * i)), 1);
        else if (type == 2)
            ret = ksceMsifReadSector(off + i, (dst + (0x200 * i)), 1);
        if (ret < 0) {
            ksceDebugPrintf("read sector 0x%08X error 0x%08X\n", off + i, ret);
            if (retry_count) {
                ksceDebugPrintf("retrying 0x%08X\n", off + i);
                retry_count--;
                goto SLOW_RETRY;
            }
            ksceDebugPrintf("FAILED to read sector 0x%08X, skipping\n", off + i);
        }
    }
}
#endif

int dump_sce_dev(int device, const char* dest, void* buf, uint32_t buf_sz_blocks, int type) {
    if (!device || !dest || !buf || !buf_sz_blocks)
        return -1;

    uint8_t *mbr = buf;
    memset(mbr, 0, 0x200);
    int ret = -1;
    if (!type)
        ret = ksceSdifReadSectorMmc(device, 0, mbr, 1);
    else if (type == 1)
        ret = ksceSdifReadSectorSd(device, 0, mbr, 1);
    else if (type == 2)
        ret = ksceMsifReadSector(0, mbr, 1);
    if (ret < 0)
        return -2;

    if (*(uint32_t*)(mbr + 0x20) != 3 || !(*(uint32_t*)(mbr + 0x24)))
        return -3;

    uint32_t device_size_blocks = *(uint32_t*)(mbr + 0x24);
    ksceDebugPrintf("device checks done, size 0x%08X blocks\n", device_size_blocks);

    memset(buf, 0, buf_sz_blocks * 0x200);

    int fd = ksceIoOpen(dest, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
    if (fd < 0)
        return -4;

    ksceDebugPrintf("dumping to %s, this will take a longer while\n", dest);
#ifdef DEV_SLOW_MODE
    ksceDebugPrintf("SLOW mode selected, allowed retry count per sector: %d\n", DEV_SLOW_MODE_RETRY_COUNT);
#endif
    uint32_t copied = 0;
    uint64_t write_off = 0;
    while ((copied + buf_sz_blocks) <= device_size_blocks) { // first copy with full buffer
#ifdef DEV_SLOW_MODE
        if (!type)
            slow_read_dev(device, copied, buf, buf_sz_blocks, type);
        else if (type == 1)
            slow_read_dev(device, copied, buf, buf_sz_blocks, type);
        else if (type == 2)
            slow_read_dev(0, copied, buf, buf_sz_blocks, type);
#else
        if (!type)
            ret = ksceSdifReadSectorMmc(device, copied, buf, buf_sz_blocks);
        else if (type == 1)
            ret = ksceSdifReadSectorSd(device, copied, buf, buf_sz_blocks);
        else if (type == 2)
            ret = ksceMsifReadSector(copied, buf, buf_sz_blocks);
        if (ret < 0) {
            ksceDebugPrintf("read 0x%08X[0x%08X] error 0x%08X\n", copied, buf_sz_blocks, ret);
            ksceIoClose(fd);
            return -5;
        }
#endif

        ret = ksceIoWrite(fd, buf, buf_sz_blocks * 0x200);
        if (ret < 0) {
            ksceDebugPrintf("write 0x%08X[0x%08X] error 0x%08X\n", copied, buf_sz_blocks, ret);
            ksceIoClose(fd);
            return -6;
        }
        
        copied = copied + buf_sz_blocks;
    }

    if (copied < device_size_blocks && (device_size_blocks - copied) <= buf_sz_blocks) { // then copy the remaining data
#ifdef DEV_SLOW_MODE
        if (!type)
            slow_read_dev(device, copied, buf, (device_size_blocks - copied), type);
        else if (type == 1)
            slow_read_dev(device, copied, buf, (device_size_blocks - copied), type);
        else if (type == 2)
            slow_read_dev(0, copied, buf, (device_size_blocks - copied), type);
#else
        if (!type)
            ret = ksceSdifReadSectorMmc(device, copied, buf, (device_size_blocks - copied));
        else if (type == 1)
            ret = ksceSdifReadSectorSd(device, copied, buf, (device_size_blocks - copied));
        else if (type == 2)
            ret = ksceMsifReadSector(copied, buf, (device_size_blocks - copied));
        if (ret < 0) {
            ksceDebugPrintf("read 0x%08X[0x%08X] error 0x%08X\n", copied, (device_size_blocks - copied), ret);
            ksceIoClose(fd);
            return -7;
        }
#endif
        ret = ksceIoWrite(fd, buf, (device_size_blocks - copied) * 0x200);
        if (ret < 0) {
            ksceDebugPrintf("write 0x%08X[0x%08X] error 0x%08X\n", copied, (device_size_blocks - copied), ret);
            ksceIoClose(fd);
            return -8;
        }
        copied = device_size_blocks;
    }

    ksceIoClose(fd);

    ksceDebugPrintf("dumped to %s\n", dest);

    return 0;
}

int dump_gc_drm_bb(const char* key_output_path) {
    uint8_t buf[0x20];
    memset(buf, 0, 0x20);
    ksceGcAuthGetDrmBBKeypair(buf); // 0xBB70DDC0
    ksceDebugPrintf("writing GcAuth DRM BB keypair to %s\n", key_output_path);
    int fd = ksceIoOpen(key_output_path, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
    if (fd < 0)
        return -1;
    ksceIoWrite(fd, buf, 0x20);
    ksceIoClose(fd);
    return 0;
}