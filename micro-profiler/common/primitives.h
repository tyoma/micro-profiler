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

namespace micro_profiler
{
	typedef unsigned char byte;
	typedef unsigned long long int count_t;
	typedef long long timestamp_t;
	typedef unsigned long long int long_address_t;

#pragma pack(push, 4)
	struct call_record
	{
		timestamp_t timestamp;
		const void *callee;	// call address + sizeof(void *) + 1 bytes
	};
#pragma pack(pop)

	struct address_compare
	{
		size_t operator ()(unsigned int key) const throw();
		size_t operator ()(unsigned long long int key) const throw();
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

	template <typename AddressT>
	struct function_statistics_detailed_t : function_statistics
	{
		typedef std::unordered_map<AddressT, function_statistics, address_compare> callees_map;
		typedef std::unordered_map<AddressT, count_t, address_compare> callers_map;

		callees_map callees;
		callers_map callers;
	};

	template <typename AddressT>
	struct statistics_map_detailed_t : std::unordered_map<AddressT, function_statistics_detailed_t<AddressT>, address_compare>
	{
		wpl::signal<void (AddressT updated_function)> entry_updated;
	};



	// address_compare - inline definitions
	inline size_t address_compare::operator ()(unsigned int key) const throw()
	{	return (key >> 4) * 2654435761;	}

	inline size_t address_compare::operator ()(unsigned long long int key) const throw()
	{	return static_cast<size_t>((key >> 4) * 0x7FFFFFFFFFFFFFFF);	}

	inline size_t address_compare::operator ()(const void *key) const throw()
	{	return (*this)(reinterpret_cast<size_t>(key));	}


	// function_statistics - inline definitions
	inline function_statistics::function_statistics(count_t times_called_, unsigned int max_reentrance_, timestamp_t inclusive_time_, timestamp_t exclusive_time_, timestamp_t max_call_time_)
		: times_called(times_called_), max_reentrance(max_reentrance_), inclusive_time(inclusive_time_), exclusive_time(exclusive_time_), max_call_time(max_call_time_)
	{	}

	inline void function_statistics::add_call(unsigned int level, timestamp_t inclusive_time_, timestamp_t exclusive_time_)
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
	inline void add_child_statistics(function_statistics_detailed_t<AddressT> &s, AddressT function, unsigned int level, timestamp_t inclusive_time, timestamp_t exclusive_time)
	{	s.callees[function].add_call(level, inclusive_time, exclusive_time);	}

	template <typename AddressT, typename AddressCompareT>
	inline void update_parent_statistics(std::unordered_map<AddressT, function_statistics_detailed_t<AddressT>, AddressCompareT> &s, AddressT address, const function_statistics_detailed_t<AddressT> &f)
	{
		for (typename function_statistics_detailed_t<AddressT>::callees_map::const_iterator i = f.callees.begin(); i != f.callees.end(); ++i)
			s[i->first].callers[address] = i->second.times_called;
	}
}
