//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <frontend/function_list.h>

#include "fields.h"

#include <frontend/nested_statistics_model.h>
#include <frontend/nested_transform.h>
#include <frontend/symbol_resolver.h>

#include <cmath>
#include <clocale>
#include <stdexcept>
#include <utility>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace
	{
		static const string c_empty;

		template <typename T, typename U>
		shared_ptr<T> make_bound(shared_ptr<U> underlying)
		{
			typedef pair<shared_ptr<U>, T> pair_t;
			auto p = make_shared<pair_t>(underlying, T(*underlying));
			return shared_ptr<T>(p, &p->second);
		}

		template <typename ResolverT>
		struct setup_columns_
		{
			template <typename ModelT>
			void operator ()(ModelT &model) const
			{
				auto resolver = _resolver;

				model.add_column([] (agge::richtext_t &text, size_t row, const call_statistics &/*item*/) {
					itoa<10>(text, row + 1);
				});
				model.add_column([resolver] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					text << resolver(item).c_str();
				}, initialize< by_name<ResolverT> >(resolver));
				model.add_column(initialize<thread_native_id>(_threads), initialize<by_threadid>(_threads));
				model.add_column([] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					itoa<10>(text, item.times_called);
				}, by_times_called(), [] (const call_statistics &item) {	return static_cast<double>(item.times_called);	});
				model.add_column(format_interval2<exclusive_time>(tick_interval), by_exclusive_time(), initialize<exclusive_time>(tick_interval));
				model.add_column(format_interval2<inclusive_time>(tick_interval), by_inclusive_time(), initialize<inclusive_time>(tick_interval));
				model.add_column(format_interval2<exclusive_time_avg>(tick_interval), by_avg_exclusive_call_time(), initialize<exclusive_time_avg>(tick_interval));
				model.add_column(format_interval2<inclusive_time_avg>(tick_interval), by_avg_inclusive_call_time(), initialize<inclusive_time_avg>(tick_interval));
				model.add_column([] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					itoa<10>(text, item.max_reentrance);
				}, by_max_reentrance(), [] (const call_statistics &item) {	return static_cast<double>(item.max_reentrance);	});
				model.add_column(format_interval2<max_call_time>(tick_interval), by_max_call_time(),
					initialize<max_call_time>(tick_interval));
			}

			double tick_interval;
			ResolverT _resolver;
			shared_ptr<const tables::threads> _threads;
		};

		template <typename ResolverT>
		setup_columns_<ResolverT> setup_columns(double tick_interval, const ResolverT &resolver,
			shared_ptr<const tables::threads> threads)
		{
			setup_columns_<ResolverT> s = {	tick_interval, resolver, threads	};
			return s;
		}
	}


	shared_ptr<linked_statistics> create_callees_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{
		return construct_nested<callees_transform>(underlying, tick_interval, resolver, scope,
			setup_columns(tick_interval, [resolver] (const call_statistics &item) -> const string & {

			return resolver->symbol_name_by_va(item.address);
		}, threads));
	}

	shared_ptr<linked_statistics> create_callers_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{
		auto &by_id = underlying->by_id;

		return construct_nested<callers_transform>(underlying, tick_interval, resolver, scope,
			setup_columns(tick_interval, [resolver, &by_id] (const call_statistics &item) -> const string & {

			const auto parent = by_id.find(item.parent_id);

			return parent ? resolver->symbol_name_by_va(parent->address) : c_empty;
		}, threads));
	}


	functions_list::functions_list(shared_ptr<tables::statistics> statistics, double tick_interval_,
			shared_ptr<symbol_resolver> resolver_, shared_ptr<const tables::threads> threads)
		: base(make_bound< views::filter<tables::statistics> >(statistics), tick_interval_, resolver_),
			_statistics(statistics)
	{
		auto setup = setup_columns(tick_interval,
			[resolver_] (const call_statistics &item) -> const string & {
	
			return resolver_->symbol_name_by_va(item.address);
		}, threads);

		setup(*this);
		_connection = statistics->invalidate += [this] {	fetch();	};
	}

	functions_list::~functions_list()
	{	}

	void functions_list::clear()
	{	_statistics->clear();	}

	void functions_list::print(string &content) const
	{
		const char* old_locale = ::setlocale(LC_NUMERIC, NULL);
		bool locale_ok = ::setlocale(LC_NUMERIC, "") != NULL;
		const auto gcvt = [&content] (double value) {
			const size_t buffer_size = 100;
			wchar_t buffer[buffer_size];
			const int l = std::swprintf(buffer, buffer_size, L"%g", value);

			if (l > 0)
				content.append(buffer, buffer + l);
		};

		content.clear();
		content.reserve(256 * (get_count() + 1)); // kind of magic number
		content += "Function\tTimes Called\tExclusive Time\tInclusive Time\tAverage Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n";
		for (index_type i = 0; i != get_count(); ++i)
		{
			const auto &row = get_entry(i);

			content += resolver->symbol_name_by_va(row.address), content += "\t";
			itoa<10>(content, row.times_called), content += "\t";
			gcvt(initialize<exclusive_time>(tick_interval)(row)), content += "\t";
			gcvt(initialize<inclusive_time>(tick_interval)(row)), content += "\t";
			gcvt(initialize<exclusive_time_avg>(tick_interval)(row)), content += "\t";
			gcvt(initialize<inclusive_time_avg>(tick_interval)(row)), content += "\t";
			itoa<10>(content, row.max_reentrance), content += "\t";
			gcvt(initialize<max_call_time>(tick_interval)(row)), content += "\r\n";
		}

		if (locale_ok)
			::setlocale(LC_NUMERIC, old_locale);
	}
}
