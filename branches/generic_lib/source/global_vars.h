/****************************************************************************
 *  Genesis Plus 1.2a
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald
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
 * Nintendo Gamecube Menus
 *
 * Please put any user menus here! - softdev March 12 2006
 ***************************************************************************/

#ifndef __GLOBAL_VARS_H
#define __GLOBAL_VARS_H
 
#include <gctypes.h>
#include <eugc_typedef.h>
#include "global_defs.h"
#include "ogc/gx.h"

/* GENESIS specific data structures */
extern GEN_CART_DATA_t genrom;

/* Global variables */
extern uint8 *vctab;
extern uint8 *hctab;
extern uint8 vc_ntsc_224[262];
extern uint8 vc_pal_224[313];
extern uint8 vc_pal_240[313];
extern uint8 cycle2hc32[488];
extern uint8 cycle2hc40[488];

extern double psg_preamp;
extern double fm_preamp;
extern int boost;
extern int hq_fm;
extern int FM_GENS;
extern int ssg_enabled;
extern int autoload;
extern int region_detect;
extern int force_dtack;
extern int dmatiming;
extern int vdptiming;
extern int old_overscan;

extern int CARDSLOT;
extern int use_SDCARD;
extern int UseSDCARD;
extern int peripherals;

extern int bios_enabled;
extern int SVP_cycles; 

extern unsigned char *gen_bmp; /*** Work bitmap ***/
extern int frameticker;
extern int ConfigRequested;
extern int padcal;
extern int RenderedFrameCount;
extern int FrameCount;
extern int FramesPerSecond;
/* Main gfx controller, used by eugc */
extern GX_INFO_CTRL_t gfx_ctrl;

/* Pad */
extern uint8 mpads[6];
extern uint8 sys_type[2];
extern uint8 old_sys_type[2];

/* Audio stuff */
extern unsigned char soundbuffer[16][3840];
extern int mixbuffer;
extern int playbuffer;
extern int IsPlaying;
extern struct ym2612__ YM2612;

/*** Scaling
 ***/
extern int xshift;
extern int yshift;
extern int xscale;
extern int yscale;
extern int overscan;
extern int aspect;
extern int use_480i;
extern int tv_mode;
extern int gc_pal;

extern GXTexObj texobj;
extern Mtx view;
extern int vwidth, vheight;
extern long long int stride;

extern int msBetweenFrames;
extern tb_t now, prev;

extern s16 square[];

/* Extra TV Modes for PAL */
extern GXRModeObj TVPal286Ds;
extern GXRModeObj TVPal286Int;
extern GXRModeObj TVPal574IntDfGenesis;
extern GXRModeObj TVNtsc240DsGenesis;
extern GXRModeObj TVNtsc240IntGenesis;
extern GXRModeObj TVNtsc480IntDfGenesis;
extern GXRModeObj *tvmodes[6];

extern int peripherals;

#endif
