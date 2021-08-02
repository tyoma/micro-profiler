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

#include <patcher/dynamic_hooking.h>

#include <algorithm>
#include <common/memory.h>
#include <limits>
#include <stdexcept>

using namespace std;

extern "C" const void *c_trampoline_proto;
extern "C" micro_profiler::byte c_trampoline_size;

namespace micro_profiler
{
	const size_t c_thunk_size = c_trampoline_size;

	namespace
	{
		const unsigned int c_signature = 0x31415926;

		template <typename T>
		T construct_replace_seq(byte index)
		{
			auto mask = (static_cast<T>(0) - 1) & ~0xFF;
			auto signature = static_cast<T>(c_signature) << 8 * (sizeof(T) - 4);

			return (signature & mask) | index;
		}

		template <typename WriterT>
		void replace(byte_range range_, byte index, const WriterT &writer)
		{
			typedef decltype(writer(0)) value_type;

			const auto seq = construct_replace_seq<value_type>(index);
			const auto sbegin = reinterpret_cast<const byte *>(&seq);
			const auto send = sbegin + sizeof(seq);
			
			for (byte *i = range_.begin(), *end = range_.end(); i = search(i, end, sbegin, send), i != end; )
			{
				const auto replacement = writer(reinterpret_cast<size_t>(i + sizeof(value_type)));

				mem_copy(i, reinterpret_cast<const byte *>(&replacement), sizeof(replacement));
				i += sizeof(replacement);
			}
		}
	}

	void initialize_hooks(void *thunk_location, const void *target_function, const void *id,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		byte_range thunk(static_cast<byte *>(thunk_location), c_trampoline_size);

		mem_copy(thunk.begin(), c_trampoline_proto, thunk.length());
		replace(thunk, 1, [interceptor] (...) {	return reinterpret_cast<size_t>(interceptor);	});
		replace(thunk, 2, [id] (...) {	return reinterpret_cast<size_t>(id);	});
		replace(thunk, 3, [on_enter] (...) {	return reinterpret_cast<size_t>(on_enter);	});
		replace(thunk, 4, [on_exit] (...) {	return reinterpret_cast<size_t>(on_exit);	});
		replace(thunk, 5, [target_function] (...) {	return reinterpret_cast<size_t>(target_function);	});
	}
}
