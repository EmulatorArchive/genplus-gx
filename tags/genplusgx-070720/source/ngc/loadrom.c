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
#include "rominfo.h"
#include <ctype.h>

extern uint8 region_detect;
extern uint8 cpu_detect;

/* 05/05/2006: new detection routine */
void genesis_set_region ()
{
	/* country codes used to differentiate region */
	/* 0001 = japan ntsc (1) */
	/* 0010 = japan  pal (2) */
	/* 0100 = usa        (4) */
	/* 1000 = europe     (8) */

	int country = 0;
	int i = 0;
	char c;

	/* reading header to find the country */
	if (!strnicmp(rominfo.country, "eur", 3)) country |= 8;
	else if (!strnicmp(rominfo.country, "usa", 3)) country |= 4;
	else if (!strnicmp(rominfo.country, "jap", 3)) country |= 1;

	else for(i = 0; i < 4; i++)
	{
		c = toupper((int)rominfo.country[i]);
		if (c == 'U') country |= 4;
		else if (c == 'J') country |= 1;
		else if (c == 'E') country |= 8;
		else if (c < 16) country |= c;
		else if ((c >= '0') && (c <= '9')) country |= c - '0';
		else if ((c >= 'A') && (c <= 'F')) country |= c - 'A' + 10;
	}

	/* automatic detection */
	/* setting region */
	/* this is used by IO register */
	if (country & 4) region_code = REGION_USA;
	else if (country & 8) region_code = REGION_EUROPE;
	else if (country & 1) region_code = REGION_JAPAN_NTSC;
	else if (country & 2) region_code = REGION_JAPAN_PAL;
	else region_code = REGION_USA;

	/* cpu mode: PAL or NTSC */
	if ((region_code == REGION_EUROPE) || (region_code == REGION_JAPAN_PAL)) vdp_pal = 1;
	else vdp_pal = 0;

	/* Force region setting */
	if (region_detect == 1) region_code = REGION_USA;
	else if (region_detect == 2) region_code = REGION_EUROPE;
	else if (region_detect == 3) region_code = REGION_JAPAN_NTSC;
	else if (region_detect == 4) region_code = REGION_JAPAN_PAL;

	/* Force CPU mode */
	if (cpu_detect == 1) vdp_pal = 0;
	else if (cpu_detect == 2) vdp_pal = 1;
}



static uint8 block[0x4000];

void deinterleave_block (uint8 * src)
{
  int i;
  memcpy (block, src, 0x4000);
  for (i = 0; i < 0x2000; i += 1)
  {
      src[i * 2 + 0] = block[0x2000 + (i)];
      src[i * 2 + 1] = block[0x0000 + (i)];
  }
}

/*
 * load_memrom
 * softdev 12 March 2006
 * Changed from using ROM buffer to a single copy in cart_rom
 *
 * Saving ROM size in bytes :)
 * Required for remote loading.
 *
 * WIP3 - Removed 5Mb SSF2TNC from main memory to Audio ROM
 */
void load_memrom (int size)
{
  int offset = 0;
  
  SSF2TNC = 0;

  /* Support for interleaved roms */
  if ((size / 512) & 1)
  {
    int i;
    size -= 512;
    offset += 512;

    for (i = 0; i < (size / 0x4000); i += 1)
	{
	  deinterleave_block (cart_rom + offset + (i * 0x4000));
	}
  }

  if (size > 0x500000) size = 0x500000;
  if (offset) memcpy (cart_rom, cart_rom + offset, size);
  if (size > 0x400000) SSF2TNC = 1; /*** Assume SSF2TNC (mapped ROM) ***/
  
  genromsize = size;
 
  /*** Clear out space ***/
  if (size < 0x500000) memset (cart_rom + size, 0, 0x500000 - size);
  return;
}

/*** Reloadrom
	 performs some initialization before running the new rom
 ***/
extern void decode_ggcodes ();
extern void ClearGGCodes ();
extern void sram_autoload();
extern uint8 autoload;

void reloadrom ()
{
	load_memrom (genromsize);
    DecodeSRAM ();			/* SRAM Infos from ROM header */
    getrominfo (cart_rom);	/* Other Infos from ROM Header */
    genesis_set_region ();	/* Region Detection */

	system_init ();
	audio_init(48000);
	ClearGGCodes ();		/* Game Genie */
	decode_ggcodes ();

	system_reset ();
	if (autoload) sram_autoload();
}
