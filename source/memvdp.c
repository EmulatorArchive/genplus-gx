/*
    memvdp.c --
    Memory handlers for when the VDP reads the V-bus during DMA.
*/

#include "shared.h"

unsigned int vdp_dma_r(unsigned int address)
{
    switch((address >> 21) & 7)
    {
        case 0: /* Cartridge ROM */
        case 1: 
			/* Backup RAM */
      		if (sram.on)
			{
				if (address >= sram.start && address <= sram.end)
				{
	   				if (sram.custom) return (EEPROM_Read(address) & 0xffff);
					return *(uint16 *)(sram.sram + ((address - sram.start) & 0xffff));
		   		}
			}

			/* ROM Data */
			if (address < rom_size) return *(uint16 *)(rom_readmap[address >> 19] + (address & 0x7ffff));

			/* j-CART */
			if (j_cart && ((address == 0x3FFFFE) || (address == 0x38FFFE)))
				return (gamepad_read(5) | (gamepad_read(6) << 8));	
			
			/* Virtua Racing SVP */
			if (address == 0x30fe02) return 0x01;

			/* default */
			return 0x00;	

        case 2: /* Unused */
        case 3: 
            return 0xFF00;

        case 4: /* Work RAM */
        case 6:
        case 7:
      		return *(uint16 *)(work_ram + (address & 0xffff));

        case 5: /* Z80 area and I/O chip */
            /* Z80 area always returns $FFFF */
            if(address <= 0xA0FFFF)
            {
                /* Return $FFFF only when the Z80 isn't hogging the Z-bus.
                   (e.g. Z80 isn't reset and 68000 has the bus) */
				return (zbusack == 0) ? 0xFFFF : *(uint16 *)(work_ram + (address & 0xffff));
			}
			
			/* The I/O chip and work RAM try to drive the data bus which
               results in both values being combined in random ways when read.
               We return the I/O chip values which seem to have precedence, */
			else if (address <= 0xA1001F)
			{
                uint8 temp = io_read((address >> 1) & 0x0F);
                return (temp << 8 | temp);
            }
            /* All remaining locations access work RAM */
			else return *(uint16 *)(work_ram + (address & 0xffff));
	}

    return -1;
}

