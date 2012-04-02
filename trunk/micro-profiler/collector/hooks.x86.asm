.586
.model flat
.code

extern ?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z:near
extern ?_instance@calls_collector@micro_profiler@@0V12@A:dword

_profile_enter	proc
	mov	dword ptr [esp - 12], eax
	mov	dword ptr [esp - 16], ecx
	mov	dword ptr [esp - 20], edx
	rdtsc
	push	edx
	push	eax
	mov	edx, esp
	sub	esp, 12
	push	edx
	lea	ecx, ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z
	pop	edx
	pop	ecx
	pop	eax
	add	esp, 8
	ret
_profile_enter	endp

_profile_exit	proc
	mov	dword ptr [esp - 16], eax
	mov	dword ptr [esp - 20], ecx
	mov	dword ptr [esp - 24], edx
	rdtsc
	push	0
	push	edx
	push	eax
	mov	edx, esp
	sub	esp, 12
	push	edx
	lea	ecx, ?_instance@calls_collector@micro_profiler@@0V12@A
	call	?track@calls_collector@micro_profiler@@QAEXABUcall_record@2@@Z
	pop	edx
	pop	ecx
	pop	eax
	add	esp, 12
	ret
_profile_exit	endp

end
