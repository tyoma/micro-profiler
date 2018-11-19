;	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
;
;	Permission is hereby granted, free of charge, to any person obtaining a copy
;	of this software and associated documentation files (the "Software"), to deal
;	in the Software without restriction, including without limitation the rights
;	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;	copies of the Software, and to permit persons to whom the Software is
;	furnished to do so, subject to the following conditions:
;
;	The above copyright notice and this permission notice shall be included in
;	all copies or substantial portions of the Software.
;
;	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
;	THE SOFTWARE.

IF _M_IX86
	.586
	.model flat
	.code

	extern ?on_enter@calls_collector@micro_profiler@@SIXPAV12@PBX_JPAPBX@Z:near
	extern ?on_exit@calls_collector@micro_profiler@@SIPBXPAV12@_J@Z:near
	extern _g_collector_ptr:dword

	PUSHREGS	macro
		push	eax
		push	ecx
		push	edx
	endm

	POPREGS	macro
		pop	edx
		pop	ecx
		pop	eax
	endm

	PUSHRDTSC	macro
		rdtsc
		push	edx
		push	eax
	endm

	_profile_enter	proc
		PUSHREGS

		mov	ecx, [_g_collector_ptr]
		lea	eax, [esp + 0Ch]
		push	eax
		PUSHRDTSC
		mov	edx, [esp + 18h]
		call	?on_enter@calls_collector@micro_profiler@@SIXPAV12@PBX_JPAPBX@Z

		POPREGS
		ret
	_profile_enter	endp

	_profile_exit	proc
		PUSHREGS

		mov	ecx, [_g_collector_ptr]
		PUSHRDTSC
		call	?on_exit@calls_collector@micro_profiler@@SIPBXPAV12@_J@Z

		POPREGS
		ret
	_profile_exit	endp
ELSEIF _M_X64
	.code

	extrn ?on_enter@calls_collector@micro_profiler@@SAXPEAV12@PEBX_JPEAPEBX@Z:near
	extrn ?on_exit@calls_collector@micro_profiler@@SAPEBXPEAV12@_J@Z:near
	extern g_collector_ptr:qword

	PUSHREGS	macro
		push	rax
		lahf
		push	rax
		push	rcx
		push	rdx
		push	r8
		push	r9
		push	r10
		push	r11
	endm

	POPREGS	macro
		pop	r11
		pop	r10
		pop	r9
		pop	r8
		pop	rdx
		pop	rcx
		pop	rax
		sahf
		pop	rax
	endm

	RDTSC64	macro
		rdtsc
		shl	rdx, 20h
		or		rdx, rax
	endm

	profile_enter	proc
		PUSHREGS
		sub	rsp, 20h

		mov	rcx, [g_collector_ptr]
		lea	r9, qword ptr [rsp + 60h]
		RDTSC64
		mov	r8, rdx
		mov rdx, [r9]
		call ?on_enter@calls_collector@micro_profiler@@SAXPEAV12@PEBX_JPEAPEBX@Z

		add	rsp, 20h
		POPREGS
		ret
	profile_enter	endp

	profile_exit	proc
		PUSHREGS
		movdqu	[rsp - 10h], xmm0
		sub	rsp, 30h

		mov	rcx, [g_collector_ptr]
		RDTSC64
		call	?on_exit@calls_collector@micro_profiler@@SAPEBXPEAV12@_J@Z

		add	rsp, 30h
		movdqu	xmm0, [rsp - 10h]
		POPREGS
		ret
	profile_exit	endp
ENDIF

end
