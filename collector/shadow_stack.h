//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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
	template <typename KeyT>
	class shadow_stack
	{
	public:
		typedef call_graph_types<KeyT> statistic_types;
		typedef typename statistic_types::nodes_map map_type;

	public:
		shadow_stack(const overhead & overhead_);

		template <typename IteratorT>
		void update(IteratorT trace_begin, IteratorT trace_end, map_type &statistics);

	private:
		struct stack_record;
		typedef pod_vector<stack_record> stack;

	private:
		void restore_state(function_statistics &root, map_type &callees);

	private:
		const timestamp_t _inner_overhead, _total_overhead;
		stack _stack;
	};

	template <typename KeyT>
	struct shadow_stack<KeyT>::stack_record
	{
		static void exit(stack &stack_, const call_record &entry, timestamp_t inner_overhead, timestamp_t total_overhead);
		static void reset_stack(stack &stack_, function_statistics &root, map_type &callees);
		static void enter(stack &stack_, const call_record &entry);
		static typename statistic_types::node &get(map_type &callees, typename statistic_types::key callee);
		static typename statistic_types::node &get_slow(map_type &callees, typename statistic_types::key callee);

		typename statistic_types::key callee;
		timestamp_t enter_at;
		timestamp_t children_time_observed, children_overhead;
		function_statistics *function;
		map_type *callees;
	};



	template <typename KeyT>
	inline shadow_stack<KeyT>::shadow_stack(const overhead &overhead_)
		: _inner_overhead(overhead_.inner), _total_overhead(overhead_.inner + overhead_.outer)
	{	_stack.push_back();	}

	template <typename KeyT>
	template <typename IteratorT>
	inline void shadow_stack<KeyT>::update(IteratorT i, IteratorT end, map_type &statistics)
	{
		function_statistics root;

		stack_record::reset_stack(_stack, root, statistics);
		for (; i != end; ++i)
		{
			if (i->callee)
				stack_record::enter(_stack, *i);
			else
				stack_record::exit(_stack, *i, _inner_overhead, _total_overhead);
		}
	}


	template <typename KeyT>
	inline void shadow_stack<KeyT>::stack_record::exit(stack &stack_, const call_record &entry,
		timestamp_t inner_overhead, timestamp_t total_overhead)
	{
		const auto &current = stack_.back();
		const timestamp_t inclusive_time_observed = (entry.timestamp - current.enter_at) - inner_overhead;
		const timestamp_t children_overhead = current.children_overhead;
		const timestamp_t inclusive_time = inclusive_time_observed - children_overhead;
		const timestamp_t exclusive_time = inclusive_time_observed - current.children_time_observed;

		add(*current.function, inclusive_time, exclusive_time);
		stack_.pop_back();

		auto &parent = stack_.back();

		parent.children_time_observed += inclusive_time_observed + total_overhead;
		parent.children_overhead += total_overhead + children_overhead;
	}


	template <typename KeyT>
	inline void shadow_stack<KeyT>::stack_record::reset_stack(stack &stack_, function_statistics &root,
		map_type &callees)
	{
		auto i = stack_.begin();

		i->function = &root;
		i->callees = &callees;
		for (auto previous = i++; i != stack_.end(); previous = i++)
		{
			auto &p = (*previous->callees)[i->callee];

			i->function = &p;
			i->callees = &p.callees;
		}
	}

	template <typename KeyT>
	inline void shadow_stack<KeyT>::stack_record::enter(stack &stack_, const call_record &entry)
	{
		stack_.push_back();

		auto i = stack_.end();
		auto &current = *--i;
		auto &previous = *--i;
		auto &in_previous = get(*previous.callees, entry.callee);// (*previous.callees)[entry.callee];

		current.callee = entry.callee;
		current.enter_at = entry.timestamp;
		current.children_time_observed = current.children_overhead = 0;
		current.function = &in_previous;
		current.callees = &in_previous.callees;
	}

	template <typename KeyT>
	inline typename call_graph_types<KeyT>::node &shadow_stack<KeyT>::stack_record::get(map_type &callees, typename statistic_types::key callee)
	{
		auto i = callees.find(callee);
		return callees.end() != i ? i->second : get_slow(callees, callee);
	}

	template <typename KeyT>
	FORCE_NOINLINE inline typename call_graph_types<KeyT>::node &shadow_stack<KeyT>::stack_record::get_slow(map_type &callees, typename statistic_types::key callee)
	{
		return callees.insert(std::make_pair(callee, typename call_graph_types<KeyT>::node())).first->second;
	}
}
