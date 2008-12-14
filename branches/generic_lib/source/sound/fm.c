/*
**
** File: fm_ym2612.c -- software implementation of Yamaha FM sound generator
**
** Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 1.4 (final beta)
**
*/

/* EkeEke (Gamecube Genesis Plus port): 

	- added DAC filtering
	- fixed internal FM timer emulation
	- removed unused multichip support and YMxxx support
	- added second address register latch (not sure if this is correct)
	- fixed CH3 Special Mode detection 
*/

/* About 2612DAC:

it was fuzzy ( serious table was unkown now ) 
static int dac2612[256]={
	
-127,-127,-126,-125,-125,-124,-123,-123,-122,-121,-121,-120,-120,-119,-119,-118,
-118,-117,-117,-117,-116,-116,-115,-115,-114,-114,-113,-113,-113,-112,-112,-111,
-111,-111,-110,-110,-109,-109,-109,-108,-108,-107,-107,-107,-106,-106,-105,-105,
-104,-104,-103,-103,-102,-102,-101,-101,-100,-100, -99, -99, -98, -98, -97, -97,
 -96, -95, -95, -94, -93, -93, -92, -91, -91, -90, -89, -89, -88, -87, -87, -86,
 -85, -85, -84, -83, -83, -82, -81, -81, -80, -79, -79, -78, -77, -76, -75, -74,
 -72, -71, -70, -69, -68, -67, -66, -65, -64, -63, -62, -61, -60, -59, -58, -57,
 -55, -54, -53, -52, -50, -48, -46, -44, -42, -38, -32, -30, -25, -20, -10,   0,

   0,  10,	20,  25,  30,  32,	38,  42,  44,  46,	48,  50,  52,  53,	54,  55,
  57,  58,	59,  60,  61,  62,	63,  64,  65,  66,	67,  68,  69,  70,	71,  72,
  74,  75,	76,  77,  78,  79,	79,  80,  81,  81,	82,  83,  83,  84,	85,  85,
  86,  87,	87,  88,  89,  89,	90,  91,  91,  92,	93,  93,  94,  95,	95,  96,

  97,  97,	98,  98,  99,  99, 100, 100, 101, 101, 102, 102, 103, 103, 104, 104,
 105, 105, 106, 106, 107, 107, 107, 108, 108, 109, 109, 109, 110, 110, 111, 111,
 111, 112, 112, 113, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 117, 118,
 118, 119, 119, 120, 120, 121, 121, 122, 123, 123, 124, 125, 125, 126, 127, 127
};


ym2612 ch#6 DAC was not liner., I think it is ...
+
 Amplify                                    :
    A                                       :
127 +                   __------~~ /        :
    |             _ --~~         /          :
    |         _-~              /            :
    |       /~               /              :
    |     /2612DAC         /                :
    |    /               /                  :
    |   /              /                    :
 63 +   /            /                      :
    |   /          /  linear dac            :
    |  /         /       (of couse logscalable at voltage) :
    |  /       /                            :
    | /      /                              :
    | /    /                                :
    | /  /                                  :
-   || /                                    :
    |/                                      :
  --+----------------+-------------|-> data :
   0|                63           127       :
  0x80      - <--                 0xff --> +
(minus as same it., but zero(0x80 or 0x7f) cross point at linear.)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "shared.h"

/* globals */
#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define EG_SH			16  /* 16.16 fixed point (envelope generator timing) */
#define LFO_SH			24  /*  8.24 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)

#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)

#define MAX_ATT_INDEX	(ENV_LEN-1) /* 1023 */
#define MIN_ATT_INDEX	(0)			/* 0 */

#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256) /* 8 bits addressing (real chip) */

/*  TL_TAB_LEN is calculated as:
*   13 - sinus amplitude bits     (Y axis)
*   2  - sinus sign bit           (Y axis)
*   TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
static signed int tl_tab[TL_TAB_LEN];

#define ENV_QUIET		(TL_TAB_LEN>>3)

/* sin waveform table in 'decibel' scale */
static unsigned int sin_tab[SIN_LEN];

/* sustain level table (3dB per step) */
/* bit0, bit1, bit2, bit3, bit4, bit5, bit6 */
/* 1,    2,    4,    8,    16,   32,   64   (value)*/
/* 0.75, 1.5,  3,    6,    12,   24,   48   (dB)*/

/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define SC(db) (UINT32) ( db * (4.0/ENV_STEP) )
static const UINT32 sl_table[16]={
 SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
 SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC


#define RATE_STEPS (8)
static const UINT8 eg_inc[19*RATE_STEPS]={

/*cycle:0 1  2 3  4 5  6 7*/

/* 0 */ 0,1, 0,1, 0,1, 0,1, /* rates 00..11 0 (increment by 0 or 1) */
/* 1 */ 0,1, 0,1, 1,1, 0,1, /* rates 00..11 1 */
/* 2 */ 0,1, 1,1, 0,1, 1,1, /* rates 00..11 2 */
/* 3 */ 0,1, 1,1, 1,1, 1,1, /* rates 00..11 3 */

/* 4 */ 1,1, 1,1, 1,1, 1,1, /* rate 12 0 (increment by 1) */
/* 5 */ 1,1, 1,2, 1,1, 1,2, /* rate 12 1 */
/* 6 */ 1,2, 1,2, 1,2, 1,2, /* rate 12 2 */
/* 7 */ 1,2, 2,2, 1,2, 2,2, /* rate 12 3 */

/* 8 */ 2,2, 2,2, 2,2, 2,2, /* rate 13 0 (increment by 2) */
/* 9 */ 2,2, 2,4, 2,2, 2,4, /* rate 13 1 */
/*10 */ 2,4, 2,4, 2,4, 2,4, /* rate 13 2 */
/*11 */ 2,4, 4,4, 2,4, 4,4, /* rate 13 3 */

/*12 */ 4,4, 4,4, 4,4, 4,4, /* rate 14 0 (increment by 4) */
/*13 */ 4,4, 4,8, 4,4, 4,8, /* rate 14 1 */
/*14 */ 4,8, 4,8, 4,8, 4,8, /* rate 14 2 */
/*15 */ 4,8, 8,8, 4,8, 8,8, /* rate 14 3 */

/*16 */ 8,8, 8,8, 8,8, 8,8, /* rates 15 0, 15 1, 15 2, 15 3 (increment by 8) */
/*17 */ 16,16,16,16,16,16,16,16, /* rates 15 2, 15 3 for attack */
/*18 */ 0,0, 0,0, 0,0, 0,0, /* infinity rates for attack and decay(s) */
};


#define O(a) (a*RATE_STEPS)

/*note that there is no O(17) in this table - it's directly in the code */
static const UINT8 eg_rate_select[32+64+32]={	/* Envelope Generator rates (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),

/* rates 00-11 */
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),

/* rate 12 */
O( 4),O( 5),O( 6),O( 7),

/* rate 13 */
O( 8),O( 9),O(10),O(11),

/* rate 14 */
O(12),O(13),O(14),O(15),

/* rate 15 */
O(16),O(16),O(16),O(16),

/* 32 dummy rates (same as 15 3) */
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16)

};
#undef O

/*rate  0,    1,    2,   3,   4,   5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15*/
/*shift 11,   10,   9,   8,   7,   6,  5,  4,  3,  2, 1,  0,  0,  0,  0,  0 */
/*mask  2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3, 1,  0,  0,  0,  0,  0 */

#define O(a) (a*1)
static const UINT8 eg_rate_shift[32+64+32]={	/* Envelope Generator counter shifts (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),

/* rates 00-11 */
O(11),O(11),O(11),O(11),
O(10),O(10),O(10),O(10),
O( 9),O( 9),O( 9),O( 9),
O( 8),O( 8),O( 8),O( 8),
O( 7),O( 7),O( 7),O( 7),
O( 6),O( 6),O( 6),O( 6),
O( 5),O( 5),O( 5),O( 5),
O( 4),O( 4),O( 4),O( 4),
O( 3),O( 3),O( 3),O( 3),
O( 2),O( 2),O( 2),O( 2),
O( 1),O( 1),O( 1),O( 1),
O( 0),O( 0),O( 0),O( 0),

/* rate 12 */
O( 0),O( 0),O( 0),O( 0),

/* rate 13 */
O( 0),O( 0),O( 0),O( 0),

/* rate 14 */
O( 0),O( 0),O( 0),O( 0),

/* rate 15 */
O( 0),O( 0),O( 0),O( 0),

/* 32 dummy rates (same as 15 3) */
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0)

};
#undef O

static const UINT8 dt_tab[4 * 32]={
/* this is YM2151 and YM2612 phase increment data (in 10.10 fixed point format)*/
/* FD=0 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* FD=1 */
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
	2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* FD=2 */
	1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
	5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* FD=3 */
	2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
	8 , 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};


/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static const UINT8 opn_fktable[16] = {0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};


/* 8 LFO speed parameters */
/* each value represents number of samples that one LFO level will last for */
static const UINT32 lfo_samples_per_step[8] = {108, 77, 71, 67, 62, 44, 8, 5};



/*There are 4 different LFO AM depths available, they are:
  0 dB, 1.4 dB, 5.9 dB, 11.8 dB
  Here is how it is generated (in EG steps):

  11.8 dB = 0, 2, 4, 6, 8, 10,12,14,16...126,126,124,122,120,118,....4,2,0
   5.9 dB = 0, 1, 2, 3, 4, 5, 6, 7, 8....63, 63, 62, 61, 60, 59,.....2,1,0
   1.4 dB = 0, 0, 0, 0, 1, 1, 1, 1, 2,...15, 15, 15, 15, 14, 14,.....0,0,0

  (1.4 dB is loosing precision as you can see)

  It's implemented as generator from 0..126 with step 2 then a shift
  right N times, where N is:
    8 for 0 dB
    3 for 1.4 dB
    1 for 5.9 dB
    0 for 11.8 dB
*/
static const UINT8 lfo_ams_depth_shift[4] = {8, 3, 1, 0};


/*There are 8 different LFO PM depths available, they are:
  0, 3.4, 6.7, 10, 14, 20, 40, 80 (cents)

  Modulation level at each depth depends on F-NUMBER bits: 4,5,6,7,8,9,10
  (bits 8,9,10 = FNUM MSB from OCT/FNUM register)

  Here we store only first quarter (positive one) of full waveform.
  Full table (lfo_pm_table) containing all 128 waveforms is build
  at run (init) time.

  One value in table below represents 4 (four) basic LFO steps
  (1 PM step = 4 AM steps).

  For example:
   at LFO SPEED=0 (which is 108 samples per basic LFO step)
   one value from "lfo_pm_output" table lasts for 432 consecutive
   samples (4*108=432) and one full LFO waveform cycle lasts for 13824
   samples (32*432=13824; 32 because we store only a quarter of whole
            waveform in the table below)
*/
static const UINT8 lfo_pm_output[7*8][8]={ /* 7 bits meaningful (of F-NUMBER), 8 LFO output levels per one depth (out of 32), 8 LFO depths */
/* FNUM BIT 4: 000 0001xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 3 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 4 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 5 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 6 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 7 */ {0,   0,   0,   0,   1,   1,   1,   1},

/* FNUM BIT 5: 000 0010xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 3 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 4 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 5 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 6 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 7 */ {0,   0,   1,   1,   2,   2,   2,   3},

/* FNUM BIT 6: 000 0100xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 3 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 4 */ {0,   0,   0,   0,   0,   0,   0,   1},
/* DEPTH 5 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 6 */ {0,   0,   1,   1,   2,   2,   2,   3},
/* DEPTH 7 */ {0,   0,   2,   3,   4,   4,   5,   6},

/* FNUM BIT 7: 000 1000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   1,   1},
/* DEPTH 3 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 4 */ {0,   0,   0,   1,   1,   1,   1,   2},
/* DEPTH 5 */ {0,   0,   1,   1,   2,   2,   2,   3},
/* DEPTH 6 */ {0,   0,   2,   3,   4,   4,   5,   6},
/* DEPTH 7 */ {0,   0,   4,   6,   8,   8, 0xa, 0xc},

/* FNUM BIT 8: 001 0000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 2 */ {0,   0,   0,   1,   1,   1,   2,   2},
/* DEPTH 3 */ {0,   0,   1,   1,   2,   2,   3,   3},
/* DEPTH 4 */ {0,   0,   1,   2,   2,   2,   3,   4},
/* DEPTH 5 */ {0,   0,   2,   3,   4,   4,   5,   6},
/* DEPTH 6 */ {0,   0,   4,   6,   8,   8, 0xa, 0xc},
/* DEPTH 7 */ {0,   0,   8, 0xc,0x10,0x10,0x14,0x18},

/* FNUM BIT 9: 010 0000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   2,   2,   2,   2},
/* DEPTH 2 */ {0,   0,   0,   2,   2,   2,   4,   4},
/* DEPTH 3 */ {0,   0,   2,   2,   4,   4,   6,   6},
/* DEPTH 4 */ {0,   0,   2,   4,   4,   4,   6,   8},
/* DEPTH 5 */ {0,   0,   4,   6,   8,   8, 0xa, 0xc},
/* DEPTH 6 */ {0,   0,   8, 0xc,0x10,0x10,0x14,0x18},
/* DEPTH 7 */ {0,   0,0x10,0x18,0x20,0x20,0x28,0x30},

/* FNUM BIT10: 100 0000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   4,   4,   4,   4},
/* DEPTH 2 */ {0,   0,   0,   4,   4,   4,   8,   8},
/* DEPTH 3 */ {0,   0,   4,   4,   8,   8, 0xc, 0xc},
/* DEPTH 4 */ {0,   0,   4,   8,   8,   8, 0xc,0x10},
/* DEPTH 5 */ {0,   0,   8, 0xc,0x10,0x10,0x14,0x18},
/* DEPTH 6 */ {0,   0,0x10,0x18,0x20,0x20,0x28,0x30},
/* DEPTH 7 */ {0,   0,0x20,0x30,0x40,0x40,0x50,0x60},

};

/* all 128 LFO PM waveforms */
static INT32 lfo_pm_table[128*8*32]; /* 128 combinations of 7 bits meaningful (of F-NUMBER), 8 LFO depths, 32 LFO output levels per one depth */

/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)

/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* struct describing a single operator (SLOT) */
typedef struct
{
	INT32	*DT;		/* detune          :dt_tab[DT] */
	UINT8	KSR;		/* key scale rate  :3-KSR */
	UINT32	ar;			/* attack rate  */
	UINT32	d1r;		/* decay rate   */
	UINT32	d2r;		/* sustain rate */
	UINT32	rr;			/* release rate */
	UINT8	ksr;		/* key scale rate  :kcode>>(3-KSR) */
	UINT32	mul;		/* multiple        :ML_TABLE[ML] */

	/* Phase Generator */
	UINT32	phase;		/* phase counter */
	UINT32	Incr;		/* phase step */

	/* Envelope Generator */
	UINT8	state;		/* phase type */
	UINT32	tl;			/* total level: TL << 3 */
	INT32	volume;		/* envelope counter */
	UINT32	sl;			/* sustain level:sl_table[SL] */
	UINT32	vol_out;	/* current output from EG circuit (without AM from LFO) */

	UINT8	eg_sh_ar;	/*  (attack state) */
	UINT8	eg_sel_ar;	/*  (attack state) */
	UINT8	eg_sh_d1r;	/*  (decay state) */
	UINT8	eg_sel_d1r;	/*  (decay state) */
	UINT8	eg_sh_d2r;	/*  (sustain state) */
	UINT8	eg_sel_d2r;	/*  (sustain state) */
	UINT8	eg_sh_rr;	/*  (release state) */
	UINT8	eg_sel_rr;	/*  (release state) */

	UINT8	ssg;		/* SSG-EG waveform */
	UINT8	ssgn;		/* SSG-EG negated output */

	UINT32	key;		/* 0=last key was KEY OFF, 1=KEY ON */

	/* LFO */
	UINT32	AMmask;		/* AM enable flag */

} FM_SLOT;

typedef struct
{
	FM_SLOT	SLOT[4];	/* four SLOTs (operators) */

	UINT8	ALGO;		/* algorithm */
	UINT8	FB;			/* feedback shift */
	INT32	op1_out[2];	/* op1 output for feedback */

	INT32	*connect1;	/* SLOT1 output pointer */
	INT32	*connect3;	/* SLOT3 output pointer */
	INT32	*connect2;	/* SLOT2 output pointer */
	INT32	*connect4;	/* SLOT4 output pointer */

	INT32	*mem_connect;/* where to put the delayed sample (MEM) */
	INT32	mem_value;	/* delayed sample (MEM) value */

	INT32	pms;		/* channel PMS */
	UINT8	ams;		/* channel AMS */

	UINT32	fc;			/* fnum,blk:adjusted to sample rate */
	UINT8	kcode;		/* key code:                        */
	UINT32	block_fnum;	/* current blk/fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_CH;


typedef struct
{
	int		clock;		/* master clock  (Hz)   */
	int		rate;		/* sampling rate (Hz)   */
	double	freqbase;	/* frequency base       */
	int	    TimerBase;	/* Timer base time      */
	UINT8	address[2];	/* address register     */
	UINT8	status;		/* status flag          */
	UINT32	mode;		/* mode  CSM / 3SLOT    */
	UINT8	fn_h;		/* freq latch           */
	int	TA;			/* timer a value        */
	double	TAL;		/* timer a base		    */
	double	TAC;		/* timer a counter      */
	int	TB;			/* timer b              */
	double	TBL;		/* timer b base		    */
	double	TBC;		/* timer b counter      */
	/* local time tables */
	INT32	dt_tab[8][32];/* DeTune table       */
} FM_ST;


/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct
{
	UINT32  fc[3];			/* fnum3,blk3: calculated */
	UINT8	fn_h;			/* freq3 latch */
	UINT8	kcode[3];		/* key code */
	UINT32	block_fnum[3];	/* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_3SLOT;

/* OPN/A/B common state */
typedef struct
{
	FM_ST	ST;				/* general state */
	FM_3SLOT SL3;			/* 3 slot mode state */
	unsigned int pan[6*2];	/* fm channels output masks (0xffffffff = enable) */

	UINT32	eg_cnt;			/* global envelope generator counter */
	UINT32	eg_timer;		/* global envelope generator counter works at frequency = chipclock/64/3 */
	UINT32	eg_timer_add;	/* step of eg_timer */
	UINT32	eg_timer_overflow;/* envelope generator timer overlfows every 3 samples (on real chip) */

	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
        but LFO works with one more bit of a precision so we really need 4096 elements */
	UINT32	fn_table[4096];	/* fnumber->increment counter */

	/* LFO */
	UINT32	lfo_cnt;
	UINT32	lfo_inc;
	UINT32	lfo_freq[8];	/* LFO FREQ table */
} FM_OPN;

typedef struct
{
	FM_CH		CH[6];		/* channel state */
	UINT8		dacen;		/* DAC mode  */
	INT32		dacout;		/* DAC output */
	FM_OPN		OPN;		/* OPN state */
} YM2612_t;

static YM2612_t ym2612;
static long dac_highpass;

/* current chip state */
static INT32	m2,c1,c2;		/* Phase Modulation input for operators 2,3,4 */
static INT32	mem;			/* one sample delay memory */
static INT32	out_fm[8];		/* outputs of working channels */
static UINT32	LFO_AM;			/* runtime LFO calculations helper */
static INT32	LFO_PM;			/* runtime LFO calculations helper */

/* OPN Mode Register Write */
INLINE void set_timers(int v )
{
	/* b7 = CSM MODE */
	/* b6 = 3 slot mode */
	/* b5 = reset b */
	/* b4 = reset a */
	/* b3 = timer enable b */
	/* b2 = timer enable a */
	/* b1 = load b */
	/* b0 = load a */

	if ((ym2612.OPN.ST.mode ^ v) & 0x40) ym2612.CH[2].SLOT[SLOT1].Incr=-1; /* recalulate phase (from gens) */
	ym2612.OPN.ST.mode = v;

	/* reset Timer b flag */
	if( v & 0x20 ) ym2612.OPN.ST.status &= ~0x02;

	/* reset Timer a flag */
	if( v & 0x10 ) ym2612.OPN.ST.status &= ~0x01;
}


INLINE void FM_KEYON(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( !SLOT->key )
	{
		SLOT->key = 1;
		SLOT->phase = 0;		/* restart Phase Generator */
		SLOT->state = EG_ATT;	/* phase -> Attack */
	}
}

INLINE void FM_KEYOFF(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( SLOT->key )
	{
		SLOT->key = 0;
		if (SLOT->state>EG_REL) SLOT->state = EG_REL; /* phase -> Release */
	}
}

/* set algorithm connection */
static void setup_connection( FM_CH *CH, int ch )
{
	INT32 *carrier = &out_fm[ch];
	INT32 **om1 = &CH->connect1;
	INT32 **om2 = &CH->connect3;
	INT32 **oc1 = &CH->connect2;
	INT32 **memc = &CH->mem_connect;

	switch( CH->ALGO )
	{
		case 0:
			/* M1---C1---MEM---M2---C2---OUT */
			*om1 = &c1;
			*oc1 = &mem;
			*om2 = &c2;
			*memc= &m2;
			break;
		case 1:
			/* M1------+-MEM---M2---C2---OUT */
			/*      C1-+                     */
			*om1 = &mem;
			*oc1 = &mem;
			*om2 = &c2;
			*memc= &m2;
			break;
		case 2:
			/* M1-----------------+-C2---OUT */
			/*      C1---MEM---M2-+          */
			*om1 = &c2;
			*oc1 = &mem;
			*om2 = &c2;
			*memc= &m2;
			break;
		case 3:
			/* M1---C1---MEM------+-C2---OUT */
			/*                 M2-+          */
			*om1 = &c1;
			*oc1 = &mem;
			*om2 = &c2;
			*memc= &c2;
			break;
		case 4:
			/* M1---C1-+-OUT */
			/* M2---C2-+     */
			/* MEM: not used */
			*om1 = &c1;
			*oc1 = carrier;
			*om2 = &c2;
			*memc= &mem;	/* store it anywhere where it will not be used */
			break;
		case 5:
			/*    +----C1----+     */
			/* M1-+-MEM---M2-+-OUT */
			/*    +----C2----+     */
			*om1 = 0;	/* special mark */
			*oc1 = carrier;
			*om2 = carrier;
			*memc= &m2;
			break;
		case 6:
			/* M1---C1-+     */
			/*      M2-+-OUT */
			/*      C2-+     */
			/* MEM: not used */
			*om1 = &c1;
			*oc1 = carrier;
			*om2 = carrier;
			*memc= &mem;	/* store it anywhere where it will not be used */
			break;
		case 7:
			/* M1-+     */
			/* C1-+-OUT */
			/* M2-+     */
			/* C2-+     */
			/* MEM: not used*/
			*om1 = carrier;
			*oc1 = carrier;
			*om2 = carrier;
			*memc= &mem;	/* store it anywhere where it will not be used */
			break;
	}

	CH->connect4 = carrier;
}

/* set detune & multiple */
INLINE void set_det_mul(FM_CH *CH,FM_SLOT *SLOT,int v)
{
	SLOT->mul = (v&0x0f)? (v&0x0f)*2 : 1;
	SLOT->DT  = ym2612.OPN.ST.dt_tab[(v>>4)&7];
	CH->SLOT[SLOT1].Incr=-1;
}

/* set total level */
INLINE void set_tl(FM_CH *CH,FM_SLOT *SLOT , int v)
{
	SLOT->tl = (v&0x7f)<<(ENV_BITS-7); /* 7bit TL */
}

/* set attack rate & key scale  */
INLINE void set_ar_ksr(FM_CH *CH,FM_SLOT *SLOT,int v)
{
	UINT8 old_KSR = SLOT->KSR;

	SLOT->ar = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;

	SLOT->KSR = 3-(v>>6);
	if (SLOT->KSR != old_KSR)
	{
		CH->SLOT[SLOT1].Incr=-1;
	}
	else
	{
		/* refresh Attack rate */
		if ((SLOT->ar + SLOT->ksr) < 32+62)
		{
			SLOT->eg_sh_ar  = eg_rate_shift [SLOT->ar  + SLOT->ksr ];
			SLOT->eg_sel_ar = eg_rate_select[SLOT->ar  + SLOT->ksr ];
		}
		else
		{
			SLOT->eg_sh_ar  = 0;
			SLOT->eg_sel_ar = 17*RATE_STEPS;
		}
	}
}

/* set decay rate */
INLINE void set_dr(FM_SLOT *SLOT,int v)
{
	SLOT->d1r = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
	SLOT->eg_sh_d1r = eg_rate_shift [SLOT->d1r + SLOT->ksr];
	SLOT->eg_sel_d1r= eg_rate_select[SLOT->d1r + SLOT->ksr];
}

/* set sustain rate */
INLINE void set_sr(FM_SLOT *SLOT,int v)
{
	SLOT->d2r = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
	SLOT->eg_sh_d2r = eg_rate_shift [SLOT->d2r + SLOT->ksr];
	SLOT->eg_sel_d2r= eg_rate_select[SLOT->d2r + SLOT->ksr];
}

/* set release rate */
INLINE void set_sl_rr(FM_SLOT *SLOT,int v)
{
	SLOT->sl = sl_table[ v>>4 ];
	SLOT->rr  = 34 + ((v&0x0f)<<2);
	SLOT->eg_sh_rr  = eg_rate_shift [SLOT->rr  + SLOT->ksr];
	SLOT->eg_sel_rr = eg_rate_select[SLOT->rr  + SLOT->ksr];
}

INLINE signed int op_calc(UINT32 phase, unsigned int env, signed int pm)
{
	UINT32 p;
	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + (pm<<15))) >> FREQ_SH ) & SIN_MASK ];
	if (p >= TL_TAB_LEN) return 0;
	return tl_tab[p];
}

INLINE signed int op_calc1(UINT32 phase, unsigned int env, signed int pm)
{
	UINT32 p;
	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + pm      )) >> FREQ_SH ) & SIN_MASK ];
	if (p >= TL_TAB_LEN) return 0;
	return tl_tab[p];
}

/* advance LFO to next sample */
INLINE void advance_lfo()
{
	UINT8 pos;
	UINT8 prev_pos;

	if (ym2612.OPN.lfo_inc)	/* LFO enabled ? */
	{
		prev_pos = ym2612.OPN.lfo_cnt>>LFO_SH & 127;
		ym2612.OPN.lfo_cnt +=  ym2612.OPN.lfo_inc;
		pos = ( ym2612.OPN.lfo_cnt >> LFO_SH) & 127;

		/* update AM when LFO output changes */
		/*if (prev_pos != pos)*/
		/* actually I can't optimize is this way without rewritting chan_calc()
        to use chip->lfo_am instead of global lfo_am */
		{
			/* triangle */
			/* AM: 0 to 126 step +2, 126 to 0 step -2 */
			if (pos<64) LFO_AM = (pos&63) * 2;
			else LFO_AM = 126 - ((pos&63) * 2);
		}

		/* PM works with 4 times slower clock */
		prev_pos >>= 2;
		pos >>= 2;
		/* update PM when LFO output changes */
		/*if (prev_pos != pos)*/ /* can't use global lfo_pm for this optimization, must be chip->lfo_pm instead*/
		{
			LFO_PM = pos;
		}
	}
	else
	{
		LFO_AM = 0;
		LFO_PM = 0;
	}
}

INLINE void advance_eg_channel(FM_SLOT *SLOT)
{
	unsigned int out;
	unsigned int swap_flag = 0;
	unsigned int i;

	i = 4; /* four operators per channel */
	do
	{
		switch(SLOT->state)
		{
			case EG_ATT:		/* attack phase */
				if (!(ym2612.OPN.eg_cnt & ((1<<SLOT->eg_sh_ar)-1)))
				{
					SLOT->volume += (~SLOT->volume * (eg_inc[SLOT->eg_sel_ar + ((ym2612.OPN.eg_cnt>>SLOT->eg_sh_ar)&7)]))>>4;
					if (SLOT->volume <= MIN_ATT_INDEX)
					{
						SLOT->volume = MIN_ATT_INDEX;
						SLOT->state = EG_DEC;
					}
				}
				break;

			case EG_DEC:	/* decay phase */
				if (SLOT->ssg&0x08)	/* SSG EG type envelope selected */
				{
					if ( !(ym2612.OPN.eg_cnt & ((1<<SLOT->eg_sh_d1r)-1) ) )
					{
						SLOT->volume += 4 * eg_inc[SLOT->eg_sel_d1r + ((ym2612.OPN.eg_cnt>>SLOT->eg_sh_d1r)&7)];

						if ( SLOT->volume >= SLOT->sl )
							SLOT->state = EG_SUS;
					}
				}
				else
				{
					if ( !(ym2612.OPN.eg_cnt & ((1<<SLOT->eg_sh_d1r)-1) ) )
					{
						SLOT->volume += eg_inc[SLOT->eg_sel_d1r + ((ym2612.OPN.eg_cnt>>SLOT->eg_sh_d1r)&7)];

						if ( SLOT->volume >= SLOT->sl )
							SLOT->state = EG_SUS;
					}
				}
				break;

			case EG_SUS:	/* sustain phase */
				if (SLOT->ssg&0x08)	/* SSG EG type envelope selected */
				{
					if ( !(ym2612.OPN.eg_cnt & ((1<<SLOT->eg_sh_d2r)-1) ) )
					{
						SLOT->volume += 4 * eg_inc[SLOT->eg_sel_d2r + ((ym2612.OPN.eg_cnt>>SLOT->eg_sh_d2r)&7)];

						if ( SLOT->volume >= MAX_ATT_INDEX )
						{
							SLOT->volume = MAX_ATT_INDEX;

							if (SLOT->ssg&0x01)	/* bit 0 = hold */
							{
								if (SLOT->ssgn&1)	/* have we swapped once ??? */
								{
									/* yes, so do nothing, just hold current level */
								}
								else swap_flag = (SLOT->ssg&0x02) | 1 ; /* bit 1 = alternate */

							}
							else
							{
								/* same as KEY-ON operation */

								/* restart of the Phase Generator should be here,
									only if AR is not maximum ??? */
								/*SLOT->phase = 0;*/

								/* phase -> Attack */
								SLOT->state = EG_ATT;

								swap_flag = (SLOT->ssg&0x02); /* bit 1 = alternate */
							}
						}
					}
				}
				else
				{
					if ( !(ym2612.OPN.eg_cnt & ((1<<SLOT->eg_sh_d2r)-1) ) )
					{
						SLOT->volume += eg_inc[SLOT->eg_sel_d2r + ((ym2612.OPN.eg_cnt>>SLOT->eg_sh_d2r)&7)];

						if ( SLOT->volume >= MAX_ATT_INDEX )
						{
							SLOT->volume = MAX_ATT_INDEX;
							/* do not change SLOT->state (verified on real chip) */
						}
					}
				}
				break;

			case EG_REL:	/* release phase */
				if ( !(ym2612.OPN.eg_cnt & ((1<<SLOT->eg_sh_rr)-1) ) )
				{
					SLOT->volume += eg_inc[SLOT->eg_sel_rr + ((ym2612.OPN.eg_cnt>>SLOT->eg_sh_rr)&7)];

					if ( SLOT->volume >= MAX_ATT_INDEX )
					{
						SLOT->volume = MAX_ATT_INDEX;
						SLOT->state = EG_OFF;
					}
				}
				break;
		}

		out = SLOT->tl + ((UINT32)SLOT->volume);

		if ((SLOT->ssg&0x08) && (SLOT->ssgn&2))	/* negate output (changes come from alternate bit, init comes from attack bit) */
			out ^= ((1<<ENV_BITS)-1); /* 1023 */

		/* we need to store the result here because we are going to change ssgn
            in next instruction */
		SLOT->vol_out = out;

		SLOT->ssgn ^= swap_flag;

		SLOT++;
		i--;

	} while (i);
}

#define volume_calc(OP) ((OP)->vol_out + (AM & (OP)->AMmask))

INLINE void chan_calc(FM_CH *CH)
{
	unsigned int eg_out;

	UINT32 AM = LFO_AM >> CH->ams;

	m2 = c1 = c2 = mem = 0;

	*CH->mem_connect = CH->mem_value;	/* restore delayed sample (MEM) value to m2 or c2 */

	eg_out = volume_calc(&CH->SLOT[SLOT1]);
	{
		INT32 out = CH->op1_out[0] + CH->op1_out[1];
		CH->op1_out[0] = CH->op1_out[1];

		if( !CH->connect1 )
		{
			/* algorithm 5  */
			mem = c1 = c2 = CH->op1_out[0];
		}
		else
		{
			/* other algorithms */
			*CH->connect1 += CH->op1_out[0];
		}

		CH->op1_out[1] = 0;
		if( eg_out < ENV_QUIET )	/* SLOT 1 */
		{
			if (!CH->FB) out=0;
			CH->op1_out[1] = op_calc1(CH->SLOT[SLOT1].phase, eg_out, (out<<CH->FB) );
		}
	}

	eg_out = volume_calc(&CH->SLOT[SLOT3]);
	if( eg_out < ENV_QUIET )		/* SLOT 3 */
		*CH->connect3 += op_calc(CH->SLOT[SLOT3].phase, eg_out, m2);

	eg_out = volume_calc(&CH->SLOT[SLOT2]);
	if( eg_out < ENV_QUIET )		/* SLOT 2 */
		*CH->connect2 += op_calc(CH->SLOT[SLOT2].phase, eg_out, c1);

	eg_out = volume_calc(&CH->SLOT[SLOT4]);
	if( eg_out < ENV_QUIET )		/* SLOT 4 */
		*CH->connect4 += op_calc(CH->SLOT[SLOT4].phase, eg_out, c2);

	/* store current MEM */
	CH->mem_value = mem;

	/* update phase counters AFTER output calculations */
	if(CH->pms)
	{
		/* add support for 3 slot mode */
		UINT32 block_fnum = CH->block_fnum;

		UINT32 fnum_lfo   = ((block_fnum & 0x7f0) >> 4) * 32 * 8;
		INT32  lfo_fn_table_index_offset = lfo_pm_table[ fnum_lfo + CH->pms + LFO_PM ];

		if (lfo_fn_table_index_offset)	/* LFO phase modulation active */
		{
			UINT8  blk;
			UINT32 fn;
			int kc,fc;

			block_fnum = block_fnum*2 + lfo_fn_table_index_offset;

			blk = (block_fnum&0x7000) >> 12;
			fn  = block_fnum & 0xfff;

			/* keyscale code */
			kc = (blk<<2) | opn_fktable[fn >> 8];
 			/* phase increment counter */
			fc = ym2612.OPN.fn_table[fn]>>(7-blk);

			CH->SLOT[SLOT1].phase += ((fc+CH->SLOT[SLOT1].DT[kc])*CH->SLOT[SLOT1].mul) >> 1;
			CH->SLOT[SLOT2].phase += ((fc+CH->SLOT[SLOT2].DT[kc])*CH->SLOT[SLOT2].mul) >> 1;
			CH->SLOT[SLOT3].phase += ((fc+CH->SLOT[SLOT3].DT[kc])*CH->SLOT[SLOT3].mul) >> 1;
			CH->SLOT[SLOT4].phase += ((fc+CH->SLOT[SLOT4].DT[kc])*CH->SLOT[SLOT4].mul) >> 1;
		}
		else	/* LFO phase modulation  = zero */
		{
			CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
			CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
			CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
			CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
		}
	}
	else	/* no LFO phase modulation */
	{
		CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
		CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
		CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
		CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
	}
}

/* update phase increment and envelope generator */
INLINE void refresh_fc_eg_slot(FM_SLOT *SLOT , int fc , int kc )
{
	int ksr;

	/* (frequency) phase increment counter */
	SLOT->Incr = ((fc+SLOT->DT[kc])*SLOT->mul) >> 1;

	ksr = kc >> SLOT->KSR;
	if( SLOT->ksr != ksr )
	{
		SLOT->ksr = ksr;

		/* calculate envelope generator rates */
		if ((SLOT->ar + SLOT->ksr) < 94/*32+62*/)
		{
			SLOT->eg_sh_ar  = eg_rate_shift [SLOT->ar  + SLOT->ksr ];
			SLOT->eg_sel_ar = eg_rate_select[SLOT->ar  + SLOT->ksr ];
		}
		else
		{
			SLOT->eg_sh_ar  = 0;
			SLOT->eg_sel_ar = 17*RATE_STEPS;
		}

		SLOT->eg_sh_d1r = eg_rate_shift [SLOT->d1r + SLOT->ksr];
		SLOT->eg_sel_d1r= eg_rate_select[SLOT->d1r + SLOT->ksr];

		SLOT->eg_sh_d2r = eg_rate_shift [SLOT->d2r + SLOT->ksr];
		SLOT->eg_sel_d2r= eg_rate_select[SLOT->d2r + SLOT->ksr];

		SLOT->eg_sh_rr  = eg_rate_shift [SLOT->rr  + SLOT->ksr];
		SLOT->eg_sel_rr = eg_rate_select[SLOT->rr  + SLOT->ksr];
	}
}

/* update phase increment counters */
INLINE void refresh_fc_eg_chan(FM_CH *CH )
{
	if( CH->SLOT[SLOT1].Incr==-1){
		int fc = CH->fc;
		int kc = CH->kcode;
		refresh_fc_eg_slot(&CH->SLOT[SLOT1] , fc , kc );
		refresh_fc_eg_slot(&CH->SLOT[SLOT2] , fc , kc );
		refresh_fc_eg_slot(&CH->SLOT[SLOT3] , fc , kc );
		refresh_fc_eg_slot(&CH->SLOT[SLOT4] , fc , kc );
	}
}

/* initialize time tables */
static void init_timetables(const UINT8 *dttable )
{
	int i,d;
	double rate;

	/* DeTune table */
	for (d = 0;d <= 3;d++)
	{
		for (i = 0;i <= 31;i++)
		{
			rate = ((double)dttable[d*32 + i]) * SIN_LEN  * ym2612.OPN.ST.freqbase * (1<<FREQ_SH) / ((double)(1<<20));
			ym2612.OPN.ST.dt_tab[d][i]   = (INT32) rate;
			ym2612.OPN.ST.dt_tab[d+4][i] = -ym2612.OPN.ST.dt_tab[d][i];
		}
	}
}


static void reset_channels(FM_CH *CH , int num )
{
	int c,s;

	for( c = 0 ; c < num ; c++ )
	{
		CH[c].fc = 0;
		for(s = 0 ; s < 4 ; s++ )
		{
			CH[c].SLOT[s].ssg = 0;
			CH[c].SLOT[s].ssgn = 0;
			CH[c].SLOT[s].state= EG_OFF;
			CH[c].SLOT[s].volume = MAX_ATT_INDEX;
			CH[c].SLOT[s].vol_out= MAX_ATT_INDEX;
		}
	}
}

/* initialize generic tables */
static int init_tables(void)
{
	signed int i,x;
	signed int n;
	double o,m;

	for (x=0; x<TL_RES_LEN; x++)
	{
		m = (1<<16) / pow(2, (x+1) * (ENV_STEP/4.0) / 8.0);
		m = floor(m);

		/* we never reach (1<<16) here due to the (x+1) */
		/* result fits within 16 bits at maximum */
		n = (int)m;		/* 16 bits here */
		n >>= 4;		/* 12 bits here */
		if (n&1)		/* round to nearest */
			n = (n>>1)+1;
		else
			n = n>>1;
						/* 11 bits here (rounded) */
		n <<= 2;		/* 13 bits here (as in real chip) */
		tl_tab[ x*2 + 0 ] = n;
		tl_tab[ x*2 + 1 ] = -tl_tab[ x*2 + 0 ];

		for (i=1; i<13; i++)
		{
			tl_tab[ x*2+0 + i*2*TL_RES_LEN ] =  tl_tab[ x*2+0 ]>>i;
			tl_tab[ x*2+1 + i*2*TL_RES_LEN ] = -tl_tab[ x*2+0 + i*2*TL_RES_LEN ];
		}
	}

	for (i=0; i<SIN_LEN; i++)
	{
		/* non-standard sinus */
		m = sin( ((i*2)+1) * M_PI / SIN_LEN ); /* checked against the real chip */

		/* we never reach zero here due to ((i*2)+1) */
		if (m>0.0) o = 8*log(1.0/m)/log(2);	/* convert to 'decibels' */
		else o = 8*log(-1.0/m)/log(2);	/* convert to 'decibels' */

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)						/* round to nearest */
			n = (n>>1)+1;
		else
			n = n>>1;

		sin_tab[ i ] = n*2 + (m>=0.0? 0: 1 );
	}

	/* build LFO PM modulation table */
	for(i = 0; i < 8; i++) /* 8 PM depths */
	{
		UINT8 fnum;
		for (fnum=0; fnum<128; fnum++) /* 7 bits meaningful of F-NUMBER */
		{
			UINT8 value;
			UINT8 step;
			UINT32 offset_depth = i;
			UINT32 offset_fnum_bit;
			UINT32 bit_tmp;

			for (step=0; step<8; step++)
			{
				value = 0;
				for (bit_tmp=0; bit_tmp<7; bit_tmp++) /* 7 bits */
				{
					if (fnum & (1<<bit_tmp)) /* only if bit "bit_tmp" is set */
					{
						offset_fnum_bit = bit_tmp * 8;
						value += lfo_pm_output[offset_fnum_bit + offset_depth][step];
					}
				}
				lfo_pm_table[(fnum*32*8) + (i*32) + step   + 0] = value;
				lfo_pm_table[(fnum*32*8) + (i*32) +(step^7)+ 8] = value;
				lfo_pm_table[(fnum*32*8) + (i*32) + step   +16] = -value;
				lfo_pm_table[(fnum*32*8) + (i*32) +(step^7)+24] = -value;
			}
		}
	}

	return 1;
}


/* CSM Key Controll */
INLINE void CSMKeyControll(FM_CH *CH)
{
	/* this is wrong, atm */

	/* all key on */
	FM_KEYON(CH,SLOT1);
	FM_KEYON(CH,SLOT2);
	FM_KEYON(CH,SLOT3);
	FM_KEYON(CH,SLOT4);
}

static void INTERNAL_TIMER_A()
{
	if (ym2612.OPN.ST.mode & 0x01)
	{
		if ((ym2612.OPN.ST.TAC -= ym2612.OPN.ST.TimerBase) <= 0)
		{
			/* set status (if enabled) */
			if (ym2612.OPN.ST.mode & 0x04) ym2612.OPN.ST.status |= 0x01;

			/* reload the counter */
			ym2612.OPN.ST.TAC += ym2612.OPN.ST.TAL;

			/* CSM mode total level latch and auto key on */
			if (ym2612.OPN.ST.mode & 0x80) CSMKeyControll (&(ym2612.CH[2]));
		}
	}
}

static void INTERNAL_TIMER_B(int step)
{
	if (ym2612.OPN.ST.mode & 0x02)
	{
		if ((ym2612.OPN.ST.TBC -= (ym2612.OPN.ST.TimerBase * (double)step)) <= 0)
		{
			/* set status (if enabled) */
			if (ym2612.OPN.ST.mode & 0x08) ym2612.OPN.ST.status |= 0x02;

			/* reload the counter */
			ym2612.OPN.ST.TBC += ym2612.OPN.ST.TBL;
		}		
	}
}

/* prescaler set (and make time tables) */
static void OPNSetPres(int pres)
{
	int i;

	/* frequency base */
	ym2612.OPN.ST.freqbase = ((double) ym2612.OPN.ST.clock / (double) ym2612.OPN.ST.rate) / ((double) pres);

#if 0
	ym2612.OPN.ST.rate = (double) ym2612.OPN.ST.clock / ((double) pres);
	ym2612.OPN.ST.freqbase = 1.0;
#endif

	ym2612.OPN.eg_timer_add  = (1<<EG_SH)  *  ym2612.OPN.ST.freqbase;
	ym2612.OPN.eg_timer_overflow = ( 3 ) * (1<<EG_SH);

	/* timer increment in usecs (timers are incremented after each updated samples) */
	ym2612.OPN.ST.TimerBase = 1000000.0 / (double)ym2612.OPN.ST.rate;

	/* make time tables */
	init_timetables(dt_tab);

	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
        but LFO works with one more bit of a precision so we really need 4096 elements */
	/* calculate fnumber -> increment counter table */
	for(i = 0; i < 4096; i++)
	{
		/* freq table for octave 7 */
		/* OPN phase increment counter = 20bit */
		ym2612.OPN.fn_table[i] = (UINT32)( (double)i * 32 * ym2612.OPN.ST.freqbase * (1<<(FREQ_SH-10)) ); /* -10 because chip works with 10.10 fixed point, while we use 16.16 */
	}

	/* LFO freq. table */
	for(i = 0; i < 8; i++)
	{
		/* Amplitude modulation: 64 output levels (triangle waveform); 1 level lasts for one of "lfo_samples_per_step" samples */
		/* Phase modulation: one entry from lfo_pm_output lasts for one of 4 * "lfo_samples_per_step" samples  */
		ym2612.OPN.lfo_freq[i] = (1.0 / lfo_samples_per_step[i]) * (1<<LFO_SH) * ym2612.OPN.ST.freqbase;
	}
}



/* write a OPN mode register 0x20-0x2f */
static void OPNWriteMode(int r, int v)
{
	UINT8 c;
	FM_CH *CH;

	switch(r)
	{
		case 0x21:	/* Test */
			break;

		case 0x22:	/* LFO FREQ (YM2608/YM2610/YM2610B/ym2612) */
			if (v&0x08) ym2612.OPN.lfo_inc = ym2612.OPN.lfo_freq[v&7]; /* LFO enabled ? */
			else ym2612.OPN.lfo_inc = 0;
			break;

		case 0x24:	/* timer A High 8*/
			ym2612.OPN.ST.TA = (ym2612.OPN.ST.TA & 0x03)|(((int)v)<<2);
			if (ym2612.OPN.ST.TAL != fm_timera_tab[ym2612.OPN.ST.TA])
			{
				ym2612.OPN.ST.TAL = ym2612.OPN.ST.TAC = fm_timera_tab[ym2612.OPN.ST.TA];
			}
			break;
		
		case 0x25:	/* timer A Low 2*/
			ym2612.OPN.ST.TA = (ym2612.OPN.ST.TA & 0x3fc)|(v&3);
			if (ym2612.OPN.ST.TAL != fm_timera_tab[ym2612.OPN.ST.TA])
			{
				ym2612.OPN.ST.TAL = ym2612.OPN.ST.TAC = fm_timera_tab[ym2612.OPN.ST.TA];
			}
            break;
		
		case 0x26:	/* timer B */
			ym2612.OPN.ST.TB = v;
			if (ym2612.OPN.ST.TBL != fm_timerb_tab[ym2612.OPN.ST.TB])
			{
				ym2612.OPN.ST.TBL = ym2612.OPN.ST.TBC = fm_timerb_tab[ym2612.OPN.ST.TB];
			}
            break;

		case 0x27:	/* mode, timer control */
			set_timers(v);
			break;
	
		case 0x28:	/* key on / off */
			c = v & 0x03;
			if( c == 3 ) break;
			if (v&0x04) c+=3; /* CH 4-6 */
			CH = &ym2612.CH[c];

			/* detect YM2612 bug :
			   The bug always happen on Channel 3 in Special Mode, with the following settings:
			     - algorithm #4
				 - feedback = 4
				 - SLOT2 TL and T1L set to zero

			   This happens in many games using GEMS sound engine (Flashback, Ariel, Shaq Fu,...)
			*/
			if ((ym2612.OPN.ST.mode & 0x40) && (c == 2) && (CH->SLOT[SLOT3].tl == 0) && (CH->ALGO == 4) && (CH->FB == 10))
			{
				/* this is a hack, but on real hardware, it seems more like the channel output is very attenuated */
				set_tl(CH,&(CH->SLOT[SLOT2]),0x3F);
				set_tl(CH,&(CH->SLOT[SLOT4]),0x3F);
			}
		
			if (v&0x10)FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
			if (v&0x20) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
			if (v&0x40) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
			if (v&0x80) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
			break;
	}
}

/* write a OPN register (0x30-0xff) */
static void OPNWriteReg(int r, int v)
{
	FM_CH *CH;
	FM_SLOT *SLOT;

	UINT8 c = OPN_CHAN(r);

	if (c == 3) return; /* 0xX3,0xX7,0xXB,0xXF */

	if (r >= 0x100) c+=3;

	CH = &ym2612.CH[c];

	SLOT = &(CH->SLOT[OPN_SLOT(r)]);

	switch( r & 0xf0 )
	{
		case 0x30:	/* DET , MUL */
			set_det_mul(CH,SLOT,v);
			break;

		case 0x40:	/* TL */
			set_tl(CH,SLOT,v);
			break;

		case 0x50:	/* KS, AR */
			set_ar_ksr(CH,SLOT,v);
			break;

		case 0x60:	/* bit7 = AM ENABLE, DR */
			set_dr(SLOT,v);
			SLOT->AMmask = (v&0x80) ? ~0 : 0;
			break;

		case 0x70:	/*     SR */
			set_sr(SLOT,v);
			break;

		case 0x80:	/* SL, RR */
			set_sl_rr(SLOT,v);
			break;

		case 0x90:	/* SSG-EG */
			if (!ssg_enabled) break;
			SLOT->ssg  =  v&0x0f;
			SLOT->ssgn = (v&0x04)>>1; /* bit 1 in ssgn = attack */

			/* SSG-EG envelope shapes :

			E AtAlH
			1 0 0 0  \\\\

			1 0 0 1  \___

			1 0 1 0  \/\/
					  ___
			1 0 1 1  \

			1 1 0 0  ////
					  ___
			1 1 0 1  /

			1 1 1 0  /\/\

			1 1 1 1  /___


			E = SSG-EG enable


			The shapes are generated using Attack, Decay and Sustain phases.

			Each single character in the diagrams above represents this whole
			sequence:

			- when KEY-ON = 1, normal Attack phase is generated (*without* any
			  difference when compared to normal mode),

			- later, when envelope level reaches minimum level (max volume),
			  the EG switches to Decay phase (which works with bigger steps
			  when compared to normal mode - see below),

			- later when envelope level passes the SL level,
			  the EG swithes to Sustain phase (which works with bigger steps
			  when compared to normal mode - see below),

			- finally when envelope level reaches maximum level (min volume),
			  the EG switches to Attack phase again (depends on actual waveform).

			Important is that when switch to Attack phase occurs, the phase counter
			of that operator will be zeroed-out (as in normal KEY-ON) but not always.
			(I havent found the rule for that - perhaps only when the output level is low)

			The difference (when compared to normal Envelope Generator mode) is
			that the resolution in Decay and Sustain phases is 4 times lower;
			this results in only 256 steps instead of normal 1024.
			In other words:
			when SSG-EG is disabled, the step inside of the EG is one,
			when SSG-EG is enabled, the step is four (in Decay and Sustain phases).

			Times between the level changes are the same in both modes.


			Important:
			Decay 1 Level (so called SL) is compared to actual SSG-EG output, so
			it is the same in both SSG and no-SSG modes, with this exception:

			when the SSG-EG is enabled and is generating raising levels
			(when the EG output is inverted) the SL will be found at wrong level !!!
			For example, when SL=02:
				0 -6 = -6dB in non-inverted EG output
				96-6 = -90dB in inverted EG output
			Which means that EG compares its level to SL as usual, and that the
			output is simply inverted afterall.


			The Yamaha's manuals say that AR should be set to 0x1f (max speed).
			That is not necessary, but then EG will be generating Attack phase.

			*/
			break;

		case 0xa0:

			switch( OPN_SLOT(r) )
			{
				case 0:		/* 0xa0-0xa2 : FNUM1 */
				{
					UINT32 fn = (((UINT32)((ym2612.OPN.ST.fn_h)&7))<<8) + v;
					UINT8 blk = ym2612.OPN.ST.fn_h>>3;
					
					/* keyscale code */
					CH->kcode = (blk<<2) | opn_fktable[fn >> 7];
					
					/* phase increment counter */
					CH->fc = ym2612.OPN.fn_table[fn*2]>>(7-blk);

					/* store fnum in clear form for LFO PM calculations */
					CH->block_fnum = (blk<<11) | fn;
					CH->SLOT[SLOT1].Incr=-1;
					break;
				}
					
				case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
					ym2612.OPN.ST.fn_h = v&0x3f;
					break;
			
				case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
					if(r < 0x100)
					{
						UINT32 fn = (((UINT32)(ym2612.OPN.SL3.fn_h&7))<<8) + v;
						UINT8 blk = ym2612.OPN.SL3.fn_h>>3;
						
						/* keyscale code */
						ym2612.OPN.SL3.kcode[c]= (blk<<2) | opn_fktable[fn >> 7];
						
						/* phase increment counter */
						ym2612.OPN.SL3.fc[c] = ym2612.OPN.fn_table[fn*2]>>(7-blk);
						ym2612.OPN.SL3.block_fnum[c] = fn;
						ym2612.CH[2].SLOT[SLOT1].Incr=-1;
					}
					break;						
				
				case 3:		/* 0xac-0xae : 3CH FNUM2,BLK */
					if(r < 0x100)
					{
						ym2612.OPN.SL3.fn_h = v&0x3f;
					}
					break;
			}
			break;

		case 0xb0:
			switch( OPN_SLOT(r) )
			{
				case 0:		/* 0xb0-0xb2 : FB,ALGO */
				{
					int feedback = (v>>3)&7;
					CH->ALGO = v&7;
					CH->FB   = feedback ? feedback+6 : 0;
					setup_connection( CH, c );
					break;
				}
				
				case 1:		/* 0xb4-0xb6 : L , R , AMS , PMS (ym2612/YM2610B/YM2610/YM2608) */
					/* b0-2 PMS */
					CH->pms = (v & 7) * 32; /* CH->pms = PM depth * 32 (index in lfo_pm_table) */

					/* b4-5 AMS */
					CH->ams = lfo_ams_depth_shift[(v>>4) & 0x03];

					/* PAN :  b7 = L, b6 = R */
					ym2612.OPN.pan[ c*2   ] = (v & 0x80) ? ~0 : 0;
					ym2612.OPN.pan[ c*2+1 ] = (v & 0x40) ? ~0 : 0;
					break;
			}
			break;
	}
}

/* Generate 32bits samples for ym2612 */
static long dac;
void YM2612UpdateOne(int **buffer, int length)
{
	int i;
	int  *bufL,*bufR;

	/* set buffer */
	bufL = buffer[0];
	bufR = buffer[1];

	/* refresh PG and EG */
	refresh_fc_eg_chan(&ym2612.CH[0]);
	refresh_fc_eg_chan(&ym2612.CH[1]);

	if (ym2612.OPN.ST.mode & 0x40) /* check only D6 ! */
	{
		/* 3SLOT MODE (operator order is 0,1,3,2) */
		if(ym2612.CH[2].SLOT[SLOT1].Incr==-1)
		{
			refresh_fc_eg_slot(&ym2612.CH[2].SLOT[SLOT1] , ym2612.OPN.SL3.fc[1] , ym2612.OPN.SL3.kcode[1] );
			refresh_fc_eg_slot(&ym2612.CH[2].SLOT[SLOT2] , ym2612.OPN.SL3.fc[2] , ym2612.OPN.SL3.kcode[2] );
			refresh_fc_eg_slot(&ym2612.CH[2].SLOT[SLOT3] , ym2612.OPN.SL3.fc[0] , ym2612.OPN.SL3.kcode[0] );
			refresh_fc_eg_slot(&ym2612.CH[2].SLOT[SLOT4] , ym2612.CH[2].fc , ym2612.CH[2].kcode );
		}
	}
	else refresh_fc_eg_chan(&ym2612.CH[2]);

	refresh_fc_eg_chan(&ym2612.CH[3]);
	refresh_fc_eg_chan(&ym2612.CH[4]);
	refresh_fc_eg_chan(&ym2612.CH[5]);

	/* buffering */
	for(i=0; i < length ; i++)
	{
		advance_lfo();

		/* clear outputs */
		out_fm[0] = 0;
		out_fm[1] = 0;
		out_fm[2] = 0;
		out_fm[3] = 0;
		out_fm[4] = 0;
		out_fm[5] = 0;

		/* advance envelope generator */
		ym2612.OPN.eg_timer += ym2612.OPN.eg_timer_add;
		while (ym2612.OPN.eg_timer >= ym2612.OPN.eg_timer_overflow)
		{
			ym2612.OPN.eg_timer -= ym2612.OPN.eg_timer_overflow;
			ym2612.OPN.eg_cnt++;

			advance_eg_channel(&ym2612.CH[0].SLOT[SLOT1]);
			advance_eg_channel(&ym2612.CH[1].SLOT[SLOT1]);
			advance_eg_channel(&ym2612.CH[2].SLOT[SLOT1]);
			advance_eg_channel(&ym2612.CH[3].SLOT[SLOT1]);
			advance_eg_channel(&ym2612.CH[4].SLOT[SLOT1]);
			advance_eg_channel(&ym2612.CH[5].SLOT[SLOT1]);
		}

		/* calculate FM */
		chan_calc(&ym2612.CH[0]);
		chan_calc(&ym2612.CH[1]);
		chan_calc(&ym2612.CH[2]);
		chan_calc(&ym2612.CH[3]);
		chan_calc(&ym2612.CH[4]);

		/* DAC Mode */
		if (ym2612.dacen)
		{
			/* High Pass Filter */
			dac = (ym2612.dacout << 15) - dac_highpass;
			dac_highpass += dac >> 9;
			dac >>= 15;
			*(ym2612.CH[5].connect4) += (int)dac;/**/
		}
		else chan_calc(&ym2612.CH[5]);

		{
			int lt,rt;

			lt  = ((out_fm[0]>>0) & ym2612.OPN.pan[0]);
			rt  = ((out_fm[0]>>0) & ym2612.OPN.pan[1]);
			lt += ((out_fm[1]>>0) & ym2612.OPN.pan[2]);
			rt += ((out_fm[1]>>0) & ym2612.OPN.pan[3]);
			lt += ((out_fm[2]>>0) & ym2612.OPN.pan[4]);
			rt += ((out_fm[2]>>0) & ym2612.OPN.pan[5]);
			lt += ((out_fm[3]>>0) & ym2612.OPN.pan[6]);
			rt += ((out_fm[3]>>0) & ym2612.OPN.pan[7]);
			lt += ((out_fm[4]>>0) & ym2612.OPN.pan[8]);
			rt += ((out_fm[4]>>0) & ym2612.OPN.pan[9]);
			lt += ((out_fm[5]>>0) & ym2612.OPN.pan[10]);
			rt += ((out_fm[5]>>0) & ym2612.OPN.pan[11]);

			/* buffering */
			bufL[i] = lt;
			bufR[i] = rt;
		}

		/* timer A control */
		INTERNAL_TIMER_A();
	}
	INTERNAL_TIMER_B(length);

}

/* initialize ym2612 emulator(s) */
int YM2612Init(int clock, int rate)
{
	memset(&ym2612,0,sizeof(YM2612));
	init_tables();
	ym2612.OPN.ST.clock = clock;
	ym2612.OPN.ST.rate = rate;
	YM2612ResetChip();
	return 0;
}

/* reset */
int YM2612ResetChip(void)
{
	int i;

	OPNSetPres(6*24);
	OPNWriteMode(0x27,0x30); /* mode 0 , timer reset */

	ym2612.OPN.eg_timer  = 0;
	ym2612.OPN.eg_cnt    = 0;
	ym2612.OPN.ST.status = 0;
	ym2612.OPN.ST.mode   = 0;
	ym2612.OPN.ST.TA     = 0;
	ym2612.OPN.ST.TAL    = 0;
	ym2612.OPN.ST.TAC    = 0;
	ym2612.OPN.ST.TB     = 0;
	ym2612.OPN.ST.TBL    = 0;
	ym2612.OPN.ST.TBC    = 0;


	reset_channels(&ym2612.CH[0] , 6 );
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(i      ,0xc0);
		OPNWriteReg(i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(i      ,0);
		OPNWriteReg(i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(i,0);

	/* DAC mode clear */
	ym2612.dacen  = 0;
	ym2612.dacout = 0;
	dac_highpass = 0;
	return 0;
}

/* ym2612 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2612Write(unsigned char a, unsigned char v)
{
	int addr;

	//v &= 0xff;	/* adjust to 8 bit bus */

	switch( a&3 )
	{
		case 0:	/* address port 0 */
			ym2612.OPN.ST.address[0] = v;
			break;

		case 1:	/* data port 0    */
			addr = ym2612.OPN.ST.address[0];
			fm_reg[0][addr] = v;
			switch( addr & 0xf0 )
			{
				case 0x20:	/* 0x20-0x2f Mode */
					switch( addr )
					{
						case 0x2a:	/* DAC data (ym2612) */
							ym2612.dacout = ((int)v - 0x80) << 6; /* level unknown (5 is too low, 8 is too loud) */
							break;
						case 0x2b:	/* DAC Sel  (ym2612) */
							/* b7 = dac enable */
							ym2612.dacen = v & 0x80;
							break;
						default:	/* OPN section */
							/* write register */
							OPNWriteMode(addr,v);
					}
					break;
				default:	/* 0x30-0xff OPN section */
					/* write register */
					OPNWriteReg(addr,v);
			}
			break;

		case 2:	/* address port 1 */
			ym2612.OPN.ST.address[1] = v;
			break;

		case 3:	/* data port 1    */
			addr = ym2612.OPN.ST.address[1];
			fm_reg[1][addr] = v;
			OPNWriteReg(addr | 0x100,v);
			break;
	}

	return 0;
}

int YM2612Read(void)
{
	return ym2612.OPN.ST.status;
}
