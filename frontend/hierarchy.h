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

namespace micro_profiler
{
	template <typename ContextT, typename T>
	FORCE_INLINE void iterate_up(ContextT &context, T *&item, unsigned int n)
	{
		while (n--)
			item = hierarchy_lookup(context, hierarchy_parent_id(*item));
	}

	// normalize_levels updates compared sides, which may be nodes of a tree, so that they are compared at the same level
	// of hierarchy. Consider the following hierarchy:
	//	<root>
	//		A
	//			B
	//				C
	//			D
	//		E
	//			F
	//
	// in here, if node C is compared against F, the actual comparison must be done for the nearest parents, contained
	// within the same node, that is A and E, respectively. If C is compared against D, B and D shall be compared, etc.
	// There is a special case: if a child (maybe indirect) node is compared against its parent, it must always follow
	// it.
	// The node type is arbitrary. The access to the properties necessary for this algorithm is done via the following
	// set of functions:
	//		unsigned short hierarchy_nesting(CtxT &context, const T &node)
	//			provides the depth at which this node is located;
	//
	//		id_t hierarchy_parent_id(const T &node)
	//			provides an id of the parent, zero means it's a top-level node;
	//
	//		const T *hierarchy_lookup(CtxT &context, id_t id)
	//			returns a pointer to a node identified by id.
	// For a tree to be formed by a sorting with such comparison, the non-leaf nodes must form TOTAL ORDER (i.e. no
	// elements are equivalent). Otherwise children nodes may 'spill' under nodes, compared equivalent to their
	// actual parents.
	//
	// Returns '0' for items that require further comparison, a negative value if lhs is a parent to rhs, and a positive
	// value otherwise.
	template <typename ContextT, typename T>
	inline int normalize_levels(ContextT &context, const T *&lhs, const T *&rhs)
	{
		// Shortcut for the sides residing within the same parent already.
		if (hierarchy_parent_id(*lhs) == hierarchy_parent_id(*rhs))
			return 0;

		const auto lhs_level = hierarchy_nesting(context, *lhs);
		const auto rhs_level = hierarchy_nesting(context, *rhs);

		// Move the side that is deeper in the hierarchy to the same level.
		if (lhs_level < rhs_level)
			iterate_up(context, rhs, rhs_level - lhs_level);
		else if (rhs_level < lhs_level)
			iterate_up(context, lhs, lhs_level - rhs_level);

		// If either side is a child of another, compare their depth. The deepmost side always follows in the order.
		if (lhs == rhs)
			return static_cast<int>(lhs_level) - static_cast<int>(rhs_level);

		// Move sides up, until the common parent is met.
		while (hierarchy_parent_id(*lhs) != hierarchy_parent_id(*rhs))
		{
			lhs = hierarchy_lookup(context, hierarchy_parent_id(*lhs));
			rhs = hierarchy_lookup(context, hierarchy_parent_id(*rhs));
		}
		return 0;
	}
}
