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
#include <algorithm>

using namespace std;
using namespace micro_profiler;

namespace
{
	const function_statistics &operator +=(function_statistics &lhs, const function_statistics &rhs)
	{
		lhs.times_called += rhs.times_called;
		if (rhs.max_reentrance > lhs.max_reentrance)
			lhs.max_reentrance = rhs.max_reentrance;
		lhs.inclusive_time += rhs.inclusive_time;
		lhs.exclusive_time += rhs.exclusive_time;
		return lhs;
	}
}

class statistics::dereferencing_wrapper
{
	statistics::sort_predicate _base;
	bool _ascending;

public:
	dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending);

	bool operator ()(const statistics::statistics_map::const_iterator &lhs, const statistics::statistics_map::const_iterator &rhs) const;
};


function_statistics_ex::function_statistics_ex(const FunctionStatistics &from, const symbol_resolver &resolver)
	: function_statistics(from.TimesCalled, from.MaxReentrance, from.InclusiveTime, from.ExclusiveTime),
		name(resolver.symbol_name_by_va(reinterpret_cast<const void *>(from.FunctionAddress)))
{	}


statistics::dereferencing_wrapper::dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending)
	: _base(p), _ascending(ascending)
{	}

bool statistics::dereferencing_wrapper::operator ()(const statistics::statistics_map::const_iterator &lhs, const statistics::statistics_map::const_iterator &rhs) const
{	return _ascending ? _base(lhs->second, rhs->second) : _base(rhs->second, lhs->second);	}


statistics::statistics(const symbol_resolver &resolver)
	: _symbol_resolver(resolver)
{	}

statistics::~statistics()
{	}

const function_statistics_ex &statistics::at(size_t index) const
{	return _sorted_statistics.at(index)->second;	}

size_t statistics::size() const
{	return _sorted_statistics.size();	}

void statistics::clear()
{
	_sorted_statistics.clear();
	_statistics.clear();
}

void statistics::sort(sort_predicate predicate, bool ascending)
{
	auto_ptr<dereferencing_wrapper> p(new dereferencing_wrapper(predicate, ascending));

	std::sort(_sorted_statistics.begin(), _sorted_statistics.end(), *p);
	_predicate = p;
}

bool statistics::update(const FunctionStatisticsDetailed *data, unsigned int count)
{
	bool new_insertions(false);

	for (; count; --count, ++data)
	{
		const FunctionStatistics &s = data->Statistics;
		statistics_map::iterator match(_statistics.find(s.FunctionAddress));

		if (match == _statistics.end())
		{
			match = _statistics.insert(make_pair(s.FunctionAddress, function_statistics_ex(s, _symbol_resolver))).first;
			new_insertions = true;
		}
		else
			match->second += function_statistics(s.TimesCalled, s.MaxReentrance, s.InclusiveTime, s.ExclusiveTime);
	}

	if (new_insertions)
	{
		_sorted_statistics.clear();
		for (statistics_map::const_iterator i = _statistics.begin(); i != _statistics.end(); ++i)
			_sorted_statistics.push_back(i);
	}
	if (_predicate.get())
		std::sort(_sorted_statistics.begin(), _sorted_statistics.end(), *_predicate);
	return new_insertions;
}
