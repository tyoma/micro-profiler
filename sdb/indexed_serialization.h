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

#include "integrated_index.h"

#include <strmd/type_traits.h>

namespace strmd
{
	template <typename StreamT, typename PackerT, int static_version> class deserializer;
}

namespace sdb
{
	namespace scontext
	{
		template <typename K, typename InnerT>
		struct indexed_by
		{
			InnerT inner;
		};

		template <typename K>
		struct indexed_by<K, void>
		{
		};
	}

	struct indexed_items_reader
	{
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	}

		template <typename ArchiveT, typename U, typename K, typename CtxT>
		void read_item(ArchiveT &archive, immutable_unique_index<U, K> &index, CtxT &context) const
		{
			typename immutable_unique_index<U, K>::key_type key;

			archive(key);

			auto rec = index[key];
			archive(*rec, context);
			rec.commit();
		}

		template <typename ContainerT>
		void complete(ContainerT &/*data*/)
		{	}
	};

	template <typename S, typename P, int v, typename T, typename C, typename K, typename I>
	inline void serialize(strmd::deserializer<S, P, v> &archive, table<T, C> &data, scontext::indexed_by<K, I> &context)
	{
		archive(unique_index(data, K()), context);
		data.invalidate();
	}

	template <typename ArchiveT, typename T, typename C, typename K, typename I>
	inline void serialize(ArchiveT &/*archive*/, table<T, C> &/*data*/, scontext::indexed_by<K, I> &/*context*/)
	{	throw 0;	}
}

namespace strmd
{
	template <typename U, typename K>
	struct type_traits< sdb::immutable_unique_index<U, K> >
	{
		typedef container_type_tag category;
		typedef sdb::indexed_items_reader item_reader_type;
	};
}
