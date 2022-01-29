//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#ifndef __COMMANDIDS_H_
#define __COMMANDIDS_H_

///////////////////////////////////////////////////////////////////////////////
// Package

// { 43a50861-7e04-4169-b47e-d4cffe1b3db8 }
#define guidMicroProfilerPkg { 0x43A50861, 0x7E04, 0x4169, { 0xB4, 0x7E, 0xD4, 0xCF, 0xFE, 0x1B, 0x3D, 0xB8 } }

///////////////////////////////////////////////////////////////////////////////
// Command Set IDs

// Global command set
// { 89fa7df1-74bb-4181-b2be-fb37a8ab7dd8 }
#define guidGlobalCmdSet { 0x89FA7DF1, 0x74BB, 0x4181, { 0xB2, 0xBE, 0xFB, 0x37, 0xA8, 0xAB, 0x7D, 0xD8 } }

// Instance command set
// {5F14C228-458D-4050-A104-2DBB6B997E6C}
#define guidInstanceCmdSet { 0x5f14c228, 0x458d, 0x4050, { 0xa1, 0x4, 0x2d, 0xbb, 0x6b, 0x99, 0x7e, 0x6c } }

///////////////////////////////////////////////////////////////////////////////
// Menu IDs

///////////////////////////////////////////////////////////////////////////////
// Menu Group IDs
#define IDG_MP_PROJECT_SETUP	0x1020
#define IDM_MP_MM_MICROPROFILER	0x1030
#define IDG_MP_MAIN	0x1031
#define IDG_MP_WINDOWS	0x1032
#define IDM_MP_PANE_TOOLBAR	0x1033
#define IDG_MP_INSTANCE_COMMANDS 0x1034
#define IDG_MP_INSTANCE_MISC_COMMANDS 0x1035
#define IDG_MP_INSTANCE_RUN_COMMANDS 0x1036
#define IDG_MP_IPC	0x1036


///////////////////////////////////////////////////////////////////////////////
// Command IDs
#define cmdidToggleProfiling 0x101
#define cmdidRemoveProfilingSupport 0x104
#define cmdidPauseUpdates 0x108
#define cmdidResumeUpdates 0x109
#define cmdidSaveStatistics 0x110
#define cmdidLoadStatistics 0x111
#define cmdidProfileProcess 0x120
#define cmdidIPCEnableRemote 0x201
#define cmdidIPCSocketPort 0x202
#define cmdidCloseAll 0x300
#define cmdidClearStatistics 0x400
#define cmdidCopyStatistics 0x401

#define cmdidProfileScope 0x501

#define cmdidSupportDeveloper 0x1000

#define cmdidWindowActivateDynamic 0x2000

#endif // __COMMANDIDS_H_
