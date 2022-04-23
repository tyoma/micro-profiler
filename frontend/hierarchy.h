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

namespace micro_profiler
{
	// hierarchical_compare updates compared sides, which may be nodes of a tree, so that they are compared at the same
	// level of hierarchy. Consider the following hierarchy:
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
	//		path hierarchy_path(CtxT &context, const T &node)
	//			provides the path of ids to this item from the root;
	//
	//		bool hierarchy_same_parent(const T &lhs, const T &rhs)
	//			returns 'true' if both lhs and rhs are siblings within the same parent;
	//
	//		const T *hierarchy_lookup(CtxT &context, path_item_t id)
	//			returns a pointer to a node identified by id.
	// For a tree to be formed by a sorting with such comparison, the non-leaf nodes must form TOTAL ORDER (i.e. no
	// elements are equivalent). Otherwise children nodes may 'spill' under nodes, compared equivalent to their
	// actual parents.
	//
	// Calls a predicate passed in for items requiring comparison or compares the depth of parent/child items.
	template <typename ContextT, typename PredicateT, typename T>
	inline bool hierarchical_compare(ContextT &context, const PredicateT &predicate, const T &lhs, const T &rhs)
	{
		typedef unsigned int depth_t;

		// Shortcut for the sides residing within the same parent already.
		if (hierarchy_same_parent(lhs, rhs))
			return predicate(lhs, rhs);

		const auto &lhs_path = hierarchy_path(context, lhs);
		const auto lhs_length = static_cast<depth_t>(lhs_path.size());
		const auto &rhs_path = hierarchy_path(context, rhs);
		const auto rhs_length = static_cast<depth_t>(rhs_path.size());

		for (depth_t i = 0u, min_length = lhs_length < rhs_length ? lhs_length : rhs_length; i != min_length; ++i)
		{
			auto lhs_id = lhs_path[i];
			auto rhs_id = rhs_path[i];

			if (lhs_id == rhs_id)
				continue;
			return predicate(*hierarchy_lookup(context, lhs_id), *hierarchy_lookup(context, rhs_id));
		}

		// If either side is a child of another, compare their depth. The deepmost side always follows in the order.
		return lhs_length < rhs_length;
	}
}
