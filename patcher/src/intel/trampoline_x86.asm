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

.model flat
.code
	trampoline_proto: ; fastcall argument passing: ECX, EDX, <stack>
		push	eax ; some MS CRT functions accept arguments in EAX register...
		push	ecx
		push	edx
		rdtsc
		mov	ecx, 31415901h ; 1st argument, interceptor
		push	31415902h ; 4th argument, callee
		push	edx
		push	eax ; 3rd argument, timestamp
		lea	edx, dword ptr [esp + 18h] ; 2nd argument, stack_ptr
		call	on_enter + 31415983h ; on_enter address (displacement)
	on_enter:
		pop	edx
		pop	ecx
		pop	eax

		lea	esp, dword ptr [esp + 04h]		
		call	target + 31415985h ; target address
	target:
		lea	esp, dword ptr [esp - 04h]

		push	eax
		rdtsc
		mov	ecx, 31415901h ; 1st argument, interceptor
		push	edx
		push	eax ; 3rd argument, timestamp
		lea	edx, dword ptr [esp + 0Ch] ; 2nd argument, stack_ptr
		call	on_exit + 31415984h ; on_exit address (displacement)
	on_exit:
		mov	dword ptr [esp + 04h], eax ; restore return address
		pop	eax
		ret
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
