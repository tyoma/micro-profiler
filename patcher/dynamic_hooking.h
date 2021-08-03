//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include <common/platform.h>
#include <common/types.h>

namespace micro_profiler
{
	template <typename InterceptorT>
	struct hook_types
	{
		typedef void (CC_(fastcall) on_enter_t)(InterceptorT *interceptor, const void **stack_ptr,
			timestamp_t timestamp, const void *callee) _CC(fastcall);
		typedef const void *(CC_(fastcall) on_exit_t)(InterceptorT *interceptor, const void **stack_ptr,
			timestamp_t timestamp) _CC(fastcall);

	};
	
	template <typename InterceptorT>
	struct hooks : hook_types<InterceptorT>
	{
		static hook_types<void>::on_enter_t *on_enter()
		{	return reinterpret_cast<hook_types<void>::on_enter_t *>(&InterceptorT::on_enter);	}

		static hook_types<void>::on_exit_t *on_exit()
		{	return reinterpret_cast<hook_types<void>::on_exit_t *>(&InterceptorT::on_exit);	}
	};

	class redirector
	{
	public:
		redirector(void *target, const void *trampoline);
		~redirector();

		const void *entry() const;
		void activate();
		void cancel();

	private:
		byte *_target;
		byte _fuse_revert[8];
		bool _active : 1, _cancelled : 1;
	};


	extern const size_t c_trampoline_size;


	void initialize_trampoline(void *trampoline, const void *target, const void *id, void *interceptor,
		hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit);

	template <typename T>
	inline void initialize_trampoline(void *trampoline, const void *target, const void *id, T *interceptor)
	{	initialize_trampoline(trampoline, target, id, interceptor, hooks<T>::on_enter(), hooks<T>::on_exit());	}
}
