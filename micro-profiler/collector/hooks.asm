;	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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

	extern ?track@calls_collector@micro_profiler@@QAEXUcall_record@2@@Z:near
	extern ?_instance@calls_collector@micro_profiler@@0V12@A:dword

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
		mov	ecx, [esp + 0Ch]
		push	ecx
		PUSHRDTSC
		mov	ecx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		call	?track@calls_collector@micro_profiler@@QAEXUcall_record@2@@Z
		POPREGS
		ret
	_profile_enter	endp

	_profile_exit	proc
		PUSHREGS
		push 0
		PUSHRDTSC
		mov	ecx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		call	?track@calls_collector@micro_profiler@@QAEXUcall_record@2@@Z
		POPREGS
		ret
	_profile_exit	endp

ELSEIF _M_X64

	.code

	extrn ?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z:near
	extrn ?_instance@calls_collector@micro_profiler@@0V12@A:qword

	PUSHREGS	macro
		push	rax
		push	rcx
		push	rdx
		push	r8
		push	r9
	endm

	POPREGS	macro
		pop	r9
		pop	r8
		pop	rdx
		pop	rcx
		pop	rax
	endm

	RDTSC64	macro
		rdtsc
		shl	rdx, 20h
		or		rdx, rax
	endm

	profile_enter	proc
		push	rax
		lahf
		PUSHREGS
		sub	rsp, 28h

		RDTSC64
		mov	rcx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		mov	r8, qword ptr [rsp + 58h]
		call	?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z

		add	rsp, 28h
		POPREGS
		sahf
		pop	rax
		ret
	profile_enter	endp

	profile_exit	proc
		PUSHREGS
		movdqu	[rsp - 10h], xmm0
		sub	rsp, 30h

		mov	rcx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		xor	r8, r8
		RDTSC64
		call	?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z

		add	rsp, 30h
		movdqu	xmm0, [rsp - 10h]
		POPREGS
		ret
	profile_exit	endp

ENDIF

end
