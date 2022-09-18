#ifndef __F00D_H__
#define __F00D_H__

int load_ussm(int* ctx_out);
int f00d_dump_nvs(const char* target_nvs_path, void* work_buf);
int f00d_dump_kr_xbar(const char* kr_dest, const char* xbar_dest);

#endif