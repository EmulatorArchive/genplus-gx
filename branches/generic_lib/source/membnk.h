#ifndef _MEMBNK_H_
#define _MEMBNK_H_

/* Function prototypes */
extern void z80_write_banked_memory(unsigned int address, unsigned int data);
extern unsigned int z80_read_banked_memory(unsigned int address);

#endif /* _MEMBNK_H_ */
