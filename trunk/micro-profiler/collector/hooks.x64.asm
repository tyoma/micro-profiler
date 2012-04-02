.code

extrn ?track@calls_collector@micro_profiler@@QEAAXAEBUcall_record@2@@Z:near
extrn ?_instance@calls_collector@micro_profiler@@0V12@A:qword

PUSHREGS	macro		; slides stack pointer 38h bytes down
	sub	esp, 10h
	push	rax
	push	rcx
	push	rdx
	push	r8
	push	r9
endm

POPREGS	macro		; slides stack pointer 38h bytes up
	pop	r9
	pop	r8
	pop	rdx
	pop	rcx
	pop	rax
	add	esp, 10h
endm

RDTSC64	macro
	rdtsc
	shl	rdx, 20h
	or		rax, rdx
endm


profile_enter	proc
	PUSHREGS
	RDTSC64
	mov	rdx, rsp
	add	rdx, 30h
	mov	qword ptr [rdx], rax
	sub	rsp, 20h
	lea	rcx, ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QEAAXAEBUcall_record@2@@Z
	add	rsp, 20h
	POPREGS
	ret
profile_enter	endp

profile_exit	proc
	PUSHREGS
	RDTSC64
	mov	rdx, rsp
	add	rdx, 28h
	mov	qword ptr [rdx + 8h], 0
	mov	qword ptr [rdx], rax
	sub	rsp, 20h
	lea	rcx, ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QEAAXAEBUcall_record@2@@Z
	add	rsp, 20h
	POPREGS
	ret
profile_exit	endp

end
