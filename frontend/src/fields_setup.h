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

#include <frontend/column_definition.h>
#include <agge.text/richtext.h>

namespace micro_profiler
{
	template <typename ResolverT>
	struct fields_setup
	{
		template <typename ModelT>
		void operator ()(ModelT &model) const
		{
			using namespace agge;
			using namespace std;

			typedef function<void (richtext_t &text, size_t row, const call_statistics &record)> get_text_cb;
			typedef function<bool (const call_statistics &lhs, const call_statistics &rhs)> less_cb;
			typedef function<double (const call_statistics &record)> get_value_cb;

			auto resolver = _resolver;
			auto make1 = [] (const get_text_cb &get_text) -> column_definition<call_statistics> {
				column_definition<call_statistics> c = {
					string(), agge::style_modifier::empty, 0, agge::align_near,	get_text,
				};
				return c;
			};
			auto make2 = [make1] (const get_text_cb &get_text, const less_cb &less) -> column_definition<call_statistics> {
				auto c = make1(get_text);

				c.less = less;
				return c;
			};
			auto make3 = [make2] (const get_text_cb &get_text, const less_cb &less, const get_value_cb &get_value) -> column_definition<call_statistics> {
				auto c = make2(get_text, less);

				c.get_value = get_value;
				return c;
			};

			column_definition<call_statistics> columns[] = {
				make1([] (richtext_t &t, size_t row, const call_statistics &) {	itoa<10>(t, row + 1);	}),
				make2([resolver] (richtext_t &t, size_t, const call_statistics &r) {	t << resolver(r).c_str();	}, initialize< by_name<ResolverT> >(resolver)),
				make2(initialize<thread_native_id>(_threads), initialize<by_threadid>(_threads)),
				make3([] (richtext_t &t, size_t, const call_statistics &r) {	itoa<10>(t, r.times_called);	}, by_times_called(), [] (const call_statistics &r) {	return static_cast<double>(r.times_called);	}),
				make3(format_interval2<exclusive_time>(tick_interval), by_exclusive_time(), initialize<exclusive_time>(tick_interval)),
				make3(format_interval2<inclusive_time>(tick_interval), by_inclusive_time(), initialize<inclusive_time>(tick_interval)),
				make3(format_interval2<exclusive_time_avg>(tick_interval), by_avg_exclusive_call_time(), initialize<exclusive_time_avg>(tick_interval)),
				make3(format_interval2<inclusive_time_avg>(tick_interval), by_avg_inclusive_call_time(), initialize<inclusive_time_avg>(tick_interval)),
				make3([] (richtext_t &t, size_t, const call_statistics &r) {	itoa<10>(t, r.max_reentrance);	}, by_max_reentrance(), [] (const call_statistics &r) {	return static_cast<double>(r.max_reentrance);	}),
				make3(format_interval2<max_call_time>(tick_interval), by_max_call_time(), initialize<max_call_time>(tick_interval)),
			};

			model.add_columns(columns);
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
