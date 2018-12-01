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
	{	init(allocator_, body, interceptor, on_enter, on_exit);	}

	void function_patch::init(executable_memory_allocator &allocator_, byte_range body,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		_target_function = body.begin();
		_chunk_length = calculate_function_length(body, jmp_size);

		byte_range source_chunk(_target_function, _chunk_length);
		const size_t size0 = c_thunk_size + _chunk_length + jmp_size;
		const size_t size = (size0 + 0x0F) & ~0x0F;

		_memory = allocator_.allocate(size);

		byte *thunk = static_cast<byte *>(_memory.get());

		// initialize thunk
		initialize_hooks(thunk, thunk + c_thunk_size, _target_function, interceptor, on_enter, on_exit);
		move_function(thunk + c_thunk_size, source_chunk);
		reinterpret_cast<intel::jmp_rel_imm32 *>(thunk + c_thunk_size + _chunk_length)
			->init(_target_function + _chunk_length);
		mem_set(thunk + size0, 0xCC, size - size0);

		// place hooking jump to original body
		intel::jmp_rel_imm32 &jmp_original = *(intel::jmp_rel_imm32 *)(_target_function);
			
		mem_copy(_saved, source_chunk.begin(), source_chunk.length());

		scoped_unprotect su(source_chunk);
		jmp_original.init(thunk);
		mem_set(_target_function + jmp_size, 0xCC, _chunk_length - jmp_size);
	}

	function_patch::~function_patch()
	{
		scoped_unprotect su(range<byte>(_target_function, _chunk_length));

		mem_copy(_target_function, _saved, _chunk_length);
	}
}
