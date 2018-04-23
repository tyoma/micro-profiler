IF _M_IX86
	.586
	.model flat
	.code

	extern _profile_enter:near
	extern _profile_exit:near

	__penter proc
		jmp _profile_enter
	__penter	endp

	__pexit	proc
		jmp _profile_exit
	__pexit	endp
ELSEIF _M_X64
	.code

	extern profile_enter:near
	extern profile_exit:near

	_penter proc
		jmp profile_enter
	_penter	endp

	_pexit	proc
		jmp profile_exit
	_pexit	endp
ENDIF

end
