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

#include "function_list.h"
#include "symbol_resolver.h"

#include <common/serialization.h>

namespace micro_profiler
{
	template <typename KeyT>
	struct deserialization_context
	{
		typename statistic_types_t<KeyT>::map_detailed *map;
		unsigned int threadid;
		KeyT caller;
	};

	struct statistics_map_reader
	{
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/)
		{	}

		template <typename ArchiveT, typename ContainerT, typename KeyT>
		void operator()(ArchiveT &archive, ContainerT &data, const micro_profiler::deserialization_context<KeyT> &context)
		{
			micro_profiler::deserialization_context<KeyT> new_context = context;

			archive(new_context.caller);
			archive(data[new_context.caller], new_context);
		}

		template <typename ArchiveT, typename ContainerT>
		void operator()(ArchiveT &archive, ContainerT &data)
		{
			typename ContainerT::key_type key;

			archive(key);
			archive(data[key]);
		}
	};

	struct functions_list_reader
	{
		void prepare(functions_list &/*container*/)
		{	}

		template <typename ArchiveT>
		void operator()(ArchiveT &archive, functions_list &container)
		{
			deserialization_context<address_t> context = { &*container._statistics, };

			if (!container.updates_enabled)
				return;
			archive(context.threadid);
			archive(*container._statistics, context);
			container.on_updated();
		}
	};



	template <typename ArchiveT, typename KeyT>
	inline void serialize(ArchiveT &archive, function_statistics &data, unsigned int/*version*/,
		const deserialization_context<KeyT> &/*context*/)
	{
		function_statistics v;

		archive(v);
		data += v;
	}

	template <typename ArchiveT, typename KeyT>
	inline void serialize(ArchiveT &archive, function_statistics_detailed_t<KeyT> &data, unsigned int/*version*/,
		const deserialization_context<KeyT> &context)
	{
		archive(static_cast<function_statistics &>(data), context);
		archive(data.callees, context);
		update_parent_statistics(*context.map, context.caller, data.callees);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_resolver &data, unsigned int /*version*/)
	{
		archive(data._mappings);
		archive(data._modules);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_resolver::module_info &data, unsigned int /*version*/)
	{
		archive(data.symbols);
		archive(data.files);
	}
}

namespace strmd
{
	template <>
	struct container_traits<micro_profiler::functions_list>
	{
		static const bool is_container = true;
		typedef micro_profiler::functions_list_reader reader_type;
	};

	template <typename KeyT, typename T>
	struct container_traits< std::unordered_map<KeyT, T, micro_profiler::address_hash> >
	{
		static const bool is_container = true;
		typedef micro_profiler::statistics_map_reader reader_type;
	};
}
