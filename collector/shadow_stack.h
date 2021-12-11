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

#include "types.h"
#include "primitives.h"

#include <common/pod_vector.h>

namespace micro_profiler
{
	template <typename OutputMapType>
	class shadow_stack
	{
	public:
		shadow_stack(const overhead & overhead_);

		template <typename ForwardConstIterator>
		void update(ForwardConstIterator trace_begin, ForwardConstIterator trace_end, OutputMapType &statistics);

	private:
		struct call_record_ex;
		typedef std::unordered_map<const void *, unsigned int, knuth_hash> entrance_counter_map;

	private:
		void restore_state(OutputMapType &statistics);

	private:
		const timestamp_t _inner_overhead, _total_overhead;
		pod_vector<call_record_ex> _stack;
		entrance_counter_map _entrance_counters;
	};

	template <typename OutputMapType>
	struct shadow_stack<OutputMapType>::call_record_ex
	{
		void set(const call_record &from, unsigned int &level, typename OutputMapType::mapped_type &entry);

		call_record call;
		timestamp_t children_time_observed, children_overhead;
		unsigned int *level;
		typename OutputMapType::mapped_type *entry;
	};



	template <typename OutputMapType>
	inline shadow_stack<OutputMapType>::shadow_stack(const overhead &overhead_)
		: _inner_overhead(overhead_.inner), _total_overhead(overhead_.inner + overhead_.outer)
	{	}

	template <typename OutputMapType>
	inline void shadow_stack<OutputMapType>::restore_state(OutputMapType &statistics)
	{
		for (typename pod_vector<call_record_ex>::iterator i = _stack.begin(); i != _stack.end(); ++i)
			i->entry = &statistics[i->call.callee];
	}

	template <typename OutputMapType>
	template <typename ForwardConstIterator>
	inline void shadow_stack<OutputMapType>::update(ForwardConstIterator i, ForwardConstIterator end,
		OutputMapType &statistics)
	{
		restore_state(statistics);
		for (; i != end; ++i)
		{
			if (i->callee)
			{
				typename OutputMapType::mapped_type &entry = statistics[i->callee];

				if (!entry.entrance_counter)
					entry.entrance_counter = &_entrance_counters[i->callee];
				_stack.push_back();
				_stack.back().set(*i, ++*entry.entrance_counter, entry);
			}
			else
			{
				const call_record_ex &current = _stack.back();
				const void *callee = current.call.callee;
				unsigned int level = --*current.level;
				const timestamp_t inclusive_time_observed = i->timestamp - current.call.timestamp  - _inner_overhead;
				const timestamp_t children_overhead = current.children_overhead;
				const timestamp_t inclusive_time = inclusive_time_observed - children_overhead;
				const timestamp_t exclusive_time = inclusive_time_observed - current.children_time_observed;

				current.entry->add_call(level, inclusive_time, exclusive_time);
				_stack.pop_back();
				if (!_stack.empty())
				{
					call_record_ex &parent = _stack.back();

					parent.children_time_observed += inclusive_time_observed + _total_overhead;
					parent.children_overhead += _total_overhead + children_overhead;
					add_child_statistics(*parent.entry, callee, 0, inclusive_time, exclusive_time);
				}
			}
		}
	}


	template <typename OutputMapType>
	inline void shadow_stack<OutputMapType>::call_record_ex::set(const call_record &from, unsigned int &level_,
		typename OutputMapType::mapped_type &entry_)
	{
		call = from;
		children_time_observed = children_overhead = 0;
		level = &level_;
		entry = &entry_;
	}
}
