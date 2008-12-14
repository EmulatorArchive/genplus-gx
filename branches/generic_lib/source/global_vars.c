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
#include <gctypes.h>
#include <gcutil.h>
#include <ogc/card.h>
#include <eugc_api.h>
#include "types.h"
#include "global_defs.h"


/* GENESIS specific data structures */
GEN_CART_DATA_t genrom; /* Main rom data variable */

double psg_preamp = 1.5;
double fm_preamp  = 1.0;
int   boost = 1;
int   hq_fm = 1;
int FM_GENS  = 0;
int ssg_enabled = 0;
int autoload = 0;
int region_detect = 0;
int force_dtack = 0;
int dmatiming = 1;
int vdptiming = 1;
int old_overscan = 1;
int bios_enabled;
int SVP_cycles = 850; 

int CARDSLOT = CARD_SLOTB;
int use_SDCARD = 0;

unsigned char *gen_bmp; /*** Work bitmap ***/
int frameticker = 0;
int ConfigRequested = 0;
int padcal = 70;
int RenderedFrameCount = 0;
int FrameCount = 0;
int FramesPerSecond = 0;
/* Main gfx controller, used by eugc */
GX_INFO_CTRL_t gfx_ctrl;

uint8 mpads[6] = {0, 1, 2, 3, 4, 5 }; /*** Default Mapping ***/
uint8 sys_type[2] = {0,0};
uint8 old_sys_type[2] = {0,0};

unsigned char soundbuffer[16][3840] ATTRIBUTE_ALIGN(32);
int mixbuffer = 0;
int playbuffer = 0;
int IsPlaying = 0;

/*** Scaling
 ***/
int xshift = 0;
int yshift = 0;
int xscale = HASPECT;
int yscale = VASPECT;
int overscan = 1;
int aspect = 1;
int use_480i = 0;
int tv_mode = 0;
int gc_pal = 0;

GXTexObj texobj;
Mtx view;
int vwidth, vheight;
long long int stride;

int msBetweenFrames = 20;
tb_t now, prev;

/*** Square Matrix
     This structure controls the size of the image on the screen.
   Think of the output as a -80 x 80 by -60 x 60 graph.
***/
s16 square[] ATTRIBUTE_ALIGN (32) =
{
  /*
   * X,   Y,  Z
   * Values set are for roughly 4:3 aspect
   */
  -HASPECT,  VASPECT, 0,  // 0
   HASPECT,  VASPECT, 0,  // 1
   HASPECT, -VASPECT, 0,  // 2
  -HASPECT, -VASPECT, 0,  // 3
};

/* Extra TV Modes for PAL */
GXRModeObj TVPal286Ds = 
{
    VI_TVMODE_PAL_DS,       // viDisplayMode
    640,             // fbWidth
    286,             // efbHeight
    286,             // xfbHeight
    (VI_MAX_WIDTH_PAL - 720)/2,         // viXOrigin
    (VI_MAX_HEIGHT_PAL - 572)/2,        // viYOrigin
    720,             // viWidth
    572,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },

    // vertical filter[7], 1/64 units, 6 bits each
  {
     0,         // line n-1
     0,         // line n-1
    21,         // line n
    22,         // line n
    21,         // line n
     0,         // line n+1
     0          // line n+1
  }
};

GXRModeObj TVPal286Int = 
{
    VI_TVMODE_PAL_INT,      // viDisplayMode
    640,             // fbWidth
    286,             // efbHeight
    286,             // xfbHeight
    (VI_MAX_WIDTH_PAL - 720)/2,         // viXOrigin
    (VI_MAX_HEIGHT_PAL - 572)/2,        // viYOrigin
    720,             // viWidth
    572,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_TRUE,         // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },

    // vertical filter[7], 1/64 units, 6 bits each
  {
     0,         // line n-1
     0,         // line n-1
    21,         // line n
    22,         // line n
    21,         // line n
     0,         // line n+1
     0          // line n+1
  }
};

GXRModeObj TVPal574IntDfGenesis = 
{
    VI_TVMODE_PAL_INT,      // viDisplayMode
    640,             // fbWidth
    480,             // efbHeight
    574,             // xfbHeight
    (VI_MAX_WIDTH_PAL - 720)/2,         // viXOrigin
    (VI_MAX_HEIGHT_PAL - 574)/2,        // viYOrigin
    720,             // viWidth
    574,             // viHeight
    VI_XFBMODE_DF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },
    // vertical filter[7], 1/64 units, 6 bits each
  {
     8,         // line n-1
     8,         // line n-1
    10,         // line n
    12,         // line n
    10,         // line n
     8,         // line n+1
     8          // line n+1
  }
};

GXRModeObj TVNtsc240DsGenesis = 
{
    VI_TVMODE_NTSC_DS,      // viDisplayMode
    640,             // fbWidth
    240,             // efbHeight
    240,             // xfbHeight
    (VI_MAX_WIDTH_NTSC - 720)/2,        // viXOrigin
    (VI_MAX_HEIGHT_NTSC/2 - 480/2)/2,       // viYOrigin
    720,             // viWidth
    480,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

     // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },

     // vertical filter[7], 1/64 units, 6 bits each
  {
      0,         // line n-1
      0,         // line n-1
     21,         // line n
     22,         // line n
     21,         // line n
      0,         // line n+1
      0          // line n+1
  }
};

GXRModeObj TVNtsc240IntGenesis = 
{
    VI_TVMODE_NTSC_INT,     // viDisplayMode
    640,             // fbWidth
    240,             // efbHeight
    240,             // xfbHeight
    (VI_MAX_WIDTH_NTSC - 720)/2,        // viXOrigin
    (VI_MAX_HEIGHT_NTSC - 480)/2,       // viYOrigin
    720,             // viWidth
    480,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_TRUE,         // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },

    // vertical filter[7], 1/64 units, 6 bits each
  {
     0,         // line n-1
     0,         // line n-1
    21,         // line n
    22,         // line n
    21,         // line n
     0,         // line n+1
     0          // line n+1
  }
};

GXRModeObj TVNtsc480IntDfGenesis = 
{
    VI_TVMODE_NTSC_INT,     // viDisplayMode
    640,             // fbWidth
    480,             // efbHeight
    480,             // xfbHeight
    (VI_MAX_WIDTH_NTSC - 720)/2,        // viXOrigin
    (VI_MAX_HEIGHT_NTSC - 480)/2,       // viYOrigin
    720,             // viWidth
    480,             // viHeight
    VI_XFBMODE_DF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

    // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },

    // vertical filter[7], 1/64 units, 6 bits each
  {
     8,         // line n-1
     8,         // line n-1
    10,         // line n
    12,         // line n
    10,         // line n
     8,         // line n+1
     8          // line n+1
  }
};

/* TV Modes */
GXRModeObj *tvmodes[6] = {
   &TVNtsc240DsGenesis, &TVNtsc240IntGenesis, &TVNtsc480IntDfGenesis, /* GC NTSC */
   &TVPal286Ds,  &TVPal286Int, &TVPal574IntDfGenesis  /* GC PAL */
};

