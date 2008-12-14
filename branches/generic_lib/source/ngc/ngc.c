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
#include "gpback.h"
#include <eugc_api.h>

/* TODO remove local extern declaration */
extern void build_menu_structure(void);
void update_inputs(u32 count);

/***************************************************************************
 * Nintendo Gamecube Hardware Specific Functions
 *
 * T I M E R
 ***************************************************************************/
#define TB_CLOCK  40500000
#define mftb(rval) ({unsigned long u; do { \
         asm volatile ("mftbu %0" : "=r" (u)); \
         asm volatile ("mftb %0" : "=r" ((rval)->l)); \
         asm volatile ("mftbu %0" : "=r" ((rval)->u)); \
         } while(u != ((rval)->u)); })


unsigned long tb_diff_msec(tb_t *end, tb_t *start)
{
        unsigned long upper, lower;
        upper = end->u - start->u;
        if (start->l > end->l) upper--;
        lower = end->l - start->l;
        return ((upper*((unsigned long)0x80000000/(TB_CLOCK/2000))) + (lower/(TB_CLOCK/1000)));
}



/***************************************************************************
 * Nintendo Gamecube Hardware Specific Functions
 *
 * V I D E O
 ***************************************************************************/

static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);
static u8 texturemem[TEX_WIDTH * (TEX_HEIGHT + 8) * 2] ATTRIBUTE_ALIGN (32);

static camera cam = { {0.0F, 0.0F, -100.0F},
{0.0F, -1.0F, 0.0F},
{0.0F, 0.0F, 0.0F}
};

/*** Framestart function
	 Simply increment the tick counter
 ***/
static void framestart()
{
	frameticker++;
	FrameCount++;
}

/*** WIP3 - Scaler Support Functions
 ***/
void draw_init (void)
{
  GX_ClearVtxDesc ();

  /* Set Position Params */
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

static void draw_vert (u8 pos, f32 s, f32 t)
{
  GX_Position1x8 (pos);
  GX_TexCoord2f32 (s, t);
}

static void draw_square ()
{
  GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
  draw_vert (3, 0.0, 0.0);
  draw_vert (2, 1.0, 0.0);
  draw_vert (1, 1.0, 1.0);
  draw_vert (0, 0.0, 1.0);
  GX_End ();
}


/*** StartGX
	 This function initialises the GX.
     WIP3 - Based on texturetest from libOGC examples.
 ***/
static void StartGX (void)
{
  Mtx p;
  GXColor gxbackground = { 0, 0, 0, 0xff };

  /*** Clear out FIFO area ***/
  memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);

  /*** Initialise GX ***/
  GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);
  GX_SetCopyClear (gxbackground, 0x00ffffff);
  GX_SetViewport (0.0F, 0.0F, gfx_ctrl.vmode->fbWidth, gfx_ctrl.vmode->efbHeight, 0.0F, 1.0F);
  GX_SetScissor (0, 0, gfx_ctrl.vmode->fbWidth, gfx_ctrl.vmode->efbHeight);
  f32 yScale = GX_GetYScaleFactor(gfx_ctrl.vmode->efbHeight, gfx_ctrl.vmode->xfbHeight);
  u16 xfbHeight = GX_SetDispCopyYScale (yScale);
  GX_SetDispCopySrc (0, 0, gfx_ctrl.vmode->fbWidth, gfx_ctrl.vmode->efbHeight);
  GX_SetDispCopyDst (gfx_ctrl.vmode->fbWidth, xfbHeight);
  GX_SetCopyFilter (gfx_ctrl.vmode->aa, gfx_ctrl.vmode->sample_pattern, GX_TRUE, gfx_ctrl.vmode->vfilter);
  GX_SetFieldMode (gfx_ctrl.vmode->field_rendering, ((gfx_ctrl.vmode->viHeight == 2 * gfx_ctrl.vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
  GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
  GX_SetCullMode (GX_CULL_NONE);
  GX_SetDispCopyGamma (GX_GM_1_0);
  GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
  GX_SetColorUpdate (GX_TRUE);
  guOrtho(p, gfx_ctrl.vmode->efbHeight/2, -(gfx_ctrl.vmode->efbHeight/2), -(gfx_ctrl.vmode->fbWidth/2), gfx_ctrl.vmode->fbWidth/2, 100, 1000);
  GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

  GX_CopyDisp (gfx_ctrl.xfb[gfx_ctrl.whichfb ^ 1], GX_TRUE);

  /*** initialize texture data ***/
  memset (texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);
}

/*** InitGCVideo
	 This function MUST be called at startup.
 ***/
static void InitGCVideo ()
{
  int *romptr = (int *)ROMOFFSET;

  /* Init some key parameyers before doing anything else */
  EUGC_SetBackFrameWidth(&gfx_ctrl, 640 );

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
  if ( memcmp((char *)romptr,"GENPLUSR",8) == 0 )
  {
	  genrom.genromsize = romptr[2];
	  ARAMPut ((char *) 0x80640020, (char *) 0x8000, genrom.genromsize);
  }
  else genrom.genromsize = 0;

  /* Init Gamepads */
  PAD_Init ();

  /*
   * Reset the video mode
   * This is always set to 60hz
   * Whether your running PAL or NTSC
   */
  EUGC_SetVMode( &gfx_ctrl, &TVNtsc480IntDf );
  VIDEO_Configure (gfx_ctrl.vmode);

  /*** Now configure the framebuffer. 
	   Really a framebuffer is just a chunk of memory
	   to hold the display line by line.
   **/
  /*** I prefer also to have a second buffer for double-buffering.
     This is not needed for the console demo.
   ***/
  u32 *ptr = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(&TVPal574IntDfGenesis));
  EUGC_SetXFBAddress( &gfx_ctrl, 0, ptr);
  ptr = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(&TVPal574IntDfGenesis));
  EUGC_SetXFBAddress( &gfx_ctrl, 1, ptr);

  /*** Define a console ***/
  console_init(gfx_ctrl.xfb[0], 20, 64, 640, 574, 574 * 2);

  /*** Clear framebuffer to black ***/
  VIDEO_ClearFrameBuffer(gfx_ctrl.vmode, gfx_ctrl.xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer(gfx_ctrl.vmode, gfx_ctrl.xfb[1], COLOR_BLACK);

  /*** Set the framebuffer to be displayed at next VBlank ***/
  VIDEO_SetNextFramebuffer(gfx_ctrl.xfb[0]);

  /*** Increment frameticker and timer ***/
  VIDEO_SetPreRetraceCallback(framestart);

  /*** Get the PAD status updated by libogc ***/
  VIDEO_SetPostRetraceCallback(update_inputs);
  VIDEO_SetBlack (FALSE);
  
  /*** Update the video for next vblank ***/
  VIDEO_Flush();

  /*** Wait for VBL ***/
  VIDEO_WaitVSync();
  if (gfx_ctrl.vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

  /*** Initialize everything ***/
  DVD_Init ();
  SDCARD_Init ();
  EUGC_UnpackBackdrop (gpback, gpback_RAW, gpback_COMPRESSED);
  EUGC_InitFont(&gfx_ctrl);
  StartGX ();

  /* Wii drive detection for 4.7Gb support */
  int driveid = dvd_inquiry();
  if ((driveid == 4) || (driveid == 6) || (driveid == 8)) EUGC_SetWiiFlag(&gfx_ctrl, 0);
  else EUGC_SetWiiFlag(&gfx_ctrl, 1);
}

void viewport_init()
{
	/* initialize aspect ratio */
	if (aspect)
	{
		/* original aspect */
		if (overscan)
		{
			/* borders are emulated */
			xscale = 320;
			yscale = vdp_pal ? ((gc_pal && !use_480i) ? 144 : 121) : 120;
			xshift = 8;
			yshift = vdp_pal ? (gc_pal ? 1 : 0) : 2;
		}
		else
		{
			/* borders are simulated (black) */
			xscale = 290;
			yscale = bitmap.viewport.h / 2;
			if (vdp_pal && (!gc_pal || use_480i)) yscale = yscale * 243 / 288;
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

/*** Video Update
     called after each emulation frame
 ***/

static void update_video ()
{
  int h, w;

  if (bitmap.viewport.changed)
  {
	  /* Check interlaced mode changes */
	  if ((bitmap.viewport.changed & 2) && (!use_480i))
	  {
		  gfx_ctrl.vmode = tvmodes[gc_pal*3 + interlaced];
		  VIDEO_Configure (gfx_ctrl.vmode);
   	      GX_SetFieldMode (gfx_ctrl.vmode->field_rendering, ((gfx_ctrl.vmode->viHeight == 2 * gfx_ctrl.vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
		  VIDEO_WaitVSync();
    	  if (gfx_ctrl.vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
		  else while (VIDEO_GetNextField()) VIDEO_WaitVSync();
	  }

	  bitmap.viewport.changed = 0;

	  /* Update Texture size */
	  vwidth  = bitmap.viewport.w + 2 * bitmap.viewport.x;
	  vheight = bitmap.viewport.h + 2 * bitmap.viewport.y;
	  if (interlaced && use_480i) vheight *= 2;
  	  stride = bitmap.width - (vwidth >> 2);

	  /* simulated borders */
	  if (aspect && !overscan) viewport_init();

	  /* initialize texture size */
	  GX_InvalidateTexAll ();
	  GX_InitTexObj (&texobj, texturemem, vwidth, vheight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, use_480i ? GX_TRUE : GX_FALSE);
  }

  /* fill texture data */
  long long int *dst = (long long int *)texturemem;
  long long int *src1 = (long long int *)(bitmap.data);
  long long int *src2 = src1 + 90;
  long long int *src3 = src2 + 90;
  long long int *src4 = src3 + 90;
 
  gfx_ctrl.whichfb ^= 1;
  
  GX_InvVtxCache ();
  GX_InvalidateTexAll ();
  
  for (h = 0; h < vheight; h += 4)
  {
    for (w = 0; w < (vwidth >> 2); w++ )
	{
		*dst++ = *src1++;
		*dst++ = *src2++;
		*dst++ = *src3++;
		*dst++ = *src4++;
	}

    src1 += stride;
	src2 += stride;
	src3 += stride;
	src4 += stride;
  }

  /* textured quad rendering */
  DCFlushRange (texturemem, vwidth * vheight * 2);
  GX_LoadTexObj (&texobj, GX_TEXMAP0);
  draw_square ();
  GX_DrawDone ();

  /* copy EFB to XFB */
  GX_CopyDisp (gfx_ctrl.xfb[gfx_ctrl.whichfb], GX_TRUE);
  GX_Flush ();

  /* set XFB */
  VIDEO_SetNextFramebuffer (gfx_ctrl.xfb[gfx_ctrl.whichfb]);
  VIDEO_Flush ();
}

/***************************************************************************
 * Nintendo Gamecube Hardware Specific Functions
 *
 * A U D I O
 ***************************************************************************/

/*** AudioSwitchBuffers
     Genesis Plus only provides sound data on completion of each frame.
     To try to make the audio less choppy, this function is called from both the
     DMA completion and update_audio.
     Testing for data in the buffer ensures that there are no clashes.
 ***/
static void AudioSwitchBuffers()
{
	u32 dma_len = (vdp_pal) ? 3840 : 3200;

	if ( !ConfigRequested )
	{
		AUDIO_InitDMA((u32) soundbuffer[playbuffer], dma_len);
		DCFlushRange(soundbuffer[playbuffer], dma_len);
		AUDIO_StartDMA();
		playbuffer++;
		playbuffer &= 0xf;
		if ( playbuffer == mixbuffer ) playbuffer--;
		if ( playbuffer < 0 ) playbuffer = 15;
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
unsigned int gppadmap[] = { INPUT_A, INPUT_B, INPUT_C,
	INPUT_X, INPUT_Y, INPUT_Z,
	INPUT_UP, INPUT_DOWN,
	INPUT_LEFT, INPUT_RIGHT,
	INPUT_START, INPUT_MODE
};

unsigned short gcpadmap[] = { PAD_BUTTON_B, PAD_BUTTON_A, PAD_BUTTON_X,
	PAD_TRIGGER_L, PAD_BUTTON_Y, PAD_TRIGGER_R,
	PAD_BUTTON_UP, PAD_BUTTON_DOWN,
	PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT,
	PAD_BUTTON_START, PAD_TRIGGER_Z
};

static inline unsigned int DecodeJoy (unsigned short p)
{
	unsigned int J = 0;
	int i;

	for (i = 0; i < 12; i++)
		if (p & gcpadmap[i]) J |= gppadmap[i];

	return J;
}

static inline unsigned int GetAnalog (int Joy)
{
	signed char x, y;
	unsigned int i = 0;

	x = PAD_StickX (Joy);
	y = PAD_StickY (Joy);

	if (x > padcal)  i |= INPUT_RIGHT;
	if (x < -padcal) i |= INPUT_LEFT;
	if (y > padcal)  i |= INPUT_UP;
	if (y < -padcal) i |= INPUT_DOWN;

	return i;
}

/*** Inputs Update
     called before each emulation frame
 ***/
void update_inputs(u32 count)
{
	int i = 0;
	int joynum = 0;	

	PAD_ScanPads();

	/*** Check for specific combo ***/
	if (PAD_ButtonsHeld (0) & PAD_TRIGGER_Z)
  {
		if (PAD_ButtonsHeld (0) & PAD_TRIGGER_L)
		{
			/* SOFT-RESET key */
			resetline = (int) ((double) (lines_per_frame - 1) * rand() / (RAND_MAX + 1.0));
			return;
  }

		/* MENU key */
		ConfigRequested = 1;
		return;
  }

	for (i=0; i<MAX_DEVICES; i++)
	{
		input.pad[i] = 0;
		if (input.dev[i] != NO_DEVICE)
		{
			input.pad[i] = DecodeJoy(PAD_ButtonsHeld (joynum));
			input.pad[i] |= GetAnalog (joynum);
			joynum ++;
			if (input.dev[i] == DEVICE_LIGHTGUN) lightgun_set();
		}
	}
}

/***************************************************************************
 * Genesis Plus support
 *
 ***************************************************************************/
static void init_machine ()
{
	/*** Allocate cart_rom here ***/
	memset(genrom.cart_rom, 0, GENESIS_CART_ROM_SIZE);
  EUGC_SetFileBufferPointer( genrom.cart_rom, &genrom.genromsize );

	/*** try to load BIOS rom ***/
	sd_file *bios_file = SDCARD_OpenFile ("dev0:\\genplus\\gen_bios.bin", "rb");
	if (bios_file != NULL)
	{
		SDCARD_ReadFile(bios_file, &genrom.bios_rom[0], 2048);
		bios_enabled = 2;
	}
	else bios_enabled = 0;

	/*** Fetch from ARAM any linked rom ***/
	if (genrom.genromsize) ARAMFetch((char *)genrom.cart_rom, (void *)0x8000, genrom.genromsize);
  
	/*** Allocate global work bitmap ***/
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
  
	/*** Default inputs ***/
	input.system[0] = SYSTEM_GAMEPAD;
	input.system[1] = SYSTEM_GAMEPAD;
}

/**************************************************
  Load a new rom and performs some initialization
***************************************************/
extern void decode_ggcodes ();
extern void ClearGGCodes ();
extern void sram_autoload();

void reloadrom ()
{
	load_rom("");			/* Load ROM */
	system_init ();			/* Initialize System */
	audio_init(48000);		/* Audio System initialization */
/** TODO Activate GG */
#if 0
	ClearGGCodes ();		/* Clear Game Genie patches */
	decode_ggcodes ();		/* Apply Game Genie patches */
#endif 
	system_reset ();		/* System Power ON */

	if (autoload) EUGC_AutoloadSRAM(&gfx_ctrl, &use_SDCARD, &CARDSLOT);

	/* PAL TV modes detection */
	if (tv_mode && vdp_pal) gc_pal = 1;
	else gc_pal = 0;

	/* GX Scaler initialization */ 
	viewport_init();
}

/***************************************************************************
 *  M A I N
 *
 ***************************************************************************/
extern void start_main_menu();
extern void reloadrom ();

int main (int argc, char *argv[])
{
	genrom.genromsize = 0;

	/* Initialize GC System */
	InitGCVideo ();
	InitGCAudio ();

	/* Initialize Virtual Genesis */
	init_machine ();
  build_menu_structure();

	/* Show Menu */
	EUGC_DisplayLegal(&gfx_ctrl);
	if (genrom.genromsize)
	{
		reloadrom ();
		start_main_menu();
	}
	else while (!genrom.genromsize) start_main_menu();

	/* Initialize Timers */
	frameticker = 0;
	mftb(&prev);

	/* Emulation Loop */
	while (1)
	{
		/* Timers */
		if (!gc_pal && vdp_pal) 
		{
			mftb(&now);
			if (tb_diff_msec(&now, &prev) > msBetweenFrames)
			{
				/** Simulate a frame **/
				memcpy(&prev, &now, sizeof(tb_t));
				system_frame(1);
			}
			else
			{
				/*** Delay ***/
				while (tb_diff_msec(&now, &prev) < msBetweenFrames) mftb(&now);
				memcpy(&prev, &now, sizeof(tb_t));

				system_frame(0);
				RenderedFrameCount++;
				update_video ();
			}
		}
      
		/* VSYNC */
		else
		{		  
			if (frameticker > 1)
			{
				/** Simulate a frame **/
				frameticker--;
				system_frame (1);
			}
			else
			{
				/*** Delay ***/
				while (!frameticker) usleep(10);	

				system_frame (0);
				RenderedFrameCount++;
				update_video ();
			}
	  		frameticker--;
		}

		/* Restart Audio Loop */
		if (!IsPlaying) AudioSwitchBuffers();
	  
		/* Check rendered frames (FPS) */
		if (FrameCount == vdp_rate)
		{
			FramesPerSecond = RenderedFrameCount;
			RenderedFrameCount = 0;
			FrameCount = 0;
		}

		/* Check for Menu */
		if (ConfigRequested)
		{
			AUDIO_StopDMA ();
			IsPlaying = 0;
			mixbuffer = 0;
			playbuffer = 0;
			start_main_menu ();
			ConfigRequested = 0;
			frameticker = 0;
			FrameCount = 0;
			RenderedFrameCount = 0;
		}
	}
	return 0;
}
