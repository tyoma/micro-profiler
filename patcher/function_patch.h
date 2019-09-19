//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "revert_buffer.h"
#include "dynamic_hooking.h"

namespace micro_profiler
{
	class function_patch : noncopyable
	{
	public:
		template <typename T>
		function_patch(executable_memory_allocator &allocator, byte_range body, T *interceptor);
		function_patch(executable_memory_allocator &allocator, byte_range body, void *interceptor,
			hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit);
		~function_patch();

	private:
		typedef revert_entry<24> revert_entry_t;

	private:
		void init(executable_memory_allocator &allocator, byte_range body, void *interceptor,
			hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit);

	private:
		std::shared_ptr<void> _thunk;
		byte_range _patched_fragment;
		revert_entry_t _original_fragment;
		revert_buffer _revert_buffer;
	};



	template <typename T>
	inline function_patch::function_patch(executable_memory_allocator &allocator, byte_range body, T *interceptor)
		: _patched_fragment(0, 0), _original_fragment(0, 0)
	{	init(allocator, body, interceptor, hooks<T>::on_enter(), hooks<T>::on_exit());	}
}
