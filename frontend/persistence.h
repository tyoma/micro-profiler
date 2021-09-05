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
#include "serialization.h"
#include "symbol_resolver.h"
#include "tables.h"

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



	template <typename ArchiveT>
	inline void reciprocal(ArchiveT &archive, double &value)
	{
		long long rv = static_cast<long long>(value ? 1 / value : 1);

		archive(rv);
		value = 1.0 / rv;
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, function_list_serialization_proxy &data, const scontext::file_v3 &/*context*/)
	{
		scontext::detailed_threaded context = { &data.statistics, 0, 0 };

		reciprocal(archive, data.tick_interval);
		archive(data.resolver);
		archive(data.statistics, context);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, function_list_serialization_proxy &data, const scontext::file_v4 &/*context*/)
	{
		reciprocal(archive, data.tick_interval);
		archive(data.resolver);
		archive(data.statistics);
		archive(data.threads);
	}

	template <typename ArchiveT, typename ContextT>
	inline void serialize(ArchiveT &archive, functions_list &data, ContextT &context)
	{
		function_list_serialization_proxy proxy = {
			data,
			data.tick_interval,
			*data.resolver,
			*data._statistics,
			*data.threads
		};

		archive(proxy, context);
		data._statistics->invalidate();
	}

	template <typename ContextT, typename ArchiveT>
	inline void snapshot_save(ArchiveT &archive, const functions_list &model)
	{
		ContextT context;
		archive(model, context);
	}

	template <typename ContextT, typename ArchiveT>
	inline std::shared_ptr<functions_list> snapshot_load(ArchiveT &archive)
	{
		struct dummy_
		{
			static void dummy_threads_request(const std::vector<unsigned int> &)
			{	}
		};

		ContextT context;
		auto statistics = std::make_shared<tables::statistics>();
		auto modules = std::make_shared<tables::modules>();
		auto mappings = std::make_shared<tables::module_mappings>();
		auto resolver = std::make_shared<symbol_resolver>(modules, mappings);
		auto threads = std::make_shared<threads_model>(&dummy_::dummy_threads_request);
		auto fl = std::make_shared<functions_list>(statistics, 1, resolver, threads);

		archive(*fl, context);
		return fl;
	}

	namespace tables
	{
		template <typename ArchiveT, typename ContextT, typename BaseT>
		inline void serialize(ArchiveT &archive, table<BaseT> &data, ContextT &context)
		{
			archive(static_cast<BaseT &>(data), context);
			data.invalidate();
		}
	}
}
