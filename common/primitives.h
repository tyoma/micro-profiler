//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include "hash.h"
#include "types.h"

#include <unordered_map>

namespace micro_profiler
{
	struct function_statistics;
	template <typename KeyT> struct function_statistics_detailed_t;

	template <typename KeyT>
	struct statistic_types_t
	{
		typedef function_statistics function;
		typedef function_statistics_detailed_t<KeyT> function_detailed;

		typedef std::unordered_map<KeyT, function_detailed, knuth_hash> map_detailed;
		typedef std::unordered_map<KeyT, function, knuth_hash> map;
		typedef std::unordered_map<KeyT, count_t, knuth_hash> map_callers;
	};

	struct function_statistics
	{
		explicit function_statistics(count_t times_called = 0, unsigned int max_reentrance = 0,
			timestamp_t inclusive_time = 0, timestamp_t exclusive_time = 0, timestamp_t max_call_time = 0);

		void add_call(unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time);

		void operator +=(const function_statistics &rhs);

		count_t times_called;
		unsigned int max_reentrance;
		timestamp_t inclusive_time;
		timestamp_t exclusive_time;
		timestamp_t max_call_time;
	};

	template <typename AddressT>
	struct function_statistics_detailed_t : function_statistics
	{
		typename statistic_types_t<AddressT>::map callees;
		typename statistic_types_t<AddressT>::map_callers callers;
	};



	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, unsigned int max_reentrance_,
			timestamp_t inclusive_time_, timestamp_t exclusive_time_, timestamp_t max_call_time_)
		: times_called(times_called_), max_reentrance(max_reentrance_), inclusive_time(inclusive_time_),
			exclusive_time(exclusive_time_), max_call_time(max_call_time_)
	{	}

	inline void function_statistics::add_call(unsigned int level, timestamp_t inclusive_time_,
		timestamp_t exclusive_time_)
	{
		++times_called;
		if (level > max_reentrance)
			max_reentrance = level;
		if (!level)
			inclusive_time += inclusive_time_;
		exclusive_time += exclusive_time_;
		if (inclusive_time_ > max_call_time)
			max_call_time = inclusive_time_;
	}

	inline void function_statistics::operator +=(const function_statistics &rhs)
	{
		times_called += rhs.times_called;
		if (rhs.max_reentrance > max_reentrance)
			max_reentrance = rhs.max_reentrance;
		inclusive_time += rhs.inclusive_time;
		exclusive_time += rhs.exclusive_time;
		if (rhs.max_call_time > max_call_time)
			max_call_time = rhs.max_call_time;
	}
	

	// helper methods - inline definitions
	template <typename AddressT>
	inline void add_child_statistics(function_statistics_detailed_t<AddressT> &s, AddressT function, unsigned int level,
		timestamp_t inclusive_time, timestamp_t exclusive_time)
	{	s.callees[function].add_call(level, inclusive_time, exclusive_time);	}

	template <typename DetailedMapT, typename AddressT, typename ChildrenMapT>
	inline void update_parent_statistics(DetailedMapT &s, AddressT key, const ChildrenMapT &callees)
	{
		for (typename ChildrenMapT::const_iterator i = callees.begin(), end = callees.end(); i != end; ++i)
			s[i->first].callers[key] = i->second.times_called;
	}
}
