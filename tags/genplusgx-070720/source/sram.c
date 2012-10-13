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

#include "shared.h"

T_SRAM sram;
T_EEPROM eeprom;
uint32 lastSSRamWrite = 0xffff0000;

/****************************************************************************
 * A quick guide to SRAM on the Genesis
 *
 * This is based on observations only - it may be completely wrong!
 *
 * The SRAM definition is held at offset 0x1b0 of the ROM header.
 * From looking at several ROMS, an ID appears:
 *
 * 0x1b0 : 0x52 0x41 0xF8 0x20 0x00200001 0x0020ffff
 *
 * Assuming 64k SRAM / Battery RAM throughout
 ****************************************************************************/
void DecodeSRAM ()
{
  int32 *st = (int32 *) & cart_rom[0x1b4];

  memset (&sram, 0, sizeof (T_SRAM));
  memset(&eeprom, 0, sizeof(T_EEPROM));

  if ((cart_rom[0x1b0] == 0x52) && (cart_rom[0x1b1] == 0x41))
  {
      sram.start = st[0];
      sram.end = st[1];
	  sram.on = 1;
	  sram.write = 1;
	  sram.detected = 1;
  }
  else
  {
      sram.start = 0x200000;
      sram.end = 0x20ffff;

  	  if (genromsize <= 0x200000)
  	  {
	  	 sram.on = 1;
	  	 sram.write = 1;
  	  }
  }

  if ((sram.start > sram.end) || ((sram.end - sram.start) >= 0x10000))
    sram.end = sram.start + 0xffff;

  

  sram.start &= 0xfffffffe;
  sram.end |= 1;

  if (sram.end - sram.start <= 2) sram.custom = 1;
  else sram.custom = 0;

  sram.crc = crc32 (0, &sram.sram[0], 0x10000);
}

// sram_reg: LAtd sela (L=pending SCL, A=pending SDA, t=type(1==uses 0x200000 for SCL and 2K bytes),
//                      d=SRAM was detected (header or by access), s=started, e=save is EEPROM, l=old SCL, a=old SDA)
void SRAMWriteEEPROM(unsigned int d) // ???? ??la (l=SCL, a=SDA)
{
  unsigned int sreg = eeprom.sram_reg, saddr = eeprom.sram_addr, scyc = eeprom.sram_cycle, ssa = eeprom.sram_slave;

  sreg |= saddr&0xc000; // we store word count in add reg: dw?a aaaa ...  (d=word count detected, w=words(0==use 2 words, else 1))
  saddr&=0x1fff;

  if (eeprom.old_scl && (d & 2))
  {
      // SCL was and is still high..
      if (eeprom.old_sda && !(d&1))
      {
          // ..and SDA went low, means it's a start command, so clear internal addr reg and clock counter
          if (!(sreg&0x8000) && (scyc >= 9))
          {
	      if(scyc != 28) sreg |= 0x4000; // 1 word
          sreg |= 0x8000;
	  }
      //saddr = 0;
      scyc = 0;
      eeprom.started = 1;
    }
    else if (!eeprom.old_sda && (d&1))
    {
      // SDA went high == stop command
      eeprom.started = 0;
    }
  }

  else if (eeprom.started && !eeprom.old_scl && (d&2))
  {
      // we are started and SCL went high - next cycle
      scyc++; // pre-increment

	  if(eeprom.type)
      {
          // X24C02+
	      if((ssa&1) && scyc == 18)
          {
              scyc = 9;
		      saddr++; // next address in read mode
		      if (sreg&0x4000) saddr&=0xff;
              else saddr&=0x1fff; // mask
	      }

	      else if ((sreg&0x4000) && scyc == 27) scyc = 18;
	  
          else if (scyc == 36) scyc = 27;
	  }
      else
      {
	      // X24C01
          if(scyc == 18)
          {
              scyc = 9;  // wrap
              if(saddr&1)
              {
                  saddr+=2;
                  saddr&=0xff;
              } // next addr in read mode
	      }
	  }
  }

  else if (eeprom.started && eeprom.old_scl && !(d&2))
  {
       // we are started and SCL went low (falling edge)
       if(eeprom.type)
       {
           // X24C02+
	       if ((scyc == 9) || (scyc == 18) || (scyc == 27)); // ACK cycles
	       
           else if ((!(sreg&0x4000) && (scyc > 27)) || ((sreg&0x4000) && (scyc > 18)))
           {
               if (!(ssa&1))
               {
                   // data write
                   unsigned char *pm = sram.sram + saddr;
                   *pm <<= 1;
                   *pm |= d&1;

                   if ((scyc == 26) || (scyc == 35))
                   {
                       saddr = (saddr&~0xf) | ((saddr+1)&0xf); // only 4 (?) lowest bits are incremented
                   }
               }
           } 
           else if (scyc > 9)
           {
               if (!(ssa&1))
               {
                   // we latch another addr bit
		           saddr<<=1;
		           if (sreg&0x4000) saddr&=0xff;
                   else saddr&=0x1fff; // mask
		           saddr |= d&1;
               }
           }

           else
           {
	           // slave address
		       ssa<<=1;
               ssa |= d&1;
           }
	   }

       else
       {
	       // X24C01
           if (scyc == 9); // ACK cycle, do nothing
           else if (scyc > 9)
           {
               if (!(saddr&1))
               {
                   // data write
                   unsigned char *pm = sram.sram + (saddr>>1);
                   *pm <<= 1;
                   *pm |= d&1;
                   
                   if (scyc == 17)
                   {
                       saddr=(saddr&0xf9)|((saddr+2)&6); // only 2 lowest bits are incremented
                   }
               }
           }
           else
           {
               // we latch another addr bit
               saddr<<=1;
               saddr|=d&1;
               saddr&=0xff;
           }
	  }
  }

  eeprom.old_scl = (d >> 1) & 1;
  eeprom.old_sda = d&1;
  eeprom.sram_reg  = (unsigned char)  sreg;
  eeprom.sram_addr = (unsigned short)(saddr|(sreg&0xc000));
  eeprom.sram_cycle= (unsigned char)  scyc;
  eeprom.sram_slave= (unsigned char)  ssa;
}

unsigned int SRAMReadEEPROM()
{
  unsigned int shift, d=0;
  unsigned int sreg, saddr, scyc, ssa;

  // flush last pending write
  SRAMWriteEEPROM(eeprom.sda + (eeprom.scl*2)); // execute pending

  sreg  = eeprom.sram_reg;
  saddr = eeprom.sram_addr & 0x1fff;
  scyc  = eeprom.sram_cycle;
  ssa   = eeprom.sram_slave;

  if ((total_m68k + count_m68k + m68k_cycles_run() - lastSSRamWrite) < 46)
  {
      // data was just written, there was no time to respond (used by sports games)
      d = eeprom.sda;
  }
  else if (eeprom.started && (scyc > 9) && (scyc != 18) && (scyc != 27))
  {
      // started and first command word received
      shift = 17 - scyc;
      if (eeprom.type)
      {
	      // X24C02+
          if (ssa&1) d = (sram.sram[saddr]>>shift)&1;
	  }
      else
      {
	      // X24C01
          if (saddr&1) d = (sram.sram[saddr>>1]>>shift)&1;
	  }
  }
  
  return d;
}

void SRAMUpdPending(unsigned int a, unsigned int d)
{
  if(!(a&1)) eeprom.type = 1;

  if (eeprom.type)
  {
      // address through 0x200000 used for CLK
      if (!(a&1)) eeprom.scl = d&1;
      else eeprom.sda = d&1;
  }
  else
  {
    eeprom.scl = (d >> 1)& 1;
	eeprom.sda = d & 1;
  }
}
