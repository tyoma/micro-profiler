// guids.h: definitions of GUIDs/IIDs/CLSIDs used in this VsPackage

/*
Do not use #pragma once, as this file needs to be included twice.  Once to declare the externs
for the GUIDs, and again right after including initguid.h to actually define the GUIDs.
*/

// package guid
// { 43a50861-7e04-4169-b47e-d4cffe1b3db8 }
#define guidMicroProfilerPkg { 0x43A50861, 0x7E04, 0x4169, { 0xB4, 0x7E, 0xD4, 0xCF, 0xFE, 0x1B, 0x3D, 0xB8 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_MicroProfilerPackage, 0x43A50861, 0x7E04, 0x4169, 0xB4, 0x7E, 0xD4, 0xCF, 0xFE, 0x1B, 0x3D, 0xB8 );
#endif

// Command set guid for our commands (used with IOleCommandTarget)
// { 89fa7df1-74bb-4181-b2be-fb37a8ab7dd8 }
#define guidMicroProfilerCmdSet { 0x89FA7DF1, 0x74BB, 0x4181, { 0xB2, 0xBE, 0xFB, 0x37, 0xA8, 0xAB, 0x7D, 0xD8 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_MicroProfilerCmdSet, 0x89FA7DF1, 0x74BB, 0x4181, 0xB2, 0xBE, 0xFB, 0x37, 0xA8, 0xAB, 0x7D, 0xD8 );
#endif
