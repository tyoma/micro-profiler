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
	PUBLIC _micro_profiler_trampoline_proto, _micro_profiler_trampoline_proto_end
	PUBLIC _micro_profiler_jumper_proto, _micro_profiler_jumper_proto_end

	_micro_profiler_trampoline_proto: ; fastcall argument passing: ECX, EDX, <stack>
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
	_micro_profiler_trampoline_proto_end:

	_micro_profiler_jumper_proto:
		jmp	_micro_profiler_jumper_proto_end + 31415981h ; trampoline address (displacement)
	_micro_profiler_jumper_proto_end:
end
