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
#include "history.h"

/***************************************************************************
 * Genesis Virtual Machine
 *
 ***************************************************************************/
static void load_bios()
{
  /* reset BIOS found flag */
  config.bios_enabled &= ~2;

  /* open file */
  FILE *fp = fopen("/genplus/BIOS.bin", "rb");
  if (fp == NULL) return;

  /* read file */
  fread(bios_rom, 1, 0x800, fp);
  fclose(fp);

  /* update BIOS flags */
  config.bios_enabled |= 2;
}

static void init_machine()
{
  /* Allocate cart_rom here */
  cart_rom = memalign(32, 0x500000);

  /* BIOS support */
  load_bios();
  
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
void reloadrom ()
{
  load_rom("");      /* Load ROM */
  system_init ();    /* Initialize System */
  audio_init(48000); /* Audio System initialization */
  ClearGGCodes ();   /* Clear Game Genie patches */
  system_reset ();   /* System Power ON */
  ogc_input__reset();
}

/***************************************************************************
 *  M A I N
 *
 ***************************************************************************/
int FramesPerSecond = 0;
int frameticker = 0;

int main (int argc, char *argv[])
{
  u16 usBetweenFrames;
  long long now, prev;
  int RenderedFrameCount = 0;
  int FrameCount = 0;
  
  /* Initialize GC subsystems */
  ogc_video__init();
  ogc_input__init();
  ogc_audio__init();
#ifndef HW_RVL
  DVD_Init ();
  dvd_drive_detect();
#endif
  fatInitDefault();
  
  /* Default Config */
  set_config_defaults();
  config_load();

#ifdef HW_RVL
  set_history_defaults();
  history_load();
#endif

  /* Initialize VM */
  init_machine ();
  
  /* Load any injected rom */
  if (genromsize)
  {
    ARAMFetch((char *)cart_rom, (void *)0x8000, genromsize);
    reloadrom ();
  }
  
  /* Show Menu */
  legal();
  MainMenu();
  ConfigRequested = 0;
  
  /* Initialize Frame timings */
  frameticker = 0;
  prev = gettime();
  
  /* Emulation Loop */
  while (1)
  {
    /* update inputs */
    ogc_input__update();
    
    /* Frame synchronization */
    if (gc_pal != vdp_pal)
    {
      /* use timers */
      usBetweenFrames = vdp_pal ? 20000 : 16666;
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
      }
      
      frameticker--;
    }
    
    /* update video & audio */
    ogc_video__update();
    ogc_audio__update();
    
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
      /* reset AUDIO */
      ogc_audio__reset();
      
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
