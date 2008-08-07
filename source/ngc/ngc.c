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
 ***************************************************************************/
#include "shared.h"
#include "gcaram.h"
#include "dvd.h"
#include "font.h"
#include <malloc.h>

#define ROMOFFSET 0x80700000

/***************************************************************************
 * Nintendo Gamecube Hardware Specific Functions
 *
 * V I D E O
 ***************************************************************************/
/*** VI ***/
unsigned int *xfb[2];	/*** Double buffered ***/
int whichfb = 0;			/*** Switch ***/
GXRModeObj *vmode;		/*** General video mode ***/

/*** GX ***/
#define TEX_WIDTH 360
#define TEX_HEIGHT 576
#define DEFAULT_FIFO_SIZE 256 * 1024
#define HASPECT 320
#define VASPECT 240

static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);
static u8 texturemem[TEX_WIDTH * (TEX_HEIGHT + 8) * 2] ATTRIBUTE_ALIGN (32);
GXTexObj texobj;
static Mtx view;
int vwidth, vheight;
long long int stride;

/*** custom Video modes (used to emulate original console video modes) ***/
/* 288 lines progressive (PAL 50Hz) */
GXRModeObj TVPal_288p = 
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

/* 288 lines interlaced (PAL 50Hz) */
GXRModeObj TVPal_288i = 
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

/* 576 lines interlaced (PAL 50Hz, scaled) */
GXRModeObj TVPal_576i_Scale = 
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

/* 240 lines progressive (NTSC or PAL 60Hz) */
GXRModeObj TVNtsc_Rgb60_240p = 
{
    VI_TVMODE_EURGB60_DS,      // viDisplayMode
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

/* 240 lines interlaced (NTSC or PAL 60Hz) */
GXRModeObj TVNtsc_Rgb60_240i = 
{
    VI_TVMODE_EURGB60_INT,     // viDisplayMode
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

/* 480 lines interlaced (NTSC or PAL 60Hz) */
GXRModeObj TVNtsc_Rgb60_480i = 
{
    VI_TVMODE_EURGB60_INT,     // viDisplayMode
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


/* TV Modes table */
GXRModeObj *tvmodes[6] = {
	 &TVNtsc_Rgb60_240p, &TVNtsc_Rgb60_240i, &TVNtsc_Rgb60_480i, /* 60hz modes */
	 &TVPal_288p,  &TVPal_288i, &TVPal_576i_Scale                /* 50Hz modes */
};

typedef struct tagcamera
{
  Vector pos;
  Vector up;
  Vector view;
} camera;

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
	-HASPECT,  VASPECT, 0,	// 0
	 HASPECT,  VASPECT, 0,	// 1
	 HASPECT, -VASPECT, 0,	// 2
	-HASPECT, -VASPECT, 0,	// 3
};

static camera cam = { {0.0F, 0.0F, -100.0F},
{0.0F, -1.0F, 0.0F},
{0.0F, 0.0F, 0.0F}
};

/* init rendering */
/* should be called each time you change quad aspect ratio */
void draw_init (void)
{
  /* Clear all Vertex params */
  GX_ClearVtxDesc ();

  /* Set Position Params (set quad aspect ratio) */
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
  GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
  GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

  /* Set Tex Coord Params */
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
  GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);
  GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
  GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
  GX_SetNumTexGens (1);
  GX_SetNumChans(0);

  /** Set Modelview **/
  memset (&view, 0, sizeof (Mtx));
  guLookAt(view, &cam.pos, &cam.up, &cam.view);
  GX_LoadPosMtxImm (view, GX_PNMTX0);
}

/* vertex rendering */
static void draw_vert (u8 pos, f32 s, f32 t)
{
  GX_Position1x8 (pos);
  GX_TexCoord2f32 (s, t);
}

/* textured quad rendering */
static void draw_square ()
{
  GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
  draw_vert (3, 0.0, 0.0);
  draw_vert (2, 1.0, 0.0);
  draw_vert (1, 1.0, 1.0);
  draw_vert (0, 0.0, 1.0);
  GX_End ();
}

/* initialize GX rendering */
static void StartGX (void)
{
  Mtx p;
  GXColor gxbackground = { 0, 0, 0, 0xff };

  /*** Clear out FIFO area ***/
  memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);

  /*** Initialise GX ***/
  GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);
  GX_SetCopyClear (gxbackground, 0x00ffffff);
  GX_SetViewport (0.0F, 0.0F, vmode->fbWidth, vmode->efbHeight, 0.0F, 1.0F);
  GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);
  f32 yScale = GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight);
  u16 xfbHeight = GX_SetDispCopyYScale (yScale);
  GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
  GX_SetDispCopyDst (vmode->fbWidth, xfbHeight);
  GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
  GX_SetFieldMode (vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
  GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
  GX_SetCullMode (GX_CULL_NONE);
  GX_SetDispCopyGamma (GX_GM_1_0);
  GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
  GX_SetColorUpdate(GX_TRUE);
  guOrtho(p, vmode->efbHeight/2, -(vmode->efbHeight/2), -(vmode->fbWidth/2), vmode->fbWidth/2, 100, 1000);
  GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

  /*** Copy EFB -> XFB ***/
  GX_CopyDisp (xfb[whichfb ^ 1], GX_TRUE);
  GX_Flush ();

  /*** Initialize texture data ***/
  memset (texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);
}

/* PreRetrace handler */
static int frameticker = 0;
static void framestart()
{
  /* simply increment the tick counter */
  frameticker++;
}

/* Rendering options */
s16 xshift = 0;
s16 yshift = 0;
s16 xscale = HASPECT;
s16 yscale = VASPECT;
uint8 overscan = 1;
uint8 aspect = 1;
uint8 use_480i = 0;
uint8 tv_mode = 0;
uint8 gc_pal = 0;

/* init GX scaler */
void viewport_init()
{
	if (aspect)
	{
		/* original aspect */
		if (overscan)
		{
			/* borders are emulated */
			xscale = 320;
			yscale = vdp_pal ? ((gc_pal && !use_480i) ? 144 : 121) : ((gc_pal && !use_480i) ? 143 : 120);
			xshift = 8;
			yshift = vdp_pal ? (gc_pal ? 1 : 0) : 2;
		}
		else
		{
			/* borders are simulated (black) */
			xscale = 290;
			yscale = bitmap.viewport.h / 2;
			if (vdp_pal && (!gc_pal || use_480i)) yscale = yscale * 243 / 288;
			else if (!vdp_pal && gc_pal && !use_480i) yscale = yscale * 288 / 243;
			xshift = 8;
			yshift = vdp_pal ? (gc_pal ? 1 : 0) : 2;
		}
	}
	else
	{
		/* fit screen */
		xscale = 320;
		yscale = (gc_pal && !use_480i) ? 134 : 112;
		xshift = 0;
		yshift = gc_pal ? 1 : 2;
	}

	/* double resolution */
	if (use_480i)
	{
		 yscale *= 2;
		 yshift *= 2;
	}

	square[6] = square[3]  =  xscale + xshift;
	square[0] = square[9]  = -xscale + xshift;
	square[4] = square[1]  =  yscale + yshift;
	square[7] = square[10] = -yscale + yshift;

	draw_init();
}

/* Update Display (called after each emulation frame) */
static void update_video ()
{
  int h, w;
  
  /* texture and bitmap buffers (buffers width is fixed to 360 pixels) */
  long long int *dst = (long long int *)texturemem;
  long long int *src1 = (long long int *)(bitmap.data); /* line n */
  long long int *src2 = src1 + 90;  /* line n+1 */
  long long int *src3 = src2 + 90;  /* line n+2 */
  long long int *src4 = src3 + 90;  /* line n+3 */

  /* check if viewport has changed */
  if (bitmap.viewport.changed)
  {
    /* Check interlaced mode changes */
    if ((bitmap.viewport.changed & 2) && (!use_480i))
    {
      GXRModeObj *rmode;
	  rmode = tvmodes[gc_pal*3 + interlaced];
      rmode->viWidth = aspect ? 720 : 640;
      rmode->viXOrigin = (720 - rmode->viWidth) / 2;
      VIDEO_Configure (rmode);
      GX_SetFieldMode (rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
      VIDEO_WaitVSync();
      if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
      else while (VIDEO_GetNextField()) VIDEO_WaitVSync();
    }
    
    bitmap.viewport.changed = 0;
    
    /* update texture size */
    vwidth  = bitmap.viewport.w + 2 * bitmap.viewport.x;
    vheight = bitmap.viewport.h + 2 * bitmap.viewport.y;
    if (interlaced && use_480i) vheight *= 2;
    stride = bitmap.width - (vwidth >> 2);
    
    /* simulated (black) border area depends on viewport.h */
    if (aspect && !overscan) viewport_init();
    
    /* reinitialize texture */
    GX_InvalidateTexAll ();
	GX_InitTexObj (&texobj, texturemem, vwidth, vheight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
  }

  GX_InvVtxCache ();
  GX_InvalidateTexAll ();
 
  /* update texture data */
  for (h = 0; h < vheight; h += 4)
  {
    for (w = 0; w < (vwidth >> 2); w++)
    {
      *dst++ = *src1++;
      *dst++ = *src2++;
      *dst++ = *src3++;
      *dst++ = *src4++;
    }
    
    /* jumpt to next four lines */
    src1 += stride;
    src2 += stride;
    src3 += stride;
    src4 += stride;
  }

  /* load texture into GX */
  DCFlushRange (texturemem, vwidth * vheight * 2);
  GX_LoadTexObj (&texobj, GX_TEXMAP0);
  
  /* render textured quad */
  draw_square ();
  GX_DrawDone ();

  /* switch external framebuffers then copy EFB to XFB */
  whichfb ^= 1;
  GX_CopyDisp (xfb[whichfb], GX_TRUE);
  GX_Flush ();

  /* set next XFB */
  VIDEO_SetNextFramebuffer (xfb[whichfb]);
  VIDEO_Flush ();
}

/***************************************************************************
 *  Video initialization
 *  this function MUST be called at startup
 ***************************************************************************/
void scanpads (u32 count)
{
	PAD_ScanPads();
}


static void InitGCVideo ()
{
  extern GXRModeObj TVEurgb60Hz480IntDf;
  int *romptr = (int *)ROMOFFSET;
    
  /*
   * Before doing anything else under libogc,
   * Call VIDEO_Init
   */
  VIDEO_Init ();

  /*
   * Before any memory is allocated etc.
   * Rescue any tagged ROM in data 2
   */
  StartARAM();
  genromsize = 0;
  if ( memcmp((char *)romptr,"GENPLUSR",8) == 0 )
  {
    genromsize = romptr[2];
    ARAMPut ((char *) ROMOFFSET + 0x20, (char *) 0x8000, genromsize);
  }

  /* Get the current video mode then :
      - set menu video mode (fullscreen, 480i or 576i)
      - set emulator rendering TV modes (PAL/MPAL/NTSC/EURGB60)
  */
  switch (VIDEO_GetCurrentTvMode())
  {
    case VI_PAL: /* only 480 lines */
      vmode = &TVPal574IntDfScale;
	  vmode->xfbHeight = 480;
	  vmode->viYOrigin = (VI_MAX_HEIGHT_PAL - 480)/2;
	  vmode->viHeight = 480;
      TVNtsc_Rgb60_240p.viTVMode = VI_TVMODE_EURGB60_DS;
      TVNtsc_Rgb60_240i.viTVMode = VI_TVMODE_EURGB60_INT;
      TVNtsc_Rgb60_480i.viTVMode = VI_TVMODE_EURGB60_INT;
	  gc_pal = 1;
	  tv_mode = 1;
      break;

#ifdef FORCE_EURGB60
   default:
      vmode = &TVEurgb60Hz480IntDf;
      TVNtsc_Rgb60_240p.viTVMode = VI_TVMODE_EURGB60_DS;
      TVNtsc_Rgb60_240i.viTVMode = VI_TVMODE_EURGB60_INT;
      TVNtsc_Rgb60_480i.viTVMode = VI_TVMODE_EURGB60_INT;
	  gc_pal = 0;
	  tv_mode = 2;
      break;

#else
	case VI_MPAL:
      vmode = &TVMpal480IntDf;
      TVNtsc_Rgb60_240p.viTVMode = VI_TVMODE_MPAL_DS;
      TVNtsc_Rgb60_240i.viTVMode = VI_TVMODE_MPAL_INT;
      TVNtsc_Rgb60_480i.viTVMode = VI_TVMODE_MPAL_INT;
	  gc_pal = 0;
	  tv_mode = 2;
      break;

	default:
      vmode = &TVNtsc480IntDf;
      TVNtsc_Rgb60_240p.viTVMode = VI_TVMODE_NTSC_DS;
      TVNtsc_Rgb60_240i.viTVMode = VI_TVMODE_NTSC_INT;
      TVNtsc_Rgb60_480i.viTVMode = VI_TVMODE_NTSC_INT;
	  gc_pal = 0;
	  tv_mode = 0;
      break;
#endif
  }
  
  /* Set default video mode */
  VIDEO_Configure (vmode);

  /* Configure the framebuffers (double-buffering) */
  xfb[0] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(&TVPal_576i_Scale));
  xfb[1] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(&TVPal_576i_Scale));

  /* Define a console */
  console_init(xfb[0], 20, 64, 640, 574, 574 * 2);

  /* Clear framebuffers to black */
  VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);

  /* Set the framebuffer to be displayed at next VBlank */
  VIDEO_SetNextFramebuffer(xfb[0]);

  /* Register Video Retrace handlers */
  VIDEO_SetPreRetraceCallback(framestart);
  VIDEO_SetPostRetraceCallback(scanpads);
  
  /* Enable Video Interface */
  VIDEO_SetBlack (FALSE);
  
  /* Update video settings for next VBlank */
  VIDEO_Flush();

  /* Wait for VBlank */
  VIDEO_WaitVSync();
  VIDEO_WaitVSync();
  
  /* Initialize everything else */
  PAD_Init ();
#ifndef HW_RVL
  DVD_Init ();
  dvd_drive_detect();
#endif
  SDCARD_Init ();
  unpackBackdrop ();
  init_font();
  StartGX ();
}

/***************************************************************************
 * Nintendo Gamecube Hardware Specific Functions
 *
 * A U D I O
 ***************************************************************************/
unsigned char soundbuffer[16][3840] ATTRIBUTE_ALIGN(32);
int mixbuffer = 0;
int playbuffer = 0;
int IsPlaying = 0;
int ConfigRequested = 0;

/*** AudioSwitchBuffers
     Genesis Plus only provides sound data on completion of each frame.
     To try to make the audio less choppy, this function is called from both the
     DMA completion and update_audio.
     Testing for data in the buffer ensures that there are no clashes.
 ***/
static void AudioSwitchBuffers()
{
  u32 dma_len = (vdp_pal) ? 3840 : 3200;
  
  if (!ConfigRequested)
  {
    /* restart audio DMA with current soundbuffer */
    AUDIO_InitDMA((u32) soundbuffer[playbuffer], dma_len);
    DCFlushRange(soundbuffer[playbuffer], dma_len);
    AUDIO_StartDMA();
    
    /* increment soundbuffers index */
    playbuffer++;
    playbuffer &= 0xf;
    if (playbuffer == mixbuffer)
    {
      playbuffer--;
      if (playbuffer < 0) playbuffer = 15;
    }
    IsPlaying = 1;
  }
  else IsPlaying = 0;
}

/*** InitGCAudio
     Stock code to set the DSP at 48Khz
 ***/
static void InitGCAudio ()
{
  AUDIO_Init (NULL);
  AUDIO_SetDSPSampleRate (AI_SAMPLERATE_48KHZ);
  AUDIO_RegisterDMACallback (AudioSwitchBuffers);
  memset(soundbuffer, 0, 16 * 3840);
}


/***************************************************************************
 * Nintendo Gamecube Hardware Specific Functions
 *
 * I N P U T
 ***************************************************************************/
/**
 * IMPORTANT
 * If you change the padmap here, be sure to update confjoy to
 * reflect the changes - or confusion will ensue!
 *
 * DEFAULT MAPPING IS:
 *		Genesis			Gamecube
 *		  A			   B
 *		  B		           A
 *		  C			   X
 *		  X			   LT
 *		  Y			   Y
 *		  Z			   RT
 *
 * Mode is unused, as it's our menu hotkey for now :)
 * Also note that libOGC has LT/RT reversed - it's not a typo!
 */
unsigned int gppadmap[] = {
  INPUT_A, INPUT_B, INPUT_C,
  INPUT_X, INPUT_Y, INPUT_Z,
  INPUT_UP, INPUT_DOWN,
  INPUT_LEFT, INPUT_RIGHT,
  INPUT_START, INPUT_MODE
};

unsigned short gcpadmap[] = {
  PAD_BUTTON_B, PAD_BUTTON_A, PAD_BUTTON_X,
  PAD_TRIGGER_L, PAD_BUTTON_Y, PAD_TRIGGER_R,
  PAD_BUTTON_UP, PAD_BUTTON_DOWN,
  PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT,
  PAD_BUTTON_START, PAD_TRIGGER_Z
};

/* global variables */
int padcal = 30;

/* get GC Pad status */
static inline unsigned int DecodeJoy (int joynum)
{
  int i;
  unsigned int ret = 0;
  unsigned short p = PAD_ButtonsHeld (joynum);
  signed char x = PAD_StickX (joynum);
  signed char y = PAD_StickY (joynum);
  
  for (i = 0; i < 12; i++)
    if (p & gcpadmap[i]) ret |= gppadmap[i];

  /* analog stick */
  if (x > padcal)  ret |= INPUT_RIGHT;
  if (x < -padcal) ret |= INPUT_LEFT;
  if (y > padcal)  ret |= INPUT_UP;
  if (y < -padcal) ret |= INPUT_DOWN;
    
  return ret;
}

/* update Genesis Inputs status (called before each emulation frame) */
void update_inputs()
{
  int i = 0;
  int joynum = 0;
  
  /* check for specific combo */
  if (PAD_ButtonsHeld (0) & PAD_TRIGGER_Z)
  {
    if (PAD_ButtonsHeld (0) & PAD_TRIGGER_L)
    {
      /* SOFT-RESET key */
      resetline = (int) ((double) (lines_per_frame - 1) * rand() / (RAND_MAX + 1.0));
    }
    else
    {
      /* MENU key */
      ConfigRequested = 1;
    }
    
    return;
  }

  /* get PAD status */
  for (i=0; i<MAX_DEVICES; i++)
  {
    input.pad[i] = 0;
    if (input.dev[i] != NO_DEVICE)
    {
      input.pad[i] = DecodeJoy(joynum);
      joynum ++;
      if (input.dev[i] == DEVICE_LIGHTGUN) lightgun_set();
    }
  }
}

/***************************************************************************
 * init Genesis Plus VM
 *
 ***************************************************************************/
static void init_machine ()
{
  /* Allocate cart_rom here */
  cart_rom = memalign(32, 0x500000);
  
  /* Look for BIOS rom */
  bios_enabled = 0;
  sd_file *bios_file = SDCARD_OpenFile ("dev0:\\genplus\\gen_bios.bin", "rb");
  if (bios_file != NULL)
  {
    SDCARD_ReadFile(bios_file, &bios_rom[0], 2048);
    SDCARD_CloseFile (bios_file);
    bios_enabled = 2;
  }
  
  /* allocate global work bitmap */
  memset (&bitmap, 0, sizeof (bitmap));
  bitmap.width  = 360;
  bitmap.height = 576;
  bitmap.depth  = 16;
  bitmap.granularity = 2;
  bitmap.pitch = bitmap.width * bitmap.granularity;
  bitmap.viewport.w = 256;
  bitmap.viewport.h = 224;
  bitmap.viewport.x = 0;
  bitmap.viewport.y = 0;
  bitmap.remap = 1;
  bitmap.data = malloc (bitmap.width * bitmap.height * bitmap.granularity);
  
  /* default inputs */
  input.system[0] = SYSTEM_GAMEPAD;
  input.system[1] = SYSTEM_GAMEPAD;
}

/**************************************************
  Load a new rom and performs some initialization
***************************************************/
extern void decode_ggcodes ();
extern void ClearGGCodes ();
extern void sram_autoload();
extern uint8 autoload;

void reloadrom ()
{
  load_rom("");      /* Load ROM */
  system_init ();    /* Initialize System */
  audio_init(48000); /* Audio System initialization */
  ClearGGCodes ();   /* Clear Game Genie patches */
  decode_ggcodes (); /* Apply Game Genie patches */
  system_reset ();   /* System Power ON */
  if (autoload) sram_autoload();

  /* PAL TV modes detection */
  if ((tv_mode == 1) || ((tv_mode == 2) && vdp_pal)) gc_pal = 1;
  else gc_pal = 0;
  
  /* reinit GX Scaler */
  viewport_init();
}

/***************************************************************************
 *  M A I N
 *
 ***************************************************************************/
extern u32 diff_usec(long long start,long long end);
extern long long gettime();

extern void legal ();
extern void MainMenu ();
extern void reloadrom ();
int FramesPerSecond = 0;

int main (int argc, char *argv[])
{
  u16 usBetweenFrames;
  long long now, prev;
  int RenderedFrameCount = 0;
  int FrameCount = 0;
  
  /* Initialize GC System */
  InitGCVideo ();
  InitGCAudio ();
  legal();
  
  /* Initialize Virtual Genesis */
  init_machine ();
  
  /* Show Menu */
  if (genromsize)
  {
    /* load injected rom */
    ARAMFetch((char *)cart_rom, (void *)0x8000, genromsize);
    reloadrom ();
    MainMenu();
  }
  else while (!genromsize) MainMenu();
  
  /* Initialize Frame timings */
  frameticker = 0;
  prev = gettime();
  
  /* Emulation Loop */
  while (1)
  {
    /* Update Inputs */
    update_inputs();
    
    /* Frame synchronization */
    if (gc_pal != vdp_pal)
    {
      /* use timers */
      usBetweenFrames = vdp_pal ? 20000 : 16667;
      now = gettime();
      if (diff_usec(prev, now) > usBetweenFrames)
      {
        /* Frame skipping */
        prev = now;
        system_frame(1);
      }
      else
      {
        /* Delay */
        while (diff_usec(prev, now) < usBetweenFrames) now = gettime();
        
        /* Render Frame */
        prev = now;
        system_frame(0);
        RenderedFrameCount++;
        update_video ();
      }
    }    
    else
    {
      /* use VSync */
      if (frameticker > 1)
      {
        /* Frame skipping */
        frameticker--;
        system_frame (1);
      }
      else
      {
        /* Delay */
        while (!frameticker) usleep(10);
        
        system_frame (0);
        RenderedFrameCount++;
        update_video ();
      }
      
      frameticker--;
    }
    
    /* Restart Audio Loop if needed */
    if (!IsPlaying) AudioSwitchBuffers();
    
    /* Check rendered frames (FPS) */
    FrameCount++;
    if (FrameCount == vdp_rate)
    {
      FramesPerSecond = RenderedFrameCount;
      RenderedFrameCount = 0;
      FrameCount = 0;
    }
    
    /* Check for Menu request */
    if (ConfigRequested)
    {
      /* stop AUDIO */
      AUDIO_StopDMA ();
      IsPlaying = 0;
      mixbuffer = 0;
      playbuffer = 0;
      
      /* go to menu */
      MainMenu ();
      ConfigRequested = 0;
      
      /* reset frame timings */
      frameticker = 0;
      prev = gettime();
      FrameCount = 0;
      RenderedFrameCount = 0;
    }
  }
  return 0;
}
