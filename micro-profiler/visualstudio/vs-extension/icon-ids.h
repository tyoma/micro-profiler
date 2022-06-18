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
	#define iconidClear 0x001C
	#define iconidPause 0x004B
	#define iconidPlay 0x004C
	#define iconidViewAsTree 92 // candidates: 71, 92
	#define iconidViewAsList 38 // candidates: 66, 38
	
	#define iconidSettings 69 // candidates: 69, 143

	//#define guidIconSet { 0xae27a6b0, 0xe345, 0x4288, { 0x96, 0xdf, 0x5e, 0xaf, 0x39, 0x4e, 0xe3, 0x69 } }

	//#define iconidOpen 3923
	//#define iconidSave 2664
	//#define iconidCloseAll 487
	//#define iconidCopy 645
	//#define iconidClear 479
	//#define iconidPause 2274
	//#define iconidPlay 2356
	//#define iconidViewAsTree 388
	//#define iconidViewAsList 1280

#else
	// Taken from: msobtnid.h
	#define guidIconSet guidOfficeIcon

	#define iconidOpen 0x0017
	#define iconidSave 0x0003
	#define iconidCloseAll 0x006A
	#define iconidCopy 0x0013
	#define iconidClear 0x002F
	#define iconidPause 0x00BD
	#define iconidPlay 0x00BA
	#define iconidViewAsTree 0x026C
	#define iconidViewAsList 0x027A
	
#endif
