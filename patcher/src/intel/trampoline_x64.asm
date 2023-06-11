;	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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
	PUBLIC micro_profiler_trampoline_proto, micro_profiler_trampoline_proto_end

	micro_profiler_trampoline_proto: ; argument passing: RCX, RDX, R8, and R9, <stack>
		push	rcx
		push	rdx
		push	r8
		push	r9
		push	r10
		push	r11
		rdtsc
		mov	rcx, [interceptor]
		shl	rdx, 20h
		or		rdx, rax
		mov	r8, rdx ; 3rd argument, timestamp
		lea	rdx, qword ptr [rsp + 30h] ; 2nd argument, stack_ptr
		mov	r9, [callee_id]
		sub	rsp, 028h
		call	[on_enter]
		add	rsp, 028h
		pop	r11
		pop	r10
		pop	r9
		pop	r8
		pop	rdx
		pop	rcx

		add	rsp, 08h
		call	[trampoline_proto_end]

		push	rax
		push	r10
		push	r11
		rdtsc
		mov	rcx, [interceptor]
		shl	rdx, 20h
		or		rdx, rax
		mov	r8, rdx ; 3rd argument, timestamp
		lea	rdx, qword ptr [rsp + 10h] ; 2nd argument, stack_ptr
		sub	rsp, 028h
		call	[on_exit]
		add	rsp, 028h
		mov	rcx, rax ; jump-as-return here
		pop	r11
		pop	r10
		pop	rax
		jmp	rcx

		interceptor	dq	3141592600000001h
		callee_id	dq	3141592600000002h
		on_enter	dq	3141592600000003h
		on_exit	dq	3141592600000004h
	trampoline_proto_end:
	micro_profiler_trampoline_proto_end:
end
