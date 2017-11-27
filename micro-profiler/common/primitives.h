//	Copyright (c) 2011-2015 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <unordered_map>
#include <wpl/base/signals.h>

namespace std { namespace tr1 { } using namespace tr1; }

namespace micro_profiler
{
	typedef unsigned long long count_t;
	typedef long long timestamp_t;

#pragma pack(push, 4)
	struct call_record
	{
		timestamp_t timestamp;
		const void *callee;	// call address + sizeof(void *) + 1 bytes
	};
#pragma pack(pop)

	struct address_compare
	{
		size_t operator ()(const void *key) const throw();
	};

	struct function_statistics
	{
		explicit function_statistics(count_t times_called = 0, unsigned int max_reentrance = 0, timestamp_t inclusive_time = 0, timestamp_t exclusive_time = 0, timestamp_t max_call_time = 0);

		void add_call(unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time);

		void operator +=(const function_statistics &rhs);

		count_t times_called;
		unsigned int max_reentrance;
		timestamp_t inclusive_time;
		timestamp_t exclusive_time;
		timestamp_t max_call_time;
	};

	typedef std::unordered_map<const void * /*address*/, function_statistics, address_compare> statistics_map;
	typedef std::unordered_map<const void * /*address*/, count_t, address_compare> statistics_map_callers;

	struct function_statistics_detailed : function_statistics
	{
		statistics_map callees;
		statistics_map_callers callers;
	};

	typedef std::unordered_map<const void * /*address*/, function_statistics_detailed, address_compare> statistics_map_detailed;

	struct statistics_map_detailed_2 : statistics_map_detailed
	{
		wpl::signal<void (const void *updated_function)> entry_updated;
	};



	// address_compare - inline definitions
	inline size_t address_compare::operator ()(const void *key) const throw()
	{	return (reinterpret_cast<size_t>(key) >> 4) * 2654435761;	}


	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, unsigned int max_reentrance_, timestamp_t inclusive_time_, timestamp_t exclusive_time_, timestamp_t max_call_time_)
		: times_called(times_called_), max_reentrance(max_reentrance_), inclusive_time(inclusive_time_), exclusive_time(exclusive_time_), max_call_time(max_call_time_)
	{	}

	inline void function_statistics::add_call(unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time)
	{
		++this->times_called;
		if (level > this->max_reentrance)
			this->max_reentrance = level;
		if (!level)
			this->inclusive_time += inclusive_time;
		this->exclusive_time += exclusive_time;
		if (inclusive_time > this->max_call_time)
			this->max_call_time = inclusive_time;
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
	inline void add_child_statistics(function_statistics &/*s*/, const void * /*function*/, unsigned int /*level*/, timestamp_t /*inclusive_time*/, timestamp_t /*exclusive_time*/)
	{	}

	inline void add_child_statistics(function_statistics_detailed &s, const void *function, unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time)
	{	s.callees[function].add_call(level, inclusive_time, exclusive_time);	}

	inline void update_parent_statistics(statistics_map_detailed &s, const void *address, const function_statistics_detailed &f)
	{
		for (statistics_map::const_iterator i = f.callees.begin(); i != f.callees.end(); ++i)
			s[i->first].callers[address] = i->second.times_called;
	}
}
