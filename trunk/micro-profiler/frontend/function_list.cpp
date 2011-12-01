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

using namespace std;
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

		void print_time(wstring &dest, double value, bool use_default_formatter)
		{
			if (use_default_formatter)
			{
				const size_t buffer_size = 24;
				wchar_t buffer[buffer_size] = { 0 };

				::swprintf(buffer, buffer_size, L"%g", value);
				dest = buffer;
			}
			else
				format_interval(dest, value);
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

				void operator ()(wstring &dest, const void *address, const function_statistics &) const
				{	dest = _resolver->symbol_name_by_va(address);	}

				void operator ()(wstring &dest, const void *address, unsigned __int64) const
				{	dest = _resolver->symbol_name_by_va(address);	}

				bool operator ()(const void *lhs_addr, const function_statistics &, const void *rhs_addr, const function_statistics &) const
				{	return _resolver->symbol_name_by_va(lhs_addr) < _resolver->symbol_name_by_va(rhs_addr);	}
			};

			struct by_times_called
			{
				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	dest = to_string2(s.times_called);	}

				void operator ()(wstring &dest, const void *, unsigned __int64 times_called) const
				{	dest = to_string2(times_called);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.times_called < rhs.times_called;	}

				bool operator ()(const void *, unsigned __int64 lhs, const void *, unsigned __int64 rhs) const
				{	return lhs < rhs;	}
			};

			class by_exclusive_time
			{
				__int64 _ticks_resolution;
				bool _use_default_formatter;
			public:
				by_exclusive_time(__int64 ticks_resolution, bool use_default_formatter = false) 
					: _ticks_resolution(ticks_resolution), _use_default_formatter(use_default_formatter) 
				{	}

				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	print_time(dest, 1.0 * s.exclusive_time / _ticks_resolution, _use_default_formatter);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.exclusive_time < rhs.exclusive_time;	}
			};

			struct by_inclusive_time
			{
				__int64 _ticks_resolution;
				bool _use_default_formatter;
			public:
				by_inclusive_time(__int64 ticks_resolution, bool use_default_formatter = false) 
					: _ticks_resolution(ticks_resolution), _use_default_formatter(use_default_formatter)
				{	}

				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	print_time(dest, 1.0 * s.inclusive_time / _ticks_resolution, _use_default_formatter);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.inclusive_time < rhs.inclusive_time;	}
			};

			struct by_avg_exclusive_call_time
			{
				__int64 _ticks_resolution;
				bool _use_default_formatter;
			public:
				by_avg_exclusive_call_time(__int64 ticks_resolution, bool use_default_formatter = false) 
					: _ticks_resolution(ticks_resolution), _use_default_formatter(use_default_formatter)
				{	}

				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	print_time(dest, s.times_called ? 1.0 * s.exclusive_time / _ticks_resolution / s.times_called : 0, _use_default_formatter);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return ((lhs.times_called && rhs.times_called) ? (lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called) : lhs.times_called < rhs.times_called);	}
			};

			struct by_avg_inclusive_call_time
			{
				__int64 _ticks_resolution;
				bool _use_default_formatter;
			public:
				by_avg_inclusive_call_time(__int64 ticks_resolution, bool use_default_formatter = false) 
					: _ticks_resolution(ticks_resolution), _use_default_formatter(use_default_formatter)
				{	}

				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	print_time(dest, s.times_called ? 1.0 * s.inclusive_time / _ticks_resolution / s.times_called : 0, _use_default_formatter);	}


				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return ((lhs.times_called && rhs.times_called) ? (lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called) : lhs.times_called < rhs.times_called);	}
			};

			struct by_max_reentrance
			{
				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	dest = to_string2(s.max_reentrance);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.max_reentrance < rhs.max_reentrance;	}
			};

			class by_max_call_time
			{
				__int64 _ticks_resolution;
				bool _use_default_formatter;

			public:
				by_max_call_time(__int64 ticks_resolution, bool use_default_formatter = false) 
					: _ticks_resolution(ticks_resolution), _use_default_formatter(use_default_formatter) 
				{	}

				void operator ()(wstring &dest, const void *, const function_statistics &s) const
				{	print_time(dest, 1.0 * s.max_call_time / _ticks_resolution, _use_default_formatter);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.max_call_time < rhs.max_call_time;	}
			};
		}
	}

	template <typename BaseT, typename MapT>
	class statistics_model_impl : public BaseT
	{
		__int64 _ticks_resolution;
		shared_ptr<symbol_resolver> _resolver;

	protected:
		ordered_view<MapT> _view;

	protected:
		typedef typename BaseT::index_type index_type;
		typedef ordered_view<MapT> view_type;

	protected:
		statistics_model_impl(const MapT &statistics, __int64 ticks_resolution, shared_ptr<symbol_resolver> resolver);

		virtual typename index_type get_count() const throw();
		virtual void get_text(index_type item, index_type subitem, wstring &text) const;
		virtual void set_order(index_type column, bool ascending);
		virtual shared_ptr<const wpl::ui::listview::trackable> track(index_type row) const;

		void updated();
	};

	class functions_list_impl : public statistics_model_impl<functions_list, micro_profiler::statistics_map>
	{
		shared_ptr<micro_profiler::statistics_map> _statistics;
		__int64 _ticks_resolution;
		shared_ptr<symbol_resolver> _resolver;

	public:
		functions_list_impl(shared_ptr<micro_profiler::statistics_map> statistics, __int64 ticks_resolution, shared_ptr<symbol_resolver> resolver);

		virtual void clear();
		virtual void update(const FunctionStatisticsDetailed *data, unsigned int count);
		virtual index_type get_index(const void *address) const;
		virtual void print(wstring &content) const;
	};



	shared_ptr<functions_list> functions_list::create(__int64 ticks_resolution, shared_ptr<symbol_resolver> resolver)
	{
		return shared_ptr<functions_list>(new functions_list_impl(
			shared_ptr<micro_profiler::statistics_map>(new micro_profiler::statistics_map), ticks_resolution, resolver));
	}

	functions_list_impl::functions_list_impl(shared_ptr<micro_profiler::statistics_map> statistics, __int64 ticks_resolution,
		shared_ptr<symbol_resolver> resolver) 
		: statistics_model_impl<functions_list, micro_profiler::statistics_map>(*statistics, ticks_resolution, resolver),
			_statistics(statistics), _ticks_resolution(ticks_resolution), _resolver(resolver)
	{	}


	template <typename BaseT, typename MapT>
	statistics_model_impl<BaseT, MapT>::statistics_model_impl(const MapT &statistics, __int64 ticks_resolution, shared_ptr<symbol_resolver> resolver)
		: _view(statistics), _ticks_resolution(ticks_resolution), _resolver(resolver)
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
		case 0:	text = to_string2(item);	break;
		case 1:	(functors::by_name(_resolver))(text, row.first, row.second);	break;
		case 2:	functors::by_times_called()(text, row.first, row.second);	break;
		case 3:	(functors::by_exclusive_time(_ticks_resolution))(text, row.first, row.second);	break;
		case 4:	(functors::by_inclusive_time(_ticks_resolution))(text, row.first, row.second);	break;
		case 5:	(functors::by_avg_exclusive_call_time(_ticks_resolution))(text, row.first, row.second);	break;
		case 6:	(functors::by_avg_inclusive_call_time(_ticks_resolution))(text, row.first, row.second);	break;
		case 7:	functors::by_max_reentrance()(text, row.first, row.second);	break;
		case 8:	(functors::by_max_call_time(_ticks_resolution))(text, row.first, row.second);	break;
		}
	}

	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view.set_order(functors::by_name(_resolver), ascending);	break;
		case 2:	_view.set_order(functors::by_times_called(), ascending);	break;
		case 3:	_view.set_order(functors::by_exclusive_time(_ticks_resolution), ascending);	break;
		case 4:	_view.set_order(functors::by_inclusive_time(_ticks_resolution), ascending);	break;
		case 5:	_view.set_order(functors::by_avg_exclusive_call_time(_ticks_resolution), ascending);	break;
		case 6:	_view.set_order(functors::by_avg_inclusive_call_time(_ticks_resolution), ascending);	break;
		case 7:	_view.set_order(functors::by_max_reentrance(), ascending);	break;
		case 8:	_view.set_order(functors::by_max_call_time(_ticks_resolution), ascending);	break;
		}
		invalidated(_view.size());
	}

	template <typename BaseT, typename MapT>
	shared_ptr<const listview::trackable> statistics_model_impl<BaseT, MapT>::track(index_type row) const
	{
		class trackable : public listview::trackable, wpl::noncopyable
		{
			const view_type &_view; //TODO: should store weak_ptr instead of reference
			const void *_address;

		public:
			trackable(const view_type &view, const void *address)
				: _view(view), _address(address)
			{	}

			virtual listview::index_type index() const
			{	return _view.find_by_key(_address);	}
		};

		return shared_ptr<const listview::trackable>(new trackable(_view, _view.at(row).first));
	}

	template <typename BaseT, typename MapT>
	void statistics_model_impl<BaseT, MapT>::updated()
	{
		_view.resort();
		invalidated(_view.size());
	}


	void functions_list_impl::update( const FunctionStatisticsDetailed *data, unsigned int count )
	{
		for (; count; --count, ++data)
		{
			const FunctionStatistics &s = data->Statistics;
			const void *address = reinterpret_cast<void *>(s.FunctionAddress);
			(*_statistics)[address] += s;
		}
		updated();
	}

	void functions_list_impl::clear()
	{
		_statistics->clear();
		updated();
	}

	functions_list_impl::index_type functions_list_impl::get_index(const void *address) const
	{
		return _view.find_by_key(address);
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
			const view_type::value_type &row = _view.at(i);

			content += (functors::by_name(_resolver)(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_times_called()(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_exclusive_time(_ticks_resolution, true)(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_inclusive_time(_ticks_resolution, true)(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_avg_exclusive_call_time(_ticks_resolution, true)(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_avg_inclusive_call_time(_ticks_resolution, true)(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_max_reentrance()(tmp, row.first, row.second), tmp) + L"\t";
			content += (functors::by_max_call_time(_ticks_resolution, true)(tmp, row.first, row.second), tmp) + L"\r\n";
		}

		if (locale_ok) 
			::setlocale(LC_NUMERIC, old_locale);
	}
}
