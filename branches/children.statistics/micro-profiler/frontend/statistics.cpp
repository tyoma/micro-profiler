//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "statistics.h"

#include "symbol_resolver.h"
#include "./../_generated/microprofilerfrontend_i.h"
#include <sstream>
#include <cmath>

using namespace std;

namespace micro_profiler
{
	extern __int64 g_ticks_resolution;

	namespace
	{
		const function_statistics &operator +=(function_statistics &lhs, const FunctionStatistics &rhs)
		{
			lhs.times_called += rhs.TimesCalled;
			if (static_cast<unsigned long long>(rhs.MaxReentrance) > lhs.max_reentrance)
				lhs.max_reentrance = rhs.MaxReentrance;
			lhs.inclusive_time += rhs.InclusiveTime;
			lhs.exclusive_time += rhs.ExclusiveTime;
			return lhs;
		}

		const function_statistics_detailed &operator +=(function_statistics_detailed &lhs, const FunctionStatisticsDetailed &rhs)
		{
			lhs += rhs.Statistics;
			for (int i = 0; i != rhs.ChildrenCount; ++i)
			{
				const FunctionStatistics &child = *(rhs.ChildrenStatistics + i);

				lhs.children_statistics[reinterpret_cast<void *>(child.FunctionAddress)] += child;
			}
			return lhs;
		}


		template <typename T>
		wstring to_string(const T &value)
		{
			basic_stringstream<wchar_t> s;

			s.precision(3);
			s << value;
			return s.str();
		}

		wstring print_time(double value)
		{
			if (0.000001 > fabs(value))
				return to_string(1000000000 * value) + L"ns";
			else if (0.001 > fabs(value))
				return to_string(1000000 * value) + L"us";
			else if (1 > fabs(value))
				return to_string(1000 * value) + L"ms";
			else
				return to_string(value) + L"s";
		}

		namespace functors
		{
			class by_name
			{
				const symbol_resolver &_resolver;

			public:
				by_name(const symbol_resolver &resolver)
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
				{	return to_string(s.times_called);	}

				wstring operator ()(const void *, unsigned __int64 times_called) const
				{	return to_string(times_called);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.times_called < rhs.times_called;	}

				bool operator ()(const void *, unsigned __int64 lhs, const void *, unsigned __int64 rhs) const
				{	return lhs < rhs;	}
			};

			struct by_exclusive_time
			{
				wstring operator ()(const void *, const function_statistics &s) const
				{	return print_time(1.0 * s.exclusive_time / g_ticks_resolution);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.exclusive_time < rhs.exclusive_time;	}
			};

			struct by_inclusive_time
			{
				wstring operator ()(const void *, const function_statistics &s) const
				{	return print_time(1.0 * s.inclusive_time / g_ticks_resolution);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.inclusive_time < rhs.inclusive_time;	}
			};

			struct by_avg_exclusive_call_time
			{
				wstring operator ()(const void *, const function_statistics &s) const
				{	return print_time(s.times_called ? 1.0 * s.exclusive_time / g_ticks_resolution / s.times_called : 0);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return (lhs.times_called ? lhs.exclusive_time / lhs.times_called : 0) < (rhs.times_called ? rhs.exclusive_time / rhs.times_called : 0);	}
			};

			struct by_avg_inclusive_call_time
			{
				wstring operator ()(const void *, const function_statistics &s) const
				{	return print_time(s.times_called ? 1.0 * s.inclusive_time / g_ticks_resolution / s.times_called : 0);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return (lhs.times_called ? lhs.inclusive_time / lhs.times_called : 0) < (rhs.times_called ? rhs.inclusive_time / rhs.times_called : 0);	}
			};

			struct by_max_reentrance
			{
				wstring operator ()(const void *, const function_statistics &s) const
				{	return to_string(s.max_reentrance);	}

				bool operator ()(const void *, const function_statistics &lhs, const void *, const function_statistics &rhs) const
				{	return lhs.max_reentrance < rhs.max_reentrance;	}
			};
		}
	}

	functions_list::~functions_list()
	{	}

	void functions_list::clear()
	{
		_watched_parents.reset();
		_watched_children.reset();
		_statistics.clear();
		_view.resort();
		invalidated(_view.size());
	}

	void functions_list::update(const FunctionStatisticsDetailed *data, unsigned int count)
	{
		for (; count; --count, ++data)
		{
			const void *address = reinterpret_cast<void *>(data->Statistics.FunctionAddress);

			_statistics[address] += *data;
			for (int i = 0; i != data->ChildrenCount; ++i)
			{
				const FunctionStatistics &c = *(data->ChildrenStatistics + i);

				_statistics[reinterpret_cast<void *>(c.FunctionAddress)].parent_statistics[address] += c.TimesCalled;
			}
		}
		_view.resort();
		invalidated(_view.size());
		if (_watched_parents)
			_watched_parents->update_view();
		if (_watched_children)
			_watched_children->update_view();
	}

	void functions_list::get_text(index_type row, index_type column, wstring &text) const
	{
		const detailed_statistics2_map::value_type &v = _view.at(row);

		switch (column)
		{
		case 0:	text = to_string(row);	break;
		case 1:	text = functors::by_name(*_resolver)(v.first, v.second);	break;
		case 2:	text = functors::by_times_called()(v.first, v.second);	break;
		case 3:	text = functors::by_exclusive_time()(v.first, v.second);	break;
		case 4:	text = functors::by_inclusive_time()(v.first, v.second);	break;
		case 5:	text = functors::by_avg_exclusive_call_time()(v.first, v.second);	break;
		case 6:	text = functors::by_avg_inclusive_call_time()(v.first, v.second);	break;
		case 7:	text = functors::by_max_reentrance()(v.first, v.second);	break;
		}
	}

	void functions_list::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 1:	_view.set_order(functors::by_name(*_resolver), ascending);	break;
		case 2:	_view.set_order(functors::by_times_called(), ascending);	break;
		case 3:	_view.set_order(functors::by_exclusive_time(), ascending);	break;
		case 4:	_view.set_order(functors::by_inclusive_time(), ascending);	break;
		case 5:	_view.set_order(functors::by_avg_exclusive_call_time(), ascending);	break;
		case 6:	_view.set_order(functors::by_avg_inclusive_call_time(), ascending);	break;
		case 7:	_view.set_order(functors::by_max_reentrance(), ascending);	break;
		}
	}

	functions_list::index_type functions_list::get_index(const void *address) const
	{	return _view.find_by_key(address);	}

	shared_ptr<dependant_calls_list> functions_list::watch_parents(index_type index)
	{
		_watched_parents.reset(new parent_calls_list(_resolver, _view.at(index).second.parent_statistics));
		return _watched_parents;
	}

	shared_ptr<dependant_calls_list> functions_list::watch_children(index_type index)
	{
		_watched_children.reset(new child_calls_list(_resolver, _view.at(index).second.children_statistics));
		return _watched_children;
	}


	parent_calls_list::parent_calls_list(shared_ptr<symbol_resolver> resolver, const parent_statistics_map &parents)
		: _resolver(resolver), _view(parents)
	{	}

	void parent_calls_list::get_text(index_type row, index_type column, wstring &text) const
	{
		const parent_statistics_map::value_type &v = _view.at(row);

		switch (column)
		{
		case 0:	text = functors::by_name(*_resolver)(v.first, v.second);	break;
		case 1:	text = functors::by_times_called()(v.first, v.second);	break;
		}
	}

	void parent_calls_list::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 0:	_view.set_order(functors::by_name(*_resolver), ascending);	break;
		case 1:	_view.set_order(functors::by_times_called(), ascending);	break;
		}
	}

	void parent_calls_list::update_view()
	{
		_view.resort();
		invalidated(_view.size());
	}


	child_calls_list::child_calls_list(shared_ptr<symbol_resolver> resolver, const statistics_map &children)
		: _resolver(resolver), _view(children)
	{	}

	void child_calls_list::get_text(index_type row, index_type column, wstring &text) const
	{
		const statistics_map::value_type &v = _view.at(row);

		switch (column)
		{
		case 0:	text = functors::by_name(*_resolver)(v.first, v.second);	break;
		case 1:	text = functors::by_times_called()(v.first, v.second);	break;
		case 2:	text = functors::by_exclusive_time()(v.first, v.second);	break;
		case 3:	text = functors::by_inclusive_time()(v.first, v.second);	break;
		case 4:	text = functors::by_avg_exclusive_call_time()(v.first, v.second);	break;
		case 5:	text = functors::by_avg_inclusive_call_time()(v.first, v.second);	break;
		case 6:	text = functors::by_max_reentrance()(v.first, v.second);	break;
		}
	}

	void child_calls_list::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 0:	_view.set_order(functors::by_name(*_resolver), ascending);	break;
		case 1:	_view.set_order(functors::by_times_called(), ascending);	break;
		case 2:	_view.set_order(functors::by_exclusive_time(), ascending);	break;
		case 3:	_view.set_order(functors::by_inclusive_time(), ascending);	break;
		case 4:	_view.set_order(functors::by_avg_exclusive_call_time(), ascending);	break;
		case 5:	_view.set_order(functors::by_avg_inclusive_call_time(), ascending);	break;
		case 6:	_view.set_order(functors::by_max_reentrance(), ascending);	break;
		}
	}

	void child_calls_list::update_view()
	{
		_view.resort();
		invalidated(_view.size());
	}
}
