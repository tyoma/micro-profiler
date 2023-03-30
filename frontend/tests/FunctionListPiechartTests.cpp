#include <frontend/statistic_models.h>

#include "helpers.h"
#include "mocks.h"
#include "primitive_helpers.h"

#include <frontend/columns_layout.h>
#include <frontend/keyer.h>
#include <frontend/models.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>
#include <sdb/integrated_index.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const double c_tolerance = 0.000001;

			void increment(int *value)
			{	++*value;	}
		}

		begin_test_suite( FunctionListPiechartTests )
			shared_ptr<tables::statistics> statistics;
			shared_ptr<tables::modules> modules;
			shared_ptr<tables::module_mappings> mappings;
			shared_ptr<tables::threads> tmodel;

			template <typename ContainerT>
			shared_ptr<table_model> create_functions_list(const ContainerT &s, timestamp_t ticks_per_second = 500)
			{
				add_records(*statistics, s);
				return make_table<table_model>(statistics, create_context(statistics, 1.0 / ticks_per_second,
					make_shared<symbol_resolver>(modules, mappings), tmodel, false), c_statistics_columns);
			}

			template <typename ContainerT>
			shared_ptr<table_model> create_functions_list_detached(const ContainerT &s, timestamp_t ticks_per_second = 500)
			{
				auto local_statistics = make_shared<tables::statistics>();

				add_records(*local_statistics, s);
				return make_table<table_model>(local_statistics, create_context(local_statistics, 1.0 / ticks_per_second,
					make_shared<symbol_resolver>(modules, mappings), tmodel, false), c_statistics_columns);
			}

			init( CreatePrerequisites )
			{
				statistics = make_shared<tables::statistics>();
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				tmodel = make_shared<tables::threads>();
			}


			test( NonNullPiechartModelIsReturnedFromFunctionsList )
			{
				// INIT
				auto fl = create_functions_list_detached(vector<call_statistics>());

				// ACT
				shared_ptr< wpl::list_model<double> > m = fl->get_column_series();

				// ASSERT
				assert_equal(0u, m->get_count());
			}


			test( PiechartModelReturnsConvertedValuesForTimesCalled )
			{
				// INIT
				auto fl = create_functions_list(plural
					+ make_call_statistics(1, 1, 0, 5, 123, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 0, 17, 127, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 0, 13, 12, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 123, 12000, 0, 0, 0, 0));
				auto m = fl->get_column_series();

				// ACT
				fl->set_order(main_columns::times_called, false);

				// ASSERT
				assert_equal(4u, m->get_count());
				assert_approx_equal(12000.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(127.0, get_value(*m, 1), c_tolerance);
				assert_approx_equal(123.0, get_value(*m, 2), c_tolerance);
				assert_approx_equal(12.0, get_value(*m, 3), c_tolerance);

				// ACT
				fl->set_order(main_columns::times_called, true);

				// ASSERT
				assert_equal(4u, m->get_count());
				assert_approx_equal(12000.0, get_value(*m, 3), c_tolerance);
				assert_approx_equal(127.0, get_value(*m, 2), c_tolerance);
				assert_approx_equal(123.0, get_value(*m, 1), c_tolerance);
				assert_approx_equal(12.0, get_value(*m, 0), c_tolerance);
			}


			test( FunctionListUpdateLeadsToPiechartModelUpdateAndSignal )
			{
				// INIT
				auto invalidated_count = 0;

				auto &by_node = sdb::unique_index<keyer::callnode>(*statistics);
				auto fl = create_functions_list(plural
					+ make_call_statistics(1, 1, 0, 5, 123, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 0, 17, 127, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 0, 13, 12, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 123, 12000, 0, 0, 0, 0));
				auto m = fl->get_column_series();

				fl->set_order(main_columns::times_called, false);

				auto conn = m->invalidate += bind(&increment, &invalidated_count);

				// ACT
				auto r = by_node[call_node_key(1, 0, 120)];

				(*r).times_called = 11001;
				r.commit();
				statistics->invalidate();

				// ASSERT
				assert_equal(1, invalidated_count);
				assert_equal(5u, m->get_count());
				assert_approx_equal(12000.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(11001.0, get_value(*m, 1), c_tolerance);
				assert_approx_equal(127.0, get_value(*m, 2), c_tolerance);
				assert_approx_equal(123.0, get_value(*m, 3), c_tolerance);
				assert_approx_equal(12.0, get_value(*m, 4), c_tolerance);
			}


			test( PiechartModelReturnsConvertedValuesForExclusiveTime )
			{
				// INIT
				auto fl1 = create_functions_list(plural
					+ make_call_statistics(1, 1, 0, 5, 0, 0, 0, 13, 0)
					+ make_call_statistics(2, 1, 0, 17, 0, 0, 0, 127, 0)
					+ make_call_statistics(3, 1, 0, 13, 0, 0, 0, 12, 0), 500);
				auto fl2 = create_functions_list_detached(plural
					+ make_call_statistics(1, 1, 0, 5, 0, 0, 0, 13, 0)
					+ make_call_statistics(2, 1, 0, 17, 0, 0, 0, 127, 0)
					+ make_call_statistics(3, 1, 0, 13, 0, 0, 0, 12, 0)
					+ make_call_statistics(4, 1, 0, 123, 0, 0, 0, 12000, 0), 100);

				auto m1 = fl1->get_column_series();
				auto m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(main_columns::exclusive, false);
				fl2->set_order(main_columns::exclusive, false);

				// ASSERT
				assert_equal(3u, m1->get_count());
				assert_approx_equal(0.254, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.026, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.024, get_value(*m1, 2), c_tolerance);
				assert_equal(4u, m2->get_count());
				assert_approx_equal(120.0, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(1.27, get_value(*m2, 1), c_tolerance);
				assert_approx_equal(0.13, get_value(*m2, 2), c_tolerance);
				assert_approx_equal(0.12, get_value(*m2, 3), c_tolerance);
			}


			test( PiechartModelReturnsConvertedValuesForTheRestOfTheFields )
			{
				// INIT
				auto fl1 = create_functions_list_detached(plural
					+ make_call_statistics(1, 1, 0, 5, 100, 0, 15, 16, 14)
					+ make_call_statistics(2, 1, 0, 17, 1000, 0, 120, 130, 128), 500);
				auto m1 = fl1->get_column_series();

				auto fl2 = create_functions_list_detached(plural
					+ make_call_statistics(1, 1, 0, 5, 100, 0, 15, 16, 14)
					+ make_call_statistics(2, 1, 0, 17, 1000, 0, 120, 130, 128), 1000);
				auto m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(main_columns::inclusive, false);
				fl2->set_order(main_columns::inclusive, true);

				// ASSERT
				assert_approx_equal(0.240, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.030, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.015, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.120, get_value(*m2, 1), c_tolerance);

				// ACT
				fl1->set_order(main_columns::exclusive_avg, false);
				fl2->set_order(main_columns::exclusive_avg, true);

				// ASSERT
				assert_approx_equal(0.00032, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.00026, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.00013, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.00016, get_value(*m2, 1), c_tolerance);

				// ACT
				fl1->set_order(main_columns::inclusive_avg, false);
				fl2->set_order(main_columns::inclusive_avg, true);

				// ASSERT
				assert_approx_equal(0.00030, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.00024, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.00012, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.00015, get_value(*m2, 1), c_tolerance);

				// ACT
				fl1->set_order(main_columns::max_time, false);
				fl2->set_order(main_columns::max_time, true);

				// ASSERT
				assert_approx_equal(0.256, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.028, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.014, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.128, get_value(*m2, 1), c_tolerance);
			}


			test( PiechartForUnsupportedColumnsIsEmpty )
			{
				// INIT
				auto fl = create_functions_list_detached(plural
					+ make_call_statistics(1, 1, 0, 5, 100, 0, 15, 16, 14)
					+ make_call_statistics(2, 1, 0, 17, 1000, 0, 120, 130, 128), 500);
				auto m = fl->get_column_series();

				// ACT / ASSERT
				fl->set_order(main_columns::times_called, true);
				fl->set_order(main_columns::order, true);
				assert_equal(0u, m->get_count());
				fl->set_order(main_columns::times_called, true);
				fl->set_order(main_columns::name, false);
				assert_equal(0u, m->get_count());
			}


			test( PiechartModelReturnsZeroesAverageValuesForUncalledFunctions )
			{
				// INIT
				auto fl = create_functions_list_detached(plural
					+ make_call_statistics(1, 1, 0, 5, 0, 0, 15, 16, 0)
					+ make_call_statistics(2, 1, 0, 17, 0, 0, 0, 0, 0));
				auto m = fl->get_column_series();

				// ACT
				fl->set_order(main_columns::exclusive_avg, false);

				// ASSERT
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 1), c_tolerance);

				// ACT
				fl->set_order(main_columns::inclusive_avg, false);

				// ASSERT
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 1), c_tolerance);
			}


			test( SwitchingSortOrderLeadsToPiechartModelUpdate )
			{
				// INIT
				auto fl = create_functions_list_detached(plural
					+ make_call_statistics(1, 1, 0, 5, 100, 0, 15, 16, 14)
					+ make_call_statistics(2, 1, 0, 17, 1000, 0, 120, 130, 128), 500);
				auto m = fl->get_column_series();
				auto invalidated_count = 0;
				wpl::slot_connection conn = m->invalidate += bind(&increment, &invalidated_count);

				// ACT
				fl->set_order(main_columns::times_called, false);

				// ASSERT
				assert_equal(2, invalidated_count);

				// ACT
				fl->set_order(main_columns::times_called, true);
				fl->set_order(main_columns::exclusive, false);
				fl->set_order(main_columns::inclusive, false);
				fl->set_order(main_columns::exclusive_avg, false);
				fl->set_order(main_columns::inclusive_avg, false);
				fl->set_order(main_columns::max_time, false);

				// ASSERT
				assert_equal(14, invalidated_count);
			}
		end_test_suite
	}
}
