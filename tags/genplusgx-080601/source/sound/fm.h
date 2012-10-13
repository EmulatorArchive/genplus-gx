/*
**
** File: fm_ym2612.h -- header for ym2612.c
** software implementation of Yamaha FM sound generator
**
** Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 1.4 (final beta)
**
*/

#ifndef _H_FM_FM_
#define _H_FM_FM_

/* compiler dependence */
#ifndef INLINE
#define INLINE static __inline__
#endif


extern int YM2612Init(int baseclock, int rate);
extern int YM2612ResetChip(void);
extern void YM2612UpdateOne(int **buffer, int length);
extern int YM2612Write(unsigned char a, unsigned char v);
extern int YM2612Read(void);


#endif /* _H_FM_FM_ */
