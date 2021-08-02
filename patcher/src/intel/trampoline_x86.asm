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
		push	ecx
		push	edx
		rdtsc
		mov	ecx, 31415901h ; 1st argument, interceptor
		push	31415902h ; 4th argument, callee
		push	edx
		push	eax ; 3rd argument, timestamp
		lea	edx, dword ptr [esp + 14h] ; 2nd argument, stack_ptr
		mov	eax, 31415903h ; on_enter address
		call	eax
		pop	edx
		pop	ecx

		lea	esp, dword ptr [esp + 04h]		
		mov	eax, 31415905h ; target_function address
		call	eax
		lea	esp, dword ptr [esp - 04h]

		push	eax
		rdtsc
		mov	ecx, 31415901h ; 1st argument, interceptor
		push	edx
		push	eax ; 3rd argument, timestamp
		lea	edx, dword ptr [esp + 0Ch] ; 2nd argument, stack_ptr
		mov	eax, 31415904h ; on_exit address
		call	eax
		mov	dword ptr [esp + 04h], eax ; restore return address
		pop	eax
		ret
	trampoline_proto_end:

	jumper:
		mov	eax, 31415901h ; trampoline address
		jmp	eax
		db		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ; 17 bytes for max instruction length and a short jump
	jumper_end:

.data
	_c_trampoline_proto	dd	trampoline_proto
	_c_trampoline_size	db	(trampoline_proto_end - trampoline_proto)
	_c_jumper_proto		dd	jumper
	_c_jumper_size			dd	(jumper_end - jumper)

	PUBLIC _c_trampoline_proto
	PUBLIC _c_trampoline_size
	PUBLIC _c_jumper_proto
	PUBLIC _c_jumper_size

end
