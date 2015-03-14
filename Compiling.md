## Disclaimer ##

This project is under a specific open-source license, which implies that the sourcecode must **always** be made available with any modified binaries and should **not** be used in any commercial activity.

You might want to compile the sourcecode yourself from current [SVN](http://code.google.com/p/genplus-gx/source/checkout) for various reasons:

  * to test the current build before it is officially released
  * to implement your own features.

However, I would appreciate if you **respect** the following points:

  * current SVN build should always be considered as unstable and work in progress: you are free to compile the current source code to test it and help debugging it by reporting issues but **don't distribute these builds publically**. You can always get the last stable code from the tagged versions.


  * you can modify the sourcecode as you want but in order to keep this project clean and structured, I would ask you to **keep ANY private build for yourself to avoid public distribution of derivative works or forks**. This project is still actively maintained so If you think the modification you made should be implemented in an official release, please contact me: I would eventually include it and obviously give credits when it is due.


## How to Compile ? ##

First, you will have to download and install the following tools/libraries:

  1. from [here](http://sourceforge.net/projects/devkitpro/files/devkitPPC), the last version of DevkitPPC. Windows user should directly run the Automated Installer. During installation, make sure you get libogc and libfat libraries installed as well. They should be installed in C:/devkitpro/libogc/
  1. from [here](http://sourceforge.net/projects/devkitpro/files/portlibs/ppc), the following libraries: zlib, libpng & libtremor. Make sure you pick the precompiled PPC versions and extract them in C:/devkitpro/portlibs/ppc/. You should have the following directories once extracted:

  * /devkitpro/portlibs/ppc/include

  * /devkitpro/portlibs/ppc/include/tremor

  * /devkitpro/portlibs/ppc/lib

Once you are done, if you haven't one yet, install a [SVN client](http://en.wikipedia.org/wiki/Comparison_of_Subversion_clients) and get the current [genplus-gx](http://code.google.com/p/genplus-gx/source/checkout) source code from SVN.

Finally, go into genplus-gx/trunk/ and run the following commands :

make -f Makefile.wii

make -f Makefile.gc

You should obtain two .dol files, genplus\_wii.dol and genplus\_cube.dol which correspond respectively to Wii and Gamecube versions (NB: the Wii version should be manually renamed to boot.dol to be used with the Homebrew Channel).

The DevkitPro download page provides all the precompiled libraries and include files required to compile the current source code of Genesis Plus GX. However, you could also get the current source code of the libraries, compile them yourself and install as specified (generally **make** then **make install**).

The following libraries are required:

  * [libogc](http://sourceforge.net/projects/devkitpro/files/libogc/)
  * [libfat](http://sourceforge.net/projects/devkitpro/files/libfat/)
  * libtremor: OGG music file support
  * libpng: PNG image file support
  * zlib: ZIP compressed file support