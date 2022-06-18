//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/analyzer.h>

#include <common/serialization.h>

namespace strmd
{
	template <typename KeyT> struct version< micro_profiler::call_graph_node<KeyT> > {	enum {	value = 5	};	};
	template <> struct type_traits<micro_profiler::thread_analyzer> { typedef container_type_tag category; };
	template <> struct type_traits<micro_profiler::analyzer> { typedef container_type_tag category; };
}

namespace micro_profiler
{
	template <typename ArchiveT, typename AddressT>
	inline void serialize(ArchiveT &archive, call_graph_node<AddressT> &data, unsigned int /*ver*/)
	{
		archive(static_cast<function_statistics &>(data));
		archive(data.callees);
	}
}
