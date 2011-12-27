//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "../common/com_helpers.h"
#include "../common/formatting.h"
#include "../common/ordered_view.h"
#include "symbol_resolver.h"

#include <utility>
#include <cmath>
#include <clocale>

namespace std
{
	using tr1::enable_shared_from_this;
	using tr1::weak_ptr;
	using namespace tr1::placeholders;
}

using namespace std;
using namespace wpl;
using namespace wpl::ui;

namespace micro_profiler
{
	namespace
	{
		wstring to_string2(unsigned long long value)
		{
			const size_t buffer_size = 24;
			wchar_t buffer[buffer_size] = { 0 };

			::swprintf(buffer, buffer_size, L"%I64u", value);
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

				bool operator ()(const void *lhs_addr, const function_statistics &, const void *rhs_addr, const function_statistics &) const
				{	return _resolver->symbol_name_by_va(lhs_addr) < _resolver->symbol_name_by_va(rhs_addr);	}

				bool operator ()(const void *lhs_addr, unsigned __int64, const void *rhs_addr, unsigned __int64) const
				{	return _resolver->symbol_name_by_va(lhs_addr) < _resolver->symbol_name_by_va(rhs_addr);	}
			};

			struct by_times_called
			{
				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.times_called < rhs.times_called;	}

				bool operator ()(const void *, unsigned __int64 lhs, const void *, unsigned __int64 rhs) const
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

	template <typename BaseT, typename MapT>
	class statistics_model_impl : public BaseT, public enable_shared_from_this< statistics_model_impl<BaseT, MapT> >
	{
		double _tick_interval;
		shared_ptr<symbol_resolver> _resolver;
		ordered_view<MapT> _view;

	protected:
		typedef typename BaseT::index_type index_type;
		typedef ordered_view<MapT> view_type;

		const view_type &view() const;
		void updated();

	public:
		statistics_model_impl(const MapT &statistics, double tick_interval, shared_ptr<symbol_resolver> resolver);

		virtual typename index_type get_count() const throw();
		virtual void get_text(index_type item, index_type subitem, wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual shared_ptr<const listview::trackable> track(index_type row) const;

		virtual index_type get_index(const void *address) const;
	};


	class functions_list_impl : public statistics_model_impl<functions_list, statistics_map_detailed>
	{
		shared_ptr<statistics_map_detailed> _statistics;
		double _tick_interval;
		shared_ptr<symbol_resolver> _resolver;

		mutable signal<void (const void *updated_function)> entry_updated;

	public:
		functions_list_impl(shared_ptr<statistics_map_detailed> statistics, double tick_interval, shared_ptr<symbol_resolver> resolver);

		virtual void clear();
		virtual void update(const FunctionStatisticsDetailed *data, unsigned int count);
		virtual void print(wstring &content) const;
		virtual shared_ptr<linked_statistics> watch_children(index_type item) const;
		virtual shared_ptr<linked_statistics> watch_parents(index_type item) const;
	};


	class children_statistics_model_impl : public statistics_model_impl<linked_statistics, statistics_map>
	{
		const void *_controlled_address;
		slot_connection _updates_connection;

		void on_updated(const void *address);

	public:
		children_statistics_model_impl(const void *controlled_address, const statistics_map &statistics,
			signal<void (const void *)> &entry_updated, double tick_interval, shared_ptr<symbol_resolver> resolver);

		virtual const void *get_address(index_type item) const;
	};


	class parents_statistics : public linked_statistics, noncopyable
	{
		typedef ordered_view<statistics_map_callers> view_type;

		view_type _view;
		shared_ptr<symbol_resolver> _resolver;
		slot_connection _updates_connection;

		void on_updated(const void *address);

	public:
		parents_statistics(const statistics_map_callers &statistics, signal<void (const void *)> &entry_updated,
			shared_ptr<symbol_resolver> resolver);

		virtual index_type get_count() const throw();
		virtual void get_text(index_type item, index_type subitem, wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual shared_ptr<const listview::trackable> track(index_type row) const;

		virtual const void *get_address(index_type item) const;
	};



	template <typename BaseT, typename MapT>
	statistics_model_impl<BaseT, MapT>::statistics_model_impl(const MapT &statistics, double tick_interval, shared_ptr<symbol_resolver> resolver)
		: _view(statistics), _tick_interval(tick_interval), _resolver(resolver)
	{ }

	template <typename BaseT, typename MapT>
	typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_count() const throw()
	{ return _view.size(); }

	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::get_text(index_type item, index_type subitem, wstring &text) const
	{
		const view_type::value_type &row = _view.at(item);

		switch (subitem)
		{
		case 0:	text = to_string2(static_cast<unsigned long long>(item + 1));	break;
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

	template <typename BaseT, typename MapT>
	shared_ptr<const listview::trackable> statistics_model_impl<BaseT, MapT>::track(index_type row) const
	{
		class trackable : public listview::trackable, noncopyable
		{
			weak_ptr<const statistics_model_impl> _model; //TODO: should store weak_ptr instead of reference
			const void *_address;

		public:
			trackable(weak_ptr<const statistics_model_impl> model, const void *address)
				: _model(model), _address(address)
			{	}

			virtual listview::index_type index() const
			{
				if (_model.expired())
					return static_cast<listview::index_type>(-1);
				else
					return shared_ptr<const statistics_model_impl>(_model)->get_index(_address);
			}
		};

		return shared_ptr<const listview::trackable>(new trackable(shared_from_this(), _view.at(row).first));
	}

	template <typename BaseT, typename MapT>
	typename statistics_model_impl<BaseT, MapT>::index_type statistics_model_impl<BaseT, MapT>::get_index(const void *address) const
	{	return _view.find_by_key(address);	}

	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::updated()
	{
		_view.resort();
		invalidated(_view.size());
	}

	template <typename BaseT, typename MapT>
	const typename statistics_model_impl<BaseT, MapT>::view_type &statistics_model_impl<BaseT, MapT>::view() const
	{	return _view;	}



	functions_list_impl::functions_list_impl(shared_ptr<statistics_map_detailed> statistics, double tick_interval,
		shared_ptr<symbol_resolver> resolver) 
		: statistics_model_impl<functions_list, statistics_map_detailed>(*statistics, tick_interval, resolver),
			_statistics(statistics), _tick_interval(tick_interval), _resolver(resolver)
	{	}

	void functions_list_impl::update(const FunctionStatisticsDetailed *data, unsigned int count)
	{
		for (; count; --count, ++data)
		{
			const void *address = reinterpret_cast<const void *>(data->Statistics.FunctionAddress);
			const function_statistics_detailed &s = (*_statistics)[address] += *data;

			update_parent_statistics(*_statistics, address, s);
			if (data->ChildrenCount)
				entry_updated(address);
		}
		updated();
	}

	void functions_list_impl::clear()
	{
		_statistics->clear();
		updated();
	}

	void functions_list_impl::print(wstring &content) const
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

	shared_ptr<linked_statistics> functions_list_impl::watch_children(index_type item) const
	{
		const statistics_map_detailed::value_type &s = view().at(item);

		return shared_ptr<linked_statistics>(new children_statistics_model_impl(s.first, s.second.callees,
			entry_updated, _tick_interval, _resolver));
	}

	shared_ptr<linked_statistics> functions_list_impl::watch_parents(index_type item) const
	{
		const statistics_map_detailed::value_type &s = view().at(item);

		return shared_ptr<linked_statistics>(new parents_statistics(s.second.callers, entry_updated, _resolver));
	}
	


	children_statistics_model_impl::children_statistics_model_impl(const void *controlled_address,
		const statistics_map &statistics, signal<void (const void *)> &entry_updated, double tick_interval,
		shared_ptr<symbol_resolver> resolver)
		: statistics_model_impl(statistics, tick_interval, resolver), _controlled_address(controlled_address)
	{
		_updates_connection = entry_updated += bind(&children_statistics_model_impl::on_updated, this, _1);
	}

	const void *children_statistics_model_impl::get_address(index_type item) const
	{	return view().at(item).first;	}

	void children_statistics_model_impl::on_updated(const void *address)
	{
		if (_controlled_address == address)
			updated();
	}


	parents_statistics::parents_statistics(const statistics_map_callers &statistics,
		signal<void (const void *)> &entry_updated, shared_ptr<symbol_resolver> resolver)
		: _view(statistics), _resolver(resolver)
	{
		_updates_connection = entry_updated += bind(&parents_statistics::on_updated, this, _1);
	}

	listview::model::index_type parents_statistics::get_count() const throw()
	{	return _view.size();	}

	void parents_statistics::get_text(index_type item, index_type subitem, wstring &text) const
	{
		const statistics_map_callers::value_type &row = _view.at(item);

		switch (subitem)
		{
		case 0:	text = to_string2(static_cast<unsigned long long>(item + 1));	break;
		case 1:	text = _resolver->symbol_name_by_va(row.first);	break;
		case 2:	text = to_string2(row.second);	break;
		}
	}

	void parents_statistics::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view.set_order(functors::by_name(_resolver), ascending);	break;
		case 2:	_view.set_order(functors::by_times_called(), ascending);	break;
		}
	}

	shared_ptr<const listview::trackable> parents_statistics::track(index_type /*row*/) const
	{	throw 0;	}

	const void *parents_statistics::get_address(index_type /*item*/) const
	{	throw 0;	}

	void parents_statistics::on_updated(const void *address)
	{
		_view.resort();
		invalidated(_view.size());
	}



	shared_ptr<functions_list> functions_list::create(__int64 ticks_resolution, shared_ptr<symbol_resolver> resolver)
	{
		return shared_ptr<functions_list>(new functions_list_impl(
			shared_ptr<statistics_map_detailed>(new statistics_map_detailed), 1.0 / ticks_resolution, resolver));
	}
}
