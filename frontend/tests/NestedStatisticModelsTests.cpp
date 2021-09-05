#include <frontend/function_list.h>

#include "helpers.h"
#include "mocks.h"

#include <frontend/nested_transform.h>
#include <frontend/tables.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef pair<statistic_types::key, statistic_types::function_detailed> threaded_addressed_function;
			typedef pair<statistic_types::key, count_t> caller_function;

			template <typename T>
			shared_ptr<T> make_shared_copy(const T &other)
			{	return make_shared<T>(other);	}

			const double c_tolerance = 0.000001;
			const vector< pair<statistic_types::key, statistic_types::function> > empty;
		}

		begin_test_suite( NestedStatisticModelsTests )
			std::shared_ptr<tables::modules> modules;
			std::shared_ptr<tables::module_mappings> mappings;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<mocks::threads_model> tmodel;

			init( CreatePrerequisites )
			{
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				resolver.reset(new mocks::symbol_resolver(modules, mappings));
				tmodel.reset(new mocks::threads_model);
			}


			test( DependentModelsPickNestedMaps )
			{
				// INIT
				threaded_addressed_function functions[] = {
					make_statistics(addr(0x1000), 1, 0, 0, 0, 0),
					make_statistics(addr(0x1001), 1, 0, 0, 0, 0,
						plural
							+ make_statistics_base(addr(0x2001), 100, 1, 0, 0, 0)
							+ make_statistics_base(addr(0x2002), 109, 1, 0, 0, 0),
						plural
							+ caller_function(addr(0x3001), 1u)
							+ caller_function(addr(0x3003), 1u)
							+ caller_function(addr(0x3009), 51u)
					),
					make_statistics(addr(0x1002), 1, 0, 0, 0, 0,
						plural
							+ make_statistics_base(addr(0x2007), 109, 1, 0, 0, 0),
						plural
							+ caller_function(addr(0x3007), 1u)
							+ caller_function(addr(0x3009), 1u)
					),
				};
				auto s = make_shared<tables::statistics>();

				// INIT / ACT
				auto callers = create_callers_model(s, 1, resolver, tmodel, make_shared_copy(plural + addr(0x1001)));
				auto callees = create_callees_model(s, 1, resolver, tmodel, make_shared_copy(plural + addr(0x1001)));

				// ACT / ASSERT
				assert_not_null(callers);
				assert_equal(0u, callers->get_count());
				assert_not_null(callees);
				assert_equal(0u, callees->get_count());

				// ACT
				assign(*s, functions);
				s->invalidate();

				// ASSERT
				assert_equal(3u, callers->get_count());
				assert_equal(2u, callees->get_count());

				// INIT / ACT
				callers = create_callers_model(s, 1, resolver, tmodel, make_shared_copy(plural
					+ addr(0x1002) + addr(0x1000)));
				callees = create_callees_model(s, 1, resolver, tmodel, make_shared_copy(plural
					+ addr(0x1002) + addr(0x1000) + addr(0x1001)));

				// ASSERT
				assert_equal(2u, callers->get_count());
				assert_equal(3u, callees->get_count());

				// INIT / ACT
				callers = create_callers_model(s, 1, resolver, tmodel, make_shared_copy(plural
					+ addr(0x1001) + addr(0x1002) + addr(0x1000)));

				// ASSERT
				assert_equal(5u, callers->get_count());

				// ACT
				s->clear();

				// ASSERT
				assert_equal(0u, callees->get_count());
				assert_equal(0u, callers->get_count());
			}


			test( CalleesModelReturnsTextualValuesExpected )
			{
				// INIT
				unsigned columns[] = {	1, 3, 4, 5, 6, 7, 8, 9,	};
				threaded_addressed_function functions[] = {
					make_statistics(addr(0x1001), 1, 0, 0, 0, 0,
						plural
							+ make_statistics_base(addr(0x2001), 100, 1, 109, 17, 19190)
							+ make_statistics_base(addr(0x2002), 109, 3, 1090, 11711, 0),
						vector<caller_function>()),
					make_statistics(addr(0x1002), 1, 0, 0, 0, 0,
						plural
							+ make_statistics_base(addr(0x2001), 9318, 123, 2, 3, 4),
						vector<caller_function>()),
				};
				auto s = make_shared<tables::statistics>();
				auto sel = make_shared_copy(plural + addr(0x1001));

				assign(*s, functions);

				auto callees = create_callees_model(s, 0.01, resolver, tmodel, sel);

				// ACT
				auto text = get_text(*callees, columns);

				// ASSERT
				string reference1[][8] = {
					{	"00002001", "100", "170ms", "1.09s", "1.7ms", "10.9ms", "1", "192s",	},
					{	"00002002", "109", "117s", "10.9s", "1.07s", "100ms", "3", "0s",	},
				};

				assert_equivalent(mkvector(reference1), text);

				// INIT
				*sel = plural + addr(0x1001) + addr(0x1002);
				s->invalidate();

				// ACT
				text = get_text(*callees, columns);

				// ASSERT
				string reference2[][8] = {
					{	"00002001", "100", "170ms", "1.09s", "1.7ms", "10.9ms", "1", "192s",	},
					{	"00002002", "109", "117s", "10.9s", "1.07s", "100ms", "3", "0s",	},
					{	"00002001", "9318", "30ms", "20ms", "3.22\xCE\xBCs", "2.15\xCE\xBCs", "123", "40ms",	},
				};

				assert_equivalent(mkvector(reference2), text);
			}


			test( CallersModelReturnsTextualValuesExpected )
			{
				// INIT
				unsigned columns[] = {	1, 3,	};
				threaded_addressed_function functions[] = {
					make_statistics(addr(0x1001), 1, 0, 0, 0, 0, empty, plural
						+ caller_function(addr(0x2001), 19190)
						+ caller_function(addr(0x2002), 179)),
					make_statistics(addr(0x1003), 1, 0, 0, 0, 0, empty, plural
						+ caller_function(addr(0x2001), 11000)
						+ caller_function(addr(0x2007), 10700)
						+ caller_function(addr(0x2003), 10090)),
				};
				auto s = make_shared<tables::statistics>();
				auto sel = make_shared_copy(plural + addr(0x1001));

				assign(*s, functions);

				shared_ptr<linked_statistics> callers = create_callers_model(s, 0.01, resolver, tmodel, sel);

				// ACT
				auto text = get_text(*callers, columns);

				// ASSERT
				string reference1[][2] = {
					{	"00002001", "19190",	},
					{	"00002002", "179",	},
				};

				assert_equivalent(mkvector(reference1), text);

				// INIT
				*sel = plural + addr(0x1001) + addr(0x1003);
				callers->fetch();

				// ACT
				text = get_text(*callers, columns);

				// ASSERT
				string reference2[][2] = {
					{	"00002001", "19190",	},
					{	"00002002", "179",	},

					{	"00002001", "11000",	},
					{	"00002007", "10700",	},
					{	"00002003", "10090",	},
				};

				assert_equivalent(mkvector(reference2), text);
			}


			test( CalleesStatisticsProvideColumnSeries )
			{
				// INIT
				auto s = make_shared<tables::statistics>();
				auto sel = make_shared_copy(plural + addr(5));

				(*s)[addr(5)].times_called = 2000;
				(*s)[addr(5)].callees[addr(11)].times_called = 29, (*s)[addr(5)].callees[addr(13)].times_called = 31;
				(*s)[addr(5)].callees[addr(11)].exclusive_time = 16, (*s)[addr(5)].callees[addr(13)].exclusive_time = 10;

				(*s)[addr(17)].times_called = 1999;
				(*s)[addr(17)].callees[addr(11)].times_called = 101, (*s)[addr(17)].callees[addr(19)].times_called = 103, (*s)[addr(17)].callees[addr(23)].times_called = 1100;
				(*s)[addr(17)].callees[addr(11)].exclusive_time = 3, (*s)[addr(17)].callees[addr(19)].exclusive_time = 112, (*s)[addr(17)].callees[addr(23)].exclusive_time = 9;


				auto ls = create_callees_model(s, 0.01, resolver, tmodel, sel);
				auto m = ls->get_column_series();

				// ACT
				ls->set_order(columns::times_called, true);

				// ASSERT
				assert_equal(2u, m->get_count());
				assert_approx_equal(29.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(31.0, get_value(*m, 1), c_tolerance);

				// ACT
				ls->set_order(columns::times_called, false);

				// ASSERT
				assert_equal(2u, m->get_count());
				assert_approx_equal(31.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(29.0, get_value(*m, 1), c_tolerance);

				// ACT
				(*sel)[0] = addr(17);
				ls->fetch();

				// ASSERT
				assert_equal(3u, m->get_count());
				assert_approx_equal(1100.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(103.0, get_value(*m, 1), c_tolerance);
				assert_approx_equal(101.0, get_value(*m, 2), c_tolerance);

				// ACT
				ls->set_order(columns::exclusive, false);

				// ASSERT
				assert_equal(3u, m->get_count());
				assert_approx_equal(1.12, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.09, get_value(*m, 1), c_tolerance);
				assert_approx_equal(0.03, get_value(*m, 2), c_tolerance);

				// ACT
				sel->push_back(addr(5));
				ls->fetch();

				// ASSERT
				assert_equal(5u, m->get_count());
				assert_approx_equal(1.12, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.16, get_value(*m, 1), c_tolerance);
				assert_approx_equal(0.1, get_value(*m, 2), c_tolerance);
				assert_approx_equal(0.09, get_value(*m, 3), c_tolerance);
				assert_approx_equal(0.03, get_value(*m, 4), c_tolerance);
			}

		end_test_suite
	}
}
