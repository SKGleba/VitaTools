#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include "../kernel/vita-bootanim.h"

uint32_t getSz(const char* src) {
    FILE* fp = fopen(src, "rb");
    if (fp == NULL)
        return 0;
    fseek(fp, 0L, SEEK_END);
    uint32_t sz = ftell(fp);
    fclose(fp);
    return sz;
}

int main(int argc, char** argv) {
    char* end_logo = NULL;
    animation_header header;
    memset(&header, 0, sizeof(animation_header));
    header.anim_w = 960;
    header.anim_h = 544;
    int compression = 6;
    
    if (argc > 1) {
        for(int i = 1; i < argc; i-=-1){
            if (!strcmp(argv[i], "-height"))
                header.anim_h = atoi(argv[i + 1]);
            else if (!strcmp(argv[i], "-width"))
                header.anim_w = atoi(argv[i + 1]);
            else if (!strcmp(argv[i], "-logo"))
                end_logo = argv[i + 1];
            else if (!strcmp(argv[i], "-priority"))
                header.priority = atoi(argv[i + 1]);
            else if (!strcmp(argv[i], "-compression"))
                compression = atoi(argv[i + 1]);
            else if (!strcmp(argv[i], "-cache_fb"))
                header.cache = 1;
            else if (!strcmp(argv[i], "-wipe_cache"))
                header.sweep = 1;
            else if (!strcmp(argv[i], "-wait_vblank"))
                header.vblank = 1;
            else if (!strcmp(argv[i], "-swap_fb"))
                header.swap = 1;
            else if (!strcmp(argv[i], "-sram"))
                header.sram = 1;
            else if (!strcmp(argv[i], "-loop"))
                header.loop = 1;
            else if (!strcmp(argv[i], "-help")) {
                printf("usage: %s [animation options] ...\n", argv[0]);
                printf(" available options : \n");
                printf(" -height X : sets animation height to X\n"); 
                printf(" -width X : sets animation width to X\n"); 
                printf(" -logo X : replaces default logo with X\n");
                printf(" -priority X : sets thread priority to X (higher X=lower priority=less bootloop risk)\n");
                printf(" -compression X : sets compression level to X (lower=faster)\n");
                printf(" -cache_fb : enable caching (faster, more artifacts)\n");
                printf(" -wipe_cache : clean cache after every frame (slower, less artifacts)\n");
                printf(" -wait_vblank : waits for VBLANK before updating the FB\n"); 
                printf(" -swap_fb : swap between two framebuffers (?slower?, less artifacts)\n");
                printf(" -sram : stay in camera SRAM (not compatible with swap_fb and logo)\n");
                printf(" -loop : loop animation until boot is finished\n");
                printf("example: ./mkanim -width 512 -height 272 -logo big.png -cache_fb -swap_fb -wipe_cache -priority 1\n");
                printf("supported res: 960x544, 768x408 (only 720x408 mapped!), 640x368 and 512x272 (only 480x272 mapped!)\n");
                return 0;
            }
        }
    }
    
    unlink("boot.rcf");
    
    printf("converting boot_animation.gif to a supported gif type...\n");
    system("convert boot_animation.gif -coalesce boot.gif");
    printf("extracting gif frames...\n");
    system("convert boot.gif frame.png");
    unlink("boot.gif");
    
    printf("converting frames to RGBA...\n");
    char file_name[32], command[128];
    int cur_frame = 0;
    FILE* fp = NULL;
    while(1) {
        sprintf(file_name, "frame-%d.png", cur_frame);
        fp = fopen(file_name, "rb");
        if (fp == NULL) 
            break;
        fclose(fp);
        sprintf(command, "convert %s -resize %dx%d! frame_%d.rgba", file_name, header.anim_w, header.anim_h, cur_frame);
        system(command);
        unlink(file_name);
        cur_frame -= -1;
    }
    
    if (end_logo) {
        sprintf(command, "convert %s -resize 960x544! end_logo.rgba", end_logo);
        system(command);
    }
    
    header.frame_count = cur_frame;
    
    printf("compressing frames...\n");
    for (int i = 0; i < header.frame_count; i-=-1) {
        sprintf(file_name, "frame_%d.rgba", i);
        sprintf(command, "gzip -%d -k %s", compression, file_name);
        system(command);
        unlink(file_name);
    }
    
    if (end_logo) {
        sprintf(command, "gzip -%d -k end_logo.rgba", compression);
        system(command);
        unlink("end_logo.rgba");
    }
    
    printf("creating the boot animation [HEADER]...\n");
    header.magic = RCF_MAGIC;
    header.version = ANIM_VERSION;
    header.fullres_frame = !(!end_logo);
    fp = fopen("boot.rcf", "wb");
    if (fp == NULL)
        return -1;
    fwrite(&header, sizeof(animation_header), 1, fp);
    fclose(fp);
    
    printf("creating the boot animation [BODY]...\n");
    uint32_t frame_size = 0;
    if (end_logo) {
        frame_size = getSz("end_logo.rgba.gz");
        fp = fopen("boot.rcf", "ab");
        fwrite(&frame_size, 4, 1, fp);
        fclose(fp);
        system("cat end_logo.rgba.gz >> boot.rcf");
        unlink("end_logo.rgba.gz");
    }
    for (int i = 0; i < header.frame_count; i -= -1) {
        sprintf(file_name, "frame_%d.rgba.gz", i);
        frame_size = getSz(file_name);
        fp = fopen("boot.rcf", "ab");
        fwrite(&frame_size, 4, 1, fp);
        fclose(fp);
        sprintf(command, "cat %s >> boot.rcf", file_name);
        system(command);
        unlink(file_name);
    }
    
    printf("all done!\n");
    return 0;
}