#include <frontend/derived_statistics.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <frontend/selection_model.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct wrapped_selection : selection<id_t>
			{
				virtual key_type get_key(size_t /*item*/) const override
				{	throw 0;	}
			};
		}

		begin_test_suite( DistinctAddressTableTests )
			test( UnderlyingModelsAreHeldByTheAddressesModel )
			{
				// INIT
				auto sel = make_shared<wrapped_selection>();
				auto hierarchy = make_shared<calls_statistics_table>();
				weak_ptr< selection<id_t> > wsel = sel;
				weak_ptr<calls_statistics_table> whierarchy = hierarchy;

				// INIT / ACT
				address_table_cptr addresses = derived_statistics::addresses(sel, hierarchy);
				sel.reset();
				hierarchy.reset();

				// ASSERT
				assert_not_null(addresses);
				assert_is_false(whierarchy.expired());
				assert_is_false(wsel.expired());

				// ACT
				addresses.reset();

				// ASSERT
				assert_is_true(whierarchy.expired());
				assert_is_true(wsel.expired());
			}


			test( IDsAreTranslatedToAddressesAccordinglyToHierarchy )
			{
				// INIT
				const auto sel = make_shared<wrapped_selection>();
				const auto hierarchy = make_shared<calls_statistics_table>();
				const auto addresses = derived_statistics::addresses(sel, hierarchy);

				// ACT
				add_records(*hierarchy, plural
					+ make_call_statistics(1, 0, 0, 123, 0, 0, 0, 0, 0)
					+ make_call_statistics(2, 0, 1, 501, 0, 0, 0, 0, 0)
					+ make_call_statistics(3, 0, 1, 502, 0, 0, 0, 0, 0)
					+ make_call_statistics(4, 0, 0, 124, 0, 0, 0, 0, 0)
					+ make_call_statistics(5, 0, 4, 503, 0, 0, 0, 0, 0));
				sel->add_key(2);

				// ASSERT
				assert_equivalent(plural + 501u, *addresses);

				// ACT
				sel->add_key(5);
				sel->add_key(4);

				// ASSERT
				assert_equivalent(plural + 501u + 503u + 124u, *addresses);
			}


			test( IDsAreTranslatedToUniqueAddresses )
			{
				// INIT
				const auto sel = make_shared<wrapped_selection>();
				const auto hierarchy = make_shared<calls_statistics_table>();
				const auto addresses = derived_statistics::addresses(sel, hierarchy);

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 0, 0, 123, 0, 0, 0, 0, 0)
					+ make_call_statistics(2, 0, 1, 501, 0, 0, 0, 0, 0)
					+ make_call_statistics(3, 0, 1, 501, 0, 0, 0, 0, 0)
					+ make_call_statistics(4, 0, 1, 123, 0, 0, 0, 0, 0)
					+ make_call_statistics(5, 0, 1, 502, 0, 0, 0, 0, 0)
					+ make_call_statistics(6, 0, 0, 503, 0, 0, 0, 0, 0)
					+ make_call_statistics(7, 0, 0, 124, 0, 0, 0, 0, 0)
					+ make_call_statistics(8, 0, 4, 503, 0, 0, 0, 0, 0));

				// ACT
				sel->add_key(2);
				sel->add_key(3);

				// ASSERT
				assert_equivalent(plural + 501u, *addresses);

				// ACT
				sel->add_key(4);

				// ASSERT
				assert_equivalent(plural + 501u + 123u, *addresses);

				// ACT
				sel->add_key(1);
				sel->add_key(6);
				sel->add_key(8);

				// ASSERT
				assert_equivalent(plural + 501u + 123u + 503u, *addresses);
			}


			test( SelectionStartsHoldingHierarchy )
			{
				// INIT
				auto sel = make_shared<wrapped_selection>();
				auto hierarchy = make_shared<calls_statistics_table>();
				auto addresses = derived_statistics::addresses(sel, hierarchy);
				weak_ptr<calls_statistics_table> whierarchy = hierarchy;

				// ACT
				addresses.reset();
				hierarchy.reset();

				// ASSERT
				assert_is_false(whierarchy.expired());

				// ACT
				sel.reset();

				// ASSERT
				assert_is_true(whierarchy.expired());
			}
		end_test_suite


		begin_test_suite( CallersStatisticTests )
			test( UnderlyingModelsAreHeldByTheCallersModel )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				weak_ptr<calls_statistics_table> whierarchy = hierarchy;
				weak_ptr<address_table> wselector = selector;

				// INIT / ACT
				auto callers = derived_statistics::callers(selector, hierarchy);
				hierarchy.reset();
				selector.reset();

				// ASSERT
				assert_not_null(callers);
				assert_is_false(whierarchy.expired());
				assert_is_false(wselector.expired());

				// ACT
				callers.reset();

				// ASSERT
				assert_is_true(whierarchy.expired());
				assert_is_true(wselector.expired());
			}


			test( CallersAreEmptyWhenSelectorIsEmpty )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 1, 0, 123, 0, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 501, 0, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 502, 0, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 124, 0, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 4, 501, 0, 0, 0, 0, 0));

				// INIT / ACT
				auto callers = derived_statistics::callers(selector, hierarchy);

				// ASSERT
				assert_equal(callers->end(), callers->begin());
			}


			test( SelectedUniqueCallersGetToTheResultTableNoAggregation )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 1, 0, 123, 101, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 501, 102, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 502, 103, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 124, 104, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 4, 123, 105, 0, 0, 0, 0)
					+ make_call_statistics(6, 1, 4, 501, 106, 0, 0, 0, 0)
					+ make_call_statistics(7, 1, 6, 503, 107, 0, 0, 0, 0));
				add_records(*selector, plural + 501);

				// INIT / ACT
				auto callers = derived_statistics::callers(selector, hierarchy);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 123, 102, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 124, 106, 0, 0, 0, 0), *callers);

				// ACT
				add_records(*selector, plural + 503);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 123, 102, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 124, 106, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 501, 107, 0, 0, 0, 0), *callers);
			}


			test( SelectedCallersGetToTheResultTableWithAggregation )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				auto callers = derived_statistics::callers(selector, hierarchy);

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 1, 0, 123, 101, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 501, 112, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 502, 173, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 124, 104, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 4, 123, 105, 0, 0, 0, 0)
					+ make_call_statistics(6, 1, 4, 501, 106, 0, 0, 0, 0)
					+ make_call_statistics(7, 1, 6, 503, 107, 0, 0, 0, 0));

				// ACT
				add_records(*selector, plural + 501 + 502);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 123, 285, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 124, 106, 0, 0, 0, 0), *callers);

				// ACT
				add_records(*selector, plural + 123);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0,   0, 101, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 123, 285, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 124, 211, 0, 0, 0, 0), *callers);
			}


			test( CallersAreSegregatedByThreadID )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				auto callers = derived_statistics::callers(selector, hierarchy);

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 7, 0, 123, 101, 0, 0, 0, 0)
					+ make_call_statistics(2, 7, 1, 501, 112, 0, 0, 0, 0)
					+ make_call_statistics(3, 7, 2, 503, 113, 0, 0, 0, 0)
					+ make_call_statistics(4, 7, 3, 501, 119, 0, 0, 0, 0)
					+ make_call_statistics(5, 7, 2, 505, 114, 0, 0, 0, 0)
					+ make_call_statistics(6, 7, 1, 502, 173, 0, 0, 0, 0)
					+ make_call_statistics(7, 3, 0, 123, 105, 0, 0, 0, 0)
					+ make_call_statistics(8, 3, 7, 501, 106, 0, 0, 0, 0)
					+ make_call_statistics(9, 3, 8, 503, 107, 0, 0, 0, 0));

				// ACT
				add_records(*selector, plural + 501 + 503);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 7, 0, 123, 112, 0, 0, 0, 0) 
					+ make_call_statistics(0, 7, 0, 503, 119, 0, 0, 0, 0)
					+ make_call_statistics(0, 3, 0, 123, 106, 0, 0, 0, 0)
					+ make_call_statistics(0, 7, 0, 501, 113, 0, 0, 0, 0)
					+ make_call_statistics(0, 3, 0, 501, 107, 0, 0, 0, 0), *callers);
			}
		end_test_suite


		begin_test_suite( CalleesStatisticTests )
			test( UnderlyingModelsAreHeldByTheCalleesModel )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				weak_ptr<calls_statistics_table> whierarchy = hierarchy;
				weak_ptr<address_table> wselector = selector;

				// INIT / ACT
				auto callers = derived_statistics::callees(selector, hierarchy);
				hierarchy.reset();
				selector.reset();

				// ASSERT
				assert_not_null(callers);
				assert_is_false(whierarchy.expired());
				assert_is_false(wselector.expired());

				// ACT
				callers.reset();

				// ASSERT
				assert_is_true(whierarchy.expired());
				assert_is_true(wselector.expired());
			}


			test( CalleesAreEmptyWhenSelectorIsEmpty )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 1, 0, 123, 0, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 501, 0, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 502, 0, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 124, 0, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 4, 501, 0, 0, 0, 0, 0));

				// INIT / ACT
				auto callees = derived_statistics::callees(selector, hierarchy);

				// ASSERT
				assert_equal(callees->end(), callees->begin());
			}


			test( SelectedUniqueCalleesGetToTheResultTableNoAggregation )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 1, 0, 123, 101, 0, 0, 0, 0)
					+ make_call_statistics(2, 1, 1, 501, 102, 0, 0, 0, 0)
					+ make_call_statistics(3, 1, 1, 502, 103, 0, 0, 0, 0)
					+ make_call_statistics(4, 1, 0, 124, 104, 0, 0, 0, 0)
					+ make_call_statistics(5, 1, 4, 123, 105, 0, 0, 0, 0)
					+ make_call_statistics(6, 1, 4, 501, 106, 0, 0, 0, 0)
					+ make_call_statistics(7, 1, 6, 503, 107, 0, 0, 0, 0));
				add_records(*selector, plural + 123);

				// INIT / ACT
				auto callees = derived_statistics::callees(selector, hierarchy);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 501, 102, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 502, 103, 0, 0, 0, 0), *callees);

				// ACT
				add_records(*selector, plural + 501);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 501, 102, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 502, 103, 0, 0, 0, 0)
					+ make_call_statistics(0, 1, 0, 503, 107, 0, 0, 0, 0), *callees);
			}


			test( SelectedCalleesGetToTheResultTableWithAggregation )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				auto callees = derived_statistics::callees(selector, hierarchy);

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 9, 0, 123, 101, 0, 0, 0, 0)
					+ make_call_statistics(2, 9, 1, 501, 112, 0, 0, 0, 0)
					+ make_call_statistics(3, 9, 1, 502, 173, 0, 0, 0, 0)
					+ make_call_statistics(4, 9, 0, 124, 104, 0, 0, 0, 0)
					+ make_call_statistics(5, 9, 4, 123, 105, 0, 0, 0, 0)
					+ make_call_statistics(6, 9, 4, 501, 106, 0, 0, 0, 0)
					+ make_call_statistics(7, 9, 6, 503, 107, 0, 0, 0, 0));

				// ACT
				add_records(*selector, plural + 123 + 124);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 9, 0, 501, 218, 0, 0, 0, 0)
					+ make_call_statistics(0, 9, 0, 502, 173, 0, 0, 0, 0)
					+ make_call_statistics(0, 9, 0, 123, 105, 0, 0, 0, 0), *callees);
			}

		end_test_suite
	}
}
