;	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

end
