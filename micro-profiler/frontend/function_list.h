//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org) and Denis Burenko
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

#include "../primitives.h"
#include "ordered_view.h"
#include <wpl/ui/listview.h>
#include <string>

typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;

struct symbol_resolver_itf
{
	virtual ~symbol_resolver_itf() {}
	virtual std::wstring symbol_name_by_va(const void *address) = 0;
};

class functions_list : public wpl::ui::listview::model, wpl::noncopyable
{
	micro_profiler::statistics_map _statistics;
	ordered_view<micro_profiler::statistics_map> _view;
	__int64 _ticks_resolution;
	symbol_resolver_itf &_resolver;

public:
	functions_list(__int64 ticks_resolution, symbol_resolver_itf& resolver);
	~functions_list();

	void clear();
	void update(const FunctionStatisticsDetailed *data, unsigned int count);

	virtual index_type get_count() const throw();
	virtual void get_text(index_type item, index_type subitem, std::wstring &text) const;
	virtual void set_order(index_type column, bool ascending);

	index_type get_index(const void *address) const;
};
