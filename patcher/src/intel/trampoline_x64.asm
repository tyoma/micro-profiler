;	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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
		mov	rcx, [interceptor]
		shl	rdx, 20h
		or		rdx, rax
		mov	r8, rdx ; 3rd argument, timestamp
		lea	rdx, qword ptr [rsp + 20h] ; 2nd argument, stack_ptr
		mov	r9, [callee_id]
		sub	rsp, 028h
		call	[on_enter]
		add	rsp, 028h
		pop	r9
		pop	r8
		pop	rdx
		pop	rcx

		add	rsp, 08h
		call	[target]
		sub	rsp, 08h

		push	rax
		rdtsc
		mov	rcx, [interceptor]
		shl	rdx, 20h
		or		rdx, rax
		mov	r8, rdx ; 3rd argument, timestamp
		lea	rdx, qword ptr [rsp + 8h] ; 2nd argument, stack_ptr
		sub	rsp, 020h
		call	[on_exit]
		add	rsp, 020h
		mov	qword ptr [rsp + 8h], rax ; restore return address
		pop	rax
		ret

		interceptor	dq	3141592600000001h
		callee_id	dq	3141592600000002h
		on_enter	dq	3141592600000003h
		on_exit	dq	3141592600000004h
		target	dq	3141592600000005h
	trampoline_proto_end:

	jumper_proto:
		jmp	[jumper_target]

		jumper_target	dq	3141592600000001h
	jumper_proto_end:

.data
	c_trampoline_proto	dq	trampoline_proto
	c_trampoline_size		db	(trampoline_proto_end - trampoline_proto)
	c_jumper_proto			dq	jumper_proto
	c_jumper_size			dd	(jumper_proto_end - jumper_proto)

	PUBLIC c_trampoline_proto, c_trampoline_size
	PUBLIC c_jumper_proto, c_jumper_size

end
