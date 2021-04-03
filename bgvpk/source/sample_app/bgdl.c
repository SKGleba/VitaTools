// Based on BGDL PoC by FAPS team

typedef struct ipmi_download_param {
    int type[2];
    char unk_0x08[0x68];
    char url[0x800];	// size is 2048 ?
    char icon_path[0x100];
    char title[0x33A];	// size is 0x33A ?
    char license_path[0x100];
    char unk_0xDAA[0x16];
} ipmi_download_param;

typedef struct sce_ipmi_download_param {

    union {
        struct {
            uint32_t* ptr_to_dc0_ptr;
            uint32_t* ptr_to_2e0_ptr;
            uint32_t unk_1; //2
            uint32_t unk_2; //-1
            uint32_t unk_3; //0
            ipmi_download_param* addr_DC0;
            uint32_t sizeDC0;
        } init;

        struct {
            uint32_t* result;
            uint32_t unk_2;//0
            uint32_t unk_3;//0
            uint32_t unk_4;//1
            uint32_t unk_5;//0
            uint32_t unk_6;
            uint32_t unk_7;//0x00000A0A
        } state;
    };
    void* addr_2E0;
    uint32_t size2E0;
    uint32_t unk_4;
    int* pBgdlId; //points to -1
    uint32_t unk_5;//4
    uint32_t* result;//points to 1
    uint32_t unk_4_2;
    uint32_t shell_func_8;
} sce_ipmi_download_param;

typedef struct ipmi_download_param_state {
    uint32_t* result;
    uint32_t unk_2;//0
    uint32_t unk_3;//0
    uint32_t unk_4;//1
    uint32_t unk_5;//0
    uint32_t unk_6;
    uint32_t unk_7;//0x00000A0A
} ipmi_download_param_state;

typedef struct shellsvc_init_struct {
    uint32_t unk_0;
    char name[0x10];
    void* unk_ptr;
    uint32_t unk_1;// 1
    uint32_t size1;// 0x1E00
    uint32_t size2;// 0x1E00 
    uint32_t unk_2;// 1
    uint32_t unk_3;// 0x0F00
    uint32_t unk_4;// 0x0F00
    uint32_t unk_5;// 1
    char padding[0x84];
    uint32_t unk_7;// 2
    uint32_t unk_8;// -1
    void* unk_ptr_2;
    char padding2[0x88];// 0x14c
} shellsvc_init_struct;

typedef struct scedownload_class_header {
    uint32_t unk0;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t** func_table;
    uint32_t unk3;
    uint32_t* bufC4;
    uint32_t* buf10000;
} scedownload_class_header;

typedef int (*SceDownloadInit)(uint32_t** ipmi_sce_download_ptr, void* ipmi_sce_download_ptr_deref, int unk_1, shellsvc_init_struct* bufc8, int unk_2);
typedef int (*SceDownloadChangeState)(uint32_t** ipmi_sce_download_ptr, int cmd, void* ptr_to_dc0_ptr, int unk_1, sce_ipmi_download_param r5);

typedef struct scedownload_class {
    shellsvc_init_struct init_header;
    scedownload_class_header* class_header;
    SceDownloadInit init;
    SceDownloadChangeState change_state;
} scedownload_class;

// sceIpmiCreateDownloadTask?
int (*SceIpmi_4E255C31)(const char* name, int unk);

// sceIpmiInitDownloadTask?
int (*SceIpmi_B282B430)(uint32_t*** func_table, const char* name, scedownload_class_header* class_header, uint32_t* buf10000);

static scedownload_class some_class;

int init_download_class(scedownload_class* class) {

    memset(class, 0, sizeof(scedownload_class));

    strncpy(class->init_header.name, "SceDownload", 0x10);
    class->init_header.unk_1 = 1;
    class->init_header.size1 = 0x1E00;
    class->init_header.size2 = 0x1E00;
    class->init_header.unk_2 = 1;
    class->init_header.unk_3 = 0x0F00;
    class->init_header.unk_4 = 0x0F00;
    class->init_header.unk_5 = 1;
    class->init_header.unk_7 = 2;
    class->init_header.unk_8 = -1;

    int res = SceIpmi_4E255C31((char*)&(class->init_header.name), 0x1E00);

    if (res != 0xc4)
        return res;

    class->class_header = calloc(1, 0x18);
    if (!class->class_header)
        return -1;

    class->class_header->bufC4 = calloc(1, res);
    if (!class->class_header->bufC4) {
        free(class->class_header);
        return -1;
    }

    class->class_header->buf10000 = calloc(1, 0x1000);
    if (!class->class_header->buf10000) {
        free(class->class_header->bufC4);
        free(class->class_header);
        return -1;
    }

    res = SceIpmi_B282B430(&(class->class_header->func_table), (char*)&(class->init_header.name), class->class_header, class->class_header->buf10000);

    if (class->class_header->func_table < 0) {
        free(class->class_header->bufC4);
        free(class->class_header->buf10000);
        free(class->class_header);
        return (int)class->class_header->func_table;
    }

    class->init = (SceDownloadInit)(*(class->class_header->func_table))[1];
    class->change_state = (SceDownloadChangeState)(*(class->class_header->func_table))[5];

    res = class->init(class->class_header->func_table, *(class->class_header->func_table), 0x14, &(class->init_header), 2);

    return res;
}

int scedownload_start(scedownload_class* class, const char* title, const char* url, int* bgdlid) {
    uint32_t result = 1;
    *bgdlid = 1;

    sce_ipmi_download_param params;
    memset(&params, 0, sizeof(params));

    params.init.ptr_to_dc0_ptr = (uint32_t*)&params.init.addr_DC0;
    params.init.ptr_to_2e0_ptr = (uint32_t*)&params.addr_2E0;
    params.init.unk_1 = 2;
    params.init.unk_2 = -1;
    params.init.unk_3 = 0;

    params.init.sizeDC0 = 0xDC0;
    params.init.addr_DC0 = calloc(1, params.init.sizeDC0);
    if (!params.init.addr_DC0)
        return -1;

    params.size2E0 = 0x2E0;
    params.addr_2E0 = calloc(1, params.size2E0);
    if (!params.addr_2E0)
        return -1;

    params.pBgdlId = (int*)&bgdlid; //points to -1
    params.unk_5 = 4;
    params.result = &result;
    params.shell_func_8 = (*(class->class_header->func_table))[8];

    strcpy((char*)params.init.addr_DC0->url, url);
    strcpy((char*)params.init.addr_DC0->title, title);

    params.init.addr_DC0->type[0] = params.init.addr_DC0->type[1] = 1;

    int res = class->change_state(class->class_header->func_table, 0x12340012, params.init.ptr_to_dc0_ptr, 1, params);

    if (res < 0) {
        free(params.init.addr_DC0);
        free(params.addr_2E0);
        return res;
    }

    result = 0;

    memset(&params, 0, sizeof(params));

    params.state.result = &result;
    params.state.unk_4 = 1;
    params.state.unk_7 = 0x00000A0A;

    res = class->change_state(class->class_header->func_table, 0x12340007, 0, 0, params);
    
    if (result < 0)
        return result;

    return (int)bgdlid;
}

// Inits the BGDL svc, call it once on startup
int bgdl_init() {
    sceKernelLoadStartModule("vs0:sys/external/libshellsvc.suprx", 0, NULL, 0, NULL, NULL);

    if (taiGetModuleExportFunc("SceShellSvc", 0xF4E34EDB, 0x4E255C31, (uintptr_t*)&SceIpmi_4E255C31) < 0
        || taiGetModuleExportFunc("SceShellSvc", 0xF4E34EDB, 0xB282B430, (uintptr_t*)&SceIpmi_B282B430) < 0)
        return -1;
    
    return init_download_class(&some_class);
}

// Queue a BG DL of [url] with title [title], returns bgdlid on success 
int bgdl_queue(const char *title, const char *url) {
    int bgdlid = 0;
    return scedownload_start(&some_class, title, url, &bgdlid);
}