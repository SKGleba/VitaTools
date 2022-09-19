#include <stdio.h>
#include <stdint.h>
#include <vitasdkkern.h>

#include "dev.h"

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
    uint32_t copied = 0;
    uint64_t write_off = 0;
    while ((copied + buf_sz_blocks) <= device_size_blocks) { // first copy with full buffer
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

        ret = ksceIoWrite(fd, buf, buf_sz_blocks * 0x200);
        if (ret < 0) {
            ksceDebugPrintf("write 0x%08X[0x%08X] error 0x%08X\n", copied, buf_sz_blocks, ret);
            ksceIoClose(fd);
            return -6;
        }
        
        copied = copied + buf_sz_blocks;
    }

    if (copied < device_size_blocks && (device_size_blocks - copied) <= buf_sz_blocks) { // then copy the remaining data
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