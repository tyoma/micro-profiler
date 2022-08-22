#include <frontend/process_list.h>

#include "helpers.h"

#include <frontend/columns_layout.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ProcessListTests )

			shared_ptr<tables::processes> processes;

			static process_info make_process(id_t pid, string name, id_t parent_pid = 0)
			{
				process_info result = {	pid, parent_pid, name,	};
				return result;
			}

			init( CreateModel )
			{
				processes = make_shared<tables::processes>();
			}


			test( ProcessListIsEmptyInitially )
			{
				// INIT / ACT
				const auto l = process_list(processes, c_processes_columns);
				wpl::richtext_table_model &t = *l;

				// ACT / ASSERT
				assert_equal(0u, t.get_count());
			}


			test( ProcessListEmptyAfterUpdateWithNoProcesses )
			{
				// INIT
				const auto l = process_list(processes, c_processes_columns);

				// ACT
				processes->invalidate();

				// ASSERT
				assert_equal(0u, l->get_count());
			}


			test( ProcessListIsPopulatedFromEnumerator )
			{
				// INIT
				const auto l = process_list(processes, c_processes_columns);
				string text;

				// ACT
				add_records(*processes, plural
					+ make_process(12, "foo")
					+ make_process(12111, "bar"));
				processes->invalidate();

				// ASSERT
				assert_equal(2u, l->get_count());
				assert_equal("foo", get_text(*l, 0, 0));
				assert_equal("12", get_text(*l, 0, 1));
				assert_equal("bar", get_text(*l, 1, 0));
				assert_equal("12111", get_text(*l, 1, 1));

				// ACT
				processes->clear();
				add_records(*processes, plural
					+ make_process(1, "FOO")
					+ make_process(11111, "bar")
					+ make_process(16111, "BAZ"));
				processes->invalidate();

				// ASSERT
				assert_equal(3u, l->get_count());
				assert_equal("FOO", get_text(*l, 0, 0));
				assert_equal("1", get_text(*l, 0, 1));
				assert_equal("bar", get_text(*l, 1, 0));
				assert_equal("11111", get_text(*l, 1, 1));
				assert_equal("BAZ", get_text(*l, 2, 0));
				assert_equal("16111", get_text(*l, 2, 1));
			}


			test( InvalidationIsSignalledOnUpdate )
			{
				// INIT
				size_t n_count = 0;
				const auto l = process_list(processes, c_processes_columns);
				wpl::slot_connection c = l->invalidate += [&] (wpl::table_model_base::index_type item) {
					n_count = l->get_count();
					assert_equal(wpl::table_model_base::npos(), item);
				};

				// ACT
				processes->invalidate();

				// ASSERT
				assert_equal(0u, n_count);

				// ACT
				add_records(*processes, plural
					+ make_process(12, "Lorem")
					+ make_process(12111, "Amet")
					+ make_process(1211, "Quand"));
				processes->invalidate();

				// ASSERT
				assert_equal(3u, n_count);
			}


			test( ProcessListIsSortable )
			{
				// INIT
				const auto l = process_list(processes, c_processes_columns);

				add_records(*processes, plural
					+ make_process(12, "Lorem")
					+ make_process(12111, "Amet")
					+ make_process(1211, "Quand"));
				processes->invalidate();

				// ACT
				l->set_order(1, true);

				// ASSERT
				assert_equal("Lorem", get_text(*l, 0, 0));
				assert_equal("Quand", get_text(*l, 1, 0));
				assert_equal("Amet", get_text(*l, 2, 0));

				// ACT
				l->set_order(1, false);

				// ASSERT
				assert_equal("Amet", get_text(*l, 0, 0));
				assert_equal("Quand", get_text(*l, 1, 0));
				assert_equal("Lorem", get_text(*l, 2, 0));

				// ACT
				l->set_order(0, true);

				// ASSERT
				assert_equal("12111", get_text(*l, 0, 1));
				assert_equal("12", get_text(*l, 1, 1));
				assert_equal("1211", get_text(*l, 2, 1));

				// ACT
				l->set_order(0, false);

				// ASSERT
				assert_equal("1211", get_text(*l, 0, 1));
				assert_equal("12", get_text(*l, 1, 1));
				assert_equal("12111", get_text(*l, 2, 1));
			}


			test( SortOrderIsAppliedOnUpdate )
			{
				// INIT
				const auto l = process_list(processes, c_processes_columns);
				string text;

				l->set_order(0, true);

				// ACT
				add_records(*processes, plural
					+ make_process(12, "Lorem")
					+ make_process(12111, "Amet")
					+ make_process(1211, "Quand"));
				processes->invalidate();

				// ASSERT
				assert_equal("12111", get_text(*l, 0, 1));
				assert_equal("12", get_text(*l, 1, 1));
				assert_equal("1211", get_text(*l, 2, 1));

				// ACT
				add_records(*processes, plural
					+ make_process(1311, "Dolor"));
				processes->invalidate();

				// ASSERT
				assert_equal("12111", get_text(*l, 0, 1));
				assert_equal("1311", get_text(*l, 1, 1));
				assert_equal("12", get_text(*l, 2, 1));
				assert_equal("1211", get_text(*l, 3, 1));
			}
		end_test_suite
	}
}
