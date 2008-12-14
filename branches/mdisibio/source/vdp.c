/***************************************************************************************
 *  Genesis Plus 1.2a
 *  Video Display Processor (memory handlers)
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  modified by Eke-Eke (compatibility fixes & additional code), GC/Wii port
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************************/

#include "shared.h"
#include "hvc.h"

/* Pack and unpack CRAM data */
#define PACK_CRAM(d)    ((((d)&0xE00)>>9)|(((d)&0x0E0)>>2)|(((d)&0x00E)<<5))
#define UNPACK_CRAM(d)  ((((d)&0x1C0)>>5)|((d)&0x038)<<2|(((d)&0x007)<<9))

/* Mark a pattern as dirty */
#define MARK_BG_DIRTY(addr)                                     \
{                                                               \
  int name = (addr >> 5) & 0x7FF;                             \
  if(bg_name_dirty[name] == 0) bg_name_list[bg_list_index++] = name; \
  bg_name_dirty[name] |= (1 << ((addr >> 2) & 0x07));         \
}

/* VDP context */
uint8 sat[0x400];     /* Internal copy of sprite attribute table */
uint8 vram[0x10000];  /* Video RAM (64Kx8) */
uint8 cram[0x80];			/* On-chip color RAM (64x9) */
uint8 vsram[0x80];    /* On-chip vertical scroll RAM (40x11) */
uint8 reg[0x20];			/* Internal VDP registers (23x8) */
uint16 addr;          /* Address register */
uint16 addr_latch;    /* Latched A15, A14 of address */
uint8 code;           /* Code register */
uint8 pending;				/* Pending write flag */
uint16 status;				/* VDP status flags */
uint8 dmafill;				/* next VDP Write is DMA Fill */
uint8 hint_pending;   /* 0= Line interrupt is pending */
uint8 vint_pending;   /* 1= Frame interrupt is pending */
uint8 vint_triggered; /* 1= Frame interrupt has been triggered */
int8 hvint_updated;   /* >= 0: Interrupt lines updated */

/* Global variables */
uint16 ntab;                      /* Name table A base address */
uint16 ntbb;                      /* Name table B base address */
uint16 ntwb;                      /* Name table W base address */
uint16 satb;                      /* Sprite attribute table base address */
uint16 hscb;				              /* Horizontal scroll table base address */
uint8 border;                     /* Border color index */
uint8 bg_name_dirty[0x800];	      /* 1= This pattern is dirty */
uint16 bg_name_list[0x800];	      /* List of modified pattern indices */
uint16 bg_list_index;		          /* # of modified patterns in list */
uint8 bg_pattern_cache[0x80000];  /* Cached and flipped patterns */
uint8 playfield_shift;		        /* Width of planes A, B (in bits) */
uint8 playfield_col_mask;         /* Vertical scroll mask */
uint16 playfield_row_mask;	      /* Horizontal scroll mask */
uint32 y_mask;                    /* Name table Y-index bits mask */
int16 h_counter;                  /* Raster counter */
int16 hc_latch;                   /* latched HCounter (INT2) */
uint16 v_counter;                 /* VDP scanline counter */
uint8 im2_flag;                   /* 1= Interlace mode 2 is being used */
uint32 dma_length;                /* Current DMA remaining bytes */
int32 fifo_write_cnt;             /* VDP writes fifo count */
uint32 fifo_lastwrite;            /* last VDP write cycle */
uint8 fifo_latency;               /* VDP write cycles latency */
uint8 vdp_pal	= 0;                /* 1: PAL , 0: NTSC (default) */

double vdp_timings[4][4];

/* Tables that define the playfield layout */
static const uint8 shift_table[] = { 6, 7, 0, 8 };
static const uint8 col_mask_table[] = { 0x0F, 0x1F, 0x0F, 0x3F };
static const uint16 row_mask_table[] = { 0x0FF, 0x1FF, 0x2FF, 0x3FF };
static const uint32 y_mask_table[] = { 0x1FC0, 0x1F80, 0x1FC0, 0x1F00 };

static uint16 sat_base_mask;	/* Base bits of SAT */
static uint16 sat_addr_mask;	/* Index bits of SAT */
static uint32 dma_endCycles;  /* 68k cycles to DMA end */
static uint8 dma_type;        /* Type of DMA */

/* TODO: set as default */
static uint8 dmatiming = 1;
static uint8 vdptiming = 1;

/* DMA Timings

 According to the manual, here's a table that describes the transfer
 rates of each of the three DMA types:

    DMA Mode      Width       Display      Transfer Count
    -----------------------------------------------------
    68K > VDP     32-cell     Active       16
                              Blanking     167
                  40-cell     Active       18
                              Blanking     205
    VRAM Fill     32-cell     Active       15
                              Blanking     166
                  40-cell     Active       17
                              Blanking     204
    VRAM Copy     32-cell     Active       8
                              Blanking     83
                  40-cell     Active       9
                              Blanking     102

 'Active' is the active display period, 'Blanking' is either the vertical
 blanking period or when the display is forcibly blanked via register #1.

 The above transfer counts are all in bytes, unless the destination is
 CRAM or VSRAM for a 68K > VDP transfer, in which case it is in words.

*/
static const uint8 dma_rates[16] = {
  8,  9, 84 , 102,	/* 68K to VRAM */
	16, 18, 167, 205, /* 68K to CRAM or VSRAM */
	15, 17, 166, 204, /* DMA fill */
  8,  9, 83 , 102,  /* DMA Copy */
};

/* Function prototypes */
static inline void data_write(unsigned int data);

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown functions                                          */
/*--------------------------------------------------------------------------*/
void vdp_init(void)
{
	int i;
	
	/* reinitialize DMA timings table */
	for (i=0; i<4; i++)
	{
		vdp_timings[0][i] = ((double)m68cycles_per_line) / ((double) dma_rates[i]);
		vdp_timings[1][i] = ((double)m68cycles_per_line) / ((double) dma_rates[i +  4]);
		vdp_timings[2][i] = ((double)m68cycles_per_line) / ((double) dma_rates[i +  8]);
		vdp_timings[3][i] = ((double)m68cycles_per_line) / ((double) dma_rates[i + 12]);
	}
}

void vdp_reset(void)
{
	memset ((char *) sat, 0, sizeof (sat));
	memset ((char *) vram, 0, sizeof (vram));
	memset ((char *) cram, 0, sizeof (cram));
	memset ((char *) vsram, 0, sizeof (vsram));
	memset ((char *) reg, 0, sizeof (reg));

	addr = 0;
	addr_latch = 0;
	code = 0;
	pending = 0;

	status = 0x200; /* fifo empty */

	ntab = 0;
	ntbb = 0;
	ntwb = 0;
	satb = 0;
	hscb = 0;

  sat_base_mask = 0xFE00;
  sat_addr_mask = 0x01FF;

  border = 0x00;

	memset ((char *) bg_name_dirty, 0, sizeof (bg_name_dirty));
	memset ((char *) bg_name_list, 0, sizeof (bg_name_list));
  bg_list_index = 0;
	memset ((char *) bg_pattern_cache, 0, sizeof (bg_pattern_cache));

  playfield_shift = 6;
	playfield_col_mask = 0x0F;
  playfield_row_mask = 0x0FF;
  y_mask = 0x1FC0;

	hint_pending = 0;
	vint_pending = 0;
	vint_triggered = 0;
	hvint_updated = -1;

	h_counter = 0;
	hc_latch = -1;
	v_counter = 0;

	dmafill = 0;
	dma_length = 0;
	dma_endCycles = 0;

	im2_flag = 0;
	interlaced = 0;

	fifo_write_cnt = 0;

	/* reset HVC tables */
	vctab = (vdp_pal) ? vc_pal_224 : vc_ntsc_224;
	hctab = cycle2hc32;

	/* reset display area */
  bitmap.viewport.w = 256;
  bitmap.viewport.h = 224;
  bitmap.viewport.oh = 256;
  bitmap.viewport.ow = 224;
  
	/* reset border area */
	bitmap.viewport.x = config.overscan ? 12 : 0;
	bitmap.viewport.y = config.overscan ? (vdp_pal ? 32 : 8) : 0;
  bitmap.viewport.changed = 1;

	/* initialize some registers (normally set by BIOS) */
	if (config.bios_enabled != 3)
	{
		vdp_reg_w(1 , 0x04);	/* Mode 5 enabled */
		vdp_reg_w(10, 0xff);	/* HINT disabled */
		vdp_reg_w(12, 0x81);	/* H40 mode */
		vdp_reg_w(15, 0x02);	/* auto increment */
	}
}

void vdp_shutdown(void)
{}

void vdp_restore(uint8 *vdp_regs)
{
	int i;
	
	for (i=0;i<0x20;i++) 
  {
		vdp_reg_w(i, vdp_regs[i]);
	}

	/* reinitialize HVC tables */
	vctab = (vdp_pal) ? ((reg[1] & 8) ? vc_pal_240 : vc_pal_224) : vc_ntsc_224;
	hctab = (reg[12] & 1) ? cycle2hc40 : cycle2hc32;

	/* reinitialize overscan area */
	bitmap.viewport.x = config.overscan ? ((reg[12] & 1) ? 16 : 12) : 0;
	bitmap.viewport.y = config.overscan ? (((reg[1] & 8) ? 0 : 8) + (vdp_pal ? 24 : 0)) : 0;
	bitmap.viewport.changed = 1;

	/* restore VDP FIFO timings */
	if (vdptiming)
	{
		/* VDP timings:
       ------------
        HDISP is 256*10/7 = approx. 366 cycles (same for both modes)
        this gives:
         H32: 16 accesses --> 366/16 = 23 cycles per access
         H40: 20 accesses --> 366/20 = 18 cycles per access

        VRAM access are byte wide --> VRAM writes takes 2x CPU cycles
    */
		fifo_latency = (reg[12] & 1) ? 27 : 30;
		if ((code & 0x0F) == 0x01) fifo_latency = fifo_latency * 2;
  }
  
	/* remake cache */
	for (i=0;i<0x800;i++) 
  {
    bg_name_list[i]=i;
    bg_name_dirty[i]=0xFF;
  }
	bg_list_index=0x800;

  /* reinitialize palette */
	for(i = 0; i < 0x40; i += 1) color_update(i, *(uint16 *)&cram[i << 1]);
	color_update(0x00, *(uint16 *)&cram[border << 1]);
}

/*--------------------------------------------------------------------------*/
/* DMA Operations                                                           */
/*--------------------------------------------------------------------------*/

/* Update DMA timings (this is call on start of DMA and then at the start of each scanline) */
void dma_update()
{
	int32 left_cycles;
	uint32 dma_cycles, dma_bytes;
	uint8 index = 0;

	if (!dmatiming) return; 

	/* get the appropriate tranfer rate (bytes/line) for this DMA operation */
	if ((status&8) || !(reg[1] & 0x40)) index = 2; /* VBLANK or Display OFF */
	index += (reg[12] & 1);						   /* 32 or 40 Horizontal Cells    */

	/* calculate transfer quantity for the remaining 68k cycles */
	left_cycles = aim_m68k - count_m68k;
	if (left_cycles < 0) left_cycles = 0;
	dma_bytes = (uint32)(((double)left_cycles / vdp_timings[dma_type][index]) + 0.5);

	/* determinate DMA length in CPU cycles */
	if (dma_length < dma_bytes)
	{
		/* DMA will be finished during this line */
		dma_cycles = (uint32)(((double)dma_length * vdp_timings[dma_type][index]) + 0.5);
		dma_length = 0;
	}
	else
	{
		/* DMA can not be finished until next scanline */
		dma_cycles = left_cycles;
		dma_length -= dma_bytes;
	}

	if (dma_type < 2)
	{
		/* 68K COPY to V-RAM */
		/* 68K is frozen during DMA operation */
		count_m68k += dma_cycles;
	}
	else
	{
		/* VRAM Fill or VRAM Copy */
		/* set DMA end cyles count */
		dma_endCycles = count_m68k + dma_cycles;
		
		/* set DMA Busy flag */
    status |= 0x0002;
	}
}

/*  DMA Copy
    Read byte from VRAM (source), write to VRAM (addr),
    bump source and add r15 to addr.

    - see how source addr is affected
      (can it make high source byte inc?)
*/
static inline void dma_copy(void)
{
  int length = (reg[20] << 8 | reg[19]) & 0xFFFF;
  int source = (reg[22] << 8 | reg[21]) & 0xFFFF;
  if (!length) length = 0x10000;

 	dma_type = 3;
	dma_length = length;
	dma_update();

	/* proceed DMA */
	do
	{
		vram[addr] = vram[source];
    MARK_BG_DIRTY(addr);
    source = (source + 1) & 0xFFFF;
		addr += reg[15];
  } while (--length);

	/* update length & source address registers */
	reg[19] = length & 0xFF;
  reg[20] = (length >> 8) & 0xFF;
	reg[21] = source & 0xFF; /* not sure */
	reg[22] = (source >> 8) & 0xFF; 
}


/* 68K Copy to VRAM, VSRAM or CRAM */
static inline void dma_vbus (void)
{
	uint32 base, source = ((reg[23] & 0x7F) << 17 | reg[22] << 9 | reg[21] << 1) & 0xFFFFFE;
	uint32 length = (reg[20] << 8 | reg[19]) & 0xFFFF;
  
	if (!length) length = 0x10000;
	base = source;

	/* DMA timings */
	dma_type = (code & 0x01) ? 0 : 1;
	dma_length = length;
	dma_update();

	/* proceed DMA */
	do
	{
    unsigned int temp = vdp_dma_r(source);
    source += 2;
    source = ((base & 0xFE0000) | (source & 0x1FFFF));
		data_write (temp);
	}
	while (--length);

	/* update length & source address registers */
	reg[19] = length & 0xFF;
  reg[20] = (length >> 8) & 0xFF;
  reg[21] = (source >> 1) & 0xFF;
  reg[22] = (source >> 9) & 0xFF;
  reg[23] = (reg[23] & 0x80) | ((source >> 17) & 0x7F);
}

/* VRAM FILL */
static inline void dma_fill(unsigned int data)
{
	int length = (reg[20] << 8 | reg[19]) & 0xFFFF;
	if (!length) length = 0x10000;

  /* DMA timings */
	dma_type = 2;
  dma_length = length;
  dma_update();

  /* proceed DMA */
  data_write(data);

	/* write MSB */
	data = (data >> 8) & 0xff;

	/* detect internal SAT modification */
	if ((addr & sat_base_mask) == satb)
	{
		do
		{
			/* update internal SAT (see Battletech) */
			WRITE_BYTE(sat, (addr & sat_addr_mask)^1, data);

			WRITE_BYTE(vram, addr^1, data);
			MARK_BG_DIRTY (addr);
			addr += reg[15];
		}
		while (--length);
	}
	else
	{
    do
    {
      WRITE_BYTE(vram, addr^1, data);
      MARK_BG_DIRTY (addr);
      addr += reg[15];
    }
    while (--length);
	}

  /* update length register */
  reg[19] = length & 0xFF;
  reg[20] = (length >> 8) & 0xFF;
  dmafill = 0;
}

/*--------------------------------------------------------------------------*/
/* FIFO emulation                                                  */
/*--------------------------------------------------------------------------*/
static inline void fifo_update()
{
	if (fifo_write_cnt > 0)
	{
		/* update FIFO reads */
		uint32 fifo_read = ((count_m68k - fifo_lastwrite) / fifo_latency);
		if (fifo_read > 0)
		{
			fifo_write_cnt -= fifo_read;
			if (fifo_write_cnt < 0) fifo_write_cnt = 0;

			/* update cycle count */
			fifo_lastwrite += fifo_read*fifo_latency;
		}
	}
}

/*--------------------------------------------------------------------------*/
/* Memory access functions                                                  */
/*--------------------------------------------------------------------------*/
static inline void data_write (unsigned int data)
{
	switch (code & 0x0F)
	{
		case 0x01:	/* VRAM */
      	  
			/* Byte-swap data if A0 is set */
			if (addr & 1) data = (data >> 8) | (data << 8);

			/* Copy SAT data to the internal SAT */
			if ((addr & sat_base_mask) == satb)
			{
				*(uint16 *) &sat[addr & sat_addr_mask] = data;
			}

			/* Only write unique data to VRAM */
			if (data != *(uint16 *) &vram[addr & 0xFFFE])
			{
				/* Write data to VRAM */
				*(uint16 *) &vram[addr & 0xFFFE] = data;

				/* Update the pattern cache */
				MARK_BG_DIRTY (addr);
			}
			break;

		case 0x03:	/* CRAM */
		{
			uint16 *p = (uint16 *) &cram[(addr & 0x7E)];
			data = PACK_CRAM (data & 0x0EEE);
			if (data != *p)
			{
				int index = (addr >> 1) & 0x3F;
				*p = data;
				color_update (index, *p);
				color_update (0x00, *(uint16 *)&cram[border << 1]);
			}
			break;
		}

		case 0x05:	/* VSRAM */
			*(uint16 *) &vsram[(addr & 0x7E)] = data;
			break;
	}

	/* Increment address register */
	addr += reg[15];
}


void vdp_ctrl_w(unsigned int data)
{
	if (pending == 0)
  {
		if ((data & 0xC000) == 0x8000)
    {
			/* VDP register write */
			uint8 r = (data >> 8) & 0x1F;
      uint8 d = data & 0xFF;
      vdp_reg_w (r, d);
    }
    else pending = 1;

    addr = ((addr_latch & 0xC000) | (data & 0x3FFF)) & 0xFFFF;
    code = ((code & 0x3C) | ((data >> 14) & 0x03)) & 0x3F;
  }
  else
  {
		/* Clear pending flag */
		pending = 0;

		/* Update address and code registers */
		addr = ((addr & 0x3FFF) | ((data & 3) << 14)) & 0xFFFF;
		code = ((code & 0x03) | ((data >> 2) & 0x3C)) & 0x3F;

		/* Save address bits A15 and A14 */
		addr_latch = (addr & 0xC000);

		if ((code & 0x20) && (reg[1] & 0x10))
		{
			switch (reg[23] & 0xC0)
			{
				case 0x00:		/* V bus to VDP DMA */
				case 0x40:		/* V bus to VDP DMA */
					dma_vbus();
					break;

				case 0x80:		/* VRAM fill */
					dmafill = 1;
					break;

				case 0xC0:		/* VRAM copy */
					dma_copy();
					break;
			}
		}
	}

	/* FIFO emulation */
	if (vdptiming)
	{
		/* VDP timings:
       ------------
        HDISP is 256*10/7 = approx. 366 cycles (same for both modes)
        this gives:
         H32: 16 accesses --> 366/16 = 23 cycles per access
         H40: 20 accesses --> 366/20 = 18 cycles per access

        VRAM access are byte wide --> VRAM writes takes 2x CPU cycles
    */
		fifo_latency = (reg[12] & 1) ? 27 : 30;
		if ((code & 0x0F) == 0x01) fifo_latency = fifo_latency * 2;
	}
}


unsigned int vdp_ctrl_r(void)
{
	/*
	 * Return vdp status
     *
     * Bits are
     * 0 	0:1 ntsc:pal
     * 1	DMA Busy
     * 2	During HBlank
     * 3	During VBlank
     * 4	Frame Interlace 0:even 1:odd
     * 5	Sprite collision
     * 6	Too many sprites per line
     * 7	v interrupt occurred
     * 8	Write FIFO full
     * 9	Write FIFO empty
     * 10 - 15	Next word on bus
     */

  /* update FIFO flags */
	if (vdptiming)
	{
		fifo_update();
		if (fifo_write_cnt < 4)
		{
			status &= 0xFEFF;							
			if (fifo_write_cnt == 0) status |= 0x200; 
		}
	}
	else status ^= 0x200;

	/* update DMA Busy flag */
	if ((status & 2) && !dma_length && (count_m68k >= dma_endCycles))
    status &= 0xFFFD;

	unsigned int temp = status | vdp_pal;

	/* display OFF: VBLANK flag is set */
	if (!(reg[1] & 0x40)) temp |= 0x8; 

	/* HBLANK flag (Sonic 3 and Sonic 2 "VS Modes", Lemmings 2) */
	if (count_m68k <= line_m68k + 84) temp |= 0x4;

	/* clear pending flag */
	pending = 0;

	/* clear SPR/SCOL flags */
	status &= 0xFF9F;

	return (temp);
}

void vdp_data_w(unsigned int data)
{
	/* Clear pending flag */
	pending = 0;

	if (dmafill)
	{
		dma_fill(data);
		return;
	}
  
	/* VDP latency (Chaos Engine, Soldiers of Fortune) */
	if (vdptiming && !(status&8) && (reg[1]&0x40))
	{
		fifo_update();
		if (fifo_write_cnt == 0)
		{
			fifo_lastwrite = count_m68k; /* reset cycle counter */
			status &= 0xFDFF;	/* FIFO is not empty anymore */
		}

		/* increase write counter */
		fifo_write_cnt ++;
		
		/* is FIFO full ? */
		if (fifo_write_cnt >= 4)
		{
			status |= 0x100; 
		  if (fifo_write_cnt > 4) count_m68k = fifo_lastwrite + fifo_latency;
	  }
	}

	/* write data */
	data_write(data);
}

unsigned int vdp_data_r(void)
{
	uint16 temp = 0;

	/* Clear pending flag */
	pending = 0;

	switch (code & 0x0F)
  {
		case 0x00:	/* VRAM */
			temp = *(uint16 *) & vram[(addr & 0xFFFE)];
			break;

		case 0x08:	/* CRAM */
			temp = *(uint16 *) & cram[(addr & 0x7E)];
			temp = UNPACK_CRAM (temp);
			break;

		case 0x04:	/* VSRAM */
			temp = *(uint16 *) & vsram[(addr & 0x7E)];
			break;
	}

	/* Increment address register */
  addr += reg[15];

	/* return data */
	return (temp);
}


/*
    The reg[] array is updated at the *end* of this function, so the new
    register data can be compared with the previous data.
*/
void vdp_reg_w(unsigned int r, unsigned int d)
{
	/* Check if Mode 4 (SMS mode) has been activated 
	   According to official doc, VDP registers #11 to #23 can not be written unless bit2 in register #1 is set
	   Fix Captain Planet & Avengers (Alt version), Bass Master Classic Pro Edition (they incidentally activate Mode 4) 
	 */
	if (!(reg[1] & 4) && (r > 10)) return;

  switch(r)
  {
    case 0x00: /* CTRL #1 */
			if ((d&0x10) != (reg[0]&0x10)) hvint_updated = 0;
			if (!(d & 0x02)) hc_latch = -1; /* latch HVC */
      break;

    case 0x01: /* CTRL #2 */
			/* Sesame Street Counting Cafe need to execute one instruction */
      if ((d&0x20) != (reg[1]&0x20)) hvint_updated = 1; 

      /* Check if the viewport height has actually been changed */
      if((reg[1] & 8) != (d & 8))
      {                
        /* Update the height of the viewport */
        bitmap.viewport.oh = bitmap.viewport.h;
        bitmap.viewport.h = (d & 8) ? 240 : 224;
				if (config.overscan) bitmap.viewport.y = ((vdp_pal ? 288 : 240) - bitmap.viewport.h) / 2;
        bitmap.viewport.changed = 1;                

				/* update VC table */
				if (vdp_pal) vctab = (d & 8) ? vc_pal_240 : vc_pal_224;
      }
			
      /* DISPLAY status modified before effective line starts rendering */
      if ((d&0x40) != (reg[1]&0x40))
      {
        if (count_m68k <= (line_m68k + 120))
        {
          /* Redraw the current line :
            - Legend of Galahad, Lemmings 2, Nigel Mansell's World Championship Racing (set display OFF)
            - Deadly Moves aka Power Athlete (set display ON)
          */
          reg[1] = d;
          render_line(v_counter,odd_frame);
        }
      }
      break;

    case 0x02: /* NTAB */
      ntab = (d << 10) & 0xE000;
      break;

    case 0x03: /* NTWB */
      ntwb = (d << 10) & 0xF800;
      if(reg[12] & 1) ntwb &= 0xF000;
      break;

    case 0x04: /* NTBB */
      ntbb = (d << 13) & 0xE000;
      break;

    case 0x05: /* SATB */
      sat_base_mask = (reg[12] & 1) ? 0xFC00 : 0xFE00;
      sat_addr_mask = (reg[12] & 1) ? 0x03FF : 0x01FF;
      satb = (d << 9) & sat_base_mask;
      break;

		case 0x07:	/* Border Color index */
      /* Check if the border color has actually changed */
      d &= 0x3F;
      if(border != d)
      {
        /* Mark the border color as modified */
        border = d;
        color_update(0x00, *(uint16 *)&cram[(border << 1)]);

        /* background color modified during HBLANK */
        if ((v_counter < bitmap.viewport.h) && (count_m68k <= (line_m68k + 84)))
        {
          /* remap current line (see Road Rash I,II,III) */
          reg[7] = d;
          remap_buffer(v_counter,bitmap.viewport.w + 2*bitmap.viewport.x);
				}
      }
      break;

    case 0x0C:
      /* Check if the viewport width has actually been changed */
      if((reg[0x0C] & 1) != (d & 1))
      {
        /* Update the width of the viewport */
        bitmap.viewport.ow = bitmap.viewport.w;
        bitmap.viewport.w = (d & 1) ? 320 : 256;
				if (config.overscan) bitmap.viewport.x = (d & 1) ? 16 : 12;
        bitmap.viewport.changed = 1;
		 
				/* update HC table */
				hctab = (d & 1) ? cycle2hc40 : cycle2hc32;
      }

      /* See if the S/TE mode bit has changed */
      if((reg[0x0C] & 8) != (d & 8))
      {
        int i;
				
        /* The following color update check this value */
        reg[0x0C] = d;

        /* Update colors */
				for (i = 0; i < 0x40; i += 1) color_update (i, *(uint16 *) & cram[i << 1]);
				color_update (0x00, *(uint16 *) & cram[border << 1]);
			}

      /* The following register updates check this value */
      reg[0x0C] = d;

      /* Update display-dependant registers */
      vdp_reg_w(0x03, reg[0x03]);
      vdp_reg_w(0x05, reg[0x05]);
      break;

    case 0x0D: /* HSCB */
      hscb = (d << 10) & 0xFC00;
      break;

    case 0x10: /* Playfield size */
      playfield_shift = shift_table[(d & 3)];
      playfield_col_mask = col_mask_table[(d & 3)];
      playfield_row_mask = row_mask_table[(d >> 4) & 3];
      y_mask = y_mask_table[(d & 3)];
      break;
  }

  /* Write new register value */
  reg[r] = d;
}


unsigned int vdp_hvc_r(void)
{
	uint8 hc = (hc_latch == -1) ? hctab[count_m68k%488] : (hc_latch & 0xFF);
	uint8 vc = vctab[v_counter];

 	/* interlace mode 2 */
	if (im2_flag) vc = (vc << 1) | ((vc >> 7) & 1);

	return ((vc << 8) | hc);
}


void vdp_test_w(unsigned int value)
{
#ifdef LOGERROR
  error("Unused VDP Write 0x%x (%08x)", value, m68k_get_reg (NULL, M68K_REG_PC));
#endif
}

int vdp_int_ack_callback(int int_level)
{
	if (vint_triggered)
	{
		vint_pending = 0;
		vint_triggered = 0;
		status &= ~0x0080; /* clear VINT flag */
  }
	else hint_pending = 0;
	hvint_updated = 0;
  return M68K_INT_ACK_AUTOVECTOR;
}
