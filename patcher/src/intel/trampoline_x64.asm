;	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
	trampoline_proto: ; argument passing: RCX, RDX, R8, and R9, <stack>
		push	rcx
		push	rdx
		push	r8
		push	r9
		rdtsc
		mov	rcx, 3141592600000001h ; 1st argument, interceptor
		shl	rdx, 20h
		or		rdx, rax
		mov	r8, rdx ; 3rd argument, timestamp
		lea	rdx, qword ptr [rsp + 20h] ; 2nd argument, stack_ptr
		mov	r9, 3141592600000002h ; 4th argument, callee
		mov	rax, 3141592600000003h ; on_enter address
		sub	rsp, 028h
		call	rax
		add	rsp, 028h
		pop	r9
		pop	r8
		pop	rdx
		pop	rcx

		add	rsp, 08h
		mov	rax, 3141592600000005h ; target_function address
		call	rax
		sub	rsp, 08h

		push	rax
		rdtsc
		mov	rcx, 3141592600000001h ; 1st argument, interceptor
		shl	rdx, 20h
		or		rdx, rax
		mov	r8, rdx ; 3rd argument, timestamp
		lea	rdx, qword ptr [rsp + 8h] ; 2nd argument, stack_ptr
		mov	rax, 3141592600000004h ; on_exit address
		sub	rsp, 020h
		call	rax
		add	rsp, 020h
		mov	qword ptr [rsp + 8h], rax ; restore return address
		pop	rax
		ret
	trampoline_proto_end:

	jumper:
		mov	rax, 3141592600000001h ; trampoline address
		jmp	rax
		db		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ; 17 bytes for max instruction length and a short jump
	jumper_end:

.data
	c_trampoline_proto	dq	trampoline_proto
	c_trampoline_size		db	(trampoline_proto_end - trampoline_proto)
	c_jumper_proto			dd	jumper
	c_jumper_size			dd	(jumper_end - jumper)

	PUBLIC c_trampoline_proto, c_trampoline_size
	PUBLIC c_jumper_proto, c_jumper_size

end
