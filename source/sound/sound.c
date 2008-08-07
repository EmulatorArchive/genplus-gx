/*
    sound.c
    FM and PSG handlers
*/

#include "shared.h"

static double m68cycles_per_sample;
static double z80cycles_per_sample;

/* YM2612 data */
uint8 fm_reg[2][0x100];		/* Register arrays (2x256) */
uint8 fm_latch[2];			/* Register latches */
double fm_timera_tab[0x400];   /* Precalculated timer A values (in usecs) */
double fm_timerb_tab[0x100];   /* Precalculated timer B values (in usecs) */
uint8 ssg_enabled = 0;
extern uint8 hq_fm;
	
void sound_init(int rate)
{
	int i;
	double vclk = Master_Clock / 7.0;  /* 68000 and YM2612 clock */
	double zclk = Master_Clock / 15.0; /* Z80 and SN76489 clock  */

	/* Make Timer A table */
	/* Formula is "time(us) = (1024 - A) * 144 * 1000000 / clock" */
	for(i = 0; i < 1024; i += 1) fm_timera_tab[i] = ((double)((1024 - i) * 144) * 1000000.0 / vclk);

	/* Make Timer B table */
	/* Formula is "time(us) = 16 * (256 - B) * 144 * 1000000 / clock" */
	for(i = 0; i < 256; i += 1) fm_timerb_tab[i] = ((double)((256 - i) * 16 * 144) * 1000000.0 / vclk);

	/* Cycle-Accurate sample generation */
	m68cycles_per_sample = (m68cycles_per_line * (double) lines_per_frame) / (double) (rate / vdp_rate);
	z80cycles_per_sample = (z80cycles_per_line * (double) lines_per_frame) / (double) (rate / vdp_rate);

	/* initialize sound chips */
	if (PSG_MAME)
	{
		_PSG_Write  = SN76496Write;
		_PSG_Update = SN76496Update;
		_PSG_Reset  = NULL;
		SN76496_sh_start((int)zclk, 0, rate);
	}
	else
	{
		_PSG_Write  = SN76489_Write;
		_PSG_Update = SN76489_Update;
		_PSG_Reset  = SN76489_Reset;
		SN76489_Init(0, (int)zclk, rate);
		SN76489_Config(0, MUTE_ALLON, VOL_FULL, FB_SEGAVDP, SRW_SEGAVDP, 0);
	}

	if (FM_GENS)
	{
		_YM2612_Write  = YM2612_Write;
		_YM2612_Read   = YM2612_Read;
		_YM2612_Update = YM2612_Update;
		_YM2612_Reset  = YM2612_Reset;
		YM2612_Init((int)vclk, rate, hq_fm);
	}
	else
	{
		_YM2612_Write  = YM2612Write;
		_YM2612_Read   = YM2612Read;
		_YM2612_Update = YM2612UpdateOne;
		_YM2612_Reset  = YM2612ResetChip;
		YM2612Init ((int)vclk, rate);
	}
} 

/* get remaining samples for the scanline */
void sound_update(void)
{
	int *fmBuffer[2];       
	int16 *psgBuffer;

	/* FM */
	fmBuffer[0] = snd.fm.buffer[0] + snd.fm.lastStage;
	fmBuffer[1] = snd.fm.buffer[1] + snd.fm.lastStage;
	_YM2612_Update(fmBuffer, snd.buffer_size - snd.fm.lastStage);
	snd.fm.lastStage = snd.fm.curStage = 0;

	/* PSG */
	if (snd.psg.lastStage > snd.buffer_size) snd.psg.lastStage = snd.buffer_size;
	psgBuffer = snd.psg.buffer + snd.psg.lastStage;
	_PSG_Update (0, psgBuffer, snd.buffer_size - snd.psg.lastStage);
	snd.psg.lastStage = snd.psg.curStage = 0;
}

/* restore sound state (only FM needed) */
void fm_restore(void)
{
	int i;

	_YM2612_Reset();

	/* feed all the registers and update internal state */
	for(i = 0; i < 0x100; i++)
	{
		_YM2612_Write(0, i);
		_YM2612_Write(1, fm_reg[0][i]);
		_YM2612_Write(2, i);
		_YM2612_Write(3, fm_reg[1][i]);
	}
}

/* return the number of samples that should have been rendered so far */
uint32 get_sample_cnt(uint8 is_z80)
{
	if (is_z80) return (uint32) ((double)(count_z80 + current_z80 - z80_ICount) / z80cycles_per_sample);
	else return (uint32) ((double) count_m68k / m68cycles_per_sample);
}

/* FM controller functions */
void fm_update(uint8 cpu)
{
	snd.fm.curStage = get_sample_cnt(cpu);	
	if(snd.fm.curStage - snd.fm.lastStage > 1)
	{
		int *tempBuffer[2];       
		tempBuffer[0] = snd.fm.buffer[0] + snd.fm.lastStage;
		tempBuffer[1] = snd.fm.buffer[1] + snd.fm.lastStage;
		_YM2612_Update(tempBuffer, snd.fm.curStage - snd.fm.lastStage);
		snd.fm.lastStage = snd.fm.curStage;

	}
}

void fm_write(uint8 cpu, int address, int data)
{
	int a0 = (address & 1);
	int a1 = (address >> 1) & 1;

	if(a0) fm_reg[a1][fm_latch[a1]] = data;
	else fm_latch[a1] = data;
	
	fm_update(cpu);
	_YM2612_Write(address & 3, data);
}

int fm_read(uint8 cpu, int address)
{
	fm_update(cpu);
	return (_YM2612_Read() & 0xff);
}

void psg_write(uint8 cpu, int data)
{
	if(snd.enabled)
	{
		snd.psg.curStage = get_sample_cnt(cpu);
		if(snd.psg.curStage - snd.psg.lastStage > 1)
		{
			int16 *tempBuffer;
			tempBuffer = snd.psg.buffer + snd.psg.lastStage;
			_PSG_Update (0, tempBuffer, snd.psg.curStage - snd.psg.lastStage);
			snd.psg.lastStage = snd.psg.curStage;
		}

		_PSG_Write(0, data);
	}
}
