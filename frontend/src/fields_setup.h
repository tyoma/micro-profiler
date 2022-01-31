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

#include "fields.h"

#include <agge.text/richtext.h>

namespace micro_profiler
{
	template <typename ResolverT>
	struct fields_setup
	{
		template <typename ModelT>
		void operator ()(ModelT &model) const
		{
			auto resolver = _resolver;

			model.add_column([] (agge::richtext_t &t, size_t row, const call_statistics &) {	itoa<10>(t, row + 1);	});
			model.add_column([resolver] (agge::richtext_t &t, size_t, const call_statistics &r) {	t << resolver(r).c_str();	})
				.on_set_order(initialize< by_name<ResolverT> >(resolver));
			model.add_column(initialize<thread_native_id>(_threads))
				.on_set_order(initialize<by_threadid>(_threads));
			model.add_column([] (agge::richtext_t &t, size_t, const call_statistics &r) {	itoa<10>(t, r.times_called);	})
				.on_set_order(by_times_called())
				.on_project([] (const call_statistics &r) {	return static_cast<double>(r.times_called);	});
			model.add_column(format_interval2<exclusive_time>(tick_interval))
				.on_set_order(by_exclusive_time())
				.on_project(initialize<exclusive_time>(tick_interval));
			model.add_column(format_interval2<inclusive_time>(tick_interval))
				.on_set_order(by_inclusive_time())
				.on_project(initialize<inclusive_time>(tick_interval));
			model.add_column(format_interval2<exclusive_time_avg>(tick_interval))
				.on_set_order(by_avg_exclusive_call_time())
				.on_project(initialize<exclusive_time_avg>(tick_interval));
			model.add_column(format_interval2<inclusive_time_avg>(tick_interval))
				.on_set_order(by_avg_inclusive_call_time())
				.on_project(initialize<inclusive_time_avg>(tick_interval));
			model.add_column([] (agge::richtext_t &t, size_t, const call_statistics &r) {	itoa<10>(t, r.max_reentrance);	})
				.on_set_order(by_max_reentrance())
				.on_project([] (const call_statistics &r) {	return static_cast<double>(r.max_reentrance);	});
			model.add_column(format_interval2<max_call_time>(tick_interval))
				.on_set_order(by_max_call_time())
				.on_project(initialize<max_call_time>(tick_interval));
		}

		double tick_interval;
		ResolverT _resolver;
		std::shared_ptr<const tables::threads> _threads;
	};

	template <typename ResolverT>
	inline fields_setup<ResolverT> setup_columns(double tick_interval, const ResolverT &resolver,
		std::shared_ptr<const tables::threads> threads)
	{
		fields_setup<ResolverT> s = {	tick_interval, resolver, threads	};
		return s;
	}
}
