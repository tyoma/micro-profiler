#pragma once

#include <vsshlids.h>

#ifndef guidVsShellIcons
	// Guid for shell icons (VS 11.0+)
	#define guidVsShellIcons	{ 0x9cd93c42, 0xceef, 0x45ab, { 0xb1, 0xb5, 0x60, 0x40, 0x88, 0xc, 0x95, 0x43 } }
#endif

#ifdef MP_MSVS11PLUS_SUPPORT
	#define guidIconSet guidVsShellIcons

	#define iconidOpen 0x0044
	#define iconidSave 0x005F
	#define iconidCloseAll 0x0012
	#define iconidCopy 0x0017
	#define iconidClear 0x001B
	#define iconidPause 0x004B
	#define iconidPlay 0x004C
#else
	#define guidIconSet guidOfficeIcon

	#define iconidOpen 0x0017
	#define iconidSave 0x0003
	#define iconidCloseAll 0x006A
	#define iconidCopy 0x0013
	#define iconidClear 0x002F
	#define iconidPause 0x00BD
	#define iconidPlay 0x00BA
#endif
