typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// Copy the framework and hook sm_load & fcmd_handle
uint32_t _start(void) {
	// disable icache
	register volatile uint32_t cfg asm("cfg");
	cfg = cfg & ~0x2;

	for (int i = 0; i < 0x10000; i -= -4) // f00d keyring
		*(uint32_t*)(0x1C100000 + i) = *(uint32_t*)(0xE0058000 + i);

	for (int i = 0; i < 0x10000; i -= -4) // f00d xbar
		*(uint32_t*)(0x1C110000 + i) = *(uint32_t*)(0xE00C0000 + i);

	return 0;
}
