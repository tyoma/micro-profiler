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

#include <frontend/representation.h>

#include <frontend/derived_statistics.h>
#include <frontend/key.h>
#include <frontend/threads_model.h>
#include <views/transforms.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		namespace
		{
			template <typename T>
			shared_ptr<T> make_shared_copy(const T &from)
			{	return make_shared<T>(from);	}
		}

		struct sum_functions
		{
			template <typename I>
			void operator ()(call_statistics &aggregated, I group_begin, I group_end) const
			{
				aggregated.thread_id = threads_model::cumulative;
				static_cast<function_statistics &>(aggregated) = function_statistics();
				for (auto i = group_begin; i != group_end; ++i)
					add(aggregated, *i);
			}
		};

		calls_statistics_table_cptr group_by_callstack(calls_statistics_table_cptr source)
		{
			const auto composite = make_shared_copy(make_tuple(source, views::group_by(*source,
				[] (const calls_statistics_table &table_, views::agnostic_key_tag) {

				return keyer::callstack<calls_statistics_table>(table_);
			}, sum_functions())));

			return calls_statistics_table_cptr(composite, &*get<1>(*composite));
		}

		filtered_calls_statistics_table_cptr join_by_thread(calls_statistics_table_cptr source,
			selector_table_ptr threads)
		{
			const auto composite = make_shared_copy(make_tuple(source, threads, views::join(*source, *threads,
				keyer::thread_id(), keyer::self())));

			return filtered_calls_statistics_table_cptr(composite, &*get<2>(*composite));
		}
	}

	template <>
	representation<true, threads_all> representation<true, threads_all>::create(calls_statistics_table_cptr source)
	{
		const auto selection_main = make_shared<selector_table>();
		const auto selection_main_addresses = derived_statistics::addresses(selection_main, source);
		representation<true, threads_all> r = {
			source, selection_main,
			derived_statistics::callers(selection_main_addresses, source),
			derived_statistics::callees(selection_main_addresses, source),
		};

		return r;
	}

	template <>
	representation<true, threads_cumulative> representation<true, threads_cumulative>::create(calls_statistics_table_cptr source_)
	{
		const auto source = group_by_callstack(source_);
		const auto selection_main = make_shared<selector_table>();
		const auto selection_main_addresses = derived_statistics::addresses(selection_main, source);
		representation<true, threads_cumulative> r = {
			source, selection_main,
			derived_statistics::callers(selection_main_addresses, source),
			derived_statistics::callees(selection_main_addresses, source),
		};

		return r;
	}

	template <>
	representation<true, threads_filtered> representation<true, threads_filtered>::create(calls_statistics_table_cptr source, id_t thread_id)
	{
		const auto from = representation<true, threads_all>::create(source);
		const auto selection_threads = make_shared<selector_table>();
		representation<true, threads_filtered> r = {
			selection_threads,
			join_by_thread(from.main, selection_threads), from.selection_main,
			join_by_thread(from.callers, selection_threads),
			join_by_thread(from.callees, selection_threads)
		};
		auto rec = selection_threads->create();

		*rec = thread_id;
		rec.commit();
		return r;
	}
}
