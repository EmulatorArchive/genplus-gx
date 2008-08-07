/*
    Copyright (C) 1999, 2000, 2001, 2002, 2003  Charles MacDonald

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

#define CLOCK_NTSC 53693175
#define CLOCK_PAL  53203424
#define SND_SIZE (snd.buffer_size * sizeof(int16))

t_bitmap bitmap;
t_snd snd;
uint8  vdp_rate;
uint16 lines_per_frame;
int32 resetline;
double Master_Clock;
double m68cycles_per_line;
double z80cycles_per_line;

uint32 aim_m68k;
uint32 count_m68k;
uint32 line_m68k;
uint32 hint_m68k;
uint32 aim_z80;
uint32 line_z80;
uint32 count_z80;
int32  current_z80;
uint8 odd_frame;
uint8 interlaced = 0;

/****************************************************************
 * CPU execution managment
 ****************************************************************/

/* Interrupt Manager
   this is called before each new executed instruction
   only if interrupts have been updated 
 */

void update_interrupts(void)
{
	uint8 vint_latency = hvint_updated;
	hvint_updated = -1;

	if (vint_pending && (reg[1] & 0x20))
	{
    	vint_triggered = 1;
    	if (vint_latency) count_m68k += m68k_execute(vint_latency);
		m68k_set_irq(6);
	}
	else if (hint_pending && (reg[0] & 0x10))
	{
		m68k_set_irq(4);
	}
	else
	{
		m68k_set_irq(0);
	}
}

void m68k_run (int cyc) 
{
	while (count_m68k < cyc)
	{
		/* check interrupts */
		if (hvint_updated >= 0) update_interrupts();

		/* execute a single instruction */
		count_m68k += m68k_execute(1);
	}
}

void z80_run (int cyc) 
{
	current_z80 = cyc - count_z80;
	if (current_z80 > 0) count_z80 += z80_execute(current_z80);
}


/****************************************************************
 * Audio System initialization 
 ****************************************************************/
int audio_init (int rate)
{
	/* Clear the sound data context */
	memset (&snd, 0, sizeof (snd));

	/* Make sure the requested sample rate is valid */
	if (!rate || ((rate < 8000) | (rate > 48000))) return (0);
	snd.sample_rate = rate;
	
	/* Calculate the sound buffer size (for one frame) */
	snd.buffer_size = (rate / vdp_rate);

	/* (re)allocate sound buffers */
	snd.fm.buffer[0] = realloc (snd.fm.buffer[0], snd.buffer_size * sizeof (int));
	snd.fm.buffer[1] = realloc (snd.fm.buffer[1], snd.buffer_size * sizeof (int));
	if (!snd.fm.buffer[0] || !snd.fm.buffer[1]) return (-1);
	memset (snd.fm.buffer[0], 0, SND_SIZE*2);
	memset (snd.fm.buffer[1], 0, SND_SIZE*2);
	snd.psg.buffer = realloc (snd.psg.buffer, SND_SIZE);
	if (!snd.psg.buffer) return (-1);
	memset (snd.psg.buffer, 0, SND_SIZE);

	/* Set audio enable flag */
	snd.enabled = 1;

	/* Initialize Sound Chips emulation */
	sound_init(rate);

	return (0);
}

/****************************************************************
 * Virtual Genesis initialization
 ****************************************************************/
void system_init (void)
{
	/* PAL/NTSC timings */
	vdp_rate        = vdp_pal ? 50 : 60;
	lines_per_frame = vdp_pal ? 313 : 262;
	Master_Clock	= vdp_pal ? (double)CLOCK_PAL : (double)CLOCK_NTSC;
	
	/* CPU cycles increments */
	z80cycles_per_line = 228.0;
	m68cycles_per_line = 489.0; /* Double Dragon II & Winter Olympics (E) need some extra cycles */

	gen_init ();
	vdp_init ();
	render_init ();
}

/****************************************************************
 * Virtual Genesis Restart
 ****************************************************************/
void system_reset (void)
{
	aim_m68k   = 0;
	count_m68k = 0;
	line_m68k  = 0;
	aim_z80    = 0;
	count_z80  = 0;
    line_z80   = 0;
	odd_frame  = 0;

	gen_reset (1); /* hard reset */
	vdp_reset ();
	render_reset ();
	io_reset();

	/* reset AUDIO system */
	memset (snd.psg.buffer, 0, SND_SIZE);
	memset (snd.fm.buffer[0], 0, SND_SIZE*2);
	memset (snd.fm.buffer[1], 0, SND_SIZE*2);
	if (_PSG_Reset) _PSG_Reset(0);
}

/****************************************************************
 * Virtual Genesis shutdown
 ****************************************************************/
void system_shutdown (void)
{
	gen_shutdown ();
	vdp_shutdown ();
	render_shutdown ();
}

/****************************************************************
 * Virtual Genesis Frame emulation
 ****************************************************************/
int system_frame (int do_skip)
{
	int line;
	
	/* reset line cycles counts */
	count_m68k = 0;
	count_z80 = 0;
  	bustakencnt = 0;
    fifo_write_cnt = 0;
    fifo_lastwrite = 0;

	if (!gen_running) return 0;

    /* Clear VBLANK & DMA BUSY flags */
    status &= 0xFFF5;

	/* Look for interlace mode change */
	uint8 old_interlaced = interlaced;
    interlaced = (reg[12] & 2) >> 1;
	if (old_interlaced != interlaced)
    {
		bitmap.viewport.changed = 2;
	    im2_flag = ((reg[12] & 6) == 6) ? 1 : 0;
		odd_frame = 1;
	}

    /* Toggle even/odd field flag (interlaced modes only) */
    odd_frame ^= 1;
    if (odd_frame && interlaced) status |= 0x0010;
    else status &= 0xFFEF;

	/* Reset HCounter */
	h_counter = reg[10];

    /* Parse sprites for line 0 (done on line 261 or 312) */
	parse_satb (0x80);

	/* Line processing */
	for (line = 0; line < lines_per_frame; line ++)
	{
      	/* Update VCounter */
		v_counter = line;
		
      	/* Update CPU cycles to go */
	  	hint_m68k = count_m68k;
	  	line_m68k = aim_m68k;
		line_z80  = aim_z80;
	    aim_z80  = (uint32)((double)(line + 1) * z80cycles_per_line);
	    aim_m68k = (uint32)((double)(line + 1) * m68cycles_per_line);

        if (FM_GENS)
	    {
		  int *tempBuffer[2];       
		  tempBuffer[0] = snd.fm.buffer[0] + Sound_Extrapol[v_counter][0];
		  tempBuffer[1] = snd.fm.buffer[1] + Sound_Extrapol[v_counter][0];
		  YM2612_DacAndTimers_Update(tempBuffer, Sound_Extrapol[v_counter][1]);
		  snd.fm.curStage +=  Sound_Extrapol[v_counter][1];
	    }

		/* 6-Buttons or Menacer update */
		input_update();

      	/* Check if there is any DMA in progess */
        if (dma_length) dma_update();

		/* Check "soft reset" */
		if (line == resetline) gen_reset(0);

        /* Horizontal Interrupt */
		if (line <= bitmap.viewport.h)
		{
			if(--h_counter < 0)
			{
				h_counter = reg[10];
				hint_pending = 1;
			    hvint_updated = 0;
		    }
		}

	  	/* Render Line */
	  	if (!do_skip) 
	  	{
            render_line(line,odd_frame);
		    if (use_480i && interlaced) render_line(line,odd_frame^1);
			if (line < (bitmap.viewport.h-1)) parse_satb(0x81 + line);
	  	}

		/* Vertical Retrace */
		if (line == bitmap.viewport.h)
		{
            /* set VBLANK flag */
            status |= 0x08;

 	        /* Z80 interrupt is 16ms period (one frame) and 64us length (one scanline) */
			zirq = 1;
			z80_set_irq_line(0, ASSERT_LINE);  
			 
		    /* delay between HINT, VBLANK and VINT (approx. 14.7 us) */
		    m68k_run(line_m68k + 84); /* need to take upcoming latency in account (Hurricanes, Outrunners) */
		  	if (zreset == 1 && zbusreq == 0) z80_run(line_z80 + 40);
		  	else count_z80 = line_z80 + 40;

			/* Vertical Interrupt */
		  	status |= 0x80;
		  	vint_pending = 1;
		    hvint_updated = 36; /* Tyrants, Mega-Lo-Mania & Ex Mutant need some cycles to read VINT flag */
		}
    	else if (zirq)
        {
    	    /* clear any pending Z80 interrupt */
    	    zirq = 0;
            z80_set_irq_line(0, CLEAR_LINE);
        }

		/* Process line */
		m68k_run(aim_m68k);
		if (zreset == 1 && zbusreq == 0) z80_run(aim_z80);
		else count_z80 = aim_z80;
	}

	audio_update ();
		
	return gen_running;
}

/****************************************************************
 * Audio System update (this is called at the end of the frame)
 ****************************************************************/
#ifdef NGC
extern unsigned char soundbuffer[16][3840];
extern int mixbuffer;
extern double psg_preamp;
extern double fm_preamp;
extern u8 boost;
static int ll, rr;

void audio_update (void)
{
	int i;
	int l, r;
	int16 *sb = (int16 *) soundbuffer[mixbuffer];

	/* get remaining samples */
	sound_update();

	/* mix samples */
	for (i = 0; i < snd.buffer_size; i ++)
	{
		l = r = (int) ((double)snd.psg.buffer[i] * psg_preamp);
		l += (int) ((double)snd.fm.buffer[0][i] * fm_preamp);
		r += (int) ((double)snd.fm.buffer[1][i] * fm_preamp);
		snd.fm.buffer[0][i] = 0;
		snd.fm.buffer[1][i] = 0;
		snd.psg.buffer[i] = 0;

		/* single-pole low-pass filter (6 dB/octave) */
		ll = (ll + l) >> 1;
		rr = (rr + r) >> 1;

		/* boost volume if asked*/
		l = ll * boost;
		r = rr * boost;

		/* clipping */
		if (l > 32767) l = 32767;
		else if (l < -32768) l = -32768;
		if (r > 32767) r = 32767;
		else if (r < -32768) r = -32768;

		*sb++ = l;
		*sb++ = r;
  	}

  	mixbuffer++;
  	mixbuffer &= 0xf;
}
#else
void audio_update (void)
{
  int i;
  int16 acc;

  /* get remaining samples */
  sound_update();

  /* mix samples */
  for (i = 0; i < snd.buffer_size; i += 1)
  {
      int16 psg = snd.psg.buffer[i] / 2;
 
	  acc = 0;
      acc += snd.fm.buffer[0][i];
      acc += psg;
      snd.buffer[0][i] = acc;

      acc = 0;
      acc += snd.fm.buffer[1][i];
      acc += psg;
      snd.buffer[1][i] = acc;

	  snd.fm.buffer[0][i] = 0;
	  snd.fm.buffer[1][i] = 0;
	  snd.psg.buffer[i] = 0;
  }
}
#endif

