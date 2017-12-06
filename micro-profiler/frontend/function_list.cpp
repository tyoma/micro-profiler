//	Copyright (c) 2011-2015 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "function_list.h"

#include "statistics_model.h"
#include "symbol_resolver.h"

#include <common/formatting.h>

#include <utility>
#include <cmath>
#include <clocale>

using namespace std;
using namespace std::placeholders;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		wstring to_string2(count_t value)
		{
			const size_t buffer_size = 24;
			wchar_t buffer[buffer_size] = { 0 };

			::swprintf(buffer, buffer_size, L"%I64u", value);
			return buffer;
		}

		wstring to_string2(unsigned int value)
		{
			const size_t buffer_size = 24;
			wchar_t buffer[buffer_size] = { 0 };

			::swprintf(buffer, buffer_size, L"%u", value);
			return buffer;
		}

		wstring to_string2(double value)
		{
			const size_t buffer_size = 24;
			wchar_t buffer[buffer_size] = { 0 };

			::swprintf(buffer, buffer_size, L"%g", value);
			return buffer;
		}

		namespace functors
		{
			class by_name
			{
				shared_ptr<symbol_resolver> _resolver;

				const by_name &operator =(const by_name &rhs);

			public:
				by_name(shared_ptr<symbol_resolver> resolver)
					: _resolver(resolver)
				{	}

				template <typename AnyT>
				bool operator ()(const void *lhs_addr, const AnyT &, const void *rhs_addr, const AnyT &) const
				{	return _resolver->symbol_name_by_va(lhs_addr) < _resolver->symbol_name_by_va(rhs_addr);	}
			};

			struct by_times_called
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.times_called < rhs.times_called;	}

				bool operator ()(const void *, count_t lhs, const void *, count_t rhs) const
				{	return lhs < rhs;	}
			};

			struct by_exclusive_time
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.exclusive_time < rhs.exclusive_time;	}
			};

			struct by_inclusive_time
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.inclusive_time < rhs.inclusive_time;	}
			};

			struct by_avg_exclusive_call_time
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.times_called && rhs.times_called ? lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called : lhs.times_called < rhs.times_called;	}
			};

			struct by_avg_inclusive_call_time
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.times_called && rhs.times_called ? lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called : lhs.times_called < rhs.times_called;	}
			};

			struct by_max_reentrance
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.max_reentrance < rhs.max_reentrance;	}
			};

			struct by_max_call_time
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.max_call_time < rhs.max_call_time;	}
			};
		}

		double exclusive_time(const function_statistics &s, double tick_interval)
		{	return tick_interval * s.exclusive_time;	}

		double inclusive_time(const function_statistics &s, double tick_interval)
		{	return tick_interval * s.inclusive_time;	}

		double max_call_time(const function_statistics &s, double tick_interval)
		{	return tick_interval * s.max_call_time;	}

		double exclusive_time_avg(const function_statistics &s, double tick_interval)
		{	return s.times_called ? tick_interval * s.exclusive_time / s.times_called : 0;	}

		double inclusive_time_avg(const function_statistics &s, double tick_interval)
		{	return s.times_called ? tick_interval * s.inclusive_time / s.times_called : 0;	}
	}



	class children_statistics_model_impl : public statistics_model_impl<linked_statistics, statistics_map>
	{
		const void *_controlled_address;
		slot_connection _updates_connection;

		void on_updated(const void *address);

	public:
		children_statistics_model_impl(const void *controlled_address, const statistics_map &statistics,
			signal<void (const void *)> &entry_updated, double tick_interval, shared_ptr<symbol_resolver> resolver);
	};


	class parents_statistics : public statistics_model_impl<linked_statistics, statistics_map_callers>
	{
		slot_connection _updates_connection;

		void on_updated(const void *address);

	public:
		parents_statistics(const statistics_map_callers &statistics, signal<void (const void *)> &entry_updated,
			shared_ptr<symbol_resolver> resolver);
	};



	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::get_text(index_type item, index_type subitem, wstring &text) const
	{
		const view_type::value_type &row = _view.at(item);

		switch (subitem)
		{
		case 0:	text = to_string2(item + 1);	break;
		case 1:	text = _resolver->symbol_name_by_va(row.first);	break;
		case 2:	text = to_string2(row.second.times_called);	break;
		case 3:	format_interval(text, exclusive_time(row.second, _tick_interval));	break;
		case 4:	format_interval(text, inclusive_time(row.second, _tick_interval));	break;
		case 5:	format_interval(text, exclusive_time_avg(row.second, _tick_interval));	break;
		case 6:	format_interval(text, inclusive_time_avg(row.second, _tick_interval));	break;
		case 7:	text = to_string2(row.second.max_reentrance);	break;
		case 8:	format_interval(text, max_call_time(row.second, _tick_interval));	break;
		}
	}

	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view.set_order(functors::by_name(_resolver), ascending);	break;
		case 2:	_view.set_order(functors::by_times_called(), ascending);	break;
		case 3:	_view.set_order(functors::by_exclusive_time(), ascending);	break;
		case 4:	_view.set_order(functors::by_inclusive_time(), ascending);	break;
		case 5:	_view.set_order(functors::by_avg_exclusive_call_time(), ascending);	break;
		case 6:	_view.set_order(functors::by_avg_inclusive_call_time(), ascending);	break;
		case 7:	_view.set_order(functors::by_max_reentrance(), ascending);	break;
		case 8:	_view.set_order(functors::by_max_call_time(), ascending);	break;
		}
		invalidated(_view.size());
	}



	functions_list::functions_list(shared_ptr<statistics_map_detailed_2> statistics, double tick_interval,
		shared_ptr<symbol_resolver> resolver) 
		: statistics_model_impl<listview::model, statistics_map_detailed_2>(*statistics, tick_interval, resolver),
			_statistics(statistics), _tick_interval(tick_interval), _resolver(resolver)
	{	}

	void functions_list::clear()
	{
		_statistics->clear();
		updated();
	}

	void functions_list::print(wstring &content) const
	{
		const char* old_locale = ::setlocale(LC_NUMERIC, NULL);  
		bool locale_ok = ::setlocale(LC_NUMERIC, "") != NULL;  

		content.clear();
		content.reserve(256 * (get_count() + 1)); // kind of magic number
		content += L"Function\tTimes Called\tExclusive Time\tInclusive Time\tAverage Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n";
		for (size_t i = 0; i != get_count(); ++i)
		{
			wstring tmp;
			const view_type::value_type &row = view().at(i);

			content += _resolver->symbol_name_by_va(row.first) + L"\t";
			content += to_string2(row.second.times_called) + L"\t";
			content += to_string2(exclusive_time(row.second, _tick_interval)) + L"\t";
			content += to_string2(inclusive_time(row.second, _tick_interval)) + L"\t";
			content += to_string2(exclusive_time_avg(row.second, _tick_interval)) + L"\t";
			content += to_string2(inclusive_time_avg(row.second, _tick_interval)) + L"\t";
			content += to_string2(row.second.max_reentrance) + L"\t";
			content += to_string2(max_call_time(row.second, _tick_interval)) + L"\r\n";
		}

		if (locale_ok) 
			::setlocale(LC_NUMERIC, old_locale);
	}

	shared_ptr<linked_statistics> functions_list::watch_children(index_type item) const
	{
		const statistics_map_detailed::value_type &s = view().at(item);

		return shared_ptr<linked_statistics>(new children_statistics_model_impl(s.first, s.second.callees,
			_statistics->entry_updated, _tick_interval, _resolver));
	}

	shared_ptr<linked_statistics> functions_list::watch_parents(index_type item) const
	{
		const statistics_map_detailed::value_type &s = view().at(item);

		return shared_ptr<linked_statistics>(new parents_statistics(s.second.callers, _statistics->entry_updated,
			_resolver));
	}
	


	children_statistics_model_impl::children_statistics_model_impl(const void *controlled_address,
		const statistics_map &statistics, signal<void (const void *)> &entry_updated, double tick_interval,
		shared_ptr<symbol_resolver> resolver)
		: statistics_model_impl(statistics, tick_interval, resolver), _controlled_address(controlled_address)
	{
		_updates_connection = entry_updated += bind(&children_statistics_model_impl::on_updated, this, _1);
	}

	void children_statistics_model_impl::on_updated(const void *address)
	{
		if (_controlled_address == address)
			updated();
	}


	parents_statistics::parents_statistics(const statistics_map_callers &statistics,
		signal<void (const void *)> &entry_updated, shared_ptr<symbol_resolver> resolver)
		: statistics_model_impl<linked_statistics, statistics_map_callers>(statistics, 0, resolver)
	{
		_updates_connection = entry_updated += bind(&parents_statistics::on_updated, this, _1);
	}

	template <>
	void statistics_model_impl<linked_statistics, statistics_map_callers>::get_text(index_type item, index_type subitem,
		wstring &text) const
	{
		const statistics_map_callers::value_type &row = _view.at(item);

		switch (subitem)
		{
		case 0:	text = to_string2(item + 1);	break;
		case 1:	text = _resolver->symbol_name_by_va(row.first);	break;
		case 2:	text = to_string2(row.second);	break;
		}
	}

	template <>
	void statistics_model_impl<linked_statistics, statistics_map_callers>::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view.set_order(functors::by_name(_resolver), ascending);	break;
		case 2:	_view.set_order(functors::by_times_called(), ascending);	break;
		}
		invalidated(_view.size());
	}

	void parents_statistics::on_updated(const void * /*address*/)
	{	updated();	}



	shared_ptr<functions_list> functions_list::create(timestamp_t ticks_resolution, shared_ptr<symbol_resolver> resolver)
	{
		return shared_ptr<functions_list>(new functions_list(
			shared_ptr<statistics_map_detailed_2>(new statistics_map_detailed_2), 1.0 / ticks_resolution, resolver));
	}
}
