//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "dynamic_hooking.h"
#include "interface.h"

namespace micro_profiler
{
	class translated_function_patch : public patch, noncopyable
	{
	public:
		template <typename T>
		translated_function_patch(void *target, std::size_t size, T *interceptor, executable_memory_allocator &allocator_);

		bool active() const;
		virtual bool activate() override;
		virtual bool revert() override;

	private:
		void init(void *target, std::size_t target_size, executable_memory_allocator &allocator_, void *interceptor,
			hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit);

	private:
		std::shared_ptr<void> _trampoline;
		byte *_target;
		std::size_t _moved_size;
		std::vector<byte> _original_prolouge;
	};



	template <typename T>
	inline translated_function_patch::translated_function_patch(void *target, std::size_t size, T *interceptor,
		executable_memory_allocator &allocator_)
	{	init(target, size, allocator_, interceptor, hooks<T>::on_enter(), hooks<T>::on_exit());	}
}
