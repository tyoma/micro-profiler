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

#include <common/compiler.h>
#include <common/hash.h>
#include <common/primitives.h>
#include <common/unordered_map.h>

namespace micro_profiler
{
	template <typename LocationT>
	struct call_graph_node : function_statistics
	{
		typedef containers::unordered_map<LocationT, call_graph_node, knuth_hash> callees_type;

		call_graph_node(const function_statistics &from = function_statistics());
		call_graph_node(const call_graph_node &other);
		~call_graph_node();

		void operator =(const call_graph_node &rhs);

		callees_type &callees;
	};

	template <typename LocationT>
	struct call_graph_types
	{
		typedef LocationT key;
		typedef call_graph_node<LocationT> node;
		typedef typename call_graph_node<LocationT>::callees_type nodes_map;
	};

	typedef call_graph_types<const void *> statistic_types;



	// call_graph_node - inline definitions
	template <typename LocationT>
	inline call_graph_node<LocationT>::call_graph_node(const function_statistics &from)
		: function_statistics(from), callees(*new callees_type())
	{	}

	template <typename LocationT>
	inline call_graph_node<LocationT>::call_graph_node(const call_graph_node &other)
		: function_statistics(other), callees(*new callees_type(other.callees))
	{	}

	template <typename LocationT>
	inline call_graph_node<LocationT>::~call_graph_node()
	{	delete &callees;	}

	template <typename LocationT>
	inline void call_graph_node<LocationT>::operator =(const call_graph_node &rhs)
	{
		static_cast<function_statistics &>(*this) = rhs;
		callees = rhs.callees;
	}


	// function_statistics - inline helpers
	inline void add(function_statistics &lhs, timestamp_t rhs_inclusive_time, timestamp_t rhs_exclusive_time)
	{
		++lhs.times_called;
		lhs.inclusive_time += rhs_inclusive_time;
		lhs.exclusive_time += rhs_exclusive_time;
		lhs.inclusive.add(rhs_inclusive_time);
		lhs.exclusive.add(rhs_exclusive_time);
	}
}
