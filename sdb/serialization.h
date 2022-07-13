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

#include "table.h"

#include <strmd/type_traits.h>

namespace sdb
{
	template <typename T, typename ConstructorT>
	struct table_wrapper
	{
		typedef table<T, ConstructorT> table_type;
		typedef typename table_type::const_iterator const_iterator;
		typedef typename table_type::const_reference const_reference;

		size_t size() const
		{	return table_.size();	}

		const_iterator begin() const
		{	return table_.begin();	}

		const_iterator end() const
		{	return table_.end();	}

		table_type &table_;
	};

	struct table_items_reader
	{
		template <typename T, typename ConstructorT>
		void prepare(table_wrapper<T, ConstructorT> &/*w*/, unsigned int /*count*/) const
		{	}

		template <typename ArchiveT, typename T, typename ConstructorT>
		void read_item(ArchiveT &archive, table_wrapper<T, ConstructorT> &w) const
		{
			auto r = w.table_.create();

			archive(*r);
			r.commit();
		}

		template <typename T, typename ConstructorT>
		void complete(table_wrapper<T, ConstructorT> &/*w*/) const
		{	}
	};
}

namespace strmd
{
	template <typename T, typename ConstructorT>
	struct type_traits< sdb::table_wrapper<T, ConstructorT> >
	{
		typedef container_type_tag category;
		typedef sdb::table_items_reader item_reader_type;
	};
}

namespace sdb
{
	template <typename ArchiveT, typename T, typename ConstructorT>
	void serialize(ArchiveT &archive, table<T, ConstructorT> &data)
	{
		table_wrapper<T, ConstructorT> w = {	data	};

		archive(w);
		archive(data._constructor);
	}

	template <typename ArchiveT, typename T>
	void serialize(ArchiveT &/*archive*/, default_constructor<T> &/*data*/)
	{	}
}
