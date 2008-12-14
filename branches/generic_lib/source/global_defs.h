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

#ifndef __GLOBAL_DEFS_H
#define __GLOBAL_DEFS_H
 
#include "gctypes.h"
#include "eugc_typedef.h"
#include "ogc/gx.h"

/* Global defines */
#define PSOSDLOADID 0x7c6000a6
#define ROMOFFSET 0x80640000

/* New texture based scaler */
#define HASPECT 320
#define VASPECT 240

/*** GX ***/
#define TEX_WIDTH 360
#define TEX_HEIGHT 576
#define DEFAULT_FIFO_SIZE 256 * 1024

/* Genesis specifc defines */
#define GENESIS_CART_ROM_SIZE  ((unsigned int)0x500000)
#define GENESIS_BIOS_ROM_SIZE  ((unsigned int)0x800)

typedef struct
{
  unsigned long l, u;
} tb_t;

typedef struct tagcamera
{
  Vector pos;
  Vector up;
  Vector view;
} camera;

typedef struct __cart_data_t {
  int      genromsize;
  uint8    cart_rom[GENESIS_CART_ROM_SIZE];
  uint8    bios_rom[0x800];
} GEN_CART_DATA_t;
  
#endif

