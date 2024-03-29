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
		begin_test_suite( DistinctAddressTableTests )
			test( UnderlyingModelsAreHeldByTheAddressesModel )
			{
				// INIT
				auto sel = make_shared<selector_table>();
				auto hierarchy = make_shared<calls_statistics_table>();
				weak_ptr<selector_table> wsel = sel;
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
				const auto sel = make_shared<selector_table>();
				const auto hierarchy = make_shared<calls_statistics_table>();
				const auto addresses = derived_statistics::addresses(sel, hierarchy);

				// ACT
				add_records(*hierarchy, plural
					+ make_call_statistics(1, 0, 0, 123, 0, 0, 0, 0, 0)
					+ make_call_statistics(2, 0, 1, 501, 0, 0, 0, 0, 0)
					+ make_call_statistics(3, 0, 1, 502, 0, 0, 0, 0, 0)
					+ make_call_statistics(4, 0, 0, 124, 0, 0, 0, 0, 0)
					+ make_call_statistics(5, 0, 4, 503, 0, 0, 0, 0, 0));
				add_records(*sel, plural + 2u);

				// ASSERT
				assert_equivalent(plural + 501u, *addresses);

				// ACT
				add_records(*sel, plural + 5u + 4u);

				// ASSERT
				assert_equivalent(plural + 501u + 503u + 124u, *addresses);
			}


			test( IDsAreTranslatedToUniqueAddresses )
			{
				// INIT
				const auto sel = make_shared<selector_table>();
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
				add_records(*sel, plural + 2u + 3u);

				// ASSERT
				assert_equivalent(plural + 501u, *addresses);

				// ACT
				add_records(*sel, plural + 4u);

				// ASSERT
				assert_equivalent(plural + 501u + 123u, *addresses);

				// ACT
				add_records(*sel, plural + 1u + 6u + 8u);

				// ASSERT
				assert_equivalent(plural + 501u + 123u + 503u, *addresses);
			}


			test( SelectionStartsHoldingHierarchy )
			{
				// INIT
				auto sel = make_shared<selector_table>();
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


			test( CallersBorrowThreadIdAndAreAwareOfRecursion )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				auto callers = derived_statistics::callers(selector, hierarchy);

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 9, 0, 123, 101, 0, 11, 13, 0)
					+ make_call_statistics(2, 9, 1, 501, 112, 0, 17, 19, 0)
					+ make_call_statistics(3, 9, 2, 501, 173, 0, 23, 29, 0)

					+ make_call_statistics(4, 9, 0, 501, 104, 0, 31, 37, 0)
					+ make_call_statistics(5, 9, 4, 123, 105, 0, 41, 43, 0)
					+ make_call_statistics(6, 9, 5, 501, 106, 0, 47, 53, 0)
					+ make_call_statistics(7, 9, 6, 123, 107, 0, 59, 67, 0)

					+ make_call_statistics(8, 7, 0, 123, 207, 0, 71, 77, 0)
					+ make_call_statistics(9, 7, 8, 123, 907, 0, 17, 97, 0)
					+ make_call_statistics(10, 7, 9, 123, 003, 0, 1, 9, 0));

				// ACT
				add_records(*selector, plural + 501);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 9, 0, 123, 218, 0, 17, 72, 0)
					+ make_call_statistics(0, 9, 0, 501, 173, 0, 0, 29, 0)
					+ make_call_statistics(0, 9, 0, 0, 104, 0, 31, 37, 0), *callers);

				// ACT
				add_records(*selector, plural + 123);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 9, 0, 123, 218, 0, 17, 72, 0)
					+ make_call_statistics(0, 9, 0, 501, 385, 0, 41, 139, 0)
					+ make_call_statistics(0, 9, 0, 0, 205, 0, 42, 50, 0)
					+ make_call_statistics(0, 7, 0, 0, 207, 0, 71, 77, 0)
					+ make_call_statistics(0, 7, 0, 123, 910, 0, 0, 106, 0)
					, *callers);
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


			test( CalleesBorrowThreadIdAndAreAwareOfRecursion )
			{
				// INIT
				auto hierarchy = make_shared<calls_statistics_table>();
				auto selector = make_shared<address_table>();
				auto callees = derived_statistics::callees(selector, hierarchy);

				add_records(*hierarchy, plural
					+ make_call_statistics(1, 9, 0, 123, 101, 0, 11, 13, 0)
					+ make_call_statistics(2, 9, 1, 501, 112, 0, 17, 19, 0)
					+ make_call_statistics(3, 9, 2, 501, 173, 0, 23, 29, 0)

					+ make_call_statistics(4, 9, 0, 501, 104, 0, 31, 37, 0)
					+ make_call_statistics(5, 9, 4, 123, 105, 0, 41, 43, 0)
					+ make_call_statistics(6, 9, 5, 501, 106, 0, 47, 53, 0)
					+ make_call_statistics(7, 9, 6, 123, 107, 0, 59, 67, 0)

					+ make_call_statistics(8, 7, 0, 123, 207, 0, 71, 77, 0)
					+ make_call_statistics(9, 7, 8, 123, 907, 0, 17, 97, 0)
					+ make_call_statistics(10, 7, 9, 123, 003, 0, 1, 9, 0));

				// ACT
				add_records(*selector, plural + 501);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 9, 0, 123, 212, 0, 41, 110, 0)
					+ make_call_statistics(0, 9, 0, 501, 173, 0, 0, 29, 0), *callees);

				// ACT
				add_records(*selector, plural + 123);

				// ASSERT
				assert_equivalent(plural
					+ make_call_statistics(0, 9, 0, 123, 212, 0, 41, 110, 0)
					+ make_call_statistics(0, 9, 0, 501, 391, 0, 17, 101, 0)
					+ make_call_statistics(0, 7, 0, 123, 910, 0, 0, 106, 0), *callees);
			}

		end_test_suite
	}
}
