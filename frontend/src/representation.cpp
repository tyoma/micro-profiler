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

#include <frontend/representation.h>

#include <common/smart_ptr.h>
#include <frontend/aggregators.h>
#include <frontend/derived_statistics.h>
#include <frontend/key.h>
#include <frontend/threads_model.h>
#include <sdb/transforms.h>

using namespace std;

namespace micro_profiler
{
	namespace keyer
	{
		struct resetting_thread_id : thread_id
		{
			using thread_id::operator ();
			id_t operator ()(const call_statistics &) const
			{	return static_cast<id_t>(threads_model::cumulative);	}
		};
	}

	namespace
	{
		struct sum_functions
		{
			template <typename I>
			void operator ()(call_statistics &aggregated, I group_begin, I group_end) const
			{
				aggregated.thread_id = static_cast<id_t>(threads_model::cumulative);
				static_cast<function_statistics &>(aggregated) = function_statistics();
				for (auto i = group_begin; i != group_end; ++i)
					add(aggregated, *i);
			}
		};

		template <typename T, typename C, typename KeyerT>
		vector< typename sdb::result<KeyerT, T>::type > collect_keys(const sdb::table<T, C> &table, const KeyerT &keyer_)
		{
			vector< typename sdb::result<KeyerT, T>::type > result;

			for (auto i = table.begin(); i != table.end(); ++i)
				result.push_back(keyer_(*i));
			return result;
		}

		calls_statistics_table_cptr group_by_callstack(calls_statistics_table_cptr source)
		{
			const auto composite = make_shared_copy(make_tuple(source, sdb::group_by(*source,
				[] (const calls_statistics_table &table_, sdb::agnostic_key_tag) {

				return keyer::callstack<calls_statistics_table>(table_);
			}, sum_functions())));

			return calls_statistics_table_cptr(composite, &*get<1>(*composite));
		}

		calls_statistics_table_cptr group_by_thread_address(calls_statistics_table_cptr source)
		{
			const auto composite = make_shared_copy(make_tuple(source, sdb::group_by<call_statistics>(*source,
				[] (const calls_statistics_table &, sdb::agnostic_key_tag) {

				return keyer::combine2<keyer::address, keyer::thread_id>();
			}, auto_increment_constructor<call_statistics>(), aggregator::sum_flat(*source))));

			return calls_statistics_table_cptr(composite, &*get<1>(*composite));
		}

		filtered_calls_statistics_table_cptr join_by_thread(calls_statistics_table_cptr source,
			selector_table_ptr threads)
		{
			const auto composite = make_shared_copy(make_tuple(source, threads, sdb::join(*source, *threads,
				keyer::thread_id(), keyer::self())));

			return filtered_calls_statistics_table_cptr(composite, &*get<2>(*composite));
		}

		template <typename T, typename C, typename V>
		void add_record(sdb::table<T, C> &table, const V &value)
		{
			auto rec = table.create();
			*rec = value;
			rec.commit();
		}

		template <typename M, typename D>
		function<void ()> make_activate_derived(shared_ptr<const M> main, selector_table_ptr selection_main,
			shared_ptr<const D> derived, selector_table_ptr selection_derived)
		{
			const auto selected_derived = sdb::join<keyer::id, keyer::self>(*derived, *selection_derived);

			return [main, selection_main, selection_derived, selected_derived] {
				typedef keyer::combine2<keyer::address, keyer::thread_id> match_keyer;

				const auto jump = collect_keys(*selected_derived, match_keyer());
				const auto &idx = sdb::multi_index<match_keyer>(*main);

				selection_main->clear();
				selection_derived->clear();
				for (auto i = jump.begin(); i != jump.end(); ++i)
					for (auto r = idx.equal_range(*i); r.first != r.second; ++r.first)
						add_record(*selection_main, keyer::id()(*r.first));
				selection_main->invalidate();
			};
		}

		template <bool callstacks>
		representation<callstacks, threads_all> create_all(calls_statistics_table_cptr main,
			calls_statistics_table_cptr source)
		{
			const auto selection_main = make_shared<selector_table>();
			const auto selection_main_addresses = derived_statistics::addresses(selection_main, main);
			representation<callstacks, threads_all> r = {
				main,
				derived_statistics::callers(selection_main_addresses, source),
				derived_statistics::callees(selection_main_addresses, source),

				selection_main, make_shared<selector_table>(), make_shared<selector_table>(),
			};

			r.activate_callers = make_activate_derived(r.main, r.selection_main, r.callers, r.selection_callers);
			r.activate_callees = make_activate_derived(r.main, r.selection_main, r.callees, r.selection_callees);
			return r;
		}

		template <bool callstacks>
		representation<callstacks, threads_cumulative> create_cumulative(calls_statistics_table_cptr source)
		{
			auto from = representation<callstacks, threads_all>::create(group_by_callstack(source));
			representation<callstacks, threads_cumulative> r = {
				from.main, from.callers, from.callees,
				from.selection_main, from.selection_callers, from.selection_callees,
				from.activate_callers, from.activate_callees,
			};

			return r;
		}

		template <bool callstacks>
		representation<callstacks, threads_filtered> create_filtered(calls_statistics_table_cptr main_all,
			calls_statistics_table_cptr source, id_t thread_id)
		{
			const auto selection_threads = make_shared<selector_table>();
			const auto main = join_by_thread(main_all, selection_threads);
			const auto selection_main = make_shared<selector_table>();
			const auto selection_main_addresses = derived_statistics::addresses(selection_main, main_all);
			representation<callstacks, threads_filtered> r = {
				selection_threads,
				main,
				join_by_thread(derived_statistics::callers(selection_main_addresses, source), selection_threads),
				join_by_thread(derived_statistics::callees(selection_main_addresses, source), selection_threads),

				selection_main, make_shared<selector_table>(), make_shared<selector_table>(),
			};

			r.activate_callers = make_activate_derived(r.main, r.selection_main, r.callers, r.selection_callers);
			r.activate_callees = make_activate_derived(r.main, r.selection_main, r.callees, r.selection_callees);
			add_record(*selection_threads, thread_id);
			return r;
		}
	}

	template <>
	representation<true, threads_all> representation<true, threads_all>::create(calls_statistics_table_cptr source)
	{	return create_all<true>(source, source);	}

	template <>
	representation<true, threads_cumulative> representation<true, threads_cumulative>::create(calls_statistics_table_cptr source)
	{	return create_cumulative<true>(source);	}

	template <>
	representation<true, threads_filtered> representation<true, threads_filtered>::create(calls_statistics_table_cptr source, id_t thread_id)
	{	return create_filtered<true>(source, source, thread_id);	}

	template <>
	representation<false, threads_all> representation<false, threads_all>::create(calls_statistics_table_cptr source)
	{	return create_all<false>(group_by_thread_address(source), source);	}

	template <>
	representation<false, threads_cumulative> representation<false, threads_cumulative>::create(calls_statistics_table_cptr source)
	{	return create_cumulative<false>(source);	}

	template <>
	representation<false, threads_filtered> representation<false, threads_filtered>::create(calls_statistics_table_cptr source, id_t thread_id)
	{	return create_filtered<false>(group_by_thread_address(source), source, thread_id);	}
}
