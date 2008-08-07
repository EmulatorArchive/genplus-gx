/*
  File: fm_ym2162.h -- header file for software emulation for FM sound generator

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
