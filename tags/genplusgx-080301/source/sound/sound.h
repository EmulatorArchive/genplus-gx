
#ifndef _SOUND_H_
#define _SOUND_H_

/* Global variables */
extern int fm_reg[2][0x100];
extern double fm_timera_tab[0x400];
extern double fm_timerb_tab[0x100];

/* Function prototypes */
extern void sound_init(int rate);
extern void sound_update(void);
extern void fm_restore(void);
extern void fm_write(unsigned int cpu, unsigned int  address, unsigned int  data);
extern unsigned int fm_read(unsigned int  cpu, unsigned int  address);
extern void psg_write(unsigned int  cpu, unsigned int  data);
extern int (*_YM2612_Reset)(void);

#endif /* _SOUND_H_ */
