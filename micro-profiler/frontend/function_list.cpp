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

#include "../_generated/microprofilerfrontend_i.h"

#include <utility>
#include <cmath>

using namespace std;
using namespace wpl::ui;

namespace
{
	using namespace micro_profiler;
	const function_statistics &operator +=(function_statistics &lhs, const FunctionStatistics &rhs)
	{
		lhs.times_called += rhs.TimesCalled;
		if (static_cast<unsigned long long>(rhs.MaxReentrance) > lhs.max_reentrance)
			lhs.max_reentrance = rhs.MaxReentrance;
		lhs.inclusive_time += rhs.InclusiveTime;
		lhs.exclusive_time += rhs.ExclusiveTime;
		return lhs;
	}

	wstring to_string2(unsigned long long value)
	{
		const size_t buffer_size = 24;
		wchar_t buffer[buffer_size] = { 0 };

		::swprintf(buffer, buffer_size, L"%I64u", value);
		return buffer;
	}

	wstring print_time(double value)
	{
		const size_t buf_size = 24;
		wchar_t buf[buf_size] = { 0 }; //X.XXe+XXXxs\0 -> 12
	
		if (999.5 > fabs(1000000000 * value))
			::swprintf(buf, buf_size, L"%.3gns", 1000000000 * value);
		else if (999.5 > fabs(1000000 * value))
			::swprintf(buf, buf_size, L"%.3gus", 1000000 * value);
		else if (999.5 > fabs(1000 * value))
			::swprintf(buf, buf_size, L"%.3gms", 1000 * value);
		else if (999.5 > fabs(value))
			::swprintf(buf, buf_size, L"%.3gs", value);
		else if (9999.5 > fabs(value))
			::swprintf(buf, buf_size, L"%.4gs", value);
		else 
			::swprintf(buf, buf_size, L"%.3gs", value);
		return buf;
	}

	namespace functors
	{
		class by_name
		{
			symbol_resolver_itf &_resolver;

			const by_name &operator =(const by_name &rhs);

		public:
			by_name(symbol_resolver_itf &resolver)
				: _resolver(resolver)
			{	}

			wstring operator ()(const void *address, const function_statistics &) const
			{	return _resolver.symbol_name_by_va(address);	}

			wstring operator ()(const void *address, unsigned __int64) const
			{	return _resolver.symbol_name_by_va(address);	}

			bool operator ()(const void *lhs_addr, const function_statistics &, const void *rhs_addr, const function_statistics &) const
			{	return _resolver.symbol_name_by_va(lhs_addr) < _resolver.symbol_name_by_va(rhs_addr);	}
		};

		struct by_times_called
		{
			wstring operator ()(const void *, const function_statistics &s) const
			{	return to_string2(s.times_called);	}

			wstring operator ()(const void *, unsigned __int64 times_called) const
			{	return to_string2(times_called);	}

			bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
			{	return lhs.times_called < rhs.times_called;	}

			bool operator ()(const void *, unsigned __int64 lhs, const void *, unsigned __int64 rhs) const
			{	return lhs < rhs;	}
		};

		class by_exclusive_time
		{
			__int64 _ticks_resolution;
		public:
			by_exclusive_time(__int64 ticks_resolution) 
				: _ticks_resolution(ticks_resolution) 
			{	}

			wstring operator ()(const void *, const function_statistics &s) const
			{	return print_time(1.0 * s.exclusive_time / _ticks_resolution);	}

			bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
			{	return lhs.exclusive_time < rhs.exclusive_time;	}
		};

		struct by_inclusive_time
		{
			__int64 _ticks_resolution;
		public:
			by_inclusive_time(__int64 ticks_resolution) 
				: _ticks_resolution(ticks_resolution) 
			{	}

			wstring operator ()(const void *, const function_statistics &s) const
			{	return print_time(1.0 * s.inclusive_time / _ticks_resolution);	}

			bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
			{	return lhs.inclusive_time < rhs.inclusive_time;	}
		};

		struct by_avg_exclusive_call_time
		{
			__int64 _ticks_resolution;
		public:
			by_avg_exclusive_call_time(__int64 ticks_resolution) 
				: _ticks_resolution(ticks_resolution) 
			{	}

			wstring operator ()(const void *, const function_statistics &s) const
			{	return print_time(s.times_called ? 1.0 * s.exclusive_time / _ticks_resolution / s.times_called : 0);	}

			bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
			{	return ((lhs.times_called && rhs.times_called) ? (lhs.exclusive_time * rhs.times_called < rhs.exclusive_time * lhs.times_called) : lhs.times_called < rhs.times_called);	}
		};

		struct by_avg_inclusive_call_time
		{
			__int64 _ticks_resolution;
		public:
			by_avg_inclusive_call_time(__int64 ticks_resolution) 
				: _ticks_resolution(ticks_resolution) 
			{	}

			wstring operator ()(const void *, const function_statistics &s) const
			{	return print_time(s.times_called ? 1.0 * s.inclusive_time / _ticks_resolution / s.times_called : 0);	}


			bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
			{	return ((lhs.times_called && rhs.times_called) ? (lhs.inclusive_time * rhs.times_called < rhs.inclusive_time * lhs.times_called) : lhs.times_called < rhs.times_called);	}
		};

		struct by_max_reentrance
		{
			wstring operator ()(const void *, const function_statistics &s) const
			{	return to_string2(s.max_reentrance);	}

			bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
			{	return lhs.max_reentrance < rhs.max_reentrance;	}
		};
	} // namespace functors

} // namespace

functions_list::functions_list(__int64 ticks_resolution, symbol_resolver_itf &resolver) 
	: _view(_statistics), _ticks_resolution(ticks_resolution), _resolver(resolver)
{	}

functions_list::~functions_list() 
{	}

functions_list::index_type functions_list::get_count() const throw()
{ return _statistics.size(); }

void functions_list::get_text( index_type item, index_type subitem, std::wstring &text ) const
{
	ordered_view<micro_profiler::statistics_map>::value_type row = _view.at(item);
	switch (subitem)
	{
	case 0:	text = to_string2(item);	break;
	case 1:	text = functors::by_name(_resolver)(row.first, row.second);	break;
	case 2:	text = functors::by_times_called()(row.first, row.second);	break;
	case 3:	text = functors::by_exclusive_time(_ticks_resolution)(row.first, row.second);	break;
	case 4:	text = functors::by_inclusive_time(_ticks_resolution)(row.first, row.second);	break;
	case 5:	text = functors::by_avg_exclusive_call_time(_ticks_resolution)(row.first, row.second);	break;
	case 6:	text = functors::by_avg_inclusive_call_time(_ticks_resolution)(row.first, row.second);	break;
	case 7:	text = functors::by_max_reentrance()(row.first, row.second);	break;
	}
}

void functions_list::set_order( index_type column, bool ascending )
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
	}
	invalidated(_view.size());
}

shared_ptr<const listview::trackable> functions_list::track(index_type row) const
{
	class trackable : public listview::trackable, noncopyable
	{
		const statistics_view &_view; //TODO: should store weak_ptr instead of reference
		const void *_address;

	public:
		trackable(const statistics_view &view, const void *address)
			: _view(view), _address(address)
		{	}

		virtual listview::index_type index() const
		{	return _view.find_by_key(_address);	}
	};

	return shared_ptr<const listview::trackable>(new trackable(_view, _view.at(row).first));
}

void functions_list::update( const FunctionStatisticsDetailed *data, unsigned int count )
{
	for (; count; --count, ++data)
	{
		const FunctionStatistics &s = data->Statistics;
		const void *address = reinterpret_cast<void *>(s.FunctionAddress);
		_statistics[address] += s;
	}
	_view.resort();
	invalidated(_view.size());
}

void functions_list::clear()
{
	_statistics.clear();
	_view.resort();
	invalidated(_view.size());
}

functions_list::index_type functions_list::get_index(const void *address) const
{
	return _view.find_by_key(address);
}