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

	extern ?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z:near
	extern ?_instance@calls_collector@micro_profiler@@0V12@A:dword


	SAVEREGS	macro
		mov	dword ptr [esp - 10h], eax
		mov	dword ptr [esp - 14h], ecx
		mov	dword ptr [esp - 18h], edx
	endm

	LOADREGS	macro
		pop	edx
		pop	ecx
		pop	eax
		add	esp, 0Ch
	endm

	PUSHRDTSC	macro
		rdtsc
		push	edx
		push	eax
	endm

	_profile_enter	proc
		SAVEREGS
		PUSHRDTSC
		mov	dword ptr [esp - 14h], esp
		sub	esp, 14h
		mov	ecx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		call	?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z
		LOADREGS
		ret
	_profile_enter	endp

	_profile_exit	proc
		SAVEREGS
		push 0
		PUSHRDTSC
		mov	dword ptr [esp - 10h], esp
		sub	esp, 10h
		mov	ecx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		call	?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z
		LOADREGS
		ret
	_profile_exit	endp
ELSEIF _M_X64
	.code

	extrn ?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z:near
	extrn ?_instance@calls_collector@micro_profiler@@0V12@A:qword

	PUSHREGS	macro		; slides stack pointer 38h bytes down
		push	rax
		lahf
		push	rax
		sub	rsp, 08h
		push	rcx
		push	rdx
		push	r8
		push	r9
	endm

	POPREGS	macro		; slides stack pointer 38h bytes up
		pop	r9
		pop	r8
		pop	rdx
		pop	rcx
		add	rsp, 08h
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
		RDTSC64
		mov	rcx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		mov	r8, qword ptr [rsp + 38h]
		sub	rsp, 20h
		call	?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z
		add	rsp, 20h
		POPREGS
		ret
	profile_enter	endp

	profile_exit	proc
		PUSHREGS
		mov	rcx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
		xor	r8, r8
		sub	rsp, 20h
		RDTSC64
		call	?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z
		add	rsp, 20h
		POPREGS
		ret
	profile_exit	endp
ENDIF

end
