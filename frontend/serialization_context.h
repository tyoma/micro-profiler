//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "primitives.h"

#include <vector>

namespace micro_profiler
{
	namespace scontext
	{
		struct additive
		{
			std::vector<unsigned int> threads;
		};

		template <typename U, typename C, int static_level>
		struct hierarchy_node
		{
			U &underlying;
			C &container;
			bool address_and_thread;
			id_t thread_id;
			id_t parent_id;
		};



		template <typename U, typename C>
		inline const hierarchy_node<U, C, -1> root_context(U &underlying, C &container, bool address_and_thread = false)
		{
			hierarchy_node<U, C, -1> ctx = {
				underlying, container, address_and_thread, 0, 0
			};

			return ctx;
		}

		template <typename U, typename C>
		inline const hierarchy_node<U, C, 0> nested_context(const hierarchy_node<U, C, -1> &from, id_t thread_id)
		{
			hierarchy_node<U, C, 0> ctx = {
				from.underlying, from.container, from.address_and_thread, thread_id, 0
			};

			return ctx;
		}

		template <int static_level, typename U, typename C, int static_level2>
		inline const hierarchy_node<U, C, static_level> nested_context(const hierarchy_node<U, C, static_level2> &from,
			id_t parent_id)
		{
			hierarchy_node<U, C, static_level> ctx = {
				from.underlying, from.container, from.address_and_thread, from.thread_id, parent_id
			};

			return ctx;
		}
	}
}
