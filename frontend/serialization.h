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

#include "function_list.h"
#include "serialization_context.h"
#include "symbol_resolver.h"
#include "tables.h"
#include "threads_model.h"

#include <common/serialization.h>

#pragma warning(disable: 4510; disable: 4610)

namespace micro_profiler
{
	struct function_list_serialization_proxy
	{
		functions_list &self;
		double &tick_interval;
		symbol_resolver &resolver;
		statistic_types::map_detailed &statistics;
		threads_model &threads;
	};


	struct statistics_map_reader : strmd::indexed_associative_container_reader
	{
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	_first = true;	}


		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map_detailed &data, scontext::wire &context)
		{
			scontext::detailed_threaded inner_context;

			archive(inner_context.threadid); // key: thread_id
			archive(data, inner_context); // value: per-thread statistics
			if (_first)
				context.threads.clear(), _first = false;
			context.threads.push_back(inner_context.threadid);
		}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map_detailed &data, const scontext::detailed_threaded &context)
		{
			scontext::detailed_threaded new_context = { &data, 0, context.threadid };

			archive(new_context.caller);
			archive(data[statistic_types::key(new_context.caller, new_context.threadid)], new_context);
		}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map &data, const scontext::detailed_threaded &context)
		{
			statistic_types::key callee_key(0, context.threadid);

			archive(callee_key.first);
			statistic_types::function &callee = data[callee_key];
			archive(callee, context);
			(*context.map)[callee_key].callers[statistic_types::key(context.caller, context.threadid)] = callee.times_called;
		}


		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map_detailed &data)
		{
			std::pair<statistic_types::map_detailed *, statistic_types::key> caller_context;

			caller_context.first = &data;
			archive(caller_context.second);
			archive(data[caller_context.second], caller_context);
		}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map &data, std::pair<statistic_types::map_detailed *, statistic_types::key> &caller)
		{
			statistic_types::key callee_key;

			archive(callee_key);
			statistic_types::function &callee = data[callee_key];
			archive(callee);
			(*caller.first)[callee_key].callers[caller.second] = callee.times_called;
		}


		template <typename ArchiveT, typename ContainerT>
		void read_item(ArchiveT &archive, ContainerT &data)
		{	strmd::indexed_associative_container_reader::read_item(archive, data);	}

	private:
		bool _first;
	};

	struct threads_model_reader : strmd::indexed_associative_container_reader
	{
		void prepare(threads_model &/*container*/, size_t /*count*/)
		{	}

		void complete(threads_model &container)
		{
			container._view.fetch();
			container._trackables.fetch();
			container.invalidate(threads_model::npos());
		}
	};



	template <typename ArchiveT, typename ContextT>
	void serialize(ArchiveT &archive, functions_list &data, ContextT &context)
	{
		function_list_serialization_proxy proxy = {
			data,
			data._tick_interval,
			*data.get_resolver(),
			*data._statistics,
			*data.get_threads()
		};

		archive(proxy, context);
		if (data.updates_enabled)
			data._statistics->invalidated();
	}

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, function_list_serialization_proxy &data, scontext::wire &context)
	{
		if (data.self.updates_enabled)
			archive(data.statistics, context);
	}

	template <typename ArchiveT>
	void reciprocal(ArchiveT &archive, double &value)
	{
		long long rv = static_cast<long long>(value ? 1 / value : 1);

		archive(rv);
		value = 1.0 / rv;
	}

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, function_list_serialization_proxy &data, const scontext::file_v3 &/*context*/)
	{
		scontext::detailed_threaded context = { &data.statistics, 0, 0 };

		reciprocal(archive, data.tick_interval);
		archive(data.resolver);
		archive(data.statistics, context);
	}

	template <typename ArchiveT>
	void serialize(ArchiveT &archive, function_list_serialization_proxy &data, const scontext::file_v4 &/*context*/)
	{
		reciprocal(archive, data.tick_interval);
		archive(data.resolver);
		archive(data.statistics);
		archive(data.threads);
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, statistic_types::function &data, const ContextT &/*context*/)
	{
		function_statistics v;

		archive(v);
		data += v;
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, statistic_types::function_detailed &data, ContextT &context)
	{
		archive(static_cast<function_statistics &>(data), context);
		archive(data.callees, context);
	}

	namespace tables
	{
		template <typename ArchiveT>
		inline void serialize(ArchiveT &archive, module_info &data)
		{
			archive(data.symbols);
			archive(data.files);
		}
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_resolver &data)
	{
		archive(static_cast<containers::unordered_map<unsigned int /*instance_id*/, mapped_module_identified> &>(*data._mappings));
		archive(static_cast<containers::unordered_map<unsigned int /*persistent_id*/, tables::module_info> &>(*data._modules));

		data._modules->invalidated();
		data._mappings->invalidated();
	}
}

namespace strmd
{
	template <typename T>
	struct type_traits< micro_profiler::containers::unordered_map<micro_profiler::function_key, T,
		micro_profiler::knuth_hash> >
	{
		typedef container_type_tag category;
		typedef micro_profiler::statistics_map_reader item_reader_type;
	};

	template <>
	struct type_traits<micro_profiler::threads_model>
	{
		typedef container_type_tag category;
		typedef micro_profiler::threads_model_reader item_reader_type;
	};
}
