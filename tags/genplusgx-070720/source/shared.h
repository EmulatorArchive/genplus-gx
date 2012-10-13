#ifndef _SHARED_H_
#define _SHARED_H_

#ifndef NGC
#define NGC 1
#endif

#include <stdio.h>
#include <math.h>
#include <zlib.h>

#include "types.h"
#include "macros.h"
#include "m68k.h"
#include "z80.h"
#include "genesis.h"
#include "vdp.h"
#include "render.h"
#include "mem68k.h"
#include "memz80.h"
#include "membnk.h"
#include "memvdp.h"
#include "system.h"
#include "io.h"
#include "input.h"
#include "sound.h"
#include "ym2612.h"
#include "fm.h"
#include "sn76489.h"
#include "sn76496.h"
#include "state.h"
#include "sram.h"
#include "ssf2tnc.h"

#ifndef NGC
#include "unzip.h"
#include "fileio.h"
#include "loadrom.h"
#else
#include "osd.h"
#endif

extern uint8 FM_GENS;
extern uint8 PSG_MAME;
#endif /* _SHARED_H_ */

