#pragma once

#include <vsshlids.h>

#if defined(guidVsShellIcons)
   #define guidIconSet { 0x9cd93c42, 0xceef, 0x45ab, { 0xb1, 0xb5, 0x60, 0x40, 0x88, 0xc, 0x95, 0x43 } }

   #define iconidDelete 0x001B
   #define iconidOpen 0x0044
   #define iconidSave 0x005F
   #define iconidCloseAll 0x0012
   #define iconidCopy 0x0017
   #define iconidClear 0x001B
#else
	#define guidIconSet { 0xd309f794, 0x903f, 0x11d0, { 0x9e, 0xfc, 0x00, 0xa0, 0xc9, 0x11, 0x00, 0x4f } }

   #define iconidDelete 478
   #define iconidOpen 23
   #define iconidSave 3
   #define iconidCloseAll 747
   #define iconidCopy 19
   #define iconidClear 478
#endif
