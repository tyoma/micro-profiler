//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "text_conversion.h"

#include <common/string.h>
#include <common/formatting.h>
#include <cmath>
#include <clocale>
#include <stdexcept>
#include <utility>

using namespace std;
using namespace placeholders;
using namespace wpl;
using namespace wpl::ui;

#pragma warning(disable: 4775)

namespace micro_profiler
{
	namespace
	{
		class by_name
		{
		public:
			by_name(const function<shared_ptr<symbol_resolver> ()> &resolver_accessor)
				: _resolver_accessor(resolver_accessor)
			{	}

			template <typename AnyT>
			bool operator ()(address_t lhs_addr, const AnyT &, address_t rhs_addr, const AnyT &) const
			{
				shared_ptr<symbol_resolver> r = _resolver_accessor();
				return r->symbol_name_by_va(lhs_addr) < r->symbol_name_by_va(rhs_addr);
			}

		private:
			function<shared_ptr<symbol_resolver> ()> _resolver_accessor;
		};

		struct by_times_called
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
			{	return lhs.times_called < rhs.times_called;	}

			bool operator ()(address_t, count_t lhs, address_t, count_t rhs) const
			{	return lhs < rhs;	}
		};

		struct by_exclusive_time
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
			{	return lhs.exclusive_time < rhs.exclusive_time;	}
		};

		struct by_inclusive_time
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
			{	return lhs.inclusive_time < rhs.inclusive_time;	}
		};

		struct by_avg_exclusive_call_time
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
			{	return lhs.times_called && rhs.times_called ? lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called : lhs.times_called < rhs.times_called;	}
		};

		struct by_avg_inclusive_call_time
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
			{	return lhs.times_called && rhs.times_called ? lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called : lhs.times_called < rhs.times_called;	}
		};

		struct by_max_reentrance
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
			{	return lhs.max_reentrance < rhs.max_reentrance;	}
		};

		struct by_max_call_time
		{
			bool operator ()(address_t, const function_statistics &lhs, address_t, const function_statistics &rhs) const
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


	typedef statistics_model_impl<linked_statistics_ex, statistics_map> children_statistics;
	typedef statistics_model_impl<linked_statistics_ex, statistics_map_callers> parents_statistics;



	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::get_text(index_type item, index_type subitem, wstring &text_) const
	{
		string text;
		const typename view_type::value_type &row = get_entry(item);

		switch (subitem)
		{
		case 0:	text = to_string2(item + 1);	break;
		case 1:	text = _resolver->symbol_name_by_va(row.first);	break;
		case 2:	text = to_string2(row.second.times_called);	break;
		case 3:	format_interval(text, exclusive_time(_tick_interval)(row.second));	break;
		case 4:	format_interval(text, inclusive_time(_tick_interval)(row.second));	break;
		case 5:	format_interval(text, exclusive_time_avg(_tick_interval)(row.second));	break;
		case 6:	format_interval(text, inclusive_time_avg(_tick_interval)(row.second));	break;
		case 7:	text = to_string2(row.second.max_reentrance);	break;
		case 8:	format_interval(text, max_call_time(_tick_interval)(row.second));	break;
		}
		text_ = unicode(text);
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
			_view->set_order(by_name([this] { return _resolver; }), ascending);
			_view->disable_projection();
			break;

		case 2:
			_view->set_order(by_times_called(), ascending);
			_view->project_value(get_times_called());
			break;

		case 3:
			_view->set_order(by_exclusive_time(), ascending);
			_view->project_value(exclusive_time(_tick_interval));
			break;

		case 4:
			_view->set_order(by_inclusive_time(), ascending);
			_view->project_value(inclusive_time(_tick_interval));
			break;

		case 5:
			_view->set_order(by_avg_exclusive_call_time(), ascending);
			_view->project_value(exclusive_time_avg(_tick_interval));
			break;

		case 6:
			_view->set_order(by_avg_inclusive_call_time(), ascending);
			_view->project_value(inclusive_time_avg(_tick_interval));
			break;

		case 7:
			_view->set_order(by_max_reentrance(), ascending);
			_view->disable_projection();
			break;

		case 8:
			_view->set_order(by_max_call_time(), ascending);
			_view->project_value(max_call_time(_tick_interval));
			break;
		}
		on_updated();
	}


	template <>
	void statistics_model_impl<linked_statistics_ex, statistics_map_callers>::get_text(index_type item, index_type subitem,
		wstring &text_) const
	{
		string text;
		const statistics_map_callers::value_type &row = get_entry(item);

		switch (subitem)
		{
		case 0:	text = to_string2(item + 1);	break;
		case 1:	text = _resolver->symbol_name_by_va(row.first);	break;
		case 2:	text = to_string2(row.second);	break;
		}
		text_ = unicode(text);
	}

	template <>
	void statistics_model_impl<linked_statistics_ex, statistics_map_callers>::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view->set_order(by_name([this] { return _resolver; }), ascending);	break;
		case 2:	_view->set_order(by_times_called(), ascending);	break;
		}
		this->invalidated(_view->size());
	}


	functions_list::functions_list(shared_ptr<statistics_map_detailed> statistics, double tick_interval,
			shared_ptr<symbol_resolver> resolver)
		: base(*statistics, tick_interval), updates_enabled(true), _statistics(statistics),
			_linked(new linked_statistics_list_t), _tick_interval(tick_interval), _resolver(resolver)
	{	set_resolver(resolver);	}

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

		content.clear();
		content.reserve(256 * (get_count() + 1)); // kind of magic number
		content += "Function\tTimes Called\tExclusive Time\tInclusive Time\tAverage Call Time (Exclusive)\tAverage Call Time (Inclusive)\tMax Recursion\tMax Call Time\r\n";
		for (index_type i = 0; i != get_count(); ++i)
		{
			const view_type::value_type &row = get_entry(i);

			content += _resolver->symbol_name_by_va(row.first) + "\t";
			content += to_string2(row.second.times_called) + "\t";
			content += to_string2(exclusive_time(_tick_interval)(row.second)) + "\t";
			content += to_string2(inclusive_time(_tick_interval)(row.second)) + "\t";
			content += to_string2(exclusive_time_avg(_tick_interval)(row.second)) + "\t";
			content += to_string2(inclusive_time_avg(_tick_interval)(row.second)) + "\t";
			content += to_string2(row.second.max_reentrance) + "\t";
			content += to_string2(max_call_time(_tick_interval)(row.second)) + "\r\n";
		}

		if (locale_ok)
			::setlocale(LC_NUMERIC, old_locale);
	}

	shared_ptr<linked_statistics> functions_list::watch_children(index_type item) const
	{
		if (item >= get_count())
			throw out_of_range("");

		const statistics_map_detailed::value_type &s = get_entry(item);
		linked_statistics_list_t::iterator i = _linked->insert(_linked->end(), nullptr);
		shared_ptr<children_statistics> children(new children_statistics(s.second.callees,
			_tick_interval), bind(&erase_entry<linked_statistics_list_t>, _1, _linked, i));

		*i = children.get();
		children->set_resolver(_resolver);
		return children;
	}

	shared_ptr<linked_statistics> functions_list::watch_parents(index_type item) const
	{
		if (item >= get_count())
			throw out_of_range("");

		const statistics_map_detailed::value_type &s = get_entry(item);
		linked_statistics_list_t::iterator i = _linked->insert(_linked->end(), nullptr);
		shared_ptr<parents_statistics> parents(new parents_statistics(s.second.callers,
			_tick_interval), bind(&erase_entry<linked_statistics_list_t>, _1, _linked, i));

		*i = parents.get();
		parents->set_resolver(_resolver);
		return parents;
	}

	std::shared_ptr<symbol_resolver> functions_list::get_resolver() const
	{	return _resolver;	}

	shared_ptr<functions_list> functions_list::create(timestamp_t ticks_per_second, shared_ptr<symbol_resolver> resolver)
	{
		return shared_ptr<functions_list>(new functions_list(
			shared_ptr<statistics_map_detailed>(new statistics_map_detailed), 1.0 / ticks_per_second, resolver));
	}

	void functions_list::on_updated()
	{
		for (linked_statistics_list_t::const_iterator i = _linked->begin(); i != _linked->end(); ++i)
			(*i)->on_updated();
		base::on_updated();
	}
}
