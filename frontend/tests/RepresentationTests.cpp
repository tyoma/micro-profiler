#include <frontend/representation.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <frontend/threads_model.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			shared_ptr<calls_statistics_table> create_sample()
			{
				auto source = make_shared<calls_statistics_table>();

				add_records(*source, plural
					+ make_call_statistics(1, 1, 0, 101, 1001, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 103, 1003, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 1, 104, 1004, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 2, 105, 1005, 0, 0, 0, 0)
					+ make_call_statistics(6, 1, 5, 106, 1006, 0, 0, 0, 0)
					+ make_call_statistics(7, 1, 5, 103, 1007, 0, 0, 0, 0)
					+ make_call_statistics(8, 2, 0, 101, 1008, 0, 0, 0, 0)
					+ make_call_statistics(9, 2, 8, 104, 1009, 0, 0, 0, 0));
				return source;
			}

			shared_ptr<calls_statistics_table> create_sample_with_recursion()
			{
				auto source = make_shared<calls_statistics_table>();

				add_records(*source, plural
					+ make_call_statistics(1, 1, 0, 101, 1011, 0, 11, 0, 0)
					+ make_call_statistics(2, 1, 1, 102, 1002, 0, 13, 0, 0)
					+ make_call_statistics(3, 1, 1, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(4, 1, 3, 102, 1004, 0, 19, 0, 0)
					+ make_call_statistics(5, 1, 3, 101, 1005, 0, 23, 0, 0)
					+ make_call_statistics(6, 5, 0, 101, 1006, 0, 29, 0, 0)
					+ make_call_statistics(7, 5, 6, 102, 1007, 0, 31, 0, 0)
					+ make_call_statistics(8, 5, 7, 102, 1008, 0, 37, 0, 0)
					+ make_call_statistics(9, 5, 8, 102, 1009, 0, 41, 0, 0));
				return source;
			}

			shared_ptr<calls_statistics_table> create_sample_multithreaded()
			{
				auto source = make_shared<calls_statistics_table>();

				add_records(*source, plural
					+ make_call_statistics(1, 1, 0, 101, 1001, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 2, 103, 1003, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 2, 104, 1004, 0, 0, 0, 0)

					+ make_call_statistics(5, 7, 0, 101, 1101, 0, 0, 0, 0)
					+ make_call_statistics(6, 7, 5, 102, 1102, 0, 0, 0, 0)
					+ make_call_statistics(7, 7, 5, 103, 1103, 0, 0, 0, 0)
					+ make_call_statistics(8, 7, 6, 104, 1104, 0, 0, 0, 0)
					+ make_call_statistics(9, 7, 7, 104, 1105, 0, 0, 0, 0)

					+ make_call_statistics(10, 9, 00, 101, 1301, 0, 0, 0, 0)
					+ make_call_statistics(11, 9, 10, 102, 1302, 0, 0, 0, 0)
					+ make_call_statistics(12, 9, 11, 103, 1303, 0, 0, 0, 0));
				return source;
			}
		}


		begin_test_suite( RepresentationCallstacksAllTests )
			shared_ptr<calls_statistics_table> source;

			init( CreateSourceTable )
			{	source = create_sample();	}


			test( MainStatisticsIsTheSameToTheSource )
			{
				// INIT / ACT
				auto rep = representation<true, threads_all>::create(source);

				// ACT / ASSERT
				assert_equal(source, rep.main);
				assert_not_null(rep.selection_main);
				assert_not_null(rep.selection_callers);
				assert_not_equal(rep.selection_main, rep.selection_callers);
				assert_not_null(rep.selection_callees);
				assert_not_equal(rep.selection_main, rep.selection_callees);
				assert_not_equal(rep.selection_callers, rep.selection_callees);
			}


			test( CallersAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<true, threads_all>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callers);
				assert_equal(rep.callers->end(), rep.callers->begin());

				// ACT
				add_records(*rep.selection_main, plural + 2u + 9u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 101, 2006, 0, 0, 0, 0)
					+ make_call_statistics(0, 2, 0, 101, 1009, 0, 0, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_main, plural + 6u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 105, 1006, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 101, 2006, 0, 0, 0, 0)
					+ make_call_statistics(0, 2, 0, 101, 1009, 0, 0, 0, 0), *rep.callers);
			}


			test( CalleesAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<true, threads_all>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callees);
				assert_equal(rep.callees->end(), rep.callees->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1003, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 104, 1004, 0, 0, 0, 0)
					+ make_call_statistics(0, 2, 0, 104, 1009, 0, 0, 0, 0), *rep.callees);

				// ACT
				add_records(*rep.selection_main, plural + 5u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 2010, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 104, 1004, 0, 0, 0, 0)
					+ make_call_statistics(0, 2, 0, 104, 1009, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 106, 1006, 0, 0, 0, 0), *rep.callees);
			}


			test( ActivationOfSelectedCallersSwitchesMainSelection )
			{
				// INIT
				auto rep = representation<true, threads_all>::create(source);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					return views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.callers)[make_tuple(address, thread_id)].id;
				};

				add_records(*rep.selection_main, plural + 3u);

				// ACT
				add_records(*rep.selection_callers, plural + get_id(105, 1));
				rep.activate_callers();

				// ASSERT
				assert_equivalent(plural + 5u, *rep.selection_main);
				assert_equal(rep.selection_callers->end(), rep.selection_callers->begin());

				// INIT
				add_records(*rep.selection_main, plural + 6u);

				// ACT
				add_records(*rep.selection_callers, plural + get_id(102, 1) + get_id(105, 1));
				rep.activate_callers();

				// ASSERT
				assert_equivalent(plural + 2u + 5u, *rep.selection_main);
				assert_equal(rep.selection_callers->end(), rep.selection_callers->begin());
			}


			test( ActivationOfSelectedCalleesSwitchesMainSelection )
			{
				// INIT
				auto rep = representation<true, threads_all>::create(source);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					return views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.callees)[make_tuple(address, thread_id)].id;
				};

				add_records(*rep.selection_main, plural + 1u);

				// ACT
				add_records(*rep.selection_callees, plural + get_id(102, 1));
				rep.activate_callees();

				// ASSERT
				assert_equivalent(plural + 2u, *rep.selection_main);
				assert_equal(rep.selection_callees->end(), rep.selection_callees->begin());
			}
		end_test_suite


		begin_test_suite( RepresentationCallstacksCumulativeTests )
			shared_ptr<calls_statistics_table> source;

			init( CreateSourceTable )
			{	source = create_sample();	}


			test( MainStatisticsIsTheadAccumulated )
			{
				// INIT / ACT
				auto rep = representation<true, threads_cumulative>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.main);
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 101, 2009, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 1, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 1, 103, 1003, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 1, 104, 2013, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 2, 105, 1005, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 5, 106, 1006, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 5, 103, 1007, 0, 0, 0, 0), *rep.main);
				assert_not_null(rep.selection_main);
				assert_not_null(rep.selection_callers);
				assert_not_equal(rep.selection_main, rep.selection_callers);
				assert_not_null(rep.selection_callees);
				assert_not_equal(rep.selection_main, rep.selection_callees);
				assert_not_equal(rep.selection_callers, rep.selection_callees);
			}


			test( AggregatedHierarchyIsHoldByRepresentation )
			{
				// INIT
				auto rep = representation<true, threads_cumulative>::create(source);
				weak_ptr<calls_statistics_table> wsource = source;

				// ACT
				source.reset();

				// ASSERT
				assert_is_false(wsource.expired());
			}


			test( CallersAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<true, threads_cumulative>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callers);
				assert_equal(rep.callers->end(), rep.callers->begin());

				// ACT
				add_records(*rep.selection_main, plural + 4u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 101, 2013, 0, 0, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 0, 2009, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 101, 2013, 0, 0, 0, 0), *rep.callers);
			}


			test( CalleesAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<true, threads_cumulative>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callees);
				assert_equal(rep.callees->end(), rep.callees->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 103, 1003, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 104, 2013, 0, 0, 0, 0), *rep.callees);

				// ACT
				add_records(*rep.selection_main, plural + 5u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 103, 2010, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 104, 2013, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 106, 1006, 0, 0, 0, 0), *rep.callees);
			}


			test( ActivationOfSelectedCallersSwitchesMainSelection )
			{
				// INIT
				auto rep = representation<true, threads_cumulative>::create(source);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					return views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.callers)[make_tuple(address, thread_id)].id;
				};

				add_records(*rep.selection_main, plural + 3u);

				// ACT
				add_records(*rep.selection_callers, plural + get_id(105, threads_model::cumulative));
				rep.activate_callers();

				// ASSERT
				assert_equivalent(plural + 5u, *rep.selection_main);
				assert_equal(rep.selection_callers->end(), rep.selection_callers->begin());

				// INIT
				add_records(*rep.selection_main, plural + 6u);

				// ACT
				add_records(*rep.selection_callers, plural + get_id(102, threads_model::cumulative) + get_id(105, threads_model::cumulative));
				rep.activate_callers();

				// ASSERT
				assert_equivalent(plural + 2u + 5u, *rep.selection_main);
				assert_equal(rep.selection_callers->end(), rep.selection_callers->begin());
			}


			test( ActivationOfSelectedCalleesSwitchesMainSelection )
			{
				// INIT
				auto rep = representation<true, threads_cumulative>::create(source);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					return views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.callees)[make_tuple(address, thread_id)].id;
				};

				add_records(*rep.selection_main, plural + 1u);

				// ACT
				add_records(*rep.selection_callees, plural + get_id(102, threads_model::cumulative));
				rep.activate_callees();

				// ASSERT
				assert_equivalent(plural + 2u, *rep.selection_main);
				assert_equal(rep.selection_callees->end(), rep.selection_callees->begin());
			}
		end_test_suite


		begin_test_suite( RepresentationCallstacksFilteredTests )
			shared_ptr<calls_statistics_table> source;

			init( CreateSourceTable )
			{	source = create_sample();	}


			test( MainStatisticsIsThreadFiltered )
			{
				// INIT / ACT
				auto rep1 = representation<true, threads_filtered>::create(source, 1);

				// ACT / ASSERT
				assert_not_null(rep1.main);
				assert_equivalent(plural
					+ make_call_statistics(1, 1, 0, 101, 1001, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 103, 1003, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 1, 104, 1004, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 2, 105, 1005, 0, 0, 0, 0)
					+ make_call_statistics(6, 1, 5, 106, 1006, 0, 0, 0, 0)
					+ make_call_statistics(7, 1, 5, 103, 1007, 0, 0, 0, 0), *rep1.main);
				assert_not_null(rep1.selection_main);
				assert_not_null(rep1.selection_callers);
				assert_not_equal(rep1.selection_main, rep1.selection_callers);
				assert_not_null(rep1.selection_callees);
				assert_not_equal(rep1.selection_main, rep1.selection_callees);
				assert_not_equal(rep1.selection_callers, rep1.selection_callees);

				// INIT / ACT
				auto rep2 = representation<true, threads_filtered>::create(source, 2);

				// ACT / ASSERT
				assert_not_null(rep2.main);
				assert_equivalent(plural
					+ make_call_statistics(8, 2, 0, 101, 1008, 0, 0, 0, 0)
					+ make_call_statistics(9, 2, 8, 104, 1009, 0, 0, 0, 0), *rep2.main);
				assert_not_null(rep2.selection_main);
			}


			test( CallersAreListedAccordinglyToSelectionAndThreadSelection )
			{
				// INIT / ACT
				auto rep1 = representation<true, threads_filtered>::create(source, 1);

				// ACT / ASSERT
				assert_not_null(rep1.callers);
				assert_equal(rep1.callers->end(), rep1.callers->begin());

				// ACT
				add_records(*rep1.selection_main, plural + 2u + 6u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 101, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 105, 1006, 0, 0, 0, 0), *rep1.callers);

				// ACT
				add_records(*rep1.selection_main, plural + 6u);

				// INIT / ACT
				auto rep2 = representation<true, threads_filtered>::create(source, 2);

				// ACT
				add_records(*rep2.selection_main, plural + 9u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 2, 0, 101, 1009, 0, 0, 0, 0), *rep2.callers);

				// INIT / ACT
				add_records(*rep2.selection_threads, plural + 1u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 2, 0, 101, 1009, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 101, 1004, 0, 0, 0, 0), *rep2.callers);
			}


			test( CalleesAreListedAccordinglyToSelectionAndThreadSelection )
			{
				// INIT / ACT
				auto rep = representation<true, threads_filtered>::create(source, 1);

				// ACT / ASSERT
				assert_not_null(rep.callees);
				assert_equal(rep.callees->end(), rep.callees->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u + 5u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 2010, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 104, 1004, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 106, 1006, 0, 0, 0, 0), *rep.callees);

				// INIT / ACT
				add_records(*rep.selection_threads, plural + 2u);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 2, 0, 104, 1009, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 2010, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 104, 1004, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 106, 1006, 0, 0, 0, 0), *rep.callees);
			}


			test( ActivationOfSelectedCallersSwitchesMainSelection )
			{
				// INIT
				auto rep = representation<true, threads_filtered>::create(source, 1);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					return views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.callers)[make_tuple(address, thread_id)].left().id;
				};

				add_records(*rep.selection_main, plural + 3u);

				// ACT
				add_records(*rep.selection_callers, plural + get_id(105, 1));
				rep.activate_callers();

				// ASSERT
				assert_equivalent(plural + 5u, *rep.selection_main);
				assert_equal(rep.selection_callers->end(), rep.selection_callers->begin());

				// INIT
				add_records(*rep.selection_main, plural + 6u);

				// ACT
				add_records(*rep.selection_callers, plural + get_id(102, 1) + get_id(105, 1));
				rep.activate_callers();

				// ASSERT
				assert_equivalent(plural + 2u + 5u, *rep.selection_main);
				assert_equal(rep.selection_callers->end(), rep.selection_callers->begin());
			}


			test( ActivationOfSelectedCalleesSwitchesMainSelection )
			{
				// INIT
				auto rep = representation<true, threads_filtered>::create(source, 1);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					return views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.callees)[make_tuple(address, thread_id)].left().id;
				};

				add_records(*rep.selection_main, plural + 1u);

				// ACT
				add_records(*rep.selection_callees, plural + get_id(102, 1));
				rep.activate_callees();

				// ASSERT
				assert_equivalent(plural + 2u, *rep.selection_main);
				assert_equal(rep.selection_callees->end(), rep.selection_callees->begin());
			}
		end_test_suite


		begin_test_suite( RepresentationFlatAllTests )
			shared_ptr<calls_statistics_table> source;

			init( CreateSourceTable )
			{	source = create_sample_with_recursion();	}


			test( MainStatisticsIsRecursionAwareAggregationOfTheSource )
			{
				// INIT / ACT
				auto rep = representation<false, threads_all>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.main);
				assert_equivalent(plural
					+ make_call_statistics(1, 1, 0, 101, 2016, 0, 11, 0, 0)
					+ make_call_statistics(2, 1, 0, 102, 2006, 0, 32, 0, 0)
					+ make_call_statistics(3, 1, 0, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(4, 5, 0, 101, 1006, 0, 29, 0, 0)
					+ make_call_statistics(5, 5, 0, 102, 3024, 0, 31, 0, 0), *rep.main);
				assert_not_null(rep.selection_main);
				assert_not_null(rep.selection_callers);
				assert_not_equal(rep.selection_main, rep.selection_callers);
				assert_not_null(rep.selection_callees);
				assert_not_equal(rep.selection_main, rep.selection_callees);
				assert_not_equal(rep.selection_callers, rep.selection_callees);

				// INIT
				weak_ptr<calls_statistics_table> wsource = source;

				// ACT
				source.reset();

				// ASSERT
				assert_is_false(wsource.expired());
			}


			test( CallersAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<false, threads_all>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callers);
				assert_equal(rep.callers->end(), rep.callers->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 0, 1011, 0, 11, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1005, 0, 0, 0, 0)
					+ make_call_statistics(0, 5, 0, 0, 1006, 0, 29, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_main, plural + 2u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 0, 1011, 0, 11, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 2009, 0, 19, 0, 0)
					+ make_call_statistics(0, 5, 0, 0, 1006, 0, 29, 0, 0)
					+ make_call_statistics(0, 1, 0, 101, 1002, 0, 13, 0, 0)
					+ make_call_statistics(0, 5, 0, 101, 1007, 0, 31, 0, 0)
					+ make_call_statistics(0, 5, 0, 102, 2017, 0, 0, 0, 0), *rep.callers);
			}


			test( CalleesAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<false, threads_all>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callees);
				assert_equal(rep.callees->end(), rep.callees->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 13, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(0, 5, 0, 102, 1007, 0, 31, 0, 0), *rep.callees);

				// ACT
				add_records(*rep.selection_main, plural + 2u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 13, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(0, 5, 0, 102, 3024, 0, 31, 0, 0), *rep.callees);
			}
		end_test_suite


		begin_test_suite( RepresentationFlatCumulativeTests )
			shared_ptr<calls_statistics_table> source;

			init( CreateSourceTable )
			{	source = create_sample_with_recursion();	}


			test( MainStatisticsIsRecursionAwareAggregationOfTheSource )
			{
				// INIT / ACT
				auto rep = representation<false, threads_cumulative>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.main);
				assert_equivalent(plural
					+ make_call_statistics(1, threads_model::cumulative, 0, 101, 3022, 0, 40, 0, 0)
					+ make_call_statistics(2, threads_model::cumulative, 0, 102, 5030, 0, 63, 0, 0)
					+ make_call_statistics(3, threads_model::cumulative, 0, 103, 1003, 0, 17, 0, 0), *rep.main);
				assert_not_null(rep.selection_main);
				assert_not_null(rep.selection_callers);
				assert_not_equal(rep.selection_main, rep.selection_callers);
				assert_not_null(rep.selection_callees);
				assert_not_equal(rep.selection_main, rep.selection_callees);
				assert_not_equal(rep.selection_callers, rep.selection_callees);

				// INIT
				weak_ptr<calls_statistics_table> wsource = source;

				// ACT
				source.reset();

				// ASSERT
				assert_is_false(wsource.expired());
			}


			test( CallersAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<false, threads_cumulative>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callers);
				assert_equal(rep.callers->end(), rep.callers->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 103, 1005, 0, 0, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 0, 2017, 0, 40, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_main, plural + 2u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 0, 2017, 0, 40, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 103, 2009, 0, 19, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 101, 2009, 0, 44, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 102, 2017, 0, 0, 0, 0), *rep.callers);
			}


			test( CalleesAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<false, threads_cumulative>::create(source);

				// ACT / ASSERT
				assert_not_null(rep.callees);
				assert_equal(rep.callees->end(), rep.callees->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 102, 2009, 0, 44, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 103, 1003, 0, 17, 0, 0), *rep.callees);

				// ACT
				add_records(*rep.selection_main, plural + 2u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, threads_model::cumulative, 0, 102, 4026, 0, 44, 0, 0)
					+ make_call_statistics(0, threads_model::cumulative, 0, 103, 1003, 0, 17, 0, 0), *rep.callees);
			}
		end_test_suite


		begin_test_suite( RepresentationFlatFilteredTests )
			shared_ptr<calls_statistics_table> source;

			init( CreateSourceTable )
			{	source = create_sample_with_recursion();	}


			test( MainStatisticsIsRecursionAwareAggregationOfTheSource )
			{
				// INIT / ACT
				auto rep1 = representation<false, threads_filtered>::create(source, 1);

				// ACT / ASSERT
				assert_not_null(rep1.main);
				assert_equivalent(plural
					+ make_call_statistics(1, 1, 0, 101, 2016, 0, 11, 0, 0)
					+ make_call_statistics(2, 1, 0, 102, 2006, 0, 32, 0, 0)
					+ make_call_statistics(3, 1, 0, 103, 1003, 0, 17, 0, 0), *rep1.main);
				assert_not_null(rep1.selection_main);
				assert_not_null(rep1.selection_callers);
				assert_not_equal(rep1.selection_main, rep1.selection_callers);
				assert_not_null(rep1.selection_callees);
				assert_not_equal(rep1.selection_main, rep1.selection_callees);
				assert_not_equal(rep1.selection_callers, rep1.selection_callees);

				// INIT / ACT
				auto rep2 = representation<false, threads_filtered>::create(source, 5);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(4, 5, 0, 101, 1006, 0, 29, 0, 0)
					+ make_call_statistics(5, 5, 0, 102, 3024, 0, 31, 0, 0), *rep2.main);

				// INIT / ACT
				add_records(*rep2.selection_threads, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(1, 1, 0, 101, 2016, 0, 11, 0, 0)
					+ make_call_statistics(2, 1, 0, 102, 2006, 0, 32, 0, 0)
					+ make_call_statistics(3, 1, 0, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(4, 5, 0, 101, 1006, 0, 29, 0, 0)
					+ make_call_statistics(5, 5, 0, 102, 3024, 0, 31, 0, 0), *rep2.main);

				// INIT
				weak_ptr<calls_statistics_table> wsource = source;

				// ACT
				source.reset();

				// ASSERT
				assert_is_false(wsource.expired());
			}


			test( CallersAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto source2 = create_sample_multithreaded();
				auto rep = representation<false, threads_filtered>::create(source2, 7);
				auto get_id = [&] (long_address_t address, id_t thread_id) -> id_t {
					keyer::id id;
					return id(views::unique_index< keyer::combine2<keyer::address, keyer::thread_id> >(*rep.main)[make_tuple(address, thread_id)]);
				};

				// ACT / ASSERT
				assert_not_null(rep.callers);
				assert_equal(rep.callers->end(), rep.callers->begin());

				// ACT
				add_records(*rep.selection_main, plural + get_id(104, 7));

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 7, 0, 102, 1104, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 103, 1105, 0, 0, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_threads, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1004, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 102, 1104, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 103, 1105, 0, 0, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_main, plural + get_id(103, 1));

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 2007, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 101, 1103, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 102, 1104, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 103, 1105, 0, 0, 0, 0), *rep.callers);

				// ACT
				add_records(*rep.selection_threads, plural + 9u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 2007, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 101, 1103, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 102, 1104, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 103, 1105, 0, 0, 0, 0)
					+ make_call_statistics(0, 9, 0, 102, 1303, 0, 0, 0, 0), *rep.callers);
			}


			test( CalleesAreListedAccordinglyToSelection )
			{
				// INIT / ACT
				auto rep = representation<false, threads_filtered>::create(source, 1);

				// ACT / ASSERT
				assert_not_null(rep.callees);
				assert_equal(rep.callees->end(), rep.callees->begin());

				// ACT
				add_records(*rep.selection_main, plural + 1u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 13, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1003, 0, 17, 0, 0), *rep.callees);

				// ACT
				add_records(*rep.selection_threads, plural + 5u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 13, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(0, 5, 0, 102, 1007, 0, 31, 0, 0), *rep.callees);

				// ACT
				add_records(*rep.selection_main, plural + 2u);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 102, 1002, 0, 13, 0, 0)
					+ make_call_statistics(0, 1, 0, 103, 1003, 0, 17, 0, 0)
					+ make_call_statistics(0, 5, 0, 102, 3024, 0, 31, 0, 0), *rep.callees);
			}
		end_test_suite

	}
}
