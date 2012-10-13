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
 * NGC SRAM Expansion
 ***************************************************************************/

typedef struct
{
  uint8 detected;
  uint8 on;
  uint8 write;
  uint8 custom;
  int start;
  int end;
  int crc;
  uint8 sram[0x10000];
} T_SRAM;

typedef struct
{
   unsigned int scl;
   unsigned int sda;
   unsigned int type; //1==uses 0x200000 for SCL and 2K bytes
   unsigned int old_scl;
   unsigned int old_sda;
   unsigned short sram_addr;  // EEPROM address register
   unsigned char sram_cycle;  // EEPROM SRAM cycle number
   unsigned char sram_slave;  // EEPROM slave word for X24C02 and better SRAMs
   unsigned int started;
   unsigned int sram_reg;
} T_EEPROM;

extern void DecodeSRAM ();

extern void SRAMWriteEEPROM(unsigned int d);
extern unsigned int SRAMReadEEPROM();
extern void SRAMUpdPending(unsigned int a, unsigned int d);

extern T_SRAM sram;
extern T_EEPROM eeprom;
extern uint32 lastSSRamWrite; // used by serial SRAM code

