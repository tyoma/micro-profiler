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

#include "table.h"

#include <strmd/type_traits.h>

namespace micro_profiler
{
	namespace views
	{
		struct table_items_reader
		{
			template <typename T, typename K>
			void prepare(table<T, K> &/*container*/, unsigned int /*count*/) const
			{	}

			template <typename ArchiveT, typename T, typename K>
			void read_item(ArchiveT &archive, table<T, K> &container) const
			{
				auto r = container.create();

				archive(*r);
				r.commit();
			}

			template <typename T, typename K>
			void complete(table<T, K> &/*container*/) const
			{	}
		};
	}
}

namespace strmd
{
	template <typename T, typename ConstructorT>
	struct type_traits< micro_profiler::views::table<T, ConstructorT> >
	{
		typedef container_type_tag category;
		typedef micro_profiler::views::table_items_reader item_reader_type;
	};
}
