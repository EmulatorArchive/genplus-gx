/******************************************************************************
 *
 *  SMS Plus GX - Sega Master System / GameGear Emulator
 *
 *  SMS Plus - Sega Master System / GameGear Emulator
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ***************************************************************************/

#include "shared.h"
#include "font.h"

#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif

/* configurable keys */
#define KEY_BUTTONA 0   
#define KEY_BUTTONB 1
#define KEY_BUTTONC 2
#define KEY_START   3
#define KEY_MENU    4
#define KEY_BUTTONX 5  // 6-buttons only
#define KEY_BUTTONY 6
#define KEY_BUTTONZ 7

int ConfigRequested = 1;

/* gamepad assignation */
s8 pad_player[MAX_INPUTS];

/* gamepad default map (this can be reconfigured) */
u16 pad_keymap[MAX_INPUTS][MAX_KEYS] =
{
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_BUTTON_X, PAD_BUTTON_START, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_BUTTON_Y, PAD_TRIGGER_R},
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_BUTTON_X, PAD_BUTTON_START, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_BUTTON_Y, PAD_TRIGGER_R},
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_BUTTON_X, PAD_BUTTON_START, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_BUTTON_Y, PAD_TRIGGER_R},
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_BUTTON_X, PAD_BUTTON_START, PAD_TRIGGER_Z, PAD_TRIGGER_L, PAD_BUTTON_Y, PAD_TRIGGER_R}
};

/* gamepad available buttons */
static u16 pad_keys[8] =
{
  PAD_TRIGGER_Z,
  PAD_TRIGGER_R,
  PAD_TRIGGER_L,
  PAD_BUTTON_A,
  PAD_BUTTON_B,
  PAD_BUTTON_X,
  PAD_BUTTON_Y,
  PAD_BUTTON_START,
};

#ifdef HW_RVL

/* wiimote & classic controller assignation */
s8 wpad_player[MAX_INPUTS];
s8 classic_player[MAX_INPUTS];

/* wiimote default map (this can be reconfigured) */
u32 wpad_keymap[MAX_INPUTS*3][MAX_KEYS] =
{
  /* Wiimote #1 */
  {WPAD_BUTTON_A, WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_NUNCHUK_BUTTON_Z, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS,
   WPAD_CLASSIC_BUTTON_HOME, WPAD_CLASSIC_BUTTON_FULL_L, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_FULL_R},
  
  /* Wiimote #2 */
  {WPAD_BUTTON_A, WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_NUNCHUK_BUTTON_Z, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS,
   WPAD_CLASSIC_BUTTON_HOME, WPAD_CLASSIC_BUTTON_FULL_L, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_FULL_R},

  /* Wiimote #3 */
  {WPAD_BUTTON_A, WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_NUNCHUK_BUTTON_Z, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS,
   WPAD_CLASSIC_BUTTON_HOME, WPAD_CLASSIC_BUTTON_FULL_L, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_FULL_R},

  /* Wiimote #4 */
  {WPAD_BUTTON_A, WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_NUNCHUK_BUTTON_Z, WPAD_BUTTON_PLUS, WPAD_BUTTON_HOME, 0, 0, 0},
  {WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS,
   WPAD_CLASSIC_BUTTON_HOME, WPAD_CLASSIC_BUTTON_FULL_L, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_FULL_R},
};

/* directional buttons default mapping (this can NOT be reconfigured) */
#define PAD_UP    0   
#define PAD_DOWN  1
#define PAD_LEFT  2
#define PAD_RIGHT 3

static u32 wpad_dirmap[3][4] =
{
  {WPAD_BUTTON_RIGHT, WPAD_BUTTON_LEFT, WPAD_BUTTON_UP, WPAD_BUTTON_DOWN},                                // WIIMOTE only
  {WPAD_BUTTON_UP, WPAD_BUTTON_DOWN, WPAD_BUTTON_LEFT, WPAD_BUTTON_RIGHT},                                // WIIMOTE + NUNCHUK
  {WPAD_CLASSIC_BUTTON_UP, WPAD_CLASSIC_BUTTON_DOWN, WPAD_CLASSIC_BUTTON_LEFT, WPAD_CLASSIC_BUTTON_RIGHT} // CLASSIC
};

/* wiimote/expansion available buttons */
static u32 wpad_keys[20] =
{
  WPAD_BUTTON_2,
  WPAD_BUTTON_1,
  WPAD_BUTTON_B,
  WPAD_BUTTON_A,
  WPAD_BUTTON_MINUS,
  WPAD_BUTTON_HOME,
  WPAD_BUTTON_PLUS,
  WPAD_NUNCHUK_BUTTON_Z,
  WPAD_NUNCHUK_BUTTON_C,
  WPAD_CLASSIC_BUTTON_ZR,
  WPAD_CLASSIC_BUTTON_X,
  WPAD_CLASSIC_BUTTON_A,
  WPAD_CLASSIC_BUTTON_Y,
  WPAD_CLASSIC_BUTTON_B,
  WPAD_CLASSIC_BUTTON_ZL,
  WPAD_CLASSIC_BUTTON_FULL_R,
  WPAD_CLASSIC_BUTTON_PLUS,
  WPAD_CLASSIC_BUTTON_HOME,
  WPAD_CLASSIC_BUTTON_MINUS,
  WPAD_CLASSIC_BUTTON_FULL_L,
};
#endif

static const char *keys_name[MAX_KEYS] =
{
  "Button A",
  "Button B",
  "Button C",
  "Button START ",
  "Menu",
  "Button X",
  "Button Y",
  "Button Z",
};

/*******************************
  gamepad support
*******************************/
static void pad_config(int num)
{
  int i,j;
  u16 p;
  u8 quit;
  char msg[30];

  u32 connected = PAD_ScanPads();

  sprintf(msg,"PLAYER %d", pad_player[num]);
  WaitPrompt(msg);
  
  if ((connected & (1<<num)) == 0)
  {
    WaitPrompt("PAD is not connected !");
    return;
  }

  /* configure keys */
  for (i=0; i<MAX_KEYS; i++)
  {
    /* remove any pending keys */
    while (PAD_ButtonsHeld(num))
    {
      VIDEO_WaitVSync();
      PAD_ScanPads();
    }

    ClearScreen();
    sprintf(msg,"Press key for %s",keys_name[i]);
    WriteCentre(254, msg);
    SetScreen();

    /* check buttons state */
    quit = 0;
    while (quit == 0)
    {
      VIDEO_WaitVSync();
      PAD_ScanPads();
      p = PAD_ButtonsDown(num);

      for (j=0; j<8; j++)
      {
        if (p & pad_keys[j])
        {
           pad_keymap[num][i] = pad_keys[j];
           quit = 1;
           j = 9;   /* exit loop */
        }
      }
    }
  }
}

static void pad_update()
{
  int i;
  u16 p;
	s8 x,y;
  int joynum = 0;

  /* update PAD status */
  PAD_ScanPads();

  for (i=0; i<MAX_DEVICES; i++)
  {
    if (input.dev[i] != NO_DEVICE)
    {
      x = PAD_StickX (joynum);
      y = PAD_StickY (joynum);
      p = PAD_ButtonsHeld(joynum);

      if ((p & PAD_BUTTON_UP)    || (y >  70)) input.pad[i] |= INPUT_UP;
      else if ((p & PAD_BUTTON_DOWN)  || (y < -70)) input.pad[i] |= INPUT_DOWN;
      if ((p & PAD_BUTTON_LEFT)  || (x < -60)) input.pad[i] |= INPUT_LEFT;
      else if ((p & PAD_BUTTON_RIGHT) || (x >  60)) input.pad[i] |= INPUT_RIGHT;

      /* MENU */
      if (p & pad_keymap[joynum][KEY_MENU])
      {
        ConfigRequested = 1;
        return;
      }

      /* SOFTRESET */
      if ((p & PAD_TRIGGER_L) && (p & PAD_TRIGGER_Z))
      {
        resetline = (int) ((double) (lines_per_frame - 1) * rand() / (RAND_MAX + 1.0));
      }

      /* BUTTONS */
      if (p & pad_keymap[joynum][KEY_BUTTONA]) input.pad[i]  |= INPUT_A;
      if (p & pad_keymap[joynum][KEY_BUTTONB]) input.pad[i]  |= INPUT_B;
      if (p & pad_keymap[joynum][KEY_BUTTONC]) input.pad[i]  |= INPUT_C;
      if (p & pad_keymap[joynum][KEY_BUTTONX]) input.pad[i]  |= INPUT_X;
      if (p & pad_keymap[joynum][KEY_BUTTONY]) input.pad[i]  |= INPUT_Y;
      if (p & pad_keymap[joynum][KEY_BUTTONZ]) input.pad[i]  |= INPUT_Z;
      if (p & pad_keymap[joynum][KEY_START])   input.pad[i]  |= INPUT_START;

      if (input.dev[i] == DEVICE_LIGHTGUN) lightgun_set();

      /* next device */
      joynum ++;
      if (joynum > MAX_INPUTS) i = MAX_DEVICES; /* no more gamepads left, leave loop */
    }
  }
}

/*******************************
  wiimote support
*******************************/
#ifdef HW_RVL
#define PI 3.14159265f

static s8 WPAD_StickX(u8 chan,u8 right)
{
  float mag = 0.0;
  float ang = 0.0;
  WPADData *data = WPAD_Data(chan);

  switch (data->exp.type)
  {
    case WPAD_EXP_NUNCHUK:
    case WPAD_EXP_GUITARHERO3:
      if (right == 0)
      {
        mag = data->exp.nunchuk.js.mag;
        ang = data->exp.nunchuk.js.ang;
      }
      break;

    case WPAD_EXP_CLASSIC:
      if (right == 0)
      {
        mag = data->exp.classic.ljs.mag;
        ang = data->exp.classic.ljs.ang;
      }
      else
      {
        mag = data->exp.classic.rjs.mag;
        ang = data->exp.classic.rjs.ang;
      }
      break;

    default:
      break;
  }

  /* calculate X value (angle need to be converted into radian) */
  if (mag > 1.0) mag = 1.0;
  else if (mag < -1.0) mag = -1.0;
  double val = mag * sin(PI * ang/180.0f);
 
  return (s8)(val * 128.0f);
}


static s8 WPAD_StickY(u8 chan, u8 right)
{
  float mag = 0.0;
  float ang = 0.0;
  WPADData *data = WPAD_Data(chan);

  switch (data->exp.type)
  {
    case WPAD_EXP_NUNCHUK:
    case WPAD_EXP_GUITARHERO3:
      if (right == 0)
      {
        mag = data->exp.nunchuk.js.mag;
        ang = data->exp.nunchuk.js.ang;
      }
      break;

    case WPAD_EXP_CLASSIC:
      if (right == 0)
      {
        mag = data->exp.classic.ljs.mag;
        ang = data->exp.classic.ljs.ang;
      }
      else
      {
        mag = data->exp.classic.rjs.mag;
        ang = data->exp.classic.rjs.ang;
      }
      break;

    default:
      break;
  }

  /* calculate X value (angle need to be converted into radian) */
  if (mag > 1.0) mag = 1.0;
  else if (mag < -1.0) mag = -1.0;
  double val = mag * cos(PI * ang/180.0f);
 
  return (s8)(val * 128.0f);
}

static void wpad_config(u8 pad)
{
  int i,j;
  u8 quit;
  u32 exp;
  char msg[30];

  sprintf(msg,"PLAYER %d (%d)", wpad_player[pad], classic_player[pad]);
  WaitPrompt(msg);

  /* check WPAD status */
  if (WPAD_Probe(pad, &exp) != WPAD_ERR_NONE)
  {
    WaitPrompt("Wiimote is not connected !");
    return;
  }

  /* index for wpad_keymap */
  u8 index = exp + (pad * 3);

  /* loop on each mapped keys */
  for (i=0; i<MAX_KEYS; i++)
  {
    /* remove any pending buttons */
    while (WPAD_ButtonsHeld(pad))
    {
      WPAD_ScanPads();
      VIDEO_WaitVSync();
    }

    /* user information */
    ClearScreen();
    sprintf(msg,"Press key for %s",keys_name[i]);
    WriteCentre(254, msg);
    SetScreen();

    /* wait for input */
    quit = 0;
    while (quit == 0)
    {
      WPAD_ScanPads();

      /* get buttons */
      for (j=0; j<20; j++)
      {
        if (WPAD_ButtonsDown(pad) & wpad_keys[j])
        {
          wpad_keymap[index][i]  = wpad_keys[j];
          quit = 1;
          j = 20;    /* leave loop */
        }
      }
    } /* wait for input */ 
  } /* loop for all keys */

  /* removed any pending buttons */
  while (WPAD_ButtonsHeld(pad))
  {
    WPAD_ScanPads();
    VIDEO_WaitVSync();
  }
}

static void wpad_update(void)
{
  int i;
  u32 exp;
  u32 p;
  s8 x,y;
  int joynum = 0;

  /* update WPAD data */
  WPAD_ScanPads();

  for (i=0; i<MAX_DEVICES; i++)
  {
    if (input.dev[i] != NO_DEVICE)
    {
      /* check WPAD status */
      if (WPAD_Probe(joynum, &exp) == WPAD_ERR_NONE)
      {
        p = WPAD_ButtonsHeld(joynum);
        x = WPAD_StickX(joynum,0);
        y = WPAD_StickY(joynum,0);

        /* directional buttons */
        if ((p & wpad_dirmap[exp][PAD_UP])          || (y >  70)) input.pad[i] |= INPUT_UP;
        else if ((p & wpad_dirmap[exp][PAD_DOWN])   || (y < -70)) input.pad[i] |= INPUT_DOWN;
        if ((p & wpad_dirmap[exp][PAD_LEFT])        || (x < -60)) input.pad[i] |= INPUT_LEFT;
        else if ((p & wpad_dirmap[exp][PAD_RIGHT])  || (x >  60)) input.pad[i] |= INPUT_RIGHT;

        /* retrieve current key mapping */
        u8 index = exp + (3 * joynum);
     
        /* MENU */
        if ((p & wpad_keymap[index][KEY_MENU]) || (p & WPAD_BUTTON_HOME))
        {
          ConfigRequested = 1;
          return;
        }

        /* SOFTRESET */
        if ((p & WPAD_CLASSIC_BUTTON_MINUS) || (p & WPAD_BUTTON_MINUS))
        {
          set_softreset();
        }

        /* BUTTONS */
        if (p & wpad_keymap[index][KEY_BUTTONA]) input.pad[i]  |= INPUT_A;
        if (p & wpad_keymap[index][KEY_BUTTONB]) input.pad[i]  |= INPUT_B;
        if (p & wpad_keymap[index][KEY_BUTTONC]) input.pad[i]  |= INPUT_C;
        if (p & wpad_keymap[index][KEY_BUTTONX]) input.pad[i]  |= INPUT_X;
        if (p & wpad_keymap[index][KEY_BUTTONY]) input.pad[i]  |= INPUT_Y;
        if (p & wpad_keymap[index][KEY_BUTTONZ]) input.pad[i]  |= INPUT_Z;
        if (p & wpad_keymap[index][KEY_START])   input.pad[i]  |= INPUT_START;

        if (input.dev[i] == DEVICE_LIGHTGUN) lightgun_set();
      }

      /* next device */
      joynum ++;
      if (joynum > MAX_INPUTS) i = MAX_DEVICES; /* no more wiimotes left, leave loop */
    }
  }
}

#endif

/*****************************************************************
                Generic input handlers 
******************************************************************/

void ogc_input__init(void)
{
  PAD_Init ();
#ifdef HW_RVL
  WPAD_Init();
	WPAD_SetIdleTimeout(60);
  WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
  WPAD_SetVRes(WPAD_CHAN_ALL,640,480);
#endif
}

void ogc_input__reset(void)
{
  int i,count,quit;
  int current = 0;
  
  /* set GAMEPADS */
  u32 connected = PAD_ScanPads();
  count = 0;
  for (i=0; i<4 ;i++)
  {
    pad_player[i] = -1;
    
    if (connected & (1<<i))
    {
      quit=0;
      while ((quit == 0) && (count < 8))
      {
        if (input.dev[current] != NO_DEVICE)
        {
          pad_player[i] = current;
          quit = 1;
        }

        current = (current + 1) & 7;
        count ++;
      }
    }
  }

#ifdef HW_RVL
  /* set WIIMOTES */
  u32 exp[4];
  count = 0;
  for (i=0; i<4 ;i++)
  {
    wpad_player[i] = -1;
    exp[i] = 0;
    
    if (WPAD_Probe(i, &exp[i]) == WPAD_ERR_NONE)
    {
      quit=0;
      while ((quit == 0) && (count < 8))
      {
        if (input.dev[current] != NO_DEVICE)
        {
          wpad_player[i] = current;
          quit = 1;
        }

        current = (current + 1) & 7;
        count ++;
      }
    }
  }

  /* set CLASSIC CONTROLLERS */
  count = 0;
  for (i=0; i<4 ;i++)
  {
    classic_player[i] = -1;
    
    if (exp[i] == WPAD_EXP_CLASSIC)
    {
      quit=0;
      while ((quit == 0) && (count < 8))
      {
        if (input.dev[current] != NO_DEVICE)
        {
          classic_player[i] = current;
          quit = 1;
        }

        current = (current + 1) & 7;
        count ++;
      }
    }
  }
#endif
}

void ogc_input__update(void)
{
  int i;
  
  /* reset inputs */
  for (i=0; i<MAX_DEVICES; i++) input.pad[i] = 0;

  pad_update();
#ifdef HW_RVL
  wpad_update();
#endif
}

void ogc_input__config(u8 pad, u8 type)
{
  switch (type)
  {
    case 0:
      pad_config(pad);
      break;
    
#ifdef HW_RVL
    case 1:
      wpad_config(pad);
      break;
#endif
    
    default:
      break;
  }
}

u16 ogc_input__getMenuButtons(void)
{
  /* gamecube pad */
  PAD_ScanPads();
  u16 p = PAD_ButtonsDown(0);
  s8 x  = PAD_StickX(0);
  s8 y  = PAD_StickY(0);
  if (x > 70) p |= PAD_BUTTON_RIGHT;
  else if (x < -70) p |= PAD_BUTTON_LEFT;
	if (y > 60) p |= PAD_BUTTON_UP;
  else if (y < -60) p |= PAD_BUTTON_DOWN;

#ifdef HW_RVL
  struct ir_t ir;
  u32 exp;
  if (WPAD_Probe(0, &exp) == WPAD_ERR_NONE)
  {
    WPAD_ScanPads();
    u32 q = WPAD_ButtonsDown(0);
    x = WPAD_StickX(0, 0);
    y = WPAD_StickY(0, 0);

    /* default directions */
    WPAD_IR(0, &ir);
    if (ir.valid)
    {
      /* Wiimote is pointed toward screen */
      if ((q & WPAD_BUTTON_UP) || (y > 70))         p |= PAD_BUTTON_UP;
      else if ((q & WPAD_BUTTON_DOWN) || (y < -70)) p |= PAD_BUTTON_DOWN;
      if ((q & WPAD_BUTTON_LEFT) || (x < -60))      p |= PAD_BUTTON_LEFT;
      else if ((q & WPAD_BUTTON_RIGHT) || (x > 60)) p |= PAD_BUTTON_RIGHT;
    }
    else
    {
      /* Wiimote is used horizontally */
      if ((q & WPAD_BUTTON_RIGHT) || (y > 70))         p |= PAD_BUTTON_UP;
      else if ((q & WPAD_BUTTON_LEFT) || (y < -70)) p |= PAD_BUTTON_DOWN;
      if ((q & WPAD_BUTTON_UP) || (x < -60))      p |= PAD_BUTTON_LEFT;
      else if ((q & WPAD_BUTTON_DOWN) || (x > 60)) p |= PAD_BUTTON_RIGHT;
    }

    /* default keys */
    if (q & WPAD_BUTTON_MINUS)  p |= PAD_TRIGGER_L;
    if (q & WPAD_BUTTON_PLUS)   p |= PAD_TRIGGER_R;
    if (q & WPAD_BUTTON_A)      p |= PAD_BUTTON_A;
    if (q & WPAD_BUTTON_B)      p |= PAD_BUTTON_B;
    if (q & WPAD_BUTTON_2)      p |= PAD_BUTTON_A;
    if (q & WPAD_BUTTON_1)      p |= PAD_BUTTON_B;
    if (q & WPAD_BUTTON_HOME)   p |= PAD_TRIGGER_Z;

    /* classic controller expansion */
    if (exp == WPAD_EXP_CLASSIC)
    {
      if (q & WPAD_CLASSIC_BUTTON_UP)         p |= PAD_BUTTON_UP;
      else if (q & WPAD_CLASSIC_BUTTON_DOWN)  p |= PAD_BUTTON_DOWN;
      if (q & WPAD_CLASSIC_BUTTON_LEFT)       p |= PAD_BUTTON_LEFT;
      else if (q & WPAD_CLASSIC_BUTTON_RIGHT) p |= PAD_BUTTON_RIGHT;

      if (q & WPAD_CLASSIC_BUTTON_FULL_L) p |= PAD_TRIGGER_L;
      if (q & WPAD_CLASSIC_BUTTON_FULL_R) p |= PAD_TRIGGER_R;
      if (q & WPAD_CLASSIC_BUTTON_A)      p |= PAD_BUTTON_A;
      if (q & WPAD_CLASSIC_BUTTON_B)      p |= PAD_BUTTON_B;
      if (q & WPAD_CLASSIC_BUTTON_HOME)   p |= PAD_TRIGGER_Z;
    }
  }
#endif

  return p;
}
