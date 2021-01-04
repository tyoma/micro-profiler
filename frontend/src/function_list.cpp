//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include <frontend/symbol_resolver.h>
#include <frontend/threads_model.h>

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
		template <typename ContainerT>
		void gcvt(ContainerT &destination, double value)
		{
			const size_t buffer_size = 100;
			wchar_t buffer[buffer_size];
			const int l = swprintf(buffer, buffer_size, L"%g", value);

			if (l > 0)
				destination.append(buffer, buffer + l);
		}

		template <typename DestT, typename SrcT>
		void assign(DestT &dest, const SrcT &src)
		{
			dest.resize(src.size());
			auto d = dest.begin();
			for (auto s = src.begin(); s != src.end(); ++d, ++s)
				*d = *s;
		}

		class by_name
		{
		public:
			by_name(const shared_ptr<symbol_resolver> &resolver)
				: _resolver(resolver)
			{	}

			template <typename AnyT>
			bool operator ()(function_key lhs_addr, const AnyT &, function_key rhs_addr, const AnyT &) const
			{	return _resolver->symbol_name_by_va(lhs_addr.first) < _resolver->symbol_name_by_va(rhs_addr.first);	}

		private:
			shared_ptr<symbol_resolver> _resolver;
		};

		struct by_threadid
		{
		public:
			by_threadid(const shared_ptr<threads_model> &threads)
				: _threads(threads)
			{	}

			bool operator ()(const function_key &lhs_, const function_statistics &, const function_key &rhs_,
				const function_statistics &) const
			{
				unsigned int lhs = 0u, rhs = 0u;

				return (_threads->get_native_id(lhs, lhs_.second), lhs) < (_threads->get_native_id(rhs, rhs_.second), rhs);
			}

		private:
			shared_ptr<threads_model> _threads;
		};

		struct by_times_called
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.times_called < rhs.times_called;	}

			bool operator ()(function_key, count_t lhs, function_key, count_t rhs) const
			{	return lhs < rhs;	}
		};

		struct by_exclusive_time
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.exclusive_time < rhs.exclusive_time;	}
		};

		struct by_inclusive_time
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.inclusive_time < rhs.inclusive_time;	}
		};

		struct by_avg_exclusive_call_time
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.times_called && rhs.times_called ? lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called : lhs.times_called < rhs.times_called;	}
		};

		struct by_avg_inclusive_call_time
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.times_called && rhs.times_called ? lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called : lhs.times_called < rhs.times_called;	}
		};

		struct by_max_reentrance
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.max_reentrance < rhs.max_reentrance;	}
		};

		struct by_max_call_time
		{
			bool operator ()(function_key, const function_statistics &lhs, function_key, const function_statistics &rhs) const
			{	return lhs.max_call_time < rhs.max_call_time;	}
		};

		struct get_times_called
		{
			double operator ()(const function_statistics &value) const
			{	return static_cast<double>(value.times_called);	}
		};

		struct exclusive_time
		{
			exclusive_time(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			double operator ()(const function_statistics &value) const
			{	return _inv_tick_count * value.exclusive_time;	}

			double _inv_tick_count;
		};

		struct inclusive_time
		{
			inclusive_time(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			double operator ()(const function_statistics &value) const
			{	return _inv_tick_count * value.inclusive_time;	}

			double _inv_tick_count;
		};

		struct exclusive_time_avg
		{
			exclusive_time_avg(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			double operator ()(const function_statistics &value) const
			{	return value.times_called ? _inv_tick_count * value.exclusive_time / value.times_called : 0.0;	}

			double _inv_tick_count;
		};

		struct inclusive_time_avg
		{
			inclusive_time_avg(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			double operator ()(const function_statistics &value) const
			{	return value.times_called ? _inv_tick_count * value.inclusive_time / value.times_called : 0.0;	}

			double _inv_tick_count;
		};

		struct max_call_time
		{
			max_call_time(double inv_tick_count)
				: _inv_tick_count(inv_tick_count)
			{	}

			double operator ()(const function_statistics &value) const
			{	return _inv_tick_count * value.max_call_time;	}

			double _inv_tick_count;
		};

		template <typename T>
		void erase_entry(linked_statistics_ex *p, const shared_ptr<T> &container, typename T::iterator i)
		{
			container->erase(i);
			delete p;
		}
	}


	typedef statistics_model_impl<linked_statistics_ex, statistic_types::map> children_statistics;
	typedef statistics_model_impl<linked_statistics_ex, statistic_types::map_callers> parents_statistics;



	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::get_text(index_type item, index_type subitem, wstring &text) const
	{
		unsigned int tmp;
		const typename view_type::value_type &row = get_entry(item);

		text.clear();
		switch (subitem)
		{
		case 0:	itoa<10>(text, item + 1);	break;
		case 1:	assign(text, _resolver->symbol_name_by_va(row.first.first));	break;
		case 2:	_threads->get_native_id(tmp, row.first.second) ? itoa<10>(text, tmp), 0 : 0;	break;
		case 3:	itoa<10>(text, row.second.times_called);	break;
		case 4:	format_interval(text, exclusive_time(_tick_interval)(row.second));	break;
		case 5:	format_interval(text, inclusive_time(_tick_interval)(row.second));	break;
		case 6:	format_interval(text, exclusive_time_avg(_tick_interval)(row.second));	break;
		case 7:	format_interval(text, inclusive_time_avg(_tick_interval)(row.second));	break;
		case 8:	itoa<10>(text, row.second.max_reentrance);	break;
		case 9:	format_interval(text, max_call_time(_tick_interval)(row.second));	break;
		}
	}

	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 0:
			_view->disable_projection();
			break;

		case 1:
			_view->set_order(by_name(_resolver), ascending);
			_view->disable_projection();
			break;

		case 2:
			_view->set_order(by_threadid(get_threads()), ascending);
			// _view->disable_projection();
			break;

		case 3:
			_view->set_order(by_times_called(), ascending);
			_view->project_value(get_times_called());
			break;

		case 4:
			_view->set_order(by_exclusive_time(), ascending);
			_view->project_value(exclusive_time(_tick_interval));
			break;

		case 5:
			_view->set_order(by_inclusive_time(), ascending);
			_view->project_value(inclusive_time(_tick_interval));
			break;

		case 6:
			_view->set_order(by_avg_exclusive_call_time(), ascending);
			_view->project_value(exclusive_time_avg(_tick_interval));
			break;

		case 7:
			_view->set_order(by_avg_inclusive_call_time(), ascending);
			_view->project_value(inclusive_time_avg(_tick_interval));
			break;

		case 8:
			_view->set_order(by_max_reentrance(), ascending);
			_view->disable_projection();
			break;

		case 9:
			_view->set_order(by_max_call_time(), ascending);
			_view->project_value(max_call_time(_tick_interval));
			break;
		}
		on_updated();
	}


	template <>
	void statistics_model_impl<linked_statistics_ex, statistic_types::map_callers>::get_text(index_type item, index_type subitem,
		wstring &text) const
	{
		const statistic_types::map_callers::value_type &row = get_entry(item);

		text.clear();
		switch (subitem)
		{
		case 0:	itoa<10>(text, item + 1);	break;
		case 1:	assign(text, _resolver->symbol_name_by_va(row.first.first));	break;
		case 3:	itoa<10>(text, row.second);	break;
		}
	}

	template <>
	void statistics_model_impl<linked_statistics_ex, statistic_types::map_callers>::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view->set_order(by_name(_resolver), ascending);	break;
		case 3:	_view->set_order(by_times_called(), ascending);	break;
		}
		this->invalidate(_view->get_count());
	}


	functions_list::functions_list(shared_ptr<statistic_types::map_detailed> statistics, double tick_interval,
			shared_ptr<symbol_resolver> resolver, std::shared_ptr<threads_model> threads)
		: base(*statistics, tick_interval, resolver, threads), updates_enabled(true), _statistics(statistics),
			_linked(new linked_statistics_list_t)
	{	}

	functions_list::~functions_list()
	{
		for (linked_statistics_list_t::const_iterator i = _linked->begin(); i != _linked->end(); ++i)
			(*i)->detach();
	}

	void functions_list::clear()
	{
		for (linked_statistics_list_t::const_iterator i = _linked->begin(); i != _linked->end(); ++i)
			(*i)->detach();
		_statistics->clear();
		base::on_updated();
	}

	void functions_list::print(string &content) const
	{
		const char* old_locale = ::setlocale(LC_NUMERIC, NULL);
		bool locale_ok = ::setlocale(LC_NUMERIC, "") != NULL;
		shared_ptr<symbol_resolver> resolver = get_resolver();

		content.clear();
		content.reserve(256 * (get_count() + 1)); // kind of magic number
		content += "Function\tTimes Called\tExclusive Time\tInclusive Time\tAverage Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n";
		for (index_type i = 0; i != get_count(); ++i)
		{
			const view_type::value_type &row = get_entry(i);

			content += resolver->symbol_name_by_va(row.first.first) + "\t";
			itoa<10>(content, row.second.times_called), content += "\t";
			gcvt(content, exclusive_time(_tick_interval)(row.second)), content += "\t";
			gcvt(content, inclusive_time(_tick_interval)(row.second)), content += "\t";
			gcvt(content, exclusive_time_avg(_tick_interval)(row.second)), content += "\t";
			gcvt(content, inclusive_time_avg(_tick_interval)(row.second)), content += "\t";
			itoa<10>(content, row.second.max_reentrance), content += "\t";
			gcvt(content, max_call_time(_tick_interval)(row.second)), content += "\r\n";
		}

		if (locale_ok)
			::setlocale(LC_NUMERIC, old_locale);
	}

	shared_ptr<linked_statistics> functions_list::watch_children(index_type item) const
	{
		if (item >= get_count())
			throw out_of_range("");

		const statistic_types::map_detailed::value_type &s = get_entry(item);
		linked_statistics_list_t::iterator i = _linked->insert(_linked->end(), nullptr);
		shared_ptr<children_statistics> children(new children_statistics(s.second.callees, _tick_interval, get_resolver(),
			get_threads()), bind(&erase_entry<linked_statistics_list_t>, _1, _linked, i));

		*i = children.get();
		return children;
	}

	shared_ptr<linked_statistics> functions_list::watch_parents(index_type item) const
	{
		if (item >= get_count())
			throw out_of_range("");

		const statistic_types::map_detailed::value_type &s = get_entry(item);
		linked_statistics_list_t::iterator i = _linked->insert(_linked->end(), nullptr);
		shared_ptr<parents_statistics> parents(new parents_statistics(s.second.callers, _tick_interval, get_resolver(),
			get_threads()), bind(&erase_entry<linked_statistics_list_t>, _1, _linked, i));

		*i = parents.get();
		return parents;
	}

	shared_ptr<functions_list> functions_list::create(timestamp_t ticks_per_second, shared_ptr<symbol_resolver> resolver,
		shared_ptr<threads_model> threads)
	{
		shared_ptr<statistic_types::map_detailed> base_map(new statistic_types::map_detailed);

		return shared_ptr<functions_list>(new functions_list(base_map, 1.0 / ticks_per_second, resolver, threads));
	}

	void functions_list::on_updated()
	{
		for (linked_statistics_list_t::const_iterator i = _linked->begin(); i != _linked->end(); ++i)
			(*i)->on_updated();
		base::on_updated();
	}
}
