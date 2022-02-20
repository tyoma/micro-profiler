#include <frontend/statistic_models.h>

#include "helpers.h"
#include "mocks.h"
#include "primitive_helpers.h"

#include <frontend/tables.h>
#include <frontend/transforms.h>
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
			struct dynamic_vector : vector<T>
			{
				dynamic_vector(const vector<T> &other)
					: vector<T>(other)
				{	}

				wpl::signal<void (size_t)> invalidate;
			};

			template <typename T>
			shared_ptr< dynamic_vector<typename T::value_type> > make_shared_copy(const T &other)
			{	return make_shared< dynamic_vector<typename T::value_type> >(other);	}

			const double c_tolerance = 0.000001;
			const vector< pair<statistic_types::key, statistic_types::function> > empty;

			template <typename T>
			void append(tables::statistics &statistics, const T &items)
			{
				for (auto i = begin(items); i != end(items); ++i)
				{
					auto r = statistics.create();

					*r = *i;
					r.commit();
				}
			}
		}

		begin_test_suite( NestedStatisticModelsTests )
			std::shared_ptr<tables::modules> modules;
			std::shared_ptr<tables::module_mappings> mappings;
			shared_ptr<symbol_resolver> resolver;
			shared_ptr<tables::threads> tmodel;

			init( CreatePrerequisites )
			{
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				resolver = make_shared<mocks::symbol_resolver>(modules, mappings);
				tmodel = make_shared<tables::threads>();
			}


			test( DependentModelsAreEmptyForFlatTopLevelCalls )
			{
				// INIT
				call_statistics functions[] = {
					make_call_statistics(1, 101, 0, 0x1000, 1, 0, 0, 0, 0),
					make_call_statistics(2, 101, 0, 0x1001, 1, 0, 0, 0, 0),
					make_call_statistics(3, 101, 0, 0x2001, 100, 0, 0, 0, 0),
					make_call_statistics(4, 101, 2, 0x2002, 109, 0, 0, 0, 0),
				};
				auto s = make_shared<tables::statistics>();

				// INIT / ACT
				auto callers = create_callers_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 1u + 3u));
				auto callees = create_callees_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 1u + 3u));

				// ACT / ASSERT
				assert_not_null(callers);
				assert_equal(0u, callers->get_count());
				assert_not_null(callees);
				assert_equal(0u, callees->get_count());

				// ACT
				append(*s, functions);
				s->invalidate();

				// ASSERT
				assert_equal(0u, callers->get_count());
				assert_equal(0u, callees->get_count());
			}


			test( DependentModelsPickNestedMaps )
			{
				// INIT
				call_statistics functions[] = {
					make_call_statistics(1, 101, 0, 0x1000, 1, 0, 0, 0, 0),
					make_call_statistics(2, 101, 1, 0x1001, 1, 0, 0, 0, 0),
					make_call_statistics(3, 101, 2, 0x2001, 100, 0, 0, 0, 0),
					make_call_statistics(4, 101, 2, 0x2002, 109, 0, 0, 0, 0),
					make_call_statistics(5, 101, 0, 0x1002, 1, 0, 0, 0, 0),
					make_call_statistics(6, 101, 5, 0x2007, 109, 0, 0, 0, 0),
					make_call_statistics(7, 101, 0, 0x1015, 1, 0, 0, 0, 0),
					make_call_statistics(8, 101, 0, 0x1016, 1, 0, 0, 0, 0),
					make_call_statistics(9, 101, 7, 0x1001, 1, 0, 0, 0, 0),
					make_call_statistics(10, 101, 8, 0x1001, 1, 0, 0, 0, 0),
				};
				auto s = make_shared<tables::statistics>();

				// INIT / ACT
				auto callers = create_callers_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 2u + 1171u /*missing*/));
				auto callees = create_callees_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 2u + 1171u /*missing*/));

				// ACT / ASSERT
				assert_not_null(callers);
				assert_equal(0u, callers->get_count());
				assert_not_null(callees);
				assert_equal(0u, callees->get_count());

				// ACT
				append(*s, functions);
				s->invalidate();

				// ASSERT
				assert_equal(3u, callers->get_count());
				assert_equal(2u, callees->get_count());

				// INIT / ACT
				callers = create_callers_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 6u + 2u));
				callees = create_callees_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 5u + 3u + 2u));

				// ASSERT
				assert_equal(4u, callers->get_count());
				assert_equal(3u, callees->get_count());

				// INIT / ACT
				callers = create_callers_model(s, create_context(s, 1, resolver, tmodel, false), make_shared_copy(plural + 2u));

				// ASSERT
				assert_equal(3u, callers->get_count());

				// ACT
				s->clear();

				// ASSERT
				assert_equal(0u, callees->get_count());
				assert_equal(0u, callers->get_count());
			}


			test( CalleesModelReturnsTextualValuesExpected )
			{
				// INIT
				unsigned columns[] = {
					callee_columns::name, callee_columns::times_called, callee_columns::exclusive, callee_columns::inclusive,
					callee_columns::exclusive_avg, callee_columns::inclusive_avg, callee_columns::max_time,
				};
				call_statistics functions[] = {
					make_call_statistics(1, 101, 0, 0x1001, 1, 0, 0, 0, 0),
					make_call_statistics(2, 101, 1, 0x2001, 100, 0, 109, 17, 19190),
					make_call_statistics(3, 101, 1, 0x2002, 109, 0, 1090, 11711, 0),
					make_call_statistics(4, 101, 0, 0x1002, 1, 0, 0, 0, 0),
					make_call_statistics(5, 101, 4, 0x2001, 9318, 0, 2, 3, 4),
				};
				auto s = make_shared<tables::statistics>();
				auto sel = make_shared_copy(plural + 1u);

				append(*s, functions);

				auto callees = create_callees_model(s, create_context(s, 0.01, resolver, tmodel, false), sel);

				// ACT
				auto text = get_text(*callees, columns);

				// ASSERT
				string reference1[][7] = {
					{	"00002001", "100", "170ms", "1.09s", "1.7ms", "10.9ms", "192s",	},
					{	"00002002", "109", "117s", "10.9s", "1.07s", "100ms", "0s",	},
				};

				assert_equivalent(mkvector(reference1), text);

				// INIT
				*sel = plural + 1u + 4u;
				s->invalidate();

				// ACT
				text = get_text(*callees, columns);

				// ASSERT
				string reference2[][7] = {
					{	"00002001", "100", "170ms", "1.09s", "1.7ms", "10.9ms", "192s",	},
					{	"00002002", "109", "117s", "10.9s", "1.07s", "100ms", "0s",	},
					{	"00002001", "9318", "30ms", "20ms", "3.22\xCE\xBCs", "2.15\xCE\xBCs", "40ms",	},
				};

				assert_equivalent(mkvector(reference2), text);
			}


			test( CallersModelReturnsTextualValuesExpected )
			{
				// INIT
				unsigned columns[] = {	callers_columns::name, callers_columns::times_called,	};
				call_statistics functions[] = {
					make_call_statistics(1, 101, 0, 0x2001, 1, 0, 0, 0, 0),
					make_call_statistics(2, 101, 0, 0x2002, 1, 0, 0, 0, 0),
					make_call_statistics(3, 101, 1, 0x1001, 19190, 3, 1000, 11711, 0),
					make_call_statistics(4, 101, 2, 0x1001, 179, 5, 1390, 1111, 0),

					make_call_statistics(5, 101, 0, 0x2007, 1, 0, 0, 0, 0),
					make_call_statistics(6, 101, 0, 0x2003, 1, 0, 0, 0, 0),
					make_call_statistics(7, 101, 1, 0x1003, 11000, 13, 1000, 11711, 0),
					make_call_statistics(8, 101, 5, 0x1003, 10700, 5, 1390, 1111, 0),
					make_call_statistics(9, 101, 6, 0x1003, 10090, 1115, 31390, 31111, 0),
				};
				auto s = make_shared<tables::statistics>();
				auto sel = make_shared_copy(plural + 3u);

				append(*s, functions);

				auto callers = create_callers_model(s, create_context(s, 0.01, resolver, tmodel, false), sel);

				// ACT
				auto text = get_text(*callers, columns);

				// ASSERT
				string reference1[][2] = {
					{	"00002001", "19190",	},
					{	"00002002", "179",	},
				};

				assert_equivalent(mkvector(reference1), text);

				// INIT
				*sel = plural + 3u + 9u;
				sel->invalidate(123);

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
				call_statistics functions[] = {
					make_call_statistics(1, 101, 0, 5, 2000, 0, 0, 0, 0),
					make_call_statistics(2, 101, 1, 11, 29, 0, 0, 16, 0),
					make_call_statistics(3, 101, 1, 13, 31, 0, 0, 10, 0),
					make_call_statistics(4, 101, 0, 17, 1999, 0, 0, 0, 0),
					make_call_statistics(5, 101, 4, 11, 101, 0, 0, 3, 0),
					make_call_statistics(6, 101, 4, 19, 103, 0, 0, 112, 0),
					make_call_statistics(7, 101, 4, 23, 1100, 0, 0, 9, 0),
				};

				auto s = make_shared<tables::statistics>();
				auto sel = make_shared_copy(plural + 1u);

				append(*s, functions);

				auto ls = create_callees_model(s, create_context(s, 0.01, resolver, tmodel, false), sel);
				auto m = ls->get_column_series();

				// ACT
				ls->set_order(callee_columns::times_called, true);

				// ASSERT
				assert_equal(2u, m->get_count());
				assert_approx_equal(29.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(31.0, get_value(*m, 1), c_tolerance);

				// ACT
				ls->set_order(callee_columns::times_called, false);

				// ASSERT
				assert_equal(2u, m->get_count());
				assert_approx_equal(31.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(29.0, get_value(*m, 1), c_tolerance);

				// ACT
				(*sel)[0] = 4u;
				sel->invalidate(1);

				// ASSERT
				assert_equal(3u, m->get_count());
				assert_approx_equal(1100.0, get_value(*m, 0), c_tolerance);
				assert_approx_equal(103.0, get_value(*m, 1), c_tolerance);
				assert_approx_equal(101.0, get_value(*m, 2), c_tolerance);

				// ACT
				ls->set_order(callee_columns::exclusive, false);

				// ASSERT
				assert_equal(3u, m->get_count());
				assert_approx_equal(1.12, get_value(*m, 0), c_tolerance);
				assert_approx_equal(0.09, get_value(*m, 1), c_tolerance);
				assert_approx_equal(0.03, get_value(*m, 2), c_tolerance);

				// ACT
				sel->push_back(1u);
				sel->invalidate(2);

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
