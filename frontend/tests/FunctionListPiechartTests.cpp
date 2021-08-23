#include <frontend/function_list.h>

#include "helpers.h"
#include "mocks.h"

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	template <typename T>
	inline void operator +=(tables::statistics &s, const T &delta)
	{
		for (auto i = begin(delta); i != end(delta); ++i)
			s[i->first] += i->second;
		s.invalidated();
	}

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
			std::shared_ptr<tables::statistics> statistics;
			std::shared_ptr<tables::modules> modules;
			std::shared_ptr<tables::module_mappings> mappings;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;

			shared_ptr<functions_list> create_functions_list(const statistic_types::map_detailed &s,
				timestamp_t ticks_per_second = 500)
			{
				static_cast<statistic_types::map_detailed &>(*statistics) = s;
				return make_shared<functions_list>(statistics, 1.0 / ticks_per_second,
					make_shared<symbol_resolver>(modules, mappings), tmodel);
			}

			init( CreatePrerequisites )
			{
				statistics = make_shared<tables::statistics>();
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				resolver.reset(new mocks::symbol_resolver(modules, mappings));
				tmodel.reset(new mocks::threads_model);
			}


			test( NonNullPiechartModelIsReturnedFromFunctionsList )
			{
				// INIT
				auto fl = create_functions_list((statistic_types::map_detailed()));

				// ACT
				shared_ptr< wpl::list_model<double> > m = fl->get_column_series();

				// ASSERT
				assert_equal(0u, m->get_count());
			}


			test( PiechartModelReturnsConvertedValuesForTimesCalled )
			{
				// INIT
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 123, s[addr(17)].times_called = 127, s[addr(13)].times_called = 12, s[addr(123)].times_called = 12000;

				auto fl = create_functions_list(s);
				auto m = fl->get_column_series();

				// ACT
				fl->set_order(columns::times_called, false);

				// ASSERT
				assert_equal(4u, m->get_count());
				assert_approx_equal(12000.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(127.0, get_value(*m, 1), c_tolerance);
				assert_approx_equal(123.0, get_value(*m, 2), c_tolerance);
				assert_approx_equal(12.0, get_value(*m, 3), c_tolerance);

				// ACT
				fl->set_order(columns::times_called, true);

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
				statistic_types::map_detailed s;
				auto invalidated_count = 0;

				s[addr(5)].times_called = 123, s[addr(17)].times_called = 127, s[addr(13)].times_called = 12, s[addr(123)].times_called = 12000;

				auto fl = create_functions_list(s);
				auto m = fl->get_column_series();

				fl->set_order(columns::times_called, false);
				s.clear(), s[addr(120)].times_called = 11001;

				auto conn = m->invalidate += bind(&increment, &invalidated_count);

				// ACT
				*statistics += s;

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
				statistic_types::map_detailed s;

				s[addr(5)].exclusive_time = 13, s[addr(17)].exclusive_time = 127, s[addr(13)].exclusive_time = 12;

				auto fl1 = create_functions_list(s, 500);
				auto m1 = fl1->get_column_series();

				auto statistics2 = make_shared<tables::statistics>();
				s[addr(123)].exclusive_time = 12000;
				static_cast<statistic_types::map_detailed &>(*statistics2) = s;
				auto fl2 = make_shared<functions_list>(statistics2, 0.01, make_shared<symbol_resolver>(modules, mappings), tmodel);
				auto m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(columns::exclusive, false);
				fl2->set_order(columns::exclusive, false);

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
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 100, s[addr(17)].times_called = 1000;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 130;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 120;
				s[addr(5)].max_call_time = 14, s[addr(17)].max_call_time = 128;

				auto fl1 = create_functions_list(s, 500);
				auto m1 = fl1->get_column_series();

				auto statistics2 = make_shared<tables::statistics>();
				static_cast<statistic_types::map_detailed &>(*statistics2) = s;
				auto fl2 = make_shared<functions_list>(statistics2, 0.001, make_shared<symbol_resolver>(modules, mappings), tmodel);
				auto m2 = fl2->get_column_series();

				// ACT
				fl1->set_order(columns::inclusive, false);
				fl2->set_order(columns::inclusive, true);

				// ASSERT
				assert_approx_equal(0.240, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.030, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.015, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.120, get_value(*m2, 1), c_tolerance);

				// ACT
				fl1->set_order(columns::exclusive_avg, false);
				fl2->set_order(columns::exclusive_avg, true);

				// ASSERT
				assert_approx_equal(0.00032, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.00026, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.00013, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.00016, get_value(*m2, 1), c_tolerance);

				// ACT
				fl1->set_order(columns::inclusive_avg, false);
				fl2->set_order(columns::inclusive_avg, true);

				// ASSERT
				assert_approx_equal(0.00030, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.00024, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.00012, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.00015, get_value(*m2, 1), c_tolerance);

				// ACT
				fl1->set_order(columns::max_time, false);
				fl2->set_order(columns::max_time, true);

				// ASSERT
				assert_approx_equal(0.256, get_value(*m1, 0), c_tolerance);
				assert_approx_equal(0.028, get_value(*m1, 1), c_tolerance);
				assert_approx_equal(0.014, get_value(*m2, 0), c_tolerance);
				assert_approx_equal(0.128, get_value(*m2, 1), c_tolerance);
			}


			test( PiechartForUnsupportedColumnsIsSimplyZero )
			{
				// INIT
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 100, s[addr(17)].times_called = 1000;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 130;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 120;
				s[addr(5)].max_call_time = 14, s[addr(17)].max_call_time = 128;

				auto fl = create_functions_list(s, 500);
				auto m = fl->get_column_series();

				// ACT / ASSERT
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::order, true);
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 3), c_tolerance);
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::name, false);
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 3), c_tolerance);
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::max_reentrance, true);
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 3), c_tolerance);
			}


			test( ChildrenStatisticsProvideColumnSeries )
			{
				// INIT
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 2000;
				s[addr(5)].callees[addr(11)].times_called = 29, s[addr(5)].callees[addr(13)].times_called = 31;
				s[addr(5)].callees[addr(11)].exclusive_time = 16, s[addr(5)].callees[addr(13)].exclusive_time = 10;

				s[addr(17)].times_called = 1999;
				s[addr(17)].callees[addr(11)].times_called = 101, s[addr(17)].callees[addr(19)].times_called = 103, s[addr(17)].callees[addr(23)].times_called = 1100;
				s[addr(17)].callees[addr(11)].exclusive_time = 3, s[addr(17)].callees[addr(19)].exclusive_time = 112, s[addr(17)].callees[addr(23)].exclusive_time = 9;

				auto fl = create_functions_list(s, 100);
				auto m = fl->get_column_series();

				shared_ptr<linked_statistics> ls;

				fl->set_order(columns::times_called, false);

				// ACT
				ls = fl->watch_children(0);
				ls->set_order(columns::times_called, true);
				m = ls->get_column_series();

				// ASSERT
				assert_equal(2u, m->get_count());
				assert_approx_equal(29.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(31.0, get_value(*m, 1), c_tolerance);

				// ACT
				ls = fl->watch_children(1);
				m = ls->get_column_series();
				ls->set_order(columns::exclusive, false);

				// ASSERT
				assert_equal(3u, m->get_count());
				assert_approx_equal(1.12, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.09, get_value(*m, 1), c_tolerance);
				assert_approx_equal(0.03, get_value(*m, 2), c_tolerance);
			}


			test( PiechartModelReturnsZeroesAverageValuesForUncalledFunctions )
			{
				// INIT
				statistic_types::map_detailed s;

				s[addr(5)].times_called = 0, s[addr(17)].times_called = 0;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 0;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 0;

				auto fl = create_functions_list(s);
				auto m = fl->get_column_series();

				// ACT
				fl->set_order(columns::exclusive_avg, false);

				// ASSERT
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 1), c_tolerance);

				// ACT
				fl->set_order(columns::inclusive_avg, false);

				// ASSERT
				assert_approx_equal(0.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.0, get_value(*m, 1), c_tolerance);
			}


			test( SwitchingSortOrderLeadsToPiechartModelUpdate )
			{
				// INIT
				statistic_types::map_detailed s;
				int invalidated_count = 0;

				s[addr(5)].times_called = 100, s[addr(17)].times_called = 1000;
				s[addr(5)].exclusive_time = 16, s[addr(17)].exclusive_time = 130;
				s[addr(5)].inclusive_time = 15, s[addr(17)].inclusive_time = 120;
				s[addr(5)].max_call_time = 14, s[addr(17)].max_call_time = 128;

				auto fl = create_functions_list(s);
				auto m = fl->get_column_series();
				wpl::slot_connection conn = m->invalidate += bind(&increment, &invalidated_count);

				// ACT
				fl->set_order(columns::times_called, false);

				// ASSERT
				assert_equal(2, invalidated_count);

				// ACT
				fl->set_order(columns::times_called, true);
				fl->set_order(columns::exclusive, false);
				fl->set_order(columns::inclusive, false);
				fl->set_order(columns::exclusive_avg, false);
				fl->set_order(columns::inclusive_avg, false);
				fl->set_order(columns::max_time, false);

				// ASSERT
				assert_equal(14, invalidated_count);
			}
		end_test_suite
	}
}
