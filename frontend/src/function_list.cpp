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
using namespace wpl;

#pragma warning(disable: 4244)

namespace micro_profiler
{
	namespace
	{
		template <typename T, typename U>
		shared_ptr<T> make_bound(shared_ptr<U> underlying)
		{
			typedef pair<shared_ptr<U>, T> pair_t;
			auto p = make_shared<pair_t>(underlying, T(*underlying));
			return shared_ptr<T>(p, &p->second);
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

		class by_name
		{
		public:
			by_name(shared_ptr<const symbol_resolver> resolver)
				: resolver(resolver)
			{	}

			template <typename T>
			bool operator ()(const T &lhs, const T &rhs) const
			{	return resolver->symbol_name_by_va(lhs.address) < resolver->symbol_name_by_va(rhs.address);	}

		private:
			shared_ptr<const symbol_resolver> resolver;
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
			exclusive_time(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			template <typename T>
			double operator ()(const T &value) const
			{	return _inv_tick_count * value.exclusive_time;	}

			double _inv_tick_count;
		};

		struct inclusive_time
		{
			inclusive_time(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			template <typename T>
			double operator ()(const T &value) const
			{	return _inv_tick_count * value.inclusive_time;	}

			double _inv_tick_count;
		};

		struct exclusive_time_avg
		{
			exclusive_time_avg(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			template <typename T>
			double operator ()(const T &value) const
			{	return value.times_called ? _inv_tick_count * value.exclusive_time / value.times_called : 0.0;	}

			double _inv_tick_count;
		};

		struct inclusive_time_avg
		{
			inclusive_time_avg(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			template <typename T>
			double operator ()(const T &value) const
			{	return value.times_called ? _inv_tick_count * value.inclusive_time / value.times_called : 0.0;	}

			double _inv_tick_count;
		};

		struct max_call_time
		{
			max_call_time(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			template <typename T>
			double operator ()(const T &value) const
			{	return _inv_tick_count * value.max_call_time;	}

			double _inv_tick_count;
		};


		template <typename ItemT>
		void get_column_text(agge::richtext_t &text, size_t index, const ItemT &item, size_t column, double tick_interval,
			const symbol_resolver &resolver, const tables::threads &threads)
		{
			switch (column)
			{
			case 0:	itoa<10>(text, index + 1);	break;
			case 1:	text << resolver.symbol_name_by_va(item.address).c_str();	break;
			case 2:	get_thread_native_id(text, item.thread_id, threads);	break;
			case 3:	itoa<10>(text, item.times_called);	break;
			case 4:	format_interval(text, exclusive_time(tick_interval)(item));	break;
			case 5:	format_interval(text, inclusive_time(tick_interval)(item));	break;
			case 6:	format_interval(text, exclusive_time_avg(tick_interval)(item));	break;
			case 7:	format_interval(text, inclusive_time_avg(tick_interval)(item));	break;
			case 8:	itoa<10>(text, item.max_reentrance);	break;
			case 9:	format_interval(text, max_call_time(tick_interval)(item));	break;
			}
		}

		template <typename ViewT>
		void set_column_order(void *, ViewT &view, size_t column, bool ascending, double tick_interval,
			shared_ptr<const symbol_resolver> resolver, shared_ptr<const tables::threads> threads)
		{
			switch (column)
			{
			case 0:
				view.projection.project();
				break;

			case 1:
				view.ordered.set_order(by_name(resolver), ascending);
				view.projection.project();
				break;

			case 2:
				view.ordered.set_order(by_threadid(threads), ascending);
				view.projection.project();
				break;

			case 3:
				view.ordered.set_order(by_times_called(), ascending);
				view.projection.project(get_times_called());
				break;

			case 4:
				view.ordered.set_order(by_exclusive_time(), ascending);
				view.projection.project(exclusive_time(tick_interval));
				break;

			case 5:
				view.ordered.set_order(by_inclusive_time(), ascending);
				view.projection.project(inclusive_time(tick_interval));
				break;

			case 6:
				view.ordered.set_order(by_avg_exclusive_call_time(), ascending);
				view.projection.project(exclusive_time_avg(tick_interval));
				break;

			case 7:
				view.ordered.set_order(by_avg_inclusive_call_time(), ascending);
				view.projection.project(inclusive_time_avg(tick_interval));
				break;

			case 8:
				view.ordered.set_order(by_max_reentrance(), ascending);
				view.projection.project();
				break;

			case 9:
				view.ordered.set_order(by_max_call_time(), ascending);
				view.projection.project(max_call_time(tick_interval));
				break;
			}
		}

		template <>
		void get_column_text<statistic_types::map_callers::value_type>(agge::richtext_t &text, size_t index,
			const statistic_types::map_callers::value_type &item, size_t column, double /*tick_interval*/,
			const symbol_resolver &resolver, const tables::threads &/*threads*/)
		{
			switch (column)
			{
			case 0:	itoa<10>(text, index + 1);	break;
			case 1:	text << resolver.symbol_name_by_va(item.first.first).c_str();	break;
			case 3:	itoa<10>(text, item.second);	break;
			}
		}

		template <typename ViewT>
		void set_column_order(statistic_types::map_callers::value_type *, ViewT &view, size_t column, bool ascending,
			double /*tick_interval*/, shared_ptr<const symbol_resolver> resolver, shared_ptr<const tables::threads> /*threads*/)
		{
			switch (column)
			{
			case 1:	view.ordered.set_order(by_name(resolver), ascending);	break;
			case 3:	view.ordered.set_order(by_times_called_parents(), ascending);	break;
			}
		}
	}


	template <typename BaseT, typename U>
	void statistics_model_impl<BaseT, U>::get_text(index_type row, index_type column, agge::richtext_t &text) const
	{	get_column_text(text, row, _view->ordered[row], column, tick_interval, *resolver, *threads);	}

	template <typename BaseT, typename U>
	void statistics_model_impl<BaseT, U>::set_order(index_type column, bool ascending)
	{
		set_column_order(static_cast<typename U::value_type *>(nullptr), *_view, column, ascending, tick_interval,
			resolver, threads);
		_view->trackables.fetch();
		_view->projection.fetch();
		this->invalidate(this->npos());
	}


	shared_ptr<linked_statistics> create_callees_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{	return construct_nested<callees_transform>(underlying, tick_interval, resolver, threads, scope);	}

	shared_ptr<linked_statistics> create_callers_model(shared_ptr<const tables::statistics> underlying,
		double tick_interval, shared_ptr<symbol_resolver> resolver, shared_ptr<const tables::threads> threads,
		shared_ptr< vector<id_t> > scope)
	{	return construct_nested<callers_transform>(underlying, tick_interval, resolver, threads, scope);	}


	functions_list::functions_list(shared_ptr<tables::statistics> statistics, double tick_interval_,
			shared_ptr<symbol_resolver> resolver_, shared_ptr<const tables::threads> threads_)
		: base(make_bound< views::filter<tables::statistics> >(statistics), tick_interval_, resolver_,
			threads_), _statistics(statistics)
	{	_connection = statistics->invalidate += [this] {	fetch();	};	}

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

			content += resolver->symbol_name_by_va(row.address) + "\t";
			itoa<10>(content, row.times_called), content += "\t";
			gcvt(content, exclusive_time(tick_interval)(row)), content += "\t";
			gcvt(content, inclusive_time(tick_interval)(row)), content += "\t";
			gcvt(content, exclusive_time_avg(tick_interval)(row)), content += "\t";
			gcvt(content, inclusive_time_avg(tick_interval)(row)), content += "\t";
			itoa<10>(content, row.max_reentrance), content += "\t";
			gcvt(content, max_call_time(tick_interval)(row)), content += "\r\n";
		}

		if (locale_ok)
			::setlocale(LC_NUMERIC, old_locale);
	}
}
