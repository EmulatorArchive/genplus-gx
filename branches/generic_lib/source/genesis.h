
#ifndef _GENESIS_H_
#define _GENESIS_H_

/* Global variables */
extern uint8 cart_rom[0x500000];
extern uint8 bios_rom[0x800];
extern uint8 work_ram[0x10000];
extern uint8 zram[0x2000];
extern uint8 zbusreq;
extern uint8 zbusack;
extern uint8 zreset;
extern uint8 zirq;
extern uint32 zbank;
extern uint8 gen_running;
extern uint32 genromsize;
extern uint32 rom_size;
extern int32 resetline;
extern uint8 *rom_readmap[8];

/* Function prototypes */
extern void gen_init(void);
extern void gen_reset(unsigned int hard_reset);
extern void gen_shutdown(void);
extern unsigned int gen_busack_r(void);
extern void gen_busreq_w(unsigned int state);
extern void gen_reset_w(unsigned int state);
extern void gen_bank_w(unsigned int state);
extern int z80_irq_callback(int param);

#endif /* _GEN_H_ */

