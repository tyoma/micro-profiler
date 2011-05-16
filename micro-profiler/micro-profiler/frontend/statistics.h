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

class symbol_resolver;

typedef std::basic_string<TCHAR> tstring;

struct function_statistics_ex : micro_profiler::function_statistics
{
	function_statistics_ex(const FunctionStatistics &from, const symbol_resolver &resolver);

	tstring name;
};

class statistics
{
	typedef stdext::hash_map<__int64, function_statistics_ex> statistics_map_;
	class dereferencing_wrapper;

	const symbol_resolver &_symbol_resolver;
	std::auto_ptr<dereferencing_wrapper> _predicate;
	statistics_map_ _statistics;
	std::vector<statistics_map_::const_iterator> _sorted_statistics;

	statistics(const statistics &);
	void operator =(const statistics &);

public:
	typedef statistics_map_ statistics_map;
	typedef bool (*sort_predicate)(const function_statistics_ex &lhs, const function_statistics_ex &rhs);

public:
	statistics(const symbol_resolver &resolver);
	virtual ~statistics();

	const function_statistics_ex &at(size_t index) const;
	size_t size() const;

	void clear();
	void sort(sort_predicate predicate, bool ascending);
	bool update(const FunctionStatistics *data, unsigned int count);
};
