/*
    membnk.c --
    Memory handlers Z80 access to the banked V-bus address space.
*/

#include "shared.h"

void z80_write_banked_memory(unsigned int address, unsigned int data)
{
    switch((address >> 21) & 7)
    {
        case 0: /* CARTRIDGE */
        case 1:
			/* External RAM */
			if (sram.on && sram.write)
			{
				if (address >= sram.start && address <= sram.end)
				{
					if (sram.custom) EEPROM_Write(address, data);
                    else WRITE_BYTE(sram.sram, (address - sram.start) & 0xffff, data & 0xff);
					return;
				}
			}

			/* Unused */
            z80bank_unused_w(address, data);
            return;

        case 2: /* Unused */
        case 3:
            z80bank_unused_w(address, data);
            return;

        case 4: /* Invalid address */
            z80bank_lockup_w(address, data);
            return;

        case 5: /* SYSTEM I/O */
            
			/* Z80 */
            if(address <= 0xA0FFFF)
            {
                z80bank_lockup_w(address, data);
                return;
            }

			/* IO registers */
			if (address <= 0xA1001F) 
			{
	  			/* I/O chip only gets /LWR */
	      		if (address & 1) io_write ((address >> 1) & 0x0F, data);
				else  z80bank_unused_w(address, data);
		  		return;
	    	}

			/* CONTROL registers */
			if (address <= 0xA1FFFF)
            {
                switch((address >> 8) & 0xFF)
                {
					case 0x11:	/* BUSREQ */
						if (address == 0xA11100) gen_busreq_w (data & 1);
						else z80bank_unused_w(address, data);
                        return;

					case 0x12:	/* RESET */
					  	if (address == 0xA11200) gen_reset_w (data & 1);
						else z80bank_unused_w(address, data);
                        return;

					case 0x00:	/* ??? */
					case 0x10:	/* MEMORY MODE */
					case 0x20:	/* MEGA-CD */
					case 0x30:	/* TIME */
					case 0x40:	/* TMSS */
					case 0x41:	/* BOOTROM */
					case 0x50:  /* SVP REGISTERS */
                        z80bank_unused_w(address, data);
                        return;

					default:	/* Invalid address */
                        z80bank_lockup_w(address, data);
                        return;
                }
            }

			/* Invalid address */
			z80bank_lockup_w (address, data);
            return;

        case 6: /* VDP */
            z80bank_vdp_w(address, data);
            return;

        case 7: /* Work RAM */
       		WRITE_BYTE(work_ram, address & 0xFFFF, data);
			return;
    }
}


int z80_read_banked_memory(unsigned int address)
{
    switch((address >> 21) & 7)
    {
        case 0: /* CARTRIDGE */
        case 1:

			/* External RAM */
      		if (sram.on && (address >= sram.start) && (address <= sram.end))
				{
	   				if (sram.custom) return (EEPROM_Read(address)&0xffff);
					return READ_BYTE(sram.sram, (address - sram.start) & 0xffff);
				}

			/* ROM */
			if (address < genromsize) return READ_BYTE(cart_rom,address);
			
			/* Unused */
            return 0x00;

		case 2: /* Unused */
        case 3:
            return z80bank_unused_r(address);

        case 4: /* Invalid address */
            return z80bank_lockup_r(address);

        case 5: /* SYSTEM IO */
			
			/* Z80 */
			if (address <= 0xA0FFFF)
			{
				return z80bank_lockup_r (address);
			}

			/* IO registers */
			if (address <= 0xA1001F)
			{		  		
				return (io_read((address >> 1) & 0x0F));
			}

			/* CONTROL registers */
			if (address <= 0xA1FFFF)
            {
                switch((address >> 8) & 0xFF)
                {        
					case 0x00:	/* ??? */
					case 0x10:	/* MEMORY MODE */
                    case 0x11: /* /BUSACK */
				    case 0x12:	/* RESET */
				    case 0x20:	/* MEGA-CD */
					case 0x30:	/* TIME */
					case 0x40:	/* TMSS */
					case 0x41:	/* BOOTROM */
					case 0x50:	/* SVP REGISTERS */
						return  z80bank_unused_r(address);

                    default: /* Invalid address */
                        return z80bank_lockup_r(address);
                }
            }

			/* Invalid address */
			return z80bank_lockup_r(address);

        case 6: /* VDP */
            return z80bank_vdp_r(address);

        case 7: /* Work RAM */
            return READ_BYTE(work_ram, address & 0xFFFF);
    }

    return (-1);
}


void z80bank_vdp_w(int address, int data)
{
	/* Valid VDP addresses */
    if((address & 0xE700E0) == 0xC00000)
    {
		switch (address & 0x1C)
        {
		    case 0x00:		/* DATA */
                vdp_data_w(data << 8 | data);
                return;

		    case 0x04:		/* CTRL */
                vdp_ctrl_w(data << 8 | data);
                return;

		    case 0x08:		/* HVC */
            case 0x0C:
                z80bank_lockup_w(address, data);
                return;

			case 0x10:		/* PSG */
            case 0x14:
			   	if (address & 1) psg_write (0, data);
			   	else z80bank_unused_w(address, data);
                return;

            case 0x18: /* Unused */
                z80bank_unused_w(address, data);
                return;

            case 0x1C: /* Test register */
                vdp_test_w(data << 8 | data);
                return;
        }
    }

	/* Invalid VDP address */
	z80bank_lockup_w(address, data);
	return;
}


int z80bank_vdp_r(int address)
{
    if((address & 0xE700E0) == 0xC00000)
    {
        switch(address & 0x1F)
        {
            case 0x00: /* Data */
            case 0x02:
                return (vdp_data_r() >> 8);

            case 0x01: /* Data */
            case 0x03:
                return (vdp_data_r() & 0xFF);

            case 0x04: /* Control */
            case 0x06:
                return (0xFC | ((vdp_ctrl_r() >> 8) & 3));

            case 0x05: /* Control */
            case 0x07:
                return (vdp_ctrl_r() & 0xFF);

            case 0x08: /* HVC */
            case 0x0A:
            case 0x0C:
            case 0x0E:
                return (vdp_hvc_r() >> 8);

            case 0x09: /* HVC */
            case 0x0B:
            case 0x0D:
            case 0x0F:
                return (vdp_hvc_r() & 0xFF);

            case 0x10: /* Lockup */
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
                return z80bank_lockup_r(address);

            case 0x18: /* Unused */
            case 0x19:
            case 0x1A:
            case 0x1B:
            case 0x1C:
            case 0x1D:
            case 0x1E:
            case 0x1F:
                return z80bank_unused_r(address);
        }
    }

    /* Invalid VDP address */
    return z80bank_lockup_r(address);
}

/*
    Handlers for access to unused addresses and those which make the
    machine lock up.
*/
void z80bank_unused_w(int address, int data)
{
    //error("Z80 bank unused write %06X = %02X\n", address, data);
}

int z80bank_unused_r(int address)
{
    //error("Z80 bank unused read %06X\n", address);
    return 0xFF;
}

void z80bank_lockup_w(int address, int data)
{
    //error("Z80 bank lockup write %06X = %02X\n", address, data);
    gen_running = 0;
}

int z80bank_lockup_r(int address)
{
    //error("Z80 bank lockup read %06X\n", address);
    gen_running = 0;
    return 0xFF;
}
