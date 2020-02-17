//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "symbol_resolver.h"

#include <common/serialization.h>

namespace micro_profiler
{
	class analyzer;
}

namespace strmd
{
	template <typename KeyT, typename ValueT> struct is_container< std::map<KeyT, ValueT> > { static const bool value = true; };

	template <typename KeyT, typename ValueT>
	struct container_reader< std::map<KeyT, ValueT> >
	{
		template <typename ArchiveT>
		void operator()(ArchiveT &archive, size_t count, std::map<KeyT, ValueT> &data)
		{
			std::pair<KeyT, ValueT> value;

			data.clear();
			while (count--)
			{
				archive(value);
				data.insert(value);
			}
		}
	};
}

namespace micro_profiler
{
	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_resolver &data)
	{
		archive(data._mappings);
		archive(data._modules);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_resolver::module_info &data)
	{
		archive(data.symbols);
		archive(data.files);
	}
}
