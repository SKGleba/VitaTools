#define RCF_MAGIC 'VRCF'
#define ANIM_VERSION 1

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t cache;
    uint8_t priority;
    uint8_t sweep;
    uint8_t vblank;
    uint8_t swap;
    uint8_t sram;
    uint8_t fullres_frame;
    uint8_t loop;
    int32_t frame_count;
    uint16_t anim_h;
    uint16_t anim_w;
} __attribute__((packed)) animation_header;