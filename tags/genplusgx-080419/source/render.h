
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

#endif /* _RENDER_H_ */

