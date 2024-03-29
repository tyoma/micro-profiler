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
#include "jump.h"
#include "jumper.h"

namespace micro_profiler
{
	class function_patch : public patch, noncopyable
	{
	public:
		template <typename T>
		function_patch(void *target, T *interceptor, executable_memory_allocator &allocator_);

		bool active() const;
		virtual bool activate() override;
		virtual bool revert() override;

	private:
		std::shared_ptr<void> _trampoline;
		jumper _jumper;
	};



	template <typename T>
	inline function_patch::function_patch(void *target, T *interceptor, executable_memory_allocator &allocator_)
		: _trampoline(allocator_.allocate(c_trampoline_size + c_jump_size)), _jumper(target, _trampoline.get())
	{
		initialize_trampoline(_trampoline.get(), target, interceptor);
		jump_initialize(static_cast<byte *>(_trampoline.get()) + c_trampoline_size, _jumper.entry());
	}

	inline bool function_patch::active() const
	{	return _jumper.active();	}

	inline bool function_patch::activate()
	{	return _jumper.activate();	}

	inline bool function_patch::revert()
	{	return _jumper.revert();	}
}
