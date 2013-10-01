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

.code

extrn ?track@calls_collector@micro_profiler@@QEAAXAEBUcall_record@2@@Z:near
extrn ?_instance@calls_collector@micro_profiler@@0V12@A:qword

PUSHREGS	macro		; slides stack pointer 38h bytes down
	sub	esp, 10h
	push	rax
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
	pop	rax
	add	esp, 10h
endm

RDTSC64	macro
	rdtsc
	shl	rdx, 20h
	or		rax, rdx
endm


profile_enter	proc
	PUSHREGS
	RDTSC64
	mov	rdx, rsp
	add	rdx, 30h
	mov	qword ptr [rdx], rax
	sub	rsp, 20h
	mov	rcx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QEAAXAEBUcall_record@2@@Z
	add	rsp, 20h
	POPREGS
	ret
profile_enter	endp

profile_exit	proc
	PUSHREGS
	RDTSC64
	mov	rdx, rsp
	add	rdx, 28h
	mov	qword ptr [rdx + 8h], 0
	mov	qword ptr [rdx], rax
	sub	rsp, 20h
	mov	rcx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QEAAXAEBUcall_record@2@@Z
	add	rsp, 20h
	POPREGS
	ret
profile_exit	endp

end
