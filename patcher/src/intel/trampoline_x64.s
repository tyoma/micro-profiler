#	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
#
#	Permission is hereby granted, free of charge, to any person obtaining a copy
#	of this software and associated documentation files (the "Software"), to deal
#	in the Software without restriction, including without limitation the rights
#	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#	copies of the Software, and to permit persons to whom the Software is
#	furnished to do so, subject to the following conditions:
#
#	The above copyright notice and this permission notice shall be included in
#	all copies or substantial portions of the Software.
#
#	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#	THE SOFTWARE.

.text
	.globl micro_profiler_trampoline_proto, micro_profiler_trampoline_proto_end
	.globl _micro_profiler_trampoline_proto, _micro_profiler_trampoline_proto_end

	micro_profiler_trampoline_proto:	# argument passing: RDI, RSI, RDX, RCX, R8, and R9, <stack>
	_micro_profiler_trampoline_proto:
		push	%rdi
		push	%rsi
		push	%rdx
		push	%rcx
		push	%r8
		push	%r9
		rdtsc
		mov	$0x3141592600000001, %rdi # 1st argument, interceptor
		lea	0x30(%rsp), %rsi # 2nd argument, stack_ptr
		shl	$0x20, %rdx
		or		%rax, %rdx # 3rd argument, timestamp
		mov	$0x3141592600000002, %rcx # 4th argument, callee
		mov	$0x3141592600000003, %rax # on_enter() address
		sub	$0x88, %rsp
		call	*%rax
		add	$0x88, %rsp
		pop	%r9
		pop	%r8
		pop	%rcx
		pop	%rdx
		pop	%rsi
		pop	%rdi

		add	$0x08, %rsp
		call	trampoline_proto_end

		push	%rax
		rdtsc
		mov	$0x3141592600000001, %rdi # 1st argument, interceptor
		lea	(%rsp), %rsi # 2nd argument, stack_ptr
		shl	$0x20, %rdx
		or		%rax, %rdx # 3rd argument, timestamp
		mov	$0x3141592600000004, %rax # on_exit() address
		sub	$0x88, %rsp
		call	*%rax
		add	$0x88, %rsp
		mov	%rax, %rcx # restore return address
		pop	%rax
		jmp	*%rcx
	trampoline_proto_end:
	micro_profiler_trampoline_proto_end:
	_micro_profiler_trampoline_proto_end:
