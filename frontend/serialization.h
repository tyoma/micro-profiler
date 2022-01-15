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

#include "db.h"
#include "profiling_preferences.h"
#include "serialization_context.h"
#include "tables.h"

#include <common/serialization.h>
#include <math/serialization.h>
#include <views/serialization.h>

#pragma warning(disable: 4510; disable: 4610)

namespace strmd
{
	template <typename StreamT, typename PackerT, int static_version>
	class deserializer;

	template <> struct version<micro_profiler::call_statistics> {	enum {	value = 5	};	};
	template <> struct version<micro_profiler::call_statistics_constructor> {	enum {	value = 0	};	};
}

namespace micro_profiler
{
	struct statistics_map_reader : strmd::indexed_associative_container_reader
	{
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	_first = true;	}


		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map_detailed &data, scontext::additive &context)
		{
			scontext::detailed_threaded inner_context = {	&data,	};

			archive(inner_context.threadid); // key: thread_id
			archive(data, inner_context); // value: per-thread statistics
			if (_first)
				context.threads.clear(), _first = false;
			context.threads.push_back(inner_context.threadid);
		}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, statistic_types::map_detailed &data, const scontext::detailed_threaded &context)
		{
			statistic_types::key callee_key(0, context.threadid);

			archive(callee_key.first);

			auto &callee = data[callee_key];
			scontext::detailed_threaded inner = {	&data, callee_key.first, context.threadid	};

			archive(callee, inner);
			if (context.caller)
				(*context.map)[callee_key].callers[statistic_types::key(context.caller, context.threadid)] = callee.times_called;
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

		template <typename ArchiveT, typename ContainerT>
		void read_item(ArchiveT &archive, ContainerT &data, std::pair<statistic_types::map_detailed *,
			statistic_types::key> &caller)
		{
			statistic_types::key callee_key;

			archive(callee_key);
			auto &callee = data[callee_key];
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
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	}
	};

	struct call_nodes_reader : strmd::container_reader_base
	{
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	_first = true;	}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, call_nodes_index &container, scontext::additive &context)
		{
			scontext::hierarchy_node inner = {};

			archive(inner.thread_id);
			archive(container, inner);
			if (_first)
				context.threads.clear(), _first = false;
			context.threads.push_back(inner.thread_id);
		}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, call_nodes_index &container, scontext::hierarchy_node &context) const
		{	read_item_internal<call_statistics>(archive, container, context);	}

		template <typename ArchiveT>
		void read_item(ArchiveT &archive, call_nodes_index &container, scontext::hierarchy_node_1 &context) const
		{	read_item_internal<function_statistics>(archive, container, context);	}

	private:
		template <typename AsT, typename ArchiveT, typename ContextT>
		void read_item_internal(ArchiveT &archive, call_nodes_index &container, ContextT &context) const
		{
			call_node_key key(context.thread_id, context.parent_id, 0);

			archive(std::get<2>(key));
			auto r = container[key];
			archive(static_cast<AsT &>(*r), container);
			r.commit();
		}

	private:
		bool _first;
	};



	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, call_statistics_constructor &data, unsigned int /*ver*/)
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

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, call_statistics &data, call_nodes_index &context, unsigned int ver)
	{
		scontext::hierarchy_node inner = {	data.thread_id, data.id	};
		
		archive(static_cast<function_statistics &>(data), context);
		if (ver < 5)
		{
			scontext::hierarchy_node_1 inner1 = {	data.thread_id, data.id	};
			
			archive(context, inner1); // callees
			return;
		}
		archive(context, inner); // callees
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, statistic_types::function &data, const ContextT &/*context*/, unsigned int ver)
	{
		function_statistics v;

		serialize(archive, v, ver);
		data += v;
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, statistic_types::function_detailed &data, ContextT &context, unsigned int ver)
	{
		archive(static_cast<function_statistics &>(data), context);
		if (ver < 5)
		{
			statistic_types::map callees;

			archive(callees, context);
			for (auto i = callees.begin(); i != callees.end(); ++i)
				static_cast<statistic_types::function &>(data.callees[i->first]) = i->second;
			return;
		}
		archive(data.callees, context);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, module_profiling_preferences &data, unsigned int /*ver*/);

	namespace tables
	{
		template <typename ArchiveT, typename BaseT>
		inline void serialize(ArchiveT &archive, table<BaseT> &data)
		{
			archive(static_cast<BaseT &>(data));
			data.invalidate();
		}

		template <typename ArchiveT, typename ContextT, typename BaseT>
		inline void serialize(ArchiveT &archive, table<BaseT> &data, ContextT &context)
		{
			archive(static_cast<BaseT &>(data), context);
			data.invalidate();
		}

		template <typename ArchiveT, typename ContextT>
		inline void serialize(ArchiveT &archive, statistics &data, ContextT &context)
		{
			archive(data.by_node, context);
			data.invalidate();
		}

		template <typename S, typename P, int v>
		inline void serialize(strmd::deserializer<S, P, v> &archive, module_mappings &data)
		{
			archive(static_cast<module_mappings::base_t &>(data));
			data.layout.assign(data.begin(), data.end());
			std::sort(data.layout.begin(), data.layout.end(), mapping_less());
		}
	}
}

namespace strmd
{
	template <typename T, typename A>
	struct type_traits< micro_profiler::containers::unordered_map<micro_profiler::function_key, T,
		micro_profiler::knuth_hash, std::equal_to<micro_profiler::function_key>, A> >
	{
		typedef container_type_tag category;
		typedef micro_profiler::statistics_map_reader item_reader_type;
	};

	template <typename T, typename H, typename A>
	struct type_traits< micro_profiler::containers::unordered_map<T, micro_profiler::thread_info, H,
		std::equal_to<T>, A> >
	{
		typedef container_type_tag category;
		typedef micro_profiler::threads_model_reader item_reader_type;
	};

	template <>
	struct type_traits<micro_profiler::call_nodes_index>
	{
		typedef container_type_tag category;
		typedef micro_profiler::call_nodes_reader item_reader_type;
	};
}
