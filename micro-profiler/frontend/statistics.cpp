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

namespace micro_profiler
{
	namespace
	{
		const function_statistics &operator +=(function_statistics &lhs, const FunctionStatistics &rhs)
		{
			lhs.times_called += rhs.TimesCalled;
			if (rhs.MaxReentrance > lhs.max_reentrance)
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
	}

	class statistics::dereferencing_wrapper
	{
		statistics::sort_predicate _base;
		bool _ascending;
		const symbol_resolver &_resolver;

	public:
		dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending, const symbol_resolver &resolver);

		bool operator ()(const detailed_statistics_map::const_iterator &lhs, const detailed_statistics_map::const_iterator &rhs) const;
	};

	class statistics::children_dereferencing_wrapper
	{
		statistics::sort_predicate _base;
		bool _ascending;
		const symbol_resolver &_resolver;

	public:
		children_dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending, const symbol_resolver &resolver);

		bool operator ()(const statistics_map::const_iterator &lhs, const statistics_map::const_iterator &rhs) const;
	};


	statistics::dereferencing_wrapper::dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending, const symbol_resolver &resolver)
		: _base(p), _ascending(ascending), _resolver(resolver)
	{	}

	bool statistics::dereferencing_wrapper::operator ()(const detailed_statistics_map::const_iterator &lhs, const detailed_statistics_map::const_iterator &rhs) const
	{	return _ascending ? _base(lhs->first, lhs->second, rhs->first, rhs->second, _resolver) : _base(rhs->first, rhs->second, lhs->first, lhs->second, _resolver);	}


	statistics::children_dereferencing_wrapper::children_dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending, const symbol_resolver &resolver)
		: _base(p), _ascending(ascending), _resolver(resolver)
	{	}

	bool statistics::children_dereferencing_wrapper::operator ()(const statistics_map::const_iterator &lhs, const statistics_map::const_iterator &rhs) const
	{	return _ascending ? _base(lhs->first, lhs->second, rhs->first, rhs->second, _resolver) : _base(rhs->first, rhs->second, lhs->first, lhs->second, _resolver);	}


	statistics::statistics(const symbol_resolver &resolver)
		: _symbol_resolver(resolver), _children_statistics(0)
	{	}

	statistics::~statistics()
	{	}

	void statistics::sort(sort_predicate predicate, bool ascending)
	{
		auto_ptr<dereferencing_wrapper> p(new dereferencing_wrapper(predicate, ascending, _symbol_resolver));

		std::sort(_sorted_statistics.begin(), _sorted_statistics.end(), *p);
		_predicate = p;
	}

	const statistics::statistics_entry_detailed &statistics::at(size_t index) const
	{	return *_sorted_statistics.at(index);	}

	size_t statistics::size() const
	{	return _sorted_statistics.size();	}


	size_t statistics::find_index(const void *address) const
	{
		detailed_statistics_map::const_iterator i = _statistics.find(address);

		return i != _statistics.end() ? distance(_sorted_statistics.begin(), find(_sorted_statistics.begin(), _sorted_statistics.end(), i)) : static_cast<size_t>(-1);
	}


	void statistics::set_focus(size_t index)
	{
		_children_statistics = &at(index).second.children_statistics;
		_sorted_children_statistics.clear();
		for (statistics_map::const_iterator i = _children_statistics->begin(); i != _children_statistics->end(); ++i)
			_sorted_children_statistics.push_back(i);
		if (_children_predicate.get())
			std::sort(_sorted_children_statistics.begin(), _sorted_children_statistics.end(), *_children_predicate);
	}

	void statistics::remove_focus()
	{
		_children_statistics = 0;
		_sorted_children_statistics.clear();
	}

	void statistics::sort_children(sort_predicate predicate, bool ascending)
	{
		auto_ptr<children_dereferencing_wrapper> p(new children_dereferencing_wrapper(predicate, ascending, _symbol_resolver));

		std::sort(_sorted_children_statistics.begin(), _sorted_children_statistics.end(), *p);
		_children_predicate = p;
	}

	const statistics::statistics_entry &statistics::at_children(size_t index) const
	{	return *_sorted_children_statistics.at(index);	}

	size_t statistics::size_children() const
	{	return _sorted_children_statistics.size();	}


	void statistics::clear()
	{
		_sorted_statistics.clear();
		_sorted_children_statistics.clear();
		_statistics.clear();
		_children_statistics = 0;
	}

	bool statistics::update(const FunctionStatisticsDetailed *data, unsigned int count)
	{
		bool new_insertions = false;

		for (; count; --count, ++data)
		{
			void *address = reinterpret_cast<void *>(data->Statistics.FunctionAddress);
			detailed_statistics_map::iterator match(_statistics.find(address));

			if (match == _statistics.end())
			{
				match = _statistics.insert(make_pair(address, function_statistics_detailed())).first;
				_sorted_statistics.push_back(match);
				new_insertions = true;
			}
			match->second += *data;
		}
		if (_predicate.get())
			std::sort(_sorted_statistics.begin(), _sorted_statistics.end(), *_predicate);
		if (_children_predicate.get())
			std::sort(_sorted_children_statistics.begin(), _sorted_children_statistics.end(), *_children_predicate);
		return new_insertions;
	}
}
