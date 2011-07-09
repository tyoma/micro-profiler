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

#include "./../_generated/microprofilerfrontend_i.h"
#include "./../primitives.h"

#include <tchar.h>
#include <hash_map>
#include <string>
#include <vector>
#include <memory>

namespace micro_profiler
{
	class symbol_resolver;

	class statistics
	{
		class dereferencing_wrapper;
		class children_dereferencing_wrapper;

		const symbol_resolver &_symbol_resolver;
		std::auto_ptr<dereferencing_wrapper> _predicate;
		std::auto_ptr<children_dereferencing_wrapper> _children_predicate;
		detailed_statistics_map _statistics;
		const statistics_map *_children_statistics;
		std::vector<detailed_statistics_map::const_iterator> _sorted_statistics;
		std::vector<statistics_map::const_iterator> _sorted_children_statistics;

		statistics(const statistics &);
		void operator =(const statistics &);

	public:
		typedef statistics_map::value_type statistics_entry;
		typedef detailed_statistics_map::value_type statistics_entry_detailed;
		typedef bool (*sort_predicate)(const void *lhs_addr, const function_statistics &lhs, const void *rhs_addr, const function_statistics &rhs, const symbol_resolver &resolver);

	public:
		statistics(const symbol_resolver &resolver);
		virtual ~statistics();

		void sort(sort_predicate predicate, bool ascending);
		const statistics_entry_detailed &at(size_t index) const;
		size_t size() const;

		size_t find_index(const void *address) const;

		void set_focus(size_t index);
		void remove_focus();
		void sort_children(sort_predicate predicate, bool ascending);
		const statistics_entry &at_children(size_t index) const;
		size_t size_children() const;

		void clear();
		bool update(const FunctionStatisticsDetailed *data, unsigned int count);
	};
}
