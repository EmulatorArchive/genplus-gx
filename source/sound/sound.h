
#ifndef _SOUND_H_
#define _SOUND_H_

/* Global variables */
extern uint8 fm_reg[2][0x100];
extern uint8 fm_latch[2];
extern double fm_timera_tab[0x400];
extern double fm_timerb_tab[0x100];
extern uint8 ssg_enabled;

/* Function prototypes */
void sound_init(int rate);
void sound_update(void);
void fm_restore(void);
void fm_write (uint8 cpu, int address, int data);
int fm_read (uint8 cpu, int address);
void fm_update(uint8 cpu);
void psg_write (uint8 cpu, int data);

/* generic functions */
int  (*_YM2612_Write)(unsigned char adr, unsigned char data);
int  (*_YM2612_Read)(void);
void (*_YM2612_Update)(int **buf, int length);
int  (*_YM2612_Reset)(void);
void (*_PSG_Write)(int address, int data);
void (*_PSG_Update)(int which, int16 *buffer, int length);
void (*_PSG_Reset)(int which);


#endif /* _SOUND_H_ */
