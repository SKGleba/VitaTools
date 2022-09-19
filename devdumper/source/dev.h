#ifndef __DEV_H__
#define __DEV_H__

int dump_sce_dev(int device, const char* dest, void* buf, uint32_t buf_sz_blocks, int type);
int dump_gc_drm_bb(const char* key_output_path);

#endif