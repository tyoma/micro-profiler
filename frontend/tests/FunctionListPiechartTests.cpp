#include <frontend/function_list.h>

#include <frontend/persistence.h>

#include "helpers.h"
#include "mocks.h"

#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<long_address_t> unthreaded_statistic_types;

			const double c_tolerance = 0.000001;

			void increment(int *value)
			{	++*value;	}
		}

		begin_test_suite( FunctionListPiechartTests )
			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;
			scontext::wire dummy_context;

			FunctionListPiechartTests()
				: ser(_buffer), dser(_buffer)
			{	}

			init( CreatePrerequisites )
			{
				resolver.reset(new mocks::symbol_resolver);
				tmodel.reset(new mocks::threads_model);
			}


			test( NonNullPiechartModelIsReturnedFromFunctionsList )
			{
				// INIT
				pair<long_address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				shared_ptr<functions_list> fl(functions_list::create(1, mocks::symbol_resolver::create(symbols), tmodel));

				// ACT
				shared_ptr< series<double> > m = fl->get_column_series();

				// ASSERT
				assert_equal(0u, m->size());
			}


			test( PiechartModelReturnsConvertedValuesForTimesCalled )
			{
				// INIT
				pair<long_address_t, string> symbols[] = {
					make_pair(5, "Lorem"), make_pair(13, "Ipsum"), make_pair(17, "Amet"), make_pair(123, "dolor"),
				};
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 123, s[addr(17)].times_called = 127, s[addr(13)].times_called = 12, s[addr(123)].times_called = 12000;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s, *tmodel);

				shared_ptr<functions_list> fl = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				// ACT
				fl->set_order(columns::times_called, false);

				// ASSERT
				assert_equal(4u, m->size());
				assert_approx_equal(12000.0, m->get_value(0), c_tolerance);
				assert_approx_equal(127.0, m->get_value(1), c_tolerance);
				assert_approx_equal(123.0, m->get_value(2), c_tolerance);
				assert_approx_equal(12.0, m->get_value(3), c_tolerance);

				// ACT
				fl->set_order(columns::times_called, true);

				// ASSERT
				assert_equal(4u, m->size());
				assert_approx_equal(12000.0, m->get_value(3), c_tolerance);
				assert_approx_equal(127.0, m->get_value(2), c_tolerance);
				assert_approx_equal(123.0, m->get_value(1), c_tolerance);
				assert_approx_equal(12.0, m->get_value(0), c_tolerance);
			}


			test( FunctionListUpdateLeadsToPiechartModelUpdateAndSignal )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				shared_ptr<functions_list> fl(functions_list::create(500, mocks::symbol_resolver::create(symbols), tmodel));
				unthreaded_statistic_types::map_detailed s;
				int invalidated_count = 0;

				s[5].times_called = 123, s[17].times_called = 127, s[13].times_called = 12, s[123].times_called = 12000;

				serialize_single_threaded(ser, s);
				dser(*fl, dummy_context);
				s.clear();

				shared_ptr< series<double> > m = fl->get_column_series();

				fl->set_order(columns::times_called, false);

				s[120].times_called = 11001;
				serialize_single_threaded(ser, s);

				wpl::slot_connection conn = m->invalidated += bind(&increment, &invalidated_count);

				// ACT
				dser(*fl, dummy_context);

				// ASSERT
				assert_equal(1, invalidated_count);
				assert_equal(5u, m->size());
				assert_approx_equal(12000.0, m->get_value(0), c_tolerance);
				assert_approx_equal(11001.0, m->get_value(1), c_tolerance);
				assert_approx_equal(127.0, m->get_value(2), c_tolerance);
				assert_approx_equal(123.0, m->get_value(3), c_tolerance);
				assert_approx_equal(12.0, m->get_value(4), c_tolerance);
			}


			test( PiechartModelReturnsConvertedValuesForExclusiveTime )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				statistic_types::map_detailed s;

				s[addr(5)].exclusive_time = 13, s[addr(17)].exclusive_time = 127, s[addr(13)].exclusive_time = 12;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl1 = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m1 = fl1->get_column_series();
				
				s[addr(123)].exclusive_time = 12000;
				emulate_save(ser, 100, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl2 = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(columns::exclusive, false);
				fl2->set_order(columns::exclusive, false);

				// ASSERT
				assert_equal(3u, m1->size());
				assert_approx_equal(0.254, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.026, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.024, m1->get_value(2), c_tolerance);
				assert_equal(4u, m2->size());
				assert_approx_equal(120.0, m2->get_value(0), c_tolerance);
				assert_approx_equal(1.27, m2->get_value(1), c_tolerance);
				assert_approx_equal(0.13, m2->get_value(2), c_tolerance);
				assert_approx_equal(0.12, m2->get_value(3), c_tolerance);
			}


			test( PiechartModelReturnsConvertedValuesForTheRestOfTheFields )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 100, s[addr(17)].times_called = 1000;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 130;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 120;
				s[addr(5)].max_call_time = 14, s[addr(17)].max_call_time = 128;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl1 = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m1 = fl1->get_column_series();
				
				emulate_save(ser, 1000, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl2 = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(columns::inclusive, false);
				fl2->set_order(columns::inclusive, true);

				// ASSERT
				assert_approx_equal(0.240, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.030, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.015, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.120, m2->get_value(1), c_tolerance);

				// ACT
				fl1->set_order(columns::exclusive_avg, false);
				fl2->set_order(columns::exclusive_avg, true);

				// ASSERT
				assert_approx_equal(0.00032, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.00026, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.00013, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.00016, m2->get_value(1), c_tolerance);

				// ACT
				fl1->set_order(columns::inclusive_avg, false);
				fl2->set_order(columns::inclusive_avg, true);

				// ASSERT
				assert_approx_equal(0.00030, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.00024, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.00012, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.00015, m2->get_value(1), c_tolerance);

				// ACT
				fl1->set_order(columns::max_time, false);
				fl2->set_order(columns::max_time, true);

				// ASSERT
				assert_approx_equal(0.256, m1->get_value(0), c_tolerance);
				assert_approx_equal(0.028, m1->get_value(1), c_tolerance);
				assert_approx_equal(0.014, m2->get_value(0), c_tolerance);
				assert_approx_equal(0.128, m2->get_value(1), c_tolerance);
			}


			test( PiechartForUnsupportedColumnsIsSimplyZero )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 100, s[addr(17)].times_called = 1000;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 130;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 120;
				s[addr(5)].max_call_time = 14, s[addr(17)].max_call_time = 128;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				// ACT / ASSERT
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::order, true);
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(3), c_tolerance);
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::name, false);
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(3), c_tolerance);
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::max_reentrance, true);
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(3), c_tolerance);
			}


			test( ChildrenStatisticsProvideColumnSeries )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				statistic_types::map_detailed s;
				shared_ptr< series<double> > m;

				s[addr(5)].times_called = 2000;
				s[addr(5)].callees[addr(11)].times_called = 29, s[addr(5)].callees[addr(13)].times_called = 31;
				s[addr(5)].callees[addr(11)].exclusive_time = 16, s[addr(5)].callees[addr(13)].exclusive_time = 10;

				s[addr(17)].times_called = 1999;
				s[addr(17)].callees[addr(11)].times_called = 101, s[addr(17)].callees[addr(19)].times_called = 103, s[addr(17)].callees[addr(23)].times_called = 1100;
				s[addr(17)].callees[addr(11)].exclusive_time = 3, s[addr(17)].callees[addr(19)].exclusive_time = 112, s[addr(17)].callees[addr(23)].exclusive_time = 9;

				emulate_save(ser, 100, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl = snapshot_load<scontext::file_v4>(dser);
				shared_ptr<linked_statistics> ls;

				fl->set_order(columns::times_called, false);

				// ACT
				ls = fl->watch_children(0);
				ls->set_order(columns::times_called, true);
				m = ls->get_column_series();

				// ASSERT
				assert_equal(2u, m->size());
				assert_approx_equal(29.0, m->get_value(0), c_tolerance);
				assert_approx_equal(31.0, m->get_value(1), c_tolerance);

				// ACT
				ls = fl->watch_children(1);
				m = ls->get_column_series();
				ls->set_order(columns::exclusive, false);

				// ASSERT
				assert_equal(3u, m->size());
				assert_approx_equal(1.12, m->get_value(0), c_tolerance);
				assert_approx_equal(0.09, m->get_value(1), c_tolerance);
				assert_approx_equal(0.03, m->get_value(2), c_tolerance);
			}


			test( PiechartModelReturnsZeroesAverageValuesForUncalledFunctions )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 0, s[addr(17)].times_called = 0;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 0;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 0;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m = fl->get_column_series();

				// ACT
				fl->set_order(columns::exclusive_avg, false);

				// ASSERT
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(1), c_tolerance);

				// ACT
				fl->set_order(columns::inclusive_avg, false);

				// ASSERT
				assert_approx_equal(0.0, m->get_value(0), c_tolerance);
				assert_approx_equal(0.0, m->get_value(1), c_tolerance);
			}


			test( SwitchingSortOrderLeadsToPiechartModelUpdate )
			{
				// INIT
				pair<long_address_t, string> symbols[] = { make_pair(0, ""), };
				statistic_types::map_detailed s;
				int invalidated_count = 0;

				s[addr(5)].times_called = 100, s[addr(17)].times_called = 1000;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 130;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 120;
				s[addr(5)].max_call_time = 14, s[addr(17)].max_call_time = 128;

				emulate_save(ser, 500, *mocks::symbol_resolver::create(symbols), s, *tmodel);
				shared_ptr<functions_list> fl = snapshot_load<scontext::file_v4>(dser);
				shared_ptr< series<double> > m = fl->get_column_series();
				wpl::slot_connection conn = m->invalidated += bind(&increment, &invalidated_count);

				// ACT
				fl->set_order(columns::times_called, false);

				// ASSERT
				assert_equal(1, invalidated_count);

				// ACT
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::exclusive, false);
				fl->set_order(columns::inclusive, false);
				fl->set_order(columns::exclusive_avg, false);
				fl->set_order(columns::inclusive_avg, false);
				fl->set_order(columns::max_time, false);

				// ASSERT
				assert_equal(7, invalidated_count);
			}
		end_test_suite
	}
}
