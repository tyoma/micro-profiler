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

#include "keyer.h"
#include "serialization_context.h"
#include "database.h"

#include <common/serialization.h>
#include <math/serialization.h>
#include <sdb/indexed_serialization.h>
#include <sdb/serialization.h>

#pragma warning(disable: 4510; disable: 4610)

namespace strmd
{
	template <typename StreamT, typename PackerT, int static_version>
	class deserializer;

	template <> struct version<micro_profiler::call_statistics> {	enum {	value = 5	};	};
	template <typename T> struct version< micro_profiler::tables::record<T> > {	enum {	value = 1	};	};
	template <typename T> struct version< micro_profiler::auto_increment_constructor<T> > {	enum {	value = 0	};	};

	template <typename ArchiveT, typename U, typename C, int sl>
	inline void serialize(ArchiveT &archive, micro_profiler::call_node_key &data,
		const micro_profiler::scontext::hierarchy_node<U, C, sl> &context)
	{
		if (context.address_and_thread)
		{
			auto v = std::make_pair(std::get<2>(data), std::get<0>(data));

			archive(v);
			std::get<0>(data) = v.second;
			std::get<2>(data) = v.first;
		}
		else
		{
			archive(std::get<2>(data));
		}
	}
}

namespace micro_profiler
{
	struct call_nodes_reader : strmd::container_reader_base
	{
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	_first = true;	}

		template <typename ArchiveT, typename TableT, typename U>
		void read_item(ArchiveT &archive, sdb::immutable_unique_index<TableT, keyer::callnode> &container,
			const scontext::hierarchy_node<U, sdb::immutable_unique_index<TableT, keyer::callnode>, -1> &context)
		{
			id_t thread_id;

			archive(thread_id);
			archive(container, scontext::nested_context(context, thread_id));
			if (_first)
				context.underlying.threads.clear(), _first = false;
			context.underlying.threads.push_back(thread_id);
		}

		template <typename ArchiveT, typename TableT, typename U, int sl>
		void read_item(ArchiveT &archive, sdb::immutable_unique_index<TableT, keyer::callnode> &container,
			const scontext::hierarchy_node<U, sdb::immutable_unique_index<TableT, keyer::callnode>, sl> &context) const
		{
			call_node_key key(context.thread_id, context.parent_id, 0);

			archive(key, context);
			container[key].commit(); // TODO: come up with something better then committing records twice.
			auto r = container[key];
			archive(*r, context);
			r.commit();
		}

	private:
		bool _first;
	};



	template <typename ArchiveT, typename T>
	inline void serialize(ArchiveT &archive, auto_increment_constructor<T> &data, unsigned int /*ver*/)
	{	archive(data._next_id);	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, call_statistics &data, unsigned int /*ver*/)
	{
		archive(data.id);
		archive(data.thread_id);
		archive(data.parent_id);
		archive(data.address);
		archive(static_cast<function_statistics &>(data));
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, function_statistics &data, ContextT &/*context*/, unsigned int ver)
	{
		function_statistics v;

		serialize(archive, v, ver);
		add(data, v);
	}

	template <typename ArchiveT, typename U, typename C, int sl>
	inline void serialize(ArchiveT &archive, call_statistics &data, const scontext::hierarchy_node<U, C, sl> &context,
		unsigned int ver)
	{
		archive(static_cast<function_statistics &>(data), context.underlying);
		if (ver >= 5)
			archive(context.container, scontext::nested_context<0>(context, data.id)); // callees
		else
			archive(context.container, scontext::nested_context<1>(context, data.id)); // callees
	}

	template <typename ArchiveT, typename U, typename C>
	inline void serialize(ArchiveT &archive, call_statistics &data, const scontext::hierarchy_node<U, C, 1> &context,
		unsigned int ver)
	{	serialize(archive, static_cast<function_statistics &>(data), context.underlying, ver);	}


	namespace tables
	{
		template <typename ArchiveT, typename T>
		inline void serialize(ArchiveT &archive, record<T> &data, unsigned int /*ver*/)
		{
			archive(data.id);
			archive(static_cast<T &>(data));
		}

		template <typename ArchiveT, typename T, typename I>
		inline void serialize(ArchiveT &archive, record<T> &data, sdb::scontext::indexed_by<keyer::external_id, I> &, int ver)
		{	serialize(archive, static_cast<T &>(data), ver);	}

		template <typename ArchiveT, typename ContextT>
		inline void serialize(ArchiveT &archive, statistics &data, ContextT &/*context*/)
		{	sdb::serialize(archive, data);	}

		template <typename S, typename P, int v, typename ContextT>
		inline void serialize(strmd::deserializer<S, P, v> &archive, statistics &data, ContextT &context)
		{
			auto &index = sdb::unique_index<keyer::callnode>(data);

			archive(index, scontext::root_context(context, index));
			data.invalidate();
		}
	}
}

namespace strmd
{
	template <typename TableT>
	struct type_traits< sdb::immutable_unique_index<TableT, micro_profiler::keyer::callnode> >
	{
		typedef container_type_tag category;
		typedef micro_profiler::call_nodes_reader item_reader_type;
	};
}
