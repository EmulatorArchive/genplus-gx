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
#include <eugc_api.h>

/*--------------------------------------------------------------------*/
/*-- Func proto                                                       */
/*--------------------------------------------------------------------*/
int play_game_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int game_info_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int reset_system_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int stop_dvd_motor_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int system_reboot_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);

int sramstate_device_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sramstate_slot_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sram_saveload_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int state_saveload_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sramstate_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sys_opt_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int joy_opt_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int gg_opt_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
/* Video options callback */
int vidopt_aspect_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int vidopt_render_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int vidopt_tvmode_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int vidopt_overscan_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
/* Audio options callback */
int audopt_hq_ym_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int audopt_ssg_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int audopt_hq_ym_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int audopt_fmcore_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
/* System options callbacks */
int sysopt_region_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sysopt_vdp_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sysopt_dmatiming_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sysopt_dtack_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sysopt_autoload_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sysopt_bios_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);
int sysopt_svpcycles_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val);

/* Some local variables */
char working_string[MENU_MAX_CHOICE_SIZE];

/* Some external functions, should be cleanup */
extern int getcompany ();
extern void set_region();

/*--------------------------------------------------------------------*/
/*-- Menu declaration                                                 */
/*--------------------------------------------------------------------*/
/* Menu containers object, tree view*/
MENU_CONTAINER_t    main_menu;
  MENU_CONTAINER_t    game_info_menu;
  MENU_CONTAINER_t    file_mngt_menu;
    MENU_CONTAINER_t    sram_manager_menu;
    MENU_CONTAINER_t    state_manager_menu;
  MENU_CONTAINER_t    emu_opt_menu;
    MENU_CONTAINER_t    video_opt_menu;
    MENU_CONTAINER_t    audio_opt_menu;
    MENU_CONTAINER_t    sys_opt_menu;
    MENU_CONTAINER_t    pad_opt_menu;
    MENU_CONTAINER_t    gg_opt_menu;

/* Menu content is : 
  string - A cb - Left cb - Right cb - submenu - pvalue */
MENU_CONTENT_t main_menu_content[] = { 
  {"Play Game",        play_game_A_cb, NULL, NULL, NULL, NULL },
  {"Game Infos",       game_info_A_cb, NULL, NULL, &game_info_menu, NULL},
  {"Reset System",     reset_system_A_cb, NULL, NULL, NULL, NULL },
  {"Load New Game",    EUGC_FileBrowser, NULL, NULL, NULL, NULL },
  {"File Management",  NULL, NULL, NULL, &file_mngt_menu, NULL },
  {"Emulator Options", NULL, NULL, NULL, &emu_opt_menu, NULL },
  {"Stop DVD Motor"  , stop_dvd_motor_A_cb, NULL, NULL, NULL, NULL },
  {"System Reboot"   , system_reboot_A_cb, NULL, NULL, NULL, NULL },
  {MENU_EMPTY_STR    , NULL, NULL, NULL, NULL, NULL },
};

#define GAME_INFO_CONTENT  32
MENU_CONTENT_t game_info_content[GAME_INFO_CONTENT] = { 
  };

MENU_CONTENT_t file_mngt_content[] = {
  {"SRAM Manager",   sramstate_A_cb, NULL, NULL, &sram_manager_menu,  NULL },
  {"STATE Manager",  sramstate_A_cb, NULL, NULL, &state_manager_menu, NULL },
  {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

MENU_CONTENT_t sram_mngt_content[] = {
  { "Device ",    sramstate_device_A_cb, NULL, NULL, NULL, NULL },
  { "Slot ",      sramstate_slot_A_cb,   NULL, NULL, NULL, NULL },
  { "Save RAM",   sram_saveload_A_cb,    NULL, NULL, NULL, NULL },
  { "Load RAM",   sram_saveload_A_cb,    NULL, NULL, NULL, NULL },
  {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

MENU_CONTENT_t state_mngt_content[] = {
  { "Device ",      sramstate_device_A_cb, NULL, NULL, NULL, NULL },
  { "Slot ",        sramstate_slot_A_cb,   NULL, NULL, NULL, NULL },
  { "Save STATE",   state_saveload_A_cb,   NULL, NULL, NULL, NULL },
  { "Load STATE",   state_saveload_A_cb,   NULL, NULL, NULL, NULL },
  {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

MENU_CONTENT_t emu_opt_content[] = { 
  {"Video Options",     NULL, NULL, NULL, &video_opt_menu,  NULL },
  {"Audio Options",     NULL, NULL, NULL, &audio_opt_menu,  NULL },
  {"System Options",    NULL,   NULL, NULL, &sys_opt_menu,    NULL },
  {"Configure Joypads", joy_opt_A_cb,   NULL, NULL, &pad_opt_menu,    NULL },
  {"Game Genie Codes",  gg_opt_A_cb,    NULL, NULL, &gg_opt_menu,     NULL },
  {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

/* Values for the video options menu */
char *vidopt_aspect_choice[] =  {"ORIGINAL", "FIT SCREEN" };
MENU_VALUE_t vidopt_aspect_value = {
  MENU_ENUM_STR, &aspect, NULL, 0, 0, 0, 1, vidopt_aspect_choice };
char *vidopt_render_choice[] =  {"BILINEAR", "ORIGINAL" };
MENU_VALUE_t vidopt_render_value = {
  MENU_ENUM_STR, &use_480i, NULL, 0, 0, 0, 1, vidopt_render_choice };
char *vidopt_tvmode_choice[] = {"50/60HZ", "60HZ" };
MENU_VALUE_t vidopt_tvmode_value = {
  MENU_ENUM_STR, &tv_mode, NULL, 0, 0, 0, 1, vidopt_tvmode_choice };
char *vidopt_overscan_choice[] = {"ON", "OFF"};
MENU_VALUE_t vidopt_overscan_value = {
  MENU_ENUM_STR, &overscan, NULL, 0, 0, 0, 1, vidopt_overscan_choice };
MENU_VALUE_t vidopt_centerx_value = {
  MENU_INCDEC_INTEGER, &xshift, NULL, 0, 1, -200, 200, NULL };
MENU_VALUE_t vidopt_centery_value = {
  MENU_INCDEC_INTEGER, &yshift, NULL, 0, 1, -200, 200, NULL };
MENU_VALUE_t vidopt_scalex_value = {
  MENU_INCDEC_INTEGER, &xscale, NULL, 0, 1, 0, 5, NULL };
MENU_VALUE_t vidopt_scaley_value = {
  MENU_INCDEC_INTEGER, &yscale, NULL, 0, 1, 0, 5, NULL };

MENU_CONTENT_t video_opt_content[] = {
 {"Aspect:   ", vidopt_aspect_A_cb,      NULL, NULL, NULL,  &vidopt_aspect_value },
 {"Render:   ", vidopt_render_A_cb,      NULL, NULL, NULL,  &vidopt_render_value },
 {"TV Mode:  ", vidopt_tvmode_A_cb,      NULL, NULL, NULL,  &vidopt_tvmode_value },
 {"Overscan: ", vidopt_overscan_A_cb,    NULL, NULL, NULL,  &vidopt_overscan_value },
 {"Center X: ", NULL,                    NULL, NULL, NULL,  &vidopt_centerx_value },
 {"Center Y: ", NULL,                    NULL, NULL, NULL,  &vidopt_centery_value },
 {"Scale  X: ", NULL,                    NULL, NULL, NULL,  &vidopt_scalex_value },
 {"Scale  Y: ", NULL,                    NULL, NULL, NULL,  &vidopt_scaley_value },
 {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

/* Automatic values setup for audio options */
MENU_VALUE_t audopt_psgvol_value = {
  MENU_INCDEC_DOUBLE, NULL, &psg_preamp, 0.01, 0, -5, 5, NULL };
MENU_VALUE_t audopt_fmvol_value = {
  MENU_INCDEC_DOUBLE, NULL, &fm_preamp, 0.01, 0, -5, 5, NULL };
MENU_VALUE_t audopt_boost_value = {
  MENU_INCDEC_INTEGER, &boost, NULL, 0, 1, 0, 4, NULL };
char *generic_yesno_choice[] = {"YES", "NO"};
MENU_VALUE_t audopt_hqym_value = {
  MENU_ENUM_STR, &hq_fm, NULL, 0, 1, 0, 1, generic_yesno_choice };
char *generic_onoff_choice[] = {"ON", "OFF"};
MENU_VALUE_t audopt_ssg_value = {
  MENU_ENUM_STR, &ssg_enabled, NULL, 0, 1, 0, 1, generic_onoff_choice };
char *audopt_fmgens_choice[] = {"GENS", "MAME"};
MENU_VALUE_t audopt_fmgens_value = {
  MENU_ENUM_STR, &FM_GENS, NULL, 0, 1, 0, 1, audopt_fmgens_choice };

/* Audio options menu content */
MENU_CONTENT_t audio_opt_content[] = {
  {"PSG Volume: ", NULL, NULL, NULL, NULL, &audopt_psgvol_value },
  {"FM Volume: ",  NULL, NULL, NULL, NULL, &audopt_fmvol_value },
  {"Volume Boost: ", NULL, NULL, NULL, NULL, &audopt_boost_value },
  {"HQ YM2612: ",   audopt_hq_ym_A_cb,  NULL, NULL, NULL,  &audopt_hqym_value },
  {"SSG-EG:    ",   NULL,    NULL, NULL, NULL,  &audopt_ssg_value },
  {"FM core:   ",   audopt_fmcore_A_cb, NULL, NULL, NULL,  &audopt_fmgens_value },
 {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

char *sysopt_region_choice[] = {"AUTO", "USA", "EUR", "JAP" };
MENU_VALUE_t sysopt_region_value = {
  MENU_ENUM_STR, &region_detect, NULL, 0, 1, 0, 3, sysopt_region_choice };
MENU_VALUE_t sysopt_vdptiming_value = {
  MENU_ENUM_STR, &vdptiming, NULL, 0, 1, 0, 1, generic_onoff_choice };
MENU_VALUE_t sysopt_dmatiming_value = {
  MENU_ENUM_STR, &dmatiming, NULL, 0, 1, 0, 1, generic_onoff_choice };
MENU_VALUE_t sysopt_fdtack_value = {
  MENU_ENUM_STR, &force_dtack, NULL, 0, 1, 0, 1, generic_yesno_choice };
MENU_VALUE_t sysopt_autoload_value = {
  MENU_ENUM_STR, &autoload, NULL, 0, 1, 0, 1, generic_yesno_choice };
MENU_VALUE_t sysopt_bios_value = {
  MENU_ENUM_STR, &bios_enabled, NULL, 0, 1, 0, 1, generic_yesno_choice };
MENU_VALUE_t sysopt_svpcycle_value = {
  MENU_INCDEC_INTEGER, &SVP_cycles, NULL, 0, 1, 0, 1500, NULL };

MENU_CONTENT_t sys_opt_content[] = {
  {"Region : ",        sysopt_region_A_cb, NULL, NULL, NULL,  &sysopt_region_value },
  {"VDP Latency : ",   sysopt_vdp_A_cb,    NULL, NULL, NULL,  &sysopt_vdptiming_value },
  {"DMA Timings : ",   sysopt_dmatiming_A_cb, NULL, NULL, NULL,  &sysopt_dmatiming_value },
  {"Force DTACK : ",   sysopt_dtack_A_cb,  NULL, NULL, NULL,  &sysopt_fdtack_value },
  {"Autoload SRAM : ", sysopt_autoload_A_cb, NULL, NULL, NULL,  &sysopt_autoload_value },
  {"TMSS BIOS : ",     sysopt_bios_A_cb, NULL, NULL, NULL,  &sysopt_bios_value },
  {"SVP Cycles : ",    sysopt_svpcycles_A_cb, NULL, NULL, NULL,  &sysopt_svpcycle_value },
 {MENU_EMPTY_STR ,  NULL, NULL, NULL, NULL, NULL }
};

void build_menu_structure(void)
{
  /* Init the menu container */
  EUGC_InitMenuContainer(&main_menu, "GENPLUS Main Menu");
  EUGC_InitMenuContainer(&game_info_menu, "Game info");
  EUGC_InitMenuContainer(&file_mngt_menu, "File management");
    EUGC_InitMenuContainer(&sram_manager_menu, "SRAM menu");
    EUGC_InitMenuContainer(&state_manager_menu, "STATE menu");
  EUGC_InitMenuContainer(&emu_opt_menu, "Options");
    EUGC_InitMenuContainer(&video_opt_menu, "Video options");
    EUGC_InitMenuContainer(&audio_opt_menu, "Audio options");
    EUGC_InitMenuContainer(&sys_opt_menu, "System options");
    EUGC_InitMenuContainer(&pad_opt_menu, "Joypad options");
    EUGC_InitMenuContainer(&gg_opt_menu, "Game Genie options");
  
  /* Then attach the contents */
  EUGC_AttachMenuContent( &main_menu, main_menu_content);
  EUGC_AttachMenuContent( &game_info_menu, game_info_content);
  EUGC_AttachMenuContent( &file_mngt_menu, file_mngt_content);
    EUGC_AttachMenuContent( &sram_manager_menu,  sram_mngt_content);
    EUGC_AttachMenuContent( &state_manager_menu, state_mngt_content);
  EUGC_AttachMenuContent( &emu_opt_menu, emu_opt_content);
    EUGC_AttachMenuContent(&video_opt_menu, video_opt_content);
    EUGC_AttachMenuContent(&audio_opt_menu, audio_opt_content);

  /* Set hierarchy tree , first level (ie below root) */
  EUGC_SetMenuParent( &game_info_menu, &main_menu);
  EUGC_SetMenuParent( &file_mngt_menu, &main_menu);
    EUGC_SetMenuParent( &sram_manager_menu,   &file_mngt_menu);
    EUGC_SetMenuParent( &state_manager_menu,  &file_mngt_menu);
  EUGC_SetMenuParent( &emu_opt_menu, &main_menu);
    EUGC_SetMenuParent( &video_opt_menu, &emu_opt_menu);
    EUGC_SetMenuParent( &audio_opt_menu, &emu_opt_menu);
    EUGC_SetMenuParent( &sys_opt_menu,   &emu_opt_menu);
    EUGC_SetMenuParent( &pad_opt_menu,   &emu_opt_menu);
    EUGC_SetMenuParent( &gg_opt_menu,    &emu_opt_menu);
}

/*--------------------------------------------------------------------*/
/*-- Play game section                                                */
/*--------------------------------------------------------------------*/
int play_game_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  EUGC_ExitMenuLoop();
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- Game info section                                                */
/*--------------------------------------------------------------------*/
/** Build dynamically the info of a game in a menu content */
int game_info_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  u8 i,j,max;
  char pName[14][21];

  j = 0;
  max = 14;
  for (i = 0; i < 14; i++)
  {
    if (peripherals & (1 << i))
    {
      sprintf(pName[max-14],"%s", peripheralinfo[i].pName);
      max ++;
    }
  }

  sprintf (working_string, "Console type: %s", rominfo.consoletype);
  EUGC_SetMenuContentString( &game_info_menu, 0, working_string);  
  sprintf (working_string, "Copyright: %s", rominfo.copyright);
  EUGC_SetMenuContentString( &game_info_menu, 1, working_string);  
  sprintf (working_string, "Company: %s", companyinfo[getcompany ()].company);
  EUGC_SetMenuContentString( &game_info_menu, 2, working_string);  
  sprintf (working_string, "Game Domestic Name:");
  EUGC_SetMenuContentString( &game_info_menu, 3, working_string);  
  sprintf(working_string, " %s",rominfo.domestic);
  EUGC_SetMenuContentString( &game_info_menu, 4, working_string);  
  sprintf (working_string, "Game International Name:");
  EUGC_SetMenuContentString( &game_info_menu, 5, working_string);  
  sprintf(working_string, " %s",rominfo.international);
  EUGC_SetMenuContentString( &game_info_menu, 6, working_string);  
  sprintf (working_string, "Type - %s : %s", rominfo.ROMType, strcmp (rominfo.ROMType, "AI") ? "Game" : "Educational");
  EUGC_SetMenuContentString( &game_info_menu, 7, working_string);    
  sprintf (working_string, "Product - %s", rominfo.product);
  EUGC_SetMenuContentString( &game_info_menu, 8, working_string);    
  sprintf (working_string, "Checksum - %04x (%04x) (%s)", rominfo.checksum, realchecksum, (rominfo.checksum == realchecksum) ? "Good" : "Bad");
  EUGC_SetMenuContentString( &game_info_menu, 9, working_string);    
  sprintf (working_string, "ROM end: $%06X", rominfo.romend);
  EUGC_SetMenuContentString( &game_info_menu, 10, working_string);    
  if (svp) sprintf (working_string, "SVP Chip detected");
  else if (sram.custom) sprintf (working_string, "EEPROM(%dK) - $%06X", ((eeprom.type.size_mask+1)* 8) /1024, (unsigned int)sram.start);
  else if (sram.detected) sprintf (working_string, "SRAM Start  - $%06X", sram.start);
  else sprintf (working_string, "External RAM undetected");
  EUGC_SetMenuContentString( &game_info_menu, 11, working_string);
  if (sram.custom) sprintf (working_string, "EEPROM(%dK) - $%06X", ((eeprom.type.size_mask+1)* 8) /1024, (unsigned int)sram.end);
  else if (sram.detected) sprintf (working_string, "SRAM End   - $%06X", sram.end);
  else if (sram.on) sprintf (working_string, "Default SRAM activated ");
  else sprintf (working_string, "SRAM is disactivated  ");
  EUGC_SetMenuContentString( &game_info_menu, 12, working_string);
  if (region_code == REGION_USA) sprintf (working_string, "Region - %s (USA)", rominfo.country);
  else if (region_code == REGION_EUROPE) sprintf (working_string, "Region - %s (EUR)", rominfo.country);
  else if (region_code == REGION_JAPAN_NTSC) sprintf (working_string, "Region - %s (JAP)", rominfo.country);
  else if (region_code == REGION_JAPAN_PAL) sprintf (working_string, "Region - %s (JPAL)", rominfo.country);
  EUGC_SetMenuContentString( &game_info_menu, 13, working_string);

  EUGC_AttachMenuContent( &game_info_menu, game_info_content);

  /* TODO To be cleanup later */
  /*sprintf (msg, "Supports - %s", pName[i+j-14]);
  EUGC_SetMenuContentString( &game_info_menu, 13, msg);*/
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- System reset section                                             */
/*--------------------------------------------------------------------*/
/* Callback for reset system */
int reset_system_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{ 
  if (genrom.genromsize)
  {
    system_reset ();
    EUGC_ExitMenuLoop();
  }
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- SRAM/STATE section                                               */
/*--------------------------------------------------------------------*/
int sramstate_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  /* Use the child callbacl functions ti update the choice string */
  /* So we "trick" the variables use_SDCARD and CARDSLOT */
  use_SDCARD ^= 1;
  sramstate_device_A_cb(pmenu->pcontent[pmenu->n_selected].psubmenu, 0);
  CARDSLOT ^= 1;
  sramstate_slot_A_cb(pmenu->pcontent[pmenu->n_selected].psubmenu, 0);
  return 0;
}

int sram_manager_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  return 0;
}

/* Function used in sram manager and state managers */
int sramstate_device_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  use_SDCARD ^= 1;
  if (use_SDCARD) sprintf(working_string, "Device: SDCARD");
  else sprintf(working_string, "Device:  MCARD");
  EUGC_SetMenuContentString( pmenu, 0, working_string);
  return 0;
}

/* Function used in sram manager and state managers */
int sramstate_slot_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{ 
  CARDSLOT ^= 1;
  if (CARDSLOT == CARD_SLOTA) sprintf(working_string, "Use: SLOT A");
  else sprintf(working_string, "Use: SLOT B");
  EUGC_SetMenuContentString( pmenu, 1, working_string);
  return 0;
}

/* Common callback for sram save/load action */
int sram_saveload_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  int id = EUGC_GetSelectedMenuId(pmenu);
  EUGC_ManageSRAM( &gfx_ctrl, id - 2, &use_SDCARD, &CARDSLOT);
  return 0;
}

/* Common callback for state save/load action */
int state_saveload_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  int id = EUGC_GetSelectedMenuId(pmenu);
  EUGC_ManageState( &gfx_ctrl, id - 2, &use_SDCARD, &CARDSLOT);
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- Video options section                                            */
/*--------------------------------------------------------------------*/
int vidopt_aspect_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  if (aspect) overscan = old_overscan;
  else
  {
    old_overscan = overscan;
    overscan = 0;
  }

  /* reinitialize overscan area */
  bitmap.viewport.x = overscan ? ((reg[12] & 1) ? 16 : 12) : 0;
  bitmap.viewport.y = overscan ? (((reg[1] & 8) ? 0 : 8) + (vdp_pal ? 24 : 0)) : 0;
  bitmap.viewport.changed = 1;

  viewport_init(); 
  
  return 0;
}

int vidopt_render_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  viewport_init();
  if (interlaced) bitmap.viewport.changed = 1;
  
  return 0;
}

int vidopt_tvmode_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{  
  if (tv_mode && vdp_pal) gc_pal = 1;
  else gc_pal = 0;
  viewport_init();

  return 0;
}

int vidopt_overscan_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  if (aspect)
  {
    /* reinitialize overscan area */
    bitmap.viewport.x = overscan ? ((reg[12] & 1) ? 16 : 12) : 0;
    bitmap.viewport.y = overscan ? (((reg[1] & 8) ? 0 : 8) + (vdp_pal ? 24 : 0)) : 0;
    bitmap.viewport.changed = 1;

    viewport_init();
  }

  return 0;
}

/*--------------------------------------------------------------------*/
/*-- Audio options section                                            */
/*--------------------------------------------------------------------*/
int audopt_hq_ym_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  if (genrom.genromsize) 
  { 
    audio_init(48000);
    fm_restore();
  }
  return 0;
}

int audopt_fmcore_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  psg_preamp = FM_GENS ? 2.5 : 1.5;
  fm_preamp  = 1.0;
  if (genrom.genromsize) 
  {
    if (!FM_GENS) memcpy(fm_reg,YM2612.REG,sizeof(fm_reg));
    audio_init(48000);
    fm_restore();
  }
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- System options section                                           */
/*--------------------------------------------------------------------*/
int sysopt_region_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  if (genrom.genromsize)
  {
    /* force region & cpu mode */
    set_region();
    
    /* reinitialize timings */
    system_init ();
    audio_init(48000);
    fm_restore();
            
    /* reinitialize HVC tables */
    vctab = (vdp_pal) ? ((reg[1] & 8) ? vc_pal_240 : vc_pal_224) : vc_ntsc_224;
    hctab = (reg[12] & 1) ? cycle2hc40 : cycle2hc32;

    /* reinitialize overscan area */
    bitmap.viewport.x = overscan ? ((reg[12] & 1) ? 16 : 12) : 0;
    bitmap.viewport.y = overscan ? (((reg[1] & 8) ? 0 : 8) + (vdp_pal ? 24 : 0)) : 0;
    bitmap.viewport.changed = 1;

    /* reinitialize viewport */
    if (tv_mode && vdp_pal) gc_pal = 1;
    else gc_pal = 0;
    viewport_init();
  }
  return 0;
}

int joy_opt_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  return 0;
}

int gg_opt_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- Misc section                                                     */
/*--------------------------------------------------------------------*/
int stop_dvd_motor_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{ 
  EUGC_ShowAction(&gfx_ctrl, "Stopping DVD Motor ...");
  dvd_motor_off();
  return 0;
}

int system_reboot_A_cb(MENU_CONTAINER_t *pmenu, unsigned int val)
{ 
  int *psoid = (int *) 0x80001800;
  void (*PSOReload) () = (void (*)()) 0x80001800;   /*** Stock PSO/SD Reload call ***/
  
  if (psoid[0] == PSOSDLOADID) PSOReload ();
  else SYS_ResetSystem(SYS_HOTRESET,0,FALSE);
  return 0;
}

/*--------------------------------------------------------------------*/
/*-- Main menu entry point                                            */
/*--------------------------------------------------------------------*/
void start_main_menu(void)
{
  uint32 crccheck;
  Mtx p;

  /* force 480 lines (60Hz) mode in menu */
  VIDEO_Configure (&TVNtsc480IntDf);
  VIDEO_WaitVSync();
  VIDEO_WaitVSync();

  crccheck = crc32 (0, &sram.sram[0], 0x10000);
  if (genrom.genromsize && (crccheck != sram.crc)) strcpy (working_string, "*** SRAM has been modified ***");
  else if (FramesPerSecond) sprintf (working_string, "%d FPS", FramesPerSecond);
  else strcpy (working_string, "TEST Main menu");
  EUGC_SetMenuTitle( &main_menu, working_string);

  EUGC_ProcessMenuLoop( &gfx_ctrl, &main_menu);

  /*** Remove any still held buttons ***/
  while(PAD_ButtonsHeld(0)) VIDEO_WaitVSync();

  /*** Reinitialize current TV mode ***/
  if (use_480i) gfx_ctrl.vmode = tvmodes[gc_pal*3 + 2];
  else gfx_ctrl.vmode = tvmodes[gc_pal*3 + interlaced];
  gfx_ctrl.vmode->viWidth = aspect ? 720 : 640;
  gfx_ctrl.vmode->viXOrigin = (720 - gfx_ctrl.vmode->viWidth) / 2;
  VIDEO_Configure (gfx_ctrl.vmode);
  VIDEO_WaitVSync();
  
  if (gfx_ctrl.vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
  else while (VIDEO_GetNextField())  VIDEO_WaitVSync();
  odd_frame = 0;

  /*** Reinitalize GX ***/ 
  GX_SetViewport (0.0F, 0.0F, gfx_ctrl.vmode->fbWidth, gfx_ctrl.vmode->efbHeight, 0.0F, 1.0F);
  GX_SetScissor (0, 0, gfx_ctrl.vmode->fbWidth, gfx_ctrl.vmode->efbHeight);
  f32 yScale = GX_GetYScaleFactor(gfx_ctrl.vmode->efbHeight, gfx_ctrl.vmode->xfbHeight);
  u16 xfbHeight  = GX_SetDispCopyYScale (yScale);
  GX_SetDispCopySrc (0, 0, gfx_ctrl.vmode->fbWidth, gfx_ctrl.vmode->efbHeight);
  GX_SetDispCopyDst (gfx_ctrl.vmode->fbWidth, xfbHeight);
  GX_SetCopyFilter (gfx_ctrl.vmode->aa, gfx_ctrl.vmode->sample_pattern, (gfx_ctrl.vmode->efbHeight == gfx_ctrl.vmode->xfbHeight) ? GX_TRUE : GX_FALSE, gfx_ctrl.vmode->vfilter);
  GX_SetFieldMode (gfx_ctrl.vmode->field_rendering, ((gfx_ctrl.vmode->viHeight == 2 * gfx_ctrl.vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt (gfx_ctrl.vmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
  GX_SetDither(GX_DISABLE);
  guOrtho(p, gfx_ctrl.vmode->efbHeight/2, -(gfx_ctrl.vmode->efbHeight/2), -(gfx_ctrl.vmode->fbWidth/2), gfx_ctrl.vmode->fbWidth/2, 100, 1000);
  GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

  /*** Stop the DVD from causing clicks while playing ***/
  uselessinquiry ();      
}
