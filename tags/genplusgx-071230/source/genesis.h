
#ifndef _GENESIS_H_
#define _GENESIS_H_

/* Global variables */
extern uint8 *cart_rom;
extern uint8 bios_rom[0x800];
extern uint8 work_ram[0x10000];
extern uint8 zram[0x2000];
extern uint8 zbusreq;
extern uint8 zbusack;
extern uint8 zreset;
extern uint8 zirq;
extern uint32 zbank;
extern uint8 gen_running;
extern uint32 bustakencnt;
extern uint8 lastbusack;
extern uint32 genromsize;
extern uint8 bios_enabled;
extern uint32 rom_size;
extern uint8 *rom_readmap[8];


/* Function prototypes */
void gen_init(void);
void gen_reset(uint8 hard_reset);
void gen_shutdown(void);
int gen_busack_r(void);
void gen_busreq_w(int state);
void gen_reset_w(int state);
void gen_bank_w(int state);
void gen_bankrom (int address, int value);
int z80_irq_callback(int param);

#endif /* _GEN_H_ */

