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

#include "intel/jump.h"

#include <algorithm>
#include <common/memory.h>
#include <limits>
#include <stdexcept>

using namespace std;

extern "C" {
	extern const void *c_trampoline_proto;
	extern micro_profiler::byte c_trampoline_size;
	extern const void *c_jumper_proto;
	extern micro_profiler::byte c_jumper_size;
}

namespace micro_profiler
{
	const size_t c_trampoline_size = ::c_trampoline_size;

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


	redirector::redirector(void *target, const void *trampoline)
		: _target(static_cast<byte *>(target)), _active(false), _cancelled(false)
	{
		if (!(_target[0] == 0x90 && _target[1] == 0x90 || _target[0] == 0x8B && _target[1] == 0xFF))
			throw runtime_error("the function is not hotpatchable!");
		byte_range jumper(_target - c_jumper_size, c_jumper_size);
		scoped_unprotect u(jumper);

		mem_copy(jumper.begin(), c_jumper_proto, jumper.length());
		replace(jumper, 1, [trampoline] (...) {	return reinterpret_cast<size_t>(trampoline);	});
		replace(jumper, 0x81, [trampoline] (ptrdiff_t address) {
			return reinterpret_cast<ptrdiff_t>(trampoline) - address;
		});
	}

	redirector::~redirector()
	{
		byte_range fuse(_target, sizeof(assembler::short_jump));
		scoped_unprotect u(fuse);

		*reinterpret_cast<assembler::short_jump *>(_target) = *reinterpret_cast<assembler::short_jump *>(_fuse_revert);
	}

	const void *redirector::entry() const
	{	return static_cast<byte *>(_target) + 2;	}

	void redirector::activate()
	{
		byte_range fuse(_target, sizeof(assembler::short_jump));
		scoped_unprotect u(fuse);

		*reinterpret_cast<assembler::short_jump *>(_fuse_revert) = *reinterpret_cast<assembler::short_jump *>(_target);
		reinterpret_cast<assembler::short_jump *>(_target)->init(_target - c_jumper_size);
	}


	void initialize_trampoline(void *trampoline_address, const void *target, const void *id,
		void *interceptor, hooks<void>::on_enter_t *on_enter, hooks<void>::on_exit_t *on_exit)
	{
		byte_range trampoline(static_cast<byte *>(trampoline_address), c_trampoline_size);

		mem_copy(trampoline.begin(), c_trampoline_proto, trampoline.length());
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
