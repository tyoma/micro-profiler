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

.model flat
.code
	trampoline_proto: ; fastcall argument passing: ECX, EDX, <stack>
		push	eax ; some MS CRT functions accept arguments in EAX register...
		push	ebx
		push	ecx
		push	edx
		rdtsc
		call	base_1
	base_1:
		pop	ebx
		mov	ecx, dword ptr [ebx + (interceptor - base_1)] ; 1st argument, interceptor
		push	dword ptr [ebx + (callee_id - base_1)] ; 4th argument, callee
		push	edx
		push	eax ; 3rd argument, timestamp
		lea	edx, dword ptr [esp + 1Ch] ; 2nd argument, stack_ptr
		call	dword ptr [ebx + (on_enter - base_1)]
		mov	ecx, dword ptr [ebx + (target - base_1)]
		mov	dword ptr [esp], ecx
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax

		add	esp, 4
		call	dword ptr [esp - 14h]
		sub	esp, 4

		push	eax
		rdtsc
		call	base_2
	base_2:
		pop	ecx
		push	edx
		push	eax ; 3rd argument, timestamp
		lea	edx, dword ptr [esp + 0Ch] ; 2nd argument, stack_ptr
		mov	eax, ecx
		mov	ecx, dword ptr [eax + (interceptor - base_2)] ; 1st argument
		call	dword ptr [eax + (on_exit - base_2)]
		mov	dword ptr [esp + 04h], eax ; restore return address
		pop	eax
		ret

		interceptor	dd	31415901h
		target		dd	31415905h
		callee_id	dd	31415902h
		on_enter		dd	31415903h
		on_exit		dd	31415904h
	trampoline_proto_end:

	jumper_proto:
		jmp	jumper_proto_end + 31415981h ; trampoline address (displacement)
	jumper_proto_end:

.data
	_c_trampoline_proto	dd	trampoline_proto
	_c_trampoline_size	db	(trampoline_proto_end - trampoline_proto)
	_c_jumper_proto		dd	jumper_proto
	_c_jumper_size			dd	(jumper_proto_end - jumper_proto)

	PUBLIC _c_trampoline_proto, _c_trampoline_size
	PUBLIC _c_jumper_proto, _c_jumper_size

end
