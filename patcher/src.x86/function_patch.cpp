#include <patcher/function_patch.h>
#include <patcher/binary_translation.h>

#include "assembler_intel.h"

#include <common/memory.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		enum {
			jmp_size = sizeof(intel::jmp_rel_imm32),
		};
	}

	function_patch::function_patch(executable_memory_allocator &allocator_, byte_range body,
			void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
		: _patched_fragment(0, 0), _original_fragment(0, 0)
	{	init(allocator_, body, interceptor, on_enter, on_exit);	}

	void function_patch::init(executable_memory_allocator &allocator_, byte_range body,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		_patched_fragment = byte_range(body.begin(), calculate_fragment_length(body, jmp_size));
		_original_fragment = revert_entry_t(_patched_fragment.begin(), static_cast<byte>(_patched_fragment.length()));

		const size_t size0 = c_thunk_size + _patched_fragment.length() + jmp_size;
		const size_t size = (size0 + 0x0F) & ~0x0F;

		_thunk = allocator_.allocate(size);

		byte *thunk = static_cast<byte *>(_thunk.get());

		// initialize thunk
		byte *moved_fragment = thunk + c_thunk_size;
		initialize_hooks(thunk, moved_fragment, _patched_fragment.begin(), interceptor, on_enter, on_exit);
		move_function(moved_fragment, _patched_fragment);
		reinterpret_cast<intel::jmp_rel_imm32 *>(moved_fragment + _patched_fragment.length())
			->init(_patched_fragment.end());
		mem_set(thunk + size0, 0xCC, size - size0);

		// place hooking jump to original body & correct displacement references
		intel::jmp_rel_imm32 &jmp_original = *(intel::jmp_rel_imm32 *)(_patched_fragment.begin());			
		scoped_unprotect su(_patched_fragment);

		offset_displaced_references(_revert_buffer, body, _patched_fragment, moved_fragment);
		jmp_original.init(thunk);
		mem_set(_patched_fragment.begin() + jmp_size, 0xCC, _patched_fragment.length() - jmp_size);
	}

	function_patch::~function_patch()
	{
		scoped_unprotect su(_patched_fragment);

		for (revert_buffer::const_iterator i = _revert_buffer.begin(); i != _revert_buffer.end(); ++i)
			i->restore();
		_original_fragment.restore();
	}
}
