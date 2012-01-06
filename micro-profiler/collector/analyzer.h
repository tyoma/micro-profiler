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

#pragma once

#include "calls_collector.h"
#include "../common/primitives.h"

#include <vector>

namespace micro_profiler
{
	template <typename OutputMapType>
	class shadow_stack
	{
		struct call_record_ex;
		typedef std::unordered_map<const void *, int, address_compare> entrance_counter_map;

		const __int64 _profiler_latency;
		std::vector<call_record_ex> _stack;
		entrance_counter_map _entrance_counter;

		const shadow_stack &operator =(const shadow_stack &rhs);

		void restore_state(OutputMapType &statistics);

	public:
		shadow_stack(__int64 profiler_latency = 0);

		template <typename ForwardConstIterator>
		void update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMapType &statistics);
	};


	template <typename OutputMapType>
	struct shadow_stack<OutputMapType>::call_record_ex : call_record
	{
		call_record_ex(const call_record &from, int &level, typename OutputMapType::mapped_type *entry);
		call_record_ex(const call_record_ex &other);

		__int64 child_time;
		int *level;
		typename OutputMapType::mapped_type *entry;
	};


	class analyzer : public calls_collector::acceptor
	{
		typedef std::unordered_map< unsigned int /*threadid*/, shadow_stack<statistics_map_detailed> > stacks_container;

		const __int64 _profiler_latency;
		statistics_map_detailed _statistics;
		stacks_container _stacks;

		const analyzer &operator =(const analyzer &rhs);

	public:
		typedef statistics_map_detailed::const_iterator const_iterator;

	public:
		analyzer(__int64 profiler_latency = 0);

		void clear();
		const_iterator begin() const;
		const_iterator end() const;

		virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count);
	};


	// shadow_stack - inline definitions
	template <typename OutputMapType>
	inline shadow_stack<OutputMapType>::shadow_stack(__int64 profiler_latency)
		: _profiler_latency(profiler_latency)
	{	}

	template <typename OutputMapType>
	inline void shadow_stack<OutputMapType>::restore_state(OutputMapType &statistics)
	{
		for (std::vector<call_record_ex>::iterator i = _stack.begin(); i != _stack.end(); ++i)
			i->entry = &statistics[i->callee];
	}

	template <typename OutputMapType>
	template <typename ForwardConstIterator>
	inline void shadow_stack<OutputMapType>::update(ForwardConstIterator i, ForwardConstIterator end, OutputMapType &statistics)
	{
		restore_state(statistics);
		for (; i != end; ++i)
			if (i->callee)
				_stack.push_back(call_record_ex(*i, ++_entrance_counter[i->callee], &statistics[i->callee]));
			else
			{
				const call_record_ex &current = _stack.back();
				const void *callee = current.callee;
				int level = --*current.level;
				__int64 inclusive_time_observed = i->timestamp - current.timestamp;
				__int64 inclusive_time = inclusive_time_observed - _profiler_latency;
				__int64 exclusive_time = inclusive_time - current.child_time;

				current.entry->add_call(level, inclusive_time, exclusive_time);
				_stack.pop_back();
				if (!_stack.empty())
				{
					call_record_ex &parent = _stack.back();

					parent.child_time += inclusive_time_observed + _profiler_latency;
					add_child_statistics(*parent.entry, callee, 0, inclusive_time, exclusive_time);
				}
			}
	}


	// shadow_stack::call_record_ex - inline definitions
	template <typename OutputMapType>
	inline shadow_stack<OutputMapType>::call_record_ex::call_record_ex(const call_record &from, int &level_,
		typename OutputMapType::mapped_type *entry_)
		: call_record(from), child_time(0), level(&level_), entry(entry_)
	{	}

	template <typename OutputMapType>
	inline shadow_stack<OutputMapType>::call_record_ex::call_record_ex(const call_record_ex &other)
		: call_record(other), child_time(other.child_time), level(other.level), entry(other.entry)
	{	}


	// analyzer - inline definitions
	inline analyzer::const_iterator analyzer::begin() const
	{	return _statistics.begin();	}

	inline analyzer::const_iterator analyzer::end() const
	{	return _statistics.end();	}
}
