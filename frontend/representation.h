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

#include "db.h"

#include <views/transforms.h>

namespace micro_profiler
{
	typedef views::table<id_t> selector_table;
	typedef std::shared_ptr<selector_table> selector_table_ptr;
	typedef views::joined_record<calls_statistics_table, selector_table> filtered_entry;
	typedef views::table<filtered_entry> filtered_calls_statistics_table;
	typedef std::shared_ptr<const filtered_calls_statistics_table> filtered_calls_statistics_table_cptr;

	enum thread_mode {	threads_all, threads_cumulative, threads_filtered	};

	template <bool callstacks, thread_mode mode>
	struct representation
	{
		static representation create(calls_statistics_table_cptr source);

		calls_statistics_table_cptr main, callers, callees;
		selector_table_ptr selection_main, selection_callers, selection_callees;
		std::function<void ()> activate_callers, activate_callees;
	};

	template <bool callstacks>
	struct representation<callstacks, threads_filtered>
	{
		static representation create(calls_statistics_table_cptr source, id_t thread_id);

		selector_table_ptr selection_threads;

		filtered_calls_statistics_table_cptr main, callers, callees;
		selector_table_ptr selection_main, selection_callers, selection_callees;
		std::function<void ()> activate_callers, activate_callees;
	};
}
