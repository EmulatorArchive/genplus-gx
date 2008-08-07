/*
  File: fm_ym2162.h -- header file for software emulation for FM sound generator

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_
#include "shared.h"

/* compiler dependence */
#ifndef INLINE
#define INLINE static __inline__
#endif


int YM2612Init(int baseclock, int rate);
int YM2612ResetChip(void);
void YM2612UpdateOne(int **buffer, int length);
int YM2612Write(unsigned char a, unsigned char v);
int YM2612Read(void);


#endif /* _H_FM_FM_ */
