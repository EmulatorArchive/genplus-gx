
#ifndef _OSD_H_
#define _OSD_H_

#define NGC 1

#include <gccore.h>
#include <ogcsys.h>
#include <sdcard.h>
#include <gcaram.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void error (char *format, ...);
extern void draw_init (void);
extern void viewport_init ();
extern void genesis_set_region ();
extern s16 square[];
extern s16 xshift;
extern s16 yshift;
extern s16 xscale;
extern s16 yscale;
extern uint8 overscan;
extern uint8 use_480i;
extern uint8 tv_mode;
extern uint8 gc_pal;
extern uint8 aspect;
extern unsigned int *xfb[2];
extern int whichfb;
extern GXRModeObj *tvmodes[6];

#endif /* _OSD_H_ */
