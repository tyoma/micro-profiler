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

#include <patcher/dynamic_hooking.h>

#include "replace.h"

#include <algorithm>
#include <common/memory.h>
#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace std;

extern "C" {
	extern const uint8_t micro_profiler_trampoline_proto;
	extern const uint8_t micro_profiler_trampoline_proto_end;
}

namespace micro_profiler
{
	const size_t c_trampoline_size = &micro_profiler_trampoline_proto_end - &micro_profiler_trampoline_proto;


	void initialize_trampoline(void *trampoline_address, const void *target, const void *id,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		byte_range trampoline(static_cast<byte *>(trampoline_address), c_trampoline_size);

		mem_copy(trampoline.begin(), &micro_profiler_trampoline_proto, trampoline.length());
		replace(trampoline, 1, [interceptor] (...) {	return reinterpret_cast<size_t>(interceptor);	});
		replace(trampoline, 2, [id] (...) {	return reinterpret_cast<size_t>(id);	});
		replace(trampoline, 3, [on_enter] (...) {	return reinterpret_cast<size_t>(on_enter);	});
		replace(trampoline, 0x83, [on_enter] (ptrdiff_t address) {
			return reinterpret_cast<ptrdiff_t>(on_enter) - address;
		});
		replace(trampoline, 4, [on_exit] (...) {	return reinterpret_cast<size_t>(on_exit);	});
		replace(trampoline, 0x84, [on_exit] (ptrdiff_t address) {
			return reinterpret_cast<ptrdiff_t>(on_exit) - address;
		});
		replace(trampoline, 5, [target] (...) {	return reinterpret_cast<size_t>(target);	});
		replace(trampoline, 0x85, [target] (ptrdiff_t address) {
			return reinterpret_cast<ptrdiff_t>(target) - address;
		});
	}
}
