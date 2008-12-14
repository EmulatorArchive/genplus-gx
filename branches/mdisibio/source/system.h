/***************************************************************************************
 *  Genesis Plus 1.2a
 *  Main Emulation
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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

typedef struct
{
  uint8 *data;			/* Bitmap data */
  int width;			/* Bitmap width (32+512+32) */
  int height;			/* Bitmap height (256) */
  int depth;			/* Color depth (8 bits) */
  int pitch;			/* Width of bitmap in bytes */
  int granularity;		/* Size of each pixel in bytes */
  int remap;			/* 1= Translate pixel data */
  struct
  {
    int x;			/* X offset of viewport within bitmap */
    int y;			/* Y offset of viewport within bitmap */
    int w;			/* Width of viewport */
    int h;			/* Height of viewport */
    int ow;			/* Previous width of viewport */
    int oh;			/* Previous height of viewport */
    int changed;		/* 1= Viewport width or height have changed */
  } viewport;
} t_bitmap;


typedef struct
{
  int sample_rate;		/* Sample rate (8000-48000) */
  int enabled;			/* 1= sound emulation is enabled */
  int buffer_size;		/* Size of sound buffer (in bytes) */
  int16 *buffer[2];		/* Signed 16-bit stereo sound data */
  struct
  {
    int curStage;
    int lastStage;
    int *buffer[2];
  } fm;
  struct
  {
    int curStage;
    int lastStage;
    int16 *buffer;
  } psg;
} t_snd;

/* Global variables */
extern t_bitmap bitmap;
extern t_snd snd;
extern uint16 lines_per_frame;
extern double Master_Clock;
extern uint8 vdp_rate;
extern uint32 m68cycles_per_line;
extern uint32 z80cycles_per_line;
extern uint32 aim_m68k;
extern uint32 count_m68k;
extern uint32 line_m68k;
extern uint32 aim_z80;
extern uint32 count_z80;
extern uint32 line_z80;
extern int32 current_z80;
extern uint8 interlaced;
extern uint8 odd_frame;
extern uint32 frame_cnt;

/* Function prototypes */
extern void system_init (void);
extern void system_reset (void);
extern void system_shutdown (void);
extern int system_frame(int skip);
extern void z80_run (int cyc);
extern int audio_init (int rate);

#endif /* _SYSTEM_H_ */

