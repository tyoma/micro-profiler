.586
.model flat
.code

extern ?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z:near
extern ?_instance@calls_collector@micro_profiler@@0V12@A:dword


SAVEREGS	macro
	mov	dword ptr [esp - 10h], eax
	mov	dword ptr [esp - 14h], ecx
	mov	dword ptr [esp - 18h], edx
endm

LOADREGS	macro
	pop	edx
	pop	ecx
	pop	eax
	add	esp, 0Ch
endm

PUSHRDTSC	macro
	rdtsc
	push	edx
	push	eax
endm

_profile_enter	proc
	SAVEREGS
	PUSHRDTSC
	mov	dword ptr [esp - 14h], esp
	sub	esp, 14h
	mov	ecx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z
	LOADREGS
	ret
_profile_enter	endp

_profile_exit	proc
	SAVEREGS
	push 0
	PUSHRDTSC
	mov	dword ptr [esp - 10h], esp
	sub	esp, 10h
	mov	ecx, offset ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z
	LOADREGS
	ret
_profile_exit	endp

end
