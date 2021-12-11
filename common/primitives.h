//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
#include "compiler.h"
#include "types.h"
#include "unordered_map.h"

#include <functional>

namespace micro_profiler
{
	struct allocator;
	struct function_statistics;
	template <typename KeyT> struct function_statistics_detailed_t;

	template <typename KeyT>
	struct statistic_types_t
	{
		typedef KeyT key;
		typedef function_statistics function;
		typedef function_statistics_detailed_t<KeyT> function_detailed;
		typedef std::function<function_detailed ()> constructor_type;

		typedef containers::unordered_map<KeyT, function_detailed, knuth_hash, std::equal_to<KeyT>, constructor_type> map_detailed;
		typedef containers::unordered_map<KeyT, function, knuth_hash> map;
		typedef containers::unordered_map<KeyT, count_t, knuth_hash> map_callers;
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
		unsigned int *entrance_counter;
	};

	template <typename AddressT>
	struct function_statistics_detailed_t : function_statistics
	{
		typedef typename statistic_types_t<AddressT>::map callees_type;
		typedef typename statistic_types_t<AddressT>::map_callers callers_type;

		function_statistics_detailed_t(allocator &allocator_);
		function_statistics_detailed_t(const function_statistics_detailed_t &rhs);
		~function_statistics_detailed_t();

		const function_statistics_detailed_t &operator =(const function_statistics_detailed_t &rhs);

		callees_type &callees;
		callers_type &callers;

	private:
		allocator &_allocator;
	};



	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, unsigned int max_reentrance_,
			timestamp_t inclusive_time_, timestamp_t exclusive_time_, timestamp_t max_call_time_)
		: times_called(times_called_), max_reentrance(max_reentrance_), inclusive_time(inclusive_time_),
			exclusive_time(exclusive_time_), max_call_time(max_call_time_), entrance_counter(0)
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


	// function_statistics_detailed_t<...> - inline definitions
	template <typename A>
	inline function_statistics_detailed_t<A>::function_statistics_detailed_t(allocator &allocator_)
		: callees(*new (allocator_.allocate(sizeof(callees_type))) callees_type(allocator_)),
			callers(*new (allocator_.allocate(sizeof(callers_type))) callers_type(allocator_)),
			_allocator(allocator_)
	{	}

	template <typename A>
	inline function_statistics_detailed_t<A>::function_statistics_detailed_t(const function_statistics_detailed_t &rhs)
		: function_statistics(rhs), callees(*new (rhs._allocator.allocate(sizeof(callees_type))) callees_type(rhs.callees)),
			callers(*new (rhs._allocator.allocate(sizeof(callers_type))) callers_type(rhs.callers)),
			_allocator(rhs._allocator)
	{	}

	template <typename A>
	inline function_statistics_detailed_t<A>::~function_statistics_detailed_t()
	{
		callers.~callers_type();
		_allocator.deallocate(&callers);
		callees.~callees_type();
		_allocator.deallocate(&callees);
	}

	template <typename AddressT>
	inline const function_statistics_detailed_t<AddressT> &function_statistics_detailed_t<AddressT>::operator =(const function_statistics_detailed_t &rhs)
	{
		static_cast<function_statistics &>(*this) = rhs;
		callees = rhs.callees;
		callers = rhs.callers;
		return *this;
	}


	// helper methods - inline definitions
	template <typename AddressT>
	FORCE_INLINE void add_child_statistics(function_statistics_detailed_t<AddressT> &s, AddressT function, unsigned int level,
		timestamp_t inclusive_time, timestamp_t exclusive_time)
	{	s.callees[function].add_call(level, inclusive_time, exclusive_time);	}
}
