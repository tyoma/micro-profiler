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

#pragma once

#include <algorithm>
#include <common/memory.h>

namespace micro_profiler
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
			
		for (byte *i = range_.begin(), *end = range_.end(); i = std::search(i, end, sbegin, send), i != end; )
		{
			const auto replacement = writer(reinterpret_cast<size_t>(i + sizeof(value_type)));

			mem_copy(i, reinterpret_cast<const byte *>(&replacement), sizeof(replacement));
			i += sizeof(replacement);
		}
	}
}
