//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "platform.h"

#include <common/types.h>

namespace micro_profiler
{
	template <typename InterceptorT>
	struct hooks
	{
		typedef void (CC_(fastcall) on_enter_t)(InterceptorT *interceptor, const void *callee, timestamp_t timestamp,
			void **return_address_ptr) _CC(fastcall);
		typedef void *(CC_(fastcall) on_exit_t)(InterceptorT *interceptor, timestamp_t timestamp) _CC(fastcall);

		static hooks<void>::on_enter_t *on_enter()
		{	return reinterpret_cast<hooks<void>::on_enter_t *>(static_cast<on_enter_t *>(&InterceptorT::on_enter));	}

		static hooks<void>::on_exit_t *on_exit()
		{	return reinterpret_cast<hooks<void>::on_exit_t *>(static_cast<on_exit_t *>(&InterceptorT::on_exit));	}
	};

	extern const size_t c_thunk_size;


	void initialize_hooks(void *thunk_location, const void *target_function, const void *id, void *instance,
		hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit);

	template <typename T>
	inline void initialize_hooks(void *thunk_location, const void *target_function, const void *id, T *interceptor)
	{	initialize_hooks(thunk_location, target_function, id, interceptor, hooks<T>::on_enter(), hooks<T>::on_exit());	}
}
