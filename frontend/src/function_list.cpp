//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <frontend/nested_statistics_model.h>
#include <frontend/nested_transform.h>
#include <frontend/symbol_resolver.h>
#include <frontend/tables.h>

#include <common/formatting.h>
#include <cmath>
#include <clocale>
#include <stdexcept>
#include <utility>

using namespace std;
using namespace placeholders;

#pragma warning(disable: 4244)

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

		template <typename ResultT, typename T>
		ResultT initialize(const T &value)
		{
			ResultT r = {	value	};
			return r;
		}

		template <typename ContainerT>
		void gcvt(ContainerT &destination, double value)
		{
			const size_t buffer_size = 100;
			wchar_t buffer[buffer_size];
			const int l = swprintf(buffer, buffer_size, L"%g", value);

			if (l > 0)
				destination.append(buffer, buffer + l);
		}

		bool get_thread_native_id(unsigned int &destination, unsigned int id, const tables::threads &threads)
		{
			const auto i = threads.find(id);

			return i != threads.end() ? destination = i->second.native_id, true : false;
		}

		template <typename T>
		void get_thread_native_id(T &destination, unsigned int id, const tables::threads &threads)
		{
			unsigned int native_id;

			if (get_thread_native_id(native_id, id, threads))
				itoa<10>(destination, native_id);
		}

		template <typename ResolverT>
		struct by_name
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return resolver(lhs) < resolver(rhs);	}

			ResolverT resolver;
		};

		struct by_threadid
		{
		public:
			by_threadid(shared_ptr<const tables::threads> threads_)
				: threads(threads_)
			{	}

			template <typename T>
			bool operator ()(const T &lhs_, const T &rhs_) const
			{
				unsigned int lhs = 0u, rhs = 0u;
				const auto lhs_valid = get_thread_native_id(lhs, lhs_.thread_id, *threads);
				const auto rhs_valid = get_thread_native_id(rhs, rhs_.thread_id, *threads);

				return !lhs_valid && !rhs_valid ? false : !lhs_valid ? true : !rhs_valid ? false : lhs < rhs;
			}

		private:
			shared_ptr<const tables::threads> threads;
		};

		struct by_times_called
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return lhs.times_called < rhs.times_called;	}
		};

		struct by_times_called_parents
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return lhs.second < rhs.second;	}
		};

		struct by_exclusive_time
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return lhs.exclusive_time < rhs.exclusive_time;	}
		};

		struct by_inclusive_time
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return lhs.inclusive_time < rhs.inclusive_time;	}
		};

		struct by_avg_exclusive_call_time
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{
				return lhs.times_called && rhs.times_called
					? lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called
					: lhs.times_called < rhs.times_called;
			}
		};

		struct by_avg_inclusive_call_time
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{
				return lhs.times_called && rhs.times_called
					? lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called
					: lhs.times_called < rhs.times_called;
			}
		};

		struct by_max_reentrance
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return lhs.max_reentrance < rhs.max_reentrance;	}
		};

		struct by_max_call_time
		{
			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return lhs.max_call_time < rhs.max_call_time;	}
		};

		struct get_times_called
		{
			template <typename T>
			double operator ()(const T &value) const
			{	return static_cast<double>(value.times_called);	}
		};

		struct exclusive_time
		{
			template <typename T>
			double operator ()(const T &value) const
			{	return tick_interval * value.exclusive_time;	}

			double tick_interval;
		};

		struct inclusive_time
		{
			template <typename T>
			double operator ()(const T &value) const
			{	return tick_interval * value.inclusive_time;	}

			double tick_interval;
		};

		struct exclusive_time_avg
		{
			template <typename T>
			double operator ()(const T &value) const
			{	return value.times_called ? tick_interval * value.exclusive_time / value.times_called : 0.0;	}

			double tick_interval;
		};

		struct inclusive_time_avg
		{
			template <typename T>
			double operator ()(const T &value) const
			{	return value.times_called ? tick_interval * value.inclusive_time / value.times_called : 0.0;	}

			double tick_interval;
		};

		struct max_call_time
		{
			template <typename T>
			double operator ()(const T &value) const
			{	return tick_interval * value.max_call_time;	}

			double tick_interval;
		};

		template <typename T>
		struct format_interval_
		{
			template <typename I>
			void operator ()(agge::richtext_t &text, size_t /*row*/, const I &item) const
			{	format_interval(text, underlying(item));	}

			T underlying;
		};

		template <typename T>
		format_interval_<T> format_interval2(double tick_interval)
		{	return initialize< format_interval_<T> >(initialize<T>(tick_interval));	}

		template <typename ResolverT>
		struct setup_columns_
		{
			template <typename ModelT>
			void operator ()(ModelT &model) const
			{
				auto resolver = _resolver;
				auto threads = _threads;

				model.add_column([] (agge::richtext_t &text, size_t row, const call_statistics &/*item*/) {
					itoa<10>(text, row + 1);
				});
				model.add_column([resolver] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					text << resolver(item).c_str();
				}, initialize< by_name<ResolverT> >(resolver));
				model.add_column([threads] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					get_thread_native_id(text, item.thread_id, *threads);
				}, by_threadid(threads));
				model.add_column([] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					itoa<10>(text, item.times_called);
				}, by_times_called(), [] (const call_statistics &item) -> double {	return item.times_called;	});
				model.add_column(format_interval2<exclusive_time>(tick_interval), by_exclusive_time(), initialize<exclusive_time>(tick_interval));
				model.add_column(format_interval2<inclusive_time>(tick_interval), by_inclusive_time(), initialize<inclusive_time>(tick_interval));
				model.add_column(format_interval2<exclusive_time_avg>(tick_interval), by_avg_exclusive_call_time(), initialize<exclusive_time_avg>(tick_interval));
				model.add_column(format_interval2<inclusive_time_avg>(tick_interval), by_avg_inclusive_call_time(), initialize<inclusive_time_avg>(tick_interval));
				model.add_column([] (agge::richtext_t &text, size_t /*row*/, const call_statistics &item) {
					itoa<10>(text, item.max_reentrance);
				}, by_max_reentrance(), [] (const call_statistics &item) -> double {	return item.max_reentrance;	});
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
		return construct_nested<callees_transform>(underlying, tick_interval, resolver, threads, scope, setup_columns(tick_interval,
			[resolver] (const call_statistics &item) -> const string & {
	
			return resolver->symbol_name_by_va(item.address);
		}, threads));
	}

	shared_ptr<linked_statistics> create_callers_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{
		auto &by_id = underlying->by_id;

		return construct_nested<callers_transform>(underlying, tick_interval, resolver, threads, scope,
			setup_columns(tick_interval, [resolver, &by_id] (const call_statistics &item) -> const string & {

			const auto parent = by_id.find(item.parent_id);

			return parent ? resolver->symbol_name_by_va(parent->address) : c_empty;
		}, threads));
	}


	functions_list::functions_list(shared_ptr<tables::statistics> statistics, double tick_interval_,
			shared_ptr<symbol_resolver> resolver_, shared_ptr<const tables::threads> threads_)
		: base(make_bound< views::filter<tables::statistics> >(statistics), tick_interval_, resolver_, threads_),
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

		content.clear();
		content.reserve(256 * (get_count() + 1)); // kind of magic number
		content += "Function\tTimes Called\tExclusive Time\tInclusive Time\tAverage Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n";
		for (index_type i = 0; i != get_count(); ++i)
		{
			const auto &row = get_entry(i);

			content += resolver->symbol_name_by_va(row.address), content += "\t";
			itoa<10>(content, row.times_called), content += "\t";
			gcvt(content, initialize<exclusive_time>(tick_interval)(row)), content += "\t";
			gcvt(content, initialize<inclusive_time>(tick_interval)(row)), content += "\t";
			gcvt(content, initialize<exclusive_time_avg>(tick_interval)(row)), content += "\t";
			gcvt(content, initialize<inclusive_time_avg>(tick_interval)(row)), content += "\t";
			itoa<10>(content, row.max_reentrance), content += "\t";
			gcvt(content, initialize<max_call_time>(tick_interval)(row)), content += "\r\n";
		}

		if (locale_ok)
			::setlocale(LC_NUMERIC, old_locale);
	}
}
