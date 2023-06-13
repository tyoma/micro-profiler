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

#include <patcher/translated_function_patch.h>

#include <patcher/binary_translation.h>
#include <patcher/jump.h>

using namespace std;

namespace micro_profiler
{
	bool translated_function_patch::active() const
	{	return !_original_prolouge.empty();	}

	bool translated_function_patch::activate()
	{
		if (active())
			return false;
		_original_prolouge.assign(_target, _target + _moved_size);
		jump_initialize(_target, _trampoline.get());
		mem_set(_target + c_jump_size, 0xCC, _moved_size - c_jump_size);
		return true;
	}

	bool translated_function_patch::revert()
	{
		if (!active())
			return false;
		mem_copy(_target, _original_prolouge.data(), _original_prolouge.size());
		_original_prolouge.clear();
		return true;
	}

	void translated_function_patch::init(void *target, size_t target_size, executable_memory_allocator &allocator_,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		byte_range src(static_cast<byte *>(target), target_size);

		_moved_size = calculate_fragment_length(src, c_jump_size);
		_trampoline = allocator_.allocate(c_trampoline_size + _moved_size + c_jump_size);
		_target = static_cast<byte *>(target);

		auto ptr = static_cast<byte *>(_trampoline.get());

		initialize_trampoline(ptr, target, interceptor, on_enter, on_exit);
		ptr += c_trampoline_size;

		move_function(ptr, byte_range(src.begin(), _moved_size));
		ptr += _moved_size;

		jump_initialize(ptr, src.begin() + _moved_size);
	}
}
