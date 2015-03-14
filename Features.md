Genesis Plus main goal is to provide the most complete & accurate emulation of the Sega Genesis/Megadrive hardware.

The original emulation core from [Charles Mac Donald](http://cgfm2.emuviews.com/) has been largely modified to improve overall accuracy and therefore compatibility, as well as adding emulation of various peripherals, cartridge and system hardware.

![http://gxdev.files.wordpress.com/2007/08/sega_megadrive_2.thumbnail.jpg](http://gxdev.files.wordpress.com/2007/08/sega_megadrive_2.thumbnail.jpg)

### Very Accurate & Full Speed Sega 8-bit / 16-bit emulation ###
  * accurate emulation of SG-1000, Mark-III, Master System (I & II), Game Gear, Genesis / Mega Drive, Sega / Mega CD hardware models (incl. backwards compatibility modes)
  * NTSC (60Hz) & PAL (50Hz) video hardware emulation
  * accurate CDD, CDC & GFX chip emulation (Sega/Mega CD)
  * CD-DA fader emulation (Sega/Mega CD)
  * Mode 1 cartridge support (Sega/Mega CD)
  * highly accurate 68000 & Z80 CPU emulation
  * highly accurate VDP emulation (all rendering modes, mid-line changes, undocumented registers,…) & timings (HBLANK, DMA, FIFO, HV interrupts,…)
  * sample-accurate YM2612,YM2413, PSG, & PCM emulation (all sound chips are running at the original frequency)
  * cycle-accurate chip synchronization (68000’s/Z80/YM2612/PSG/PCM)
  * high-quality audio resampling using Blip Buffer
  * basic hardware latency emulation (VDP/68k, Z80/68k)
  * full overscan area emulation (horizontal & vertical color borders)
  * optional Game Gear extended screen mode
  * internal BOOT ROM support (Master System, Genesis / Mega Drive, Sega / Mega CD)
  * optional TMSS hardware emulation (Genesis / Mega Drive)
  * support for Blargg's software NTSC filters
  * preliminary PICO emulation
  * support for raw (.bin, .gen, .md, .sms, .gg & .sg) and interleaved (.smd & .mdx) ROM files
  * support for CUE+BIN, ISO+OGG & ISO+WAV CD image files


http://gxdev.files.wordpress.com/2007/12/street.gif?w=320&h=240

### Support for various peripherals ###
  * 2-buttons, 3-buttons & 6-buttons Control Pads
  * Sega Team Player & EA 4-Way Play multitaps
  * Sega Mouse
  * Sega Paddle Control
  * Sega Sports Pad
  * Terebi Oekaki
  * Sega Light Phaser
  * Sega Menacer
  * Konami Justifiers
  * Sega Activator
  * XE-1AP analog controller
  * Furrtek's homemade Master System multitap

![http://gxdev.wordpress.com/files/2008/08/menacer.jpg](http://gxdev.wordpress.com/files/2008/08/menacer.jpg)

### Support for various cartridges extra hardware ###
  * SVP DSP (Virtua Racing)
  * J-Cart adapter (Micro Machines & Pete Sampras series, Super Skidmarks)
  * Backup RAM (max. 64KB)
  * I2C (24Cxx), SPI (95xxx) & MicroWire (93C46) EEPROMs
  * RAM cart (max. 512KB) (Sega/Mega CD)
  * “official” ROM bankswitch hardware (Super Street Fighter 2)
  * “official” backup RAM bankswitch hardware (Phantasy Star 4, Legend of Thor, Sonic the Hedgehog 3)
  * all known unlicensed/pirate cartridges bankswitch & copy protection hardware
  * all known Master System & Game Gear cartridge “mappers” (incl. unlicensed Korean ones)
  * Game Genie & Action Replay hardware emulation
  * Sonic & Knuckles “Lock-On” hardware emulation
  * support for ROM image up to 10MB (Ultimate MK3 hack)

![http://gxdev.wordpress.com/files/2008/08/vracing.png](http://gxdev.wordpress.com/files/2008/08/vracing.png)

### Gamecube/Wii generic features ###

  * fully featured & optimized Graphical User Interface
  * 48 kHz stereo sound
  * optimized GX video rendering engine
  * 100% smooth & skipping-free audio/video synchronization
  * 50/60 Hz video output support
  * original low-resolution video modes support (interlaced & non-interlaced)
  * high-resolution interlaced (480i/576i) & progressive (480p) video modes support
  * hardware bilinear filtering
  * configurable sound mixer (FM/PSG levels) and filtering (Low-Pass filter & 3-Band equalizer)
  * independently configurable region mode, VDP mode & Master Clock
  * 1~4 Players support
  * automatic Backup RAM and State files loading/saving
  * automatic game files loading
  * game files loading history
  * load files from SD/SDHC or DVD
  * support for zipped ROM files
  * game internal header information display
  * internal game screenshots
  * Game Genie & Pro Action Replay codes support through .pat files
  * cartridge "hot-swap"
  * automatic disc swap


### Wii extra features ###
  * up to 8 Players support
  * Wii Remote, Nunchuk & Classic controllers support
  * Wii Remote IR support for light guns
  * USB mouse support for mouse emulation
  * load files from USB drives (USB2 support through IOS58)
  * configurable hardware “Trap” filter & Gamma correction

http://gxdev.files.wordpress.com/2008/08/wiimote.jpg?w=240&h=241