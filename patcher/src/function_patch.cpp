#include <patcher/function_patch.h>
#include <patcher/binary_translation.h>

#include "intel/jump.h"

#include <common/memory.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		enum {
			jmp_size = sizeof(assembler::jump),
		};
	}

	function_patch::function_patch(executable_memory_allocator &a, byte_range body,
			void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
		: _patched_fragment(0, 0), _original_fragment(0, 0)
	{	init(a, body, interceptor, on_enter, on_exit);	}

	void function_patch::init(executable_memory_allocator &a, byte_range body,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		_patched_fragment = byte_range(body.begin(), jmp_size);
		_original_fragment = revert_entry_t(_patched_fragment.begin(), static_cast<byte>(_patched_fragment.length()));
		_thunk = a.allocate(c_thunk_size);

		byte *thunk = static_cast<byte *>(_thunk.get());

		// initialize thunk
		initialize_hooks(thunk, body.begin() + jmp_size, _patched_fragment.begin(), interceptor, on_enter, on_exit);

		// place hooking jump to original body & correct displacement references
		auto &jmp = *reinterpret_cast<assembler::jump *>(_patched_fragment.begin());
		scoped_unprotect su(_patched_fragment);

		jmp.init(thunk);
	}

	function_patch::~function_patch()
	{
		scoped_unprotect su(_patched_fragment);

		for (revert_buffer::const_iterator i = _revert_buffer.begin(); i != _revert_buffer.end(); ++i)
			i->restore();
		_original_fragment.restore();
	}
}
