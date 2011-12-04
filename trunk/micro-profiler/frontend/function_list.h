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

#include "../common/primitives.h"

#include <wpl/ui/listview.h>
#include <string>
#include <memory>

namespace std
{
	using std::tr1::shared_ptr;
}

typedef struct FunctionStatisticsDetailedTag FunctionStatisticsDetailed;
class symbol_resolver;

namespace micro_profiler
{
	typedef wpl::ui::listview::model linked_statistics;

	class functions_list : public wpl::ui::listview::model
	{
	public:
		static std::shared_ptr<functions_list> create(__int64 ticks_resolution, std::shared_ptr<symbol_resolver> resolver);

		virtual void clear() = 0;
		virtual void update(const FunctionStatisticsDetailed *data, unsigned int count) = 0;
		virtual void print(std::wstring &content) const = 0;

		virtual std::shared_ptr<linked_statistics> children_of(index_type item) const = 0;

		// TODO: must be removed - model does not have to have these members
		static const index_type npos = static_cast<index_type>(-1);
		virtual index_type get_index(const void *address) const = 0;
	};
}
