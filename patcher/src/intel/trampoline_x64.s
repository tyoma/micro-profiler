#	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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
	trampoline_proto:	# argument passing: RDI, RSI, RDX, RCX, R8, and R9, <stack>
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
		mov	$0x3141592600000005, %rax # target address
		call	*%rax
		sub	$0x08, %rsp

		push	%rax
		rdtsc
		mov	$0x3141592600000001, %rdi # 1st argument, interceptor
		lea	0x08(%rsp), %rsi # 2nd argument, stack_ptr
		shl	$0x20, %rdx
		or		%rax, %rdx # 3rd argument, timestamp
		mov	$0x3141592600000004, %rax # on_exit() address
		sub	$0x80, %rsp
		call	*%rax
		add	$0x80, %rsp
		mov	%rax, 0x08(%rsp) # restore return address
		pop	%rax
		ret
	trampoline_proto_end:

	jumper_proto:
		mov	$0x3141592600000001, %rax # trampoline address
		jmp	*%rax
	jumper_proto_end:

.data
	c_trampoline_proto: .quad trampoline_proto
	c_trampoline_size: .byte (trampoline_proto_end - trampoline_proto)
	c_jumper_proto: .quad jumper_proto
	c_jumper_size: .byte (jumper_proto_end - jumper_proto)

	.global c_trampoline_proto, c_trampoline_size
	.global c_jumper_proto, c_jumper_size
