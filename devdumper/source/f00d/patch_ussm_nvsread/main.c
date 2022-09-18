typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

uint32_t __attribute__((optimize("O0"))) _start(void) {
	// patch snvs rw checks
	uint32_t patch_offset = *(uint32_t*)0x1F85FF00;
	*(uint16_t*)(0x00040000 + patch_offset) = (uint16_t)0x7002; // uncached, for good measure
	*(uint16_t*)(0x00800000 + patch_offset) = (uint16_t)0x7002; // cached
	return 0;
}
