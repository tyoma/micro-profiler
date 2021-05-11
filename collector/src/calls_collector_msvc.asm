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

IFDEF _M_IX86
ELSEIFDEF _M_X64
	.code

	extrn ?construct_thread_trace@calls_collector@micro_profiler@@AEAAAEAVcalls_collector_thread@2@XZ:near
	extrn ?flush@calls_collector_thread@micro_profiler@@QEAAXXZ:near

	READ_TLS macro
;		mov	rax, qword ptr gs:[eax * 8 + 1480h]
		cmp	eax, 40h
		cmovl	rax, qword ptr gs:[eax * 8 + 1480h]
		jl		short done
		push	rbx
		movzx	rax, ax
		mov	rbx, qword ptr gs:[1780h]
		mov	rax, qword ptr [rbx + 8 * rax - 200h]
		pop	rbx
	done:
		exitm
	endm

	?on_enter_nostack@calls_collector@micro_profiler@@QEAAX_JPEBX@Z PROC ; micro_profiler::calls_collector::on_enter_nostack
		; rcx: micro_profiler::calls_collector *this
		; rdx: timestamp
		; r8: calee

		mov	eax, dword ptr [rcx + 08h]
		READ_TLS
		test	rax, rax
		je		short construct_trace
	proceed_store:
		mov	rcx, qword ptr [rax]				; ptr = calls_collector_thread::_ptr;
		mov	qword ptr [rcx], rdx				; *ptr = {	timestamp, callee }...
		mov	qword ptr [rcx + 08h], r8		;	...;
		lea	rcx, qword ptr [rcx + 10h]		; ptr++;
		mov	qword ptr [rax], rcx				; calls_collector_thread::_ptr = ptr;
		dec	dword ptr [rax + 08h]			; --calls_collector_thread::_n_left;
		je		flush
		ret

	construct_trace:
		mov	qword ptr [rsp + 18h], rdx		; timestamp
		mov	qword ptr [rsp + 20h], r8		; callee
		sub	rsp, 28h
		call	?construct_thread_trace@calls_collector@micro_profiler@@AEAAAEAVcalls_collector_thread@2@XZ ; micro_profiler::calls_collector::construct_thread_trace
		add	rsp, 28h
		mov	rdx, qword ptr [rsp + 18h]
		mov	r8, qword ptr [rsp + 20h]
		jmp	short proceed_store

	flush:
		sub	rsp, 28h
		mov	rcx, rax
		call	?flush@calls_collector_thread@micro_profiler@@QEAAXXZ ; micro_profiler::calls_collector_thread::flush
		add	rsp, 28h
		ret
	?on_enter_nostack@calls_collector@micro_profiler@@QEAAX_JPEBX@Z ENDP ; micro_profiler::calls_collector::on_enter_nostack


	?on_exit_nostack@calls_collector@micro_profiler@@QEAAX_J@Z PROC ; micro_profiler::calls_collector::on_exit_nostack
		; rcx: micro_profiler::calls_collector *this
		; rdx: timestamp

		mov	eax, dword ptr [rcx + 08h]
		READ_TLS
		mov	rcx, qword ptr [rax]				; ptr = calls_collector_thread::_ptr;
		mov	qword ptr [rcx], rdx				; *ptr = {	timestamp, 0 }...
		mov	qword ptr [rcx + 08h], 0		;	...;
		lea	rcx, qword ptr [rcx + 10h]		; ptr++;
		mov	qword ptr [rax], rcx				; calls_collector_thread::_ptr = ptr;
		dec	dword ptr [rax + 08h]			; --calls_collector_thread::_n_left;
		je		flush
		ret

	flush:
		sub	rsp, 28h
		mov	rcx, rax
		call	?flush@calls_collector_thread@micro_profiler@@QEAAXXZ ; micro_profiler::calls_collector_thread::flush
		add	rsp, 28h
		ret
	?on_exit_nostack@calls_collector@micro_profiler@@QEAAX_J@Z ENDP ; micro_profiler::calls_collector::on_exit_nostack

ENDIF

end
