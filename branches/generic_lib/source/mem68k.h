#ifndef _MEM68K_H_
#define _MEM68K_H_

enum {
    SRAM,
    EEPROM,
    J_CART,
	SVP_DRAM,
	SVP_CELL,
	CART_HW,
	REALTEC_ROM,
	VDP,
	SYSTEM_IO,
	UNUSED,
	ILLEGAL,
	WRAM,
	ROM,
};

extern uint8 m68k_readmap_8[32];
extern uint8 m68k_readmap_16[32];
extern uint8 m68k_writemap_8[32];
extern uint8 m68k_writemap_16[32];


#endif /* _MEM68K_H_ */
