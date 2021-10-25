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

#include "serialization_context.h"
#include "tables.h"

#include <common/serialization.h>
#include <math/serialization.h>

#pragma warning(disable: 4510; disable: 4610)

namespace strmd
{
	template <typename StreamT, typename PackerT, int static_version>
	class deserializer;
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
		template <typename ContainerT>
		void prepare(ContainerT &/*data*/, size_t /*count*/)
		{	}
	};



	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, statistic_types::function &data, const ContextT &/*context*/, unsigned int ver)
	{
		function_statistics v;

		serialize(archive, v, ver);
		data += v;
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, statistic_types::function_detailed &data, ContextT &context, unsigned int /*ver*/)
	{
		archive(static_cast<function_statistics &>(data), context);
		archive(data.callees, context);
	}

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
}
