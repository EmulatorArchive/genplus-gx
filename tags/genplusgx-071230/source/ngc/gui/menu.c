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
#include "shared.h"
#include "dvd.h"
#include "rominfo.h"
#include "font.h"

#define PSOSDLOADID 0x7c6000a6

/***************************************************************************
 * drawmenu
 *
 * As it says, simply draws the menu with a highlight on the currently
 * selected item :)
 ***************************************************************************/
char menutitle[60] = { "" };
int menu = 0;

void drawmenu (char items[][20], int maxitems, int selected)
{
  int i;
  int ypos;

  ypos = (310 - (fheight * maxitems)) >> 1;
  ypos += 130;

  ClearScreen ();
  WriteCentre (134, menutitle);

  for (i = 0; i < maxitems; i++)
  {
      if (i == selected) WriteCentre_HL (i * fheight + ypos, (char *) items[i]);
      else WriteCentre (i * fheight + ypos, (char *) items[i]);
  }

  SetScreen ();
}

/****************************************************************************
 * domenu
 *
 * Returns index into menu array when A is pressed, -1 for B
 ****************************************************************************/
int domenu (char items[][20], int maxitems)
{
  int redraw = 1;
  int quit = 0;
  short p;
  int ret = 0;
  signed char a,b;

  while (quit == 0)
  {
      if (redraw)
	  {
	      drawmenu (&items[0], maxitems, menu);
	      redraw = 0;
	  }

      p = PAD_ButtonsDown (0);
      a = PAD_StickY (0);
	  b = PAD_StickX (0);

	  /*** Look for up ***/
      if ((p & PAD_BUTTON_UP) || (a > 70))
	  {
	    redraw = 1;
	    menu--;
        if (menu < 0) menu = maxitems - 1;
	  }

		  /*** Look for down ***/
      if ((p & PAD_BUTTON_DOWN) || (a < -70))
	  {
	    redraw = 1;
	    menu++;
        if (menu == maxitems) menu = 0;
	  }

	  if ((p & PAD_BUTTON_A) || (b > 40) || (p & PAD_BUTTON_RIGHT))
	  {
	    quit = 1;
	    ret = menu;
	  }
	  
	  if ((b < -40) || (p & PAD_BUTTON_LEFT))
	  {
	    quit = 1;
	    ret = 0 - 2 - menu;
	  }

      if (p & PAD_BUTTON_B)
	  {
	    quit = 1;
	    ret = -1;
	  }
  }

  return ret;
}

/****************************************************************************
 * Sound Option menu
 *
 ****************************************************************************/
double psg_preamp = 1.5;
double fm_preamp  = 1.0;
u8 boost = 1;
uint8 hq_fm = 1;
uint8 FM_GENS  = 0;
uint8 PSG_MAME = 0;

void soundmenu ()
{
	int ret;
	int quit = 0;
	int prevmenu = menu;
	int count = 7;
	char items[7][20];

	strcpy (menutitle, "Press B to return");

	menu = 0;
	while (quit == 0)
	{
		sprintf (items[0], "PSG Volume: %1.2f", psg_preamp);
		sprintf (items[1], "FM Volume: %1.2f", fm_preamp);
		sprintf (items[2], "Volume Boost: %dX", boost);
		sprintf (items[3], "HQ YM2612: %s", hq_fm ? "Y" : "N");
		sprintf (items[4], "SSG-EG: %s", ssg_enabled ? "ON" : "OFF");
		sprintf (items[5], "FM core: %s", FM_GENS ? "GENS" : "MAME");
		sprintf (items[6], "PSG core: %s", PSG_MAME ? "MAME" : "SMSP");

		ret = domenu (&items[0], count);
		switch (ret)
		{
			case 0:
			case -2:
				if (ret<0) psg_preamp -= 0.01;
				else psg_preamp += 0.01;
				if (psg_preamp < 0.0) psg_preamp = 5.0;
				if (psg_preamp > 5.0) psg_preamp = 0.0;
				break;

			case 1:
			case -3:
				if (ret<0) fm_preamp -= 0.01;
				else fm_preamp += 0.01;
				if (fm_preamp < 0.0) fm_preamp = 5.0;
				if (fm_preamp > 5.0) fm_preamp = 0.0;
				break;

			case 2:
				boost ++;
				if (boost > 4) boost = 0;
				break;
			
			case 3:
				hq_fm ^= 1;
				if (genromsize) 
				{	audio_init(48000);
					fm_restore();
				}
				break;

			case 4:
				ssg_enabled ^= 1;
				break;
			
			case 5:
				FM_GENS ^= 1;
				psg_preamp = PSG_MAME ? (FM_GENS ? 0.8 : 0.5) : (FM_GENS ? 2.5 : 1.5);
				fm_preamp  = 1.0;
				if (genromsize) 
				{
					audio_init(48000);
					fm_restore();
				}
				break;

			case 6:
				PSG_MAME ^= 1;
				psg_preamp = PSG_MAME ? (FM_GENS ? 0.8 : 0.5) : (FM_GENS ? 2.5 : 1.5);
				fm_preamp  = 1.0;
				if (genromsize)
				{
					audio_init(48000);
					fm_restore();
				}
				break;

			case -1:
				quit = 1;
				break;
		}
	}
	menu = prevmenu;
}

#ifdef NGC_DEBUG
/****************************************************************************
 * Palette Option menu
 *
 ****************************************************************************/
void palettemenu ()
{
	int ret, i;
	int quit = 0;
	int prevmenu = menu;
	int count = 7;
	char items[7][20];
	int8 val;
	
	strcpy (menutitle, "Press B to return");
	menu = 0;
	
	while (quit == 0)
	{
		sprintf (items[0], "Color 1:  %d", rgb565_norm[1][1]);
		sprintf (items[1], "Color 2:  %d", rgb565_norm[1][2]);
		sprintf (items[2], "Color 3:  %d", rgb565_norm[1][3]);
		sprintf (items[3], "Color 4:  %d", rgb565_norm[1][4]);
		sprintf (items[4], "Color 5:  %d", rgb565_norm[1][5]);
		sprintf (items[5], "Color 6:  %d", rgb565_norm[1][6]);
		sprintf (items[6], "Color 7:  %d", rgb565_norm[1][7]);
		
		ret = domenu (&items[0], count);
		switch (ret)
		{
			case 0:
			case -2:
				val = rgb565_norm[1][1];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][1] = val;
				rgb565_norm[0][1] = val / 2;
				break;

			case 1:
			case -3:
				val = rgb565_norm[1][2];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][2] = val;
				rgb565_norm[0][2] = val / 2;
				break;

			case 2:
			case -4:
				val = rgb565_norm[1][3];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][3] = val;
				rgb565_norm[0][3] = val / 2;
				break;

			case 3:
			case -5:
				val = rgb565_norm[1][4];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][4] = val;
				rgb565_norm[0][4] = val / 2;
				break;

			case 4:
			case -6:
				val = rgb565_norm[1][5];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][5] = val;
				rgb565_norm[0][5] = val / 2;
				break;

			case 5:
			case -7:
				val = rgb565_norm[1][6];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][6] = val;
				rgb565_norm[0][6] = val / 2;
				break;

			case 6:
			case -8:
				val = rgb565_norm[1][7];
				if (ret < 0) val --;
				else val ++;
				if (val > 63) val = 0;
				else if (val < 0) val = 63;
				rgb565_norm[1][7] = val;
				rgb565_norm[0][7] = val / 2;
				break;

			case -1:
				quit = 1;
				break;
		}
	}

	render_init();
	for(i = 0; i < 0x40; i += 1) color_update(i, *(uint16 *)&cram[i << 1]);
	color_update(0x00, *(uint16 *)&cram[border << 1]);

	menu = prevmenu;
}
#endif

/****************************************************************************
 * Misc Option menu
 *
 ****************************************************************************/
uint8 autoload = 0;
uint8 region_detect = 0;

void miscmenu ()
{
	int ret;
	int quit = 0;
	int prevmenu = menu;
#ifdef NGC_DEBUG
	int count = 6;
	char items[6][20];
#else
	int count = 5;
	char items[5][20];
#endif
	strcpy (menutitle, "Press B to return");
	menu = 0;
	
	while (quit == 0)
	{
		if (region_detect == 0)	     sprintf (items[0], "Region: AUTO");
		else if (region_detect == 1) sprintf (items[0], "Region:  USA");
		else if (region_detect == 2) sprintf (items[0], "Region:  EUR");
		else if (region_detect == 3) sprintf (items[0], "Region:  JAP");
		sprintf (items[1], "VDP Latency: %s", vdptiming ? " ON" : "OFF");
		sprintf (items[2], "DMA Timings: %s", dmatiming ? " ON" : "OFF");
		sprintf (items[3], "SRAM Autoload: %s", autoload ? "Y" : "N");
		sprintf (items[4], "TMSS Bios: %s", bios_enabled&1 ? " ON" : "OFF");
#ifdef NGC_DEBUG
		sprintf (items[5], "H. Border: %02d", bitmap.viewport.x);*/
#endif
		ret = domenu (&items[0], count);
		switch (ret)
		{
			case 0:	/*** Region Force ***/
				region_detect = (region_detect + 1) % 4;
				if (genromsize)
				{
					genesis_set_region();
					if (tv_mode && vdp_pal) gc_pal = 1;
					else gc_pal = 0;
					system_init ();
					viewport_init();
					audio_init(48000);
					fm_restore();
				}
				break;

			case 1:	/*** VDP latency ***/
				vdptiming ^= 1;
				break;

			case 2:	/*** DMA timing ***/
				dmatiming ^= 1;
				break;

			case 3:	/*** SRAM autoload ***/
				autoload ^= 1;
				break;

			case 4:	/*** BIOS support ***/
				bios_enabled ^= 1;
				break;

#ifdef NGC_DEBUG
			case 5:	
			case -7:
				if (ret<0) bitmap.viewport.x -= 2;
				else bitmap.viewport.x += 2;
				if (bitmap.viewport.x < 0) bitmap.viewport.x = 20;
				if (bitmap.viewport.x > 20) bitmap.viewport.x = 0;
				bitmap.viewport.changed = 1;
				break;
#endif

			case -1:
				quit = 1;
				break;
		}
	}
	menu = prevmenu;
}

/****************************************************************************
 * Display Option menu
 *
 ****************************************************************************/
uint8 old_overscan = 1;

void dispmenu ()
{
	int ret;
	int quit = 0;
	int prevmenu = menu;
#ifdef NGC_DEBUG
	int count = 8;
#else
	int count = aspect ? 6 : 8;
#endif
	char items[8][20];

	strcpy (menutitle, "Press B to return");
	menu = 0;

	while (quit == 0)
	{
		sprintf (items[0], "Aspect: %s", aspect ? "ORIGINAL" : "MANUAL SET");
		sprintf (items[1], "Render: %s", use_480i ? "BILINEAR" : "ORIGINAL");
		sprintf (items[2], "TV Mode: %s", tv_mode ? "50/60HZ" : "60HZ");
		sprintf (items[3], "Overscan: %s", overscan ? " ON" : "OFF");
		sprintf (items[4], "Center X: %s%02d", xshift < 0 ? "-":"+", abs(xshift));
		sprintf (items[5], "Center Y: %s%02d", yshift < 0 ? "-":"+", abs(yshift));
		sprintf (items[6], "Scale  X:  %02d", xscale);
		sprintf (items[7], "Scale  Y:  %02d", yscale);

		ret = domenu (&items[0], count);

		switch (ret)
		{
			case 0: /*** aspect ratio ***/
				aspect ^= 1;
				if (aspect) overscan = old_overscan;
				else
				{
					old_overscan = overscan;
					overscan = 0;
				}
				count = aspect ? 6 : 8;
				vdp_init();
				viewport_init();
				break;

			case 1:	/*** rendering ***/
				use_480i ^= 1;
				viewport_init();
				if (interlaced) bitmap.viewport.changed = 1;
				break;

			case 2: /*** tv mode ***/
				tv_mode ^= 1;
				if (tv_mode && vdp_pal) gc_pal = 1;
				else gc_pal = 0;
				viewport_init();
				break;
		
			case 3: /*** Overscan emulation ***/
				if (aspect)
				{
					overscan ^= 1;
					vdp_init();
					viewport_init();
				}
				break;

			case 4:	/*** Center X ***/
			case -6:
				if (ret<0) xshift --;
				else xshift ++;
				square[6] = square[3]  =  xscale + xshift;
				square[0] = square[9]  = -xscale + xshift;
				draw_init();
				break;

			case 5:	/*** Center Y ***/
			case -7:
				if (ret<0) yshift --;
				else yshift ++;
				square[4] = square[1]  =  yscale + yshift;
				square[7] = square[10] = -yscale + yshift;
				draw_init();
				break;
			
			case 6:	/*** Scale X ***/
			case -8:
				if (ret<0) xscale --;
				else xscale ++;
				square[6] = square[3]  =  xscale + xshift;
				square[0] = square[9]  = -xscale + xshift;
				draw_init();
				break;

			case 7:	/*** Scale Y ***/
			case -9:
				if (ret<0) yscale --;
				else yscale ++;
				square[4] = square[1]  =  yscale + yshift;
				square[7] = square[10] = -yscale + yshift;
				draw_init();
				break;

			case -1:
				quit = 1;
				break;
		}		
	}
	menu = prevmenu;
}

/****************************************************************************
 * Main Option menu
 *
 ****************************************************************************/
extern void ConfigureJoypads();
extern void GetGGEntries();

void optionmenu ()
{
	int ret;
	int quit = 0;
	int prevmenu = menu;
#ifdef NGC_DEBUG
	int count = 6;
	char items[6][20] = {
#else
	int count = 5;
	char items[5][20] = {
#endif
		"Video Options",
		"Sound Options",
		"System Options",
		"Configure Joypads",
		"Game Genie Codes"
#ifdef NGC_DEBUG
		,"Configure Palette"
#endif
	};

	menu = 0;
	while (quit == 0)
	{
		strcpy (menutitle, "Press B to return");
		ret = domenu (&items[0], count);
		switch (ret)
		{
			case 0:
				dispmenu();
				break;
			case 1:
				soundmenu();
				break;
			case 2:
				miscmenu();
				break;
			case 3:
				ConfigureJoypads();
				break;
			case 4:
				GetGGEntries();
				break;
#ifdef NGC_DEBUG
			case 5:
				palettemenu();
				break;
#endif
			case -1:
				quit = 1;
				break;
		}
	}
	menu = prevmenu;
}

/****************************************************************************
* Generic Load/Save menu
*
****************************************************************************/
int CARDSLOT = CARD_SLOTB;
int use_SDCARD = 0;
extern int ManageSRAM (int direction);
extern int ManageState (int direction);

int loadsavemenu (int which)
{
	int prevmenu = menu;
	int quit = 0;
	int ret;
	int count = 4;
	char items[4][20];

	strcpy (menutitle, "Press B to return");

	if (use_SDCARD) sprintf(items[0], "Device: SDCARD");
	else sprintf(items[0], "Device:  MCARD");

    if (CARDSLOT == CARD_SLOTA) sprintf(items[1], "Use: SLOT A");
    else sprintf(items[1], "Use: SLOT B");

	if (which)
	{
		sprintf(items[2], "Save State");
		sprintf(items[3], "Load State");
	}
	else
	{
		sprintf(items[2], "Save SRAM");
		sprintf(items[3], "Load SRAM");
	}

	menu = 2;

	while (quit == 0)
	{
		ret = domenu (&items[0], count);
		switch (ret)
		{
			case -1:
				quit = 1;
				break;

			case 0:
				use_SDCARD ^= 1;
				if (use_SDCARD) sprintf(items[0], "Device: SDCARD");
				else sprintf(items[0], "Device:  MCARD");
				break;
			case 1:
				CARDSLOT ^= 1;
				if (CARDSLOT == CARD_SLOTA) sprintf(items[1], "Use: SLOT A");
				else sprintf(items[1], "Use: SLOT B");
				break;
			case 2:
			case 3:
				if (which) quit = ManageState(ret-2);
				else quit = ManageSRAM(ret-2);
				if (quit) return 1;
				break;
		}
	}

	menu = prevmenu;
	return 0;
}


/****************************************************************************
 * File Manager menu
 *
 ****************************************************************************/
int filemenu ()
{
	int prevmenu = menu;
	int ret;
	int quit = 0;
	uint32 crccheck;
	int count = 3;
	char items[3][20] = {
		{"SRAM Manager"},
		{"STATE Manager"},
		{"Return to previous"}
	};

	crccheck = crc32 (0, &sram.sram[0], 0x10000);
	if (genromsize && (crccheck != sram.crc)) strcpy (menutitle, "*** SRAM has been modified ***");
	else strcpy (menutitle, "");

	menu = 0;

	while (quit == 0)
	{
		ret = domenu (&items[0], count);
		switch (ret)
		{
			case -1: /*** Button B ***/
			case 2:  /*** Quit ***/
				ret = 0;
				quit = 1;
				break;
			case 0:	 /*** SRAM Manager ***/
			case 1:  /*** SaveState Manager ***/
				if (loadsavemenu(ret)) return 1;
				break;
		}
	}

	menu = prevmenu;
	return 0;
}

/****************************************************************************
 * Load Rom menu
 *
 ****************************************************************************/
extern void OpenDVD ();
extern int OpenSD ();
extern u8 UseSDCARD;

void loadmenu ()
{
	int prevmenu = menu;
	int ret;
	int quit = 0;
	int count = 3;
	char item[3][20] = {
		{"Load from DVD"},
		{"Load from SDCARD"},
		{"Return to previous"}
	};

	menu = UseSDCARD ? 1 : 0;
	
	while (quit == 0)
	{
		strcpy (menutitle, "");
		ret = domenu (&item[0], count);
		switch (ret)
		{
			case -1: /*** Button B ***/
			case 2:  /*** Quit ***/
				quit = 1;
				menu = prevmenu;
				break;
			case 0:	 /*** Load from DVD ***/
				OpenDVD ();
				quit = 1;
				break;
			case 1:  /*** Load from SCDARD ***/
				OpenSD ();
				quit = 1;
				break;
		}
	}
}

/****************************************************************************
 * Main menu
 *
 ****************************************************************************/

void MainMenu ()
{
	GXRModeObj *vmode;
    Mtx p;
	menu = 0;
	int ret;
	int quit = 0;
	int *psoid = (int *) 0x80001800;
	void (*PSOReload) () = (void (*)()) 0x80001800; 	/*** Stock PSO/SD Reload call ***/
	int count = 8;
	char items[8][20] = {
		{"Play Game"},
		{"Game Infos"},
		{"Reset System"},
		{"Load New Game"},
		{"File Management"},
		{"Emulator Options"},
		{"Stop DVD Motor"},
		{"System Reboot"}
	};

	/* force 480 lines (60Hz) mode in menu */
	VIDEO_Configure (&TVNtsc480IntDf);
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();

	while (quit == 0)
	{
		strcpy (menutitle, "");
		ret = domenu (&items[0], count);
		switch (ret)
		{
			case -1: /*** Button B ***/
			case 0:  /*** Play Game ***/
				quit = 1;
				break;
			case 1:	 /*** ROM Information ***/
				showrominfo ();
				break;
			case 2:  /*** Emulator Reset ***/
				if (genromsize)
				{
					system_reset ();
					quit = 1;
				}
				break;
			case 3:  /*** Load ROM Menu ***/
				loadmenu();
				menu = 0;
				break;
			case 4:  /*** Memory Manager ***/
				quit = filemenu ();
				break;
			case 5:  /*** Emulator Options */
				optionmenu ();
				break;
			case 6:  /*** Stop DVD Motor ***/
				ShowAction("Stopping DVD Motor ...");
				dvd_motor_off();
				break;
			case 7:  /*** SD/PSO Reload ***/
				if (psoid[0] == PSOSDLOADID) PSOReload ();
				else SYS_ResetSystem(SYS_HOTRESET,0,FALSE);
				break;
		}
	}

	/*** Remove any still held buttons ***/
	while(PAD_ButtonsHeld(0)) VIDEO_WaitVSync();

	/*** Reinitialize current TV mode ***/
	if (use_480i) vmode = tvmodes[gc_pal*3 + 2];
	else vmode = tvmodes[gc_pal*3 + interlaced];
	vmode->viWidth = aspect ? 720 : 640;
	vmode->viXOrigin = (720 - vmode->viWidth) / 2;
	VIDEO_Configure (vmode);
    VIDEO_WaitVSync();
    if (vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
	else while (VIDEO_GetNextField())  VIDEO_WaitVSync();
	odd_frame = 0;

	/*** Reinitalize GX ***/ 
    GX_SetViewport (0.0F, 0.0F, vmode->fbWidth, vmode->efbHeight, 0.0F, 1.0F);
	GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);
    f32 yScale = GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight);
    u16 xfbHeight  = GX_SetDispCopyYScale (yScale);
	GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
	GX_SetDispCopyDst (vmode->fbWidth, xfbHeight);
	GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, (vmode->efbHeight == vmode->xfbHeight) ? GX_TRUE : GX_FALSE, vmode->vfilter);
	GX_SetFieldMode (vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt (vmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	guOrtho(p, vmode->efbHeight/2, -(vmode->efbHeight/2), -(vmode->fbWidth/2), vmode->fbWidth/2, 100, 1000);
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

	/*** Stop the DVD from causing clicks while playing ***/
	uselessinquiry ();			
}
