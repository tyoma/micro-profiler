//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

// package guid
// { 43a50861-7e04-4169-b47e-d4cffe1b3db8 }
#define guidMicroProfilerPkg { 0x43A50861, 0x7E04, 0x4169, { 0xB4, 0x7E, 0xD4, 0xCF, 0xFE, 0x1B, 0x3D, 0xB8 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_MicroProfilerPackage, 0x43A50861, 0x7E04, 0x4169, 0xB4, 0x7E, 0xD4, 0xCF, 0xFE, 0x1B, 0x3D, 0xB8);
#endif

// Command set guid for our commands (used with IOleCommandTarget)
// { 89fa7df1-74bb-4181-b2be-fb37a8ab7dd8 }
#define guidMicroProfilerCmdSet { 0x89FA7DF1, 0x74BB, 0x4181, { 0xB2, 0xBE, 0xFB, 0x37, 0xA8, 0xAB, 0x7D, 0xD8 } }
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_MicroProfilerCmdSet, 0x89FA7DF1, 0x74BB, 0x4181, 0xB2, 0xBE, 0xFB, 0x37, 0xA8, 0xAB, 0x7D, 0xD8);
#endif

// { 8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942 }
#define UICONTEXT_VCProjectVSCT { 0x8BC9CEB8, 0x8B4A, 0x11D0, { 0x8D, 0x11, 0x00, 0xA0, 0xC9, 0x1B, 0xC9, 0x42 } }
#ifdef DEFINE_GUID
DEFINE_GUID(UICONTEXT_VCProject, 0x8BC9CEB8, 0x8B4A, 0x11D0, 0x8D, 0x11, 0x00, 0xA0, 0xC9, 0x1B, 0xC9, 0x42);
#endif
