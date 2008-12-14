/***************************************************************************************
 *  Genesis Plus 1.2a
 *  Video Display Processor (Rendering)
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  modified by Eke-Eke (compatibility fixes & additional code), GC/Wii port
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
 ****************************************************************************************/

#ifndef _RENDER_H_
#define _RENDER_H_

/* Global variables */
extern uint8 rgb565_norm[2][8];
extern uint8 rgb565_half[2][8];
extern uint8 rgb565_high[2][8];

/* Function prototypes */
extern int render_init(void);
extern void palette_init(void);
extern void render_reset(void);
extern void render_shutdown(void);
extern void render_line(int line, uint8 odd_frame);
#ifndef NGC
extern void color_update_8(int index, uint16 data);
extern void color_update_15(int index, uint16 data);
extern void color_update_32(int index, uint16 data);
#endif
extern void color_update_16(int index, uint16 data);
extern void parse_satb(int line);
extern void (*color_update)(int index, uint16 data);
extern void remap_buffer(int line,int width);

#endif /* _RENDER_H_ */

