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

IFDEF _M_IX86
ELSEIFDEF _M_X64
	.code

	extrn ?construct_thread_trace@calls_collector@micro_profiler@@AEAAAEAVcalls_collector_thread@2@XZ:near
	extrn ?flush@calls_collector_thread@micro_profiler@@QEAAXXZ:near

	FRAME_IN macro
		push	r9
		push	r10
		push	r11
		push	rbp
		mov	rbp, rsp
		and	sp, 0FFF0h
		lea	rsp, [rsp - 80h]
		movaps	[rsp + 20h], xmm0
		movaps	[rsp + 30h], xmm1
		movaps	[rsp + 40h], xmm2
		movaps	[rsp + 50h], xmm3
		movaps	[rsp + 60h], xmm4
		movaps	[rsp + 70h], xmm5
	endm

	FRAME_OUT macro
		movaps	xmm0, [rsp + 20h]
		movaps	xmm1, [rsp + 30h]
		movaps	xmm2, [rsp + 40h]
		movaps	xmm3, [rsp + 50h]
		movaps	xmm4, [rsp + 60h]
		movaps	xmm5, [rsp + 70h]
		mov	rsp, rbp
		pop	rbp
		pop	r11
		pop	r10
		pop	r9
	endm

	READ_TLS macro
		cmp	ax, 40h
		jae	dynamic_tls
		mov	rax, qword ptr gs:[eax * 8 + 1480h]
		jmp	done
	dynamic_tls:
		push	rbx
		movzx	rbx, ax
		xor	rax, rax
		or		rax, qword ptr gs:[1780h] ; load a pointer to a dynamically allocated TLS array from TIB.
		jz		uninitialized ; as a result of a non-allocated dynamic TLS page, rax holds zero, as a never written TLS value.
		mov	rax, qword ptr [rax + 8 * rbx - 200h]
	uninitialized:
		pop	rbx
		jmp	done
	done:
		exitm
	endm

	?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z PROC ; micro_profiler::calls_collector::track
		; rcx: micro_profiler::calls_collector *this
		; rdx: timestamp
		; r8: calee (or nullptr for exit)

		mov	eax, dword ptr [rcx + 08h]
		READ_TLS
		test	rax, rax
		jz		short track$construct_trace

	track$proceed_store:
		mov	rcx, qword ptr [rax]				; ptr = calls_collector_thread::_ptr;
		mov	qword ptr [rcx], rdx				; *ptr = {	timestamp, callee }...
		mov	qword ptr [rcx + 08h], r8		;	...;
		lea	rcx, qword ptr [rcx + 10h]		; ptr++;
		mov	qword ptr [rax], rcx				; calls_collector_thread::_ptr = ptr;
		dec	dword ptr [rax + 08h]			; --calls_collector_thread::_n_left;
		jz		short track$flush
		ret

	track$flush:
		FRAME_IN
		mov	rcx, rax
		call	?flush@calls_collector_thread@micro_profiler@@QEAAXXZ ; micro_profiler::calls_collector_thread::flush
		FRAME_OUT
		ret

	track$construct_trace:
		push	rdx	; timestamp
		push	r8		; callee
		FRAME_IN
		call	?construct_thread_trace@calls_collector@micro_profiler@@AEAAAEAVcalls_collector_thread@2@XZ ; micro_profiler::calls_collector::construct_thread_trace
		FRAME_OUT
		pop	r8
		pop	rdx
		jmp	track$proceed_store

	?track@calls_collector@micro_profiler@@QEAAX_JPEBX@Z ENDP ; micro_profiler::calls_collector::track
ENDIF

end
