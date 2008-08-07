/*
    Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "shared.h"

uint8 *cart_rom;          /* cart_rom NEED to be previously dynamically allocated */
uint8 bios_rom[0x800];
uint8 work_ram[0x10000]; /* 68K work RAM */
uint8 zram[0x2000];		 /* Z80 work RAM */
uint8 zbusreq;			 /* /BUSREQ from Z80 */
uint8 zreset;			 /* /RESET to Z80 */
uint8 zbusack;			 /* /BUSACK to Z80 */
uint8 zirq;			     /* /IRQ to Z80 */
uint32 zbank;			 /* Address of Z80 bank window */
uint8 gen_running;
uint32 bustakencnt;
uint8 lastbusack;
uint32 genromsize;
uint8 bios_enabled;
uint32 rom_size;
uint8 *rom_readmap[8];
uint16 cpu_sync[512];

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown functions                                          */
/*--------------------------------------------------------------------------*/

void gen_init (void)
{
	int i;
	m68k_set_cpu_type (M68K_CPU_TYPE_68000);
	m68k_init();
	for (i=0; i<512; i++) cpu_sync[i] = (uint16)((((double)i * 7.0) / 15.0) + 0.5);
	z80_init(0,0,0,z80_irq_callback);
}

void gen_reset (uint8 hard_reset)
{
	if (hard_reset)
	{
		/* Clear RAM */
		memset (work_ram, 0, sizeof (work_ram));
		memset (zram, 0, sizeof (zram));

		/* Reset ROM Area */
	   int i;
	   rom_readmap[0] = (bios_enabled == 3) ? &bios_rom[0] : &cart_rom[0];
	   for (i=1; i<8; i++) rom_readmap[i] = &cart_rom[i << 19];
	   rom_size = (bios_enabled == 3) ? 0x800 : genromsize;
	}

	gen_running = 1;
	zreset = 0;			/* Z80 is reset */
	zbusreq = 0;		/* Z80 has control of the Z bus */
	zbusack = 1;		/* Z80 is busy using the Z bus */
	zbank = 0;			/* Assume default bank is 000000-007FFF */
	zirq = 0;			/* No interrupts occuring */
	bustakencnt   = 0;
	lastbusack = 1;
	resetline = -1;

	/* Reset 68000 and Z80 CPUs */
	m68k_pulse_reset ();
	z80_reset ();

	/* Reset YM2612 */
	_YM2612_Reset();	
}

void gen_shutdown (void)
{
}

/*-----------------------------------------------------------------------
  Bus controller chip functions                                            
  -----------------------------------------------------------------------*/
int gen_busack_r (void)
{
	if (!zbusack)
	{
		/* bus taken ? (from GENS - maybe not useful for genesis emulation only) */
		if (count_m68k > bustakencnt)  return 0;
		else return (lastbusack&1);
	}
	else return 1;
}

void gen_busreq_w (int state)
{
	int z80_cycles_to_run;

	input_raz (); /* from GENS */
  
	if (state == 1)
	{
		/* Bus Request */
		bustakencnt = count_m68k + 16;
		lastbusack = zbusack;
		if ((zbusreq == 0) && (zreset == 1))
		{
			/* Z80 stopped */
			/* z80 was ON during the last 68k cycles */
			/* we execute the appropriate number of z80 cycles */
			z80_cycles_to_run = line_z80 + cpu_sync[count_m68k - line_m68k];
			z80_run(z80_cycles_to_run);
		}
	}
	else
	{
		/* Bus released */
		if ((zbusreq == 1) && (zreset == 1))
		{
			/* Z80 started */
			/* z80 was OFF during the last 68k cycles */
			/* we burn the appropriate number of z80 cycles */
			z80_cycles_to_run = line_z80 + cpu_sync[count_m68k - line_m68k];
			count_z80 = z80_cycles_to_run;
		}
	}

	zbusreq = (state & 1);
	zbusack = 1 ^ (zbusreq & zreset);
}

void gen_reset_w (int state)
{
	int z80_cycles_to_run;

	if (state == 0)
	{
		/* Start Reset process */
		if ((zbusreq == 0) && (zreset == 1))
		{
			/* Z80 stopped */
			/* z80 was ON during the last 68k cycles */
			/* we execute the appropriate number of z80 cycles */
			z80_cycles_to_run = line_z80 + cpu_sync[count_m68k - line_m68k];
			z80_run(z80_cycles_to_run);
		}

		/* Reset Z80 & YM2612 */
		bustakencnt = 0;
		lastbusack = 1;
		_YM2612_Reset();
		z80_reset ();
	}
	else
	{
		/* Stop Reset process */
		if ((zbusreq == 0) && (zreset == 0))
		{
			/* Z80 started */
			/* z80 was OFF during the last 68k cycles */
			/* we burn the appropriate number of z80 cycles */
			z80_cycles_to_run = line_z80 + cpu_sync[count_m68k - line_m68k];
			count_z80 = z80_cycles_to_run;
		}
	}

	zreset = (state & 1);
	zbusack = 1 ^ (zbusreq & zreset);
}

void gen_bank_w (int state)
{
	zbank = ((zbank >> 1) | ((state & 1) << 23)) & 0xFF8000;
}

/* ROM Bankswitch mechanism
   This is only used by Super Street Fighter II
   The mechanism is explained here: http://www.trzy.org/files/ssf2.txt
   There are eight addressable banks and each bank is 512 Kbytes
   Bank 0 is fixed
 */
void gen_bankrom (int address, int value)
{
	uint8 bank = (address >> 1) & 7;
	
	switch (bank)
	{
		case 0:
			/* RAM bankswitch */
			sram.on = value & 1;
			sram.write = (value & 2) ? 0 : 1;
			break;
		
		default:
			/* ROM Bank 1-7 */
			rom_readmap[bank] = &cart_rom[value << 19];
			break;
	}
}


int z80_irq_callback (int param)
{
	zirq = 0;
	z80_set_irq_line (0, CLEAR_LINE);
	return 0xFF;
}
