#include <frontend/columns_layout.h>

#include "helpers.h"
#include "primitive_helpers.h"

#include <frontend/model_context.h>
#include <ut/assert.h>
#include <ut/test.h>

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ColumnsDefinitionTests )
			statistics_model_context ctx;

			template <typename FieldT, typename T, size_t n>
			void verify_comparisons(const FieldT &field, T (&data)[n], const ut::LocationInfo &li)
			{
				for (auto left = 0u; left != n; ++left)
				{
					for (auto right = 0u; right != n; ++right)
					{
						auto r = field.compare(ctx, data[left], data[right]);

						if (left < right)
							ut::is_true(r < 0, li);
						else if (left > right)
							ut::is_true(r > 0, li);
						else
							ut::are_equal(0, r, li);
					}
				}
			}

#define assert_comparison_valid(f, d) verify_comparisons((f), (d), ut::LocationInfo(__FILE__, __LINE__))

			test( TimesCalledAreComparedAsExpected )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(0, 0, 0, 0, 10, 0, 0),
					make_call_statistics(0, 0, 0, 0, 11, 0, 0),
					make_call_statistics(0, 0, 0, 0, 10000000000, 0, 0),
					make_call_statistics(0, 0, 0, 0, 10000000001, 0, 0),
					make_call_statistics(0, 0, 0, 0, 0xF000000000000000, 0, 0),
				};

				// ACT / ASSERT
				assert_comparison_valid(c_statistics_columns[main_columns::times_called], data);
			}


			test( InclusiveTimesAreComparedAsExpected )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(0, 0, 0, 0, 0, -0x7F00000000000000, 0),
					make_call_statistics(0, 0, 0, 0, 0, -10, 0),
					make_call_statistics(0, 0, 0, 0, 0, 11, 0),
					make_call_statistics(0, 0, 0, 0, 0, 10000000000, 0),
					make_call_statistics(0, 0, 0, 0, 0, 10000000001, 0),
					make_call_statistics(0, 0, 0, 0, 0, 0x7F00000000000000, 0),
				};

				// ACT / ASSERT
				assert_comparison_valid(c_statistics_columns[main_columns::inclusive], data);
			}


			test( ExclusiveTimesAreComparedAsExpected )
			{
				// INIT
				call_statistics data[] = {
					make_call_statistics(0, 0, 0, 0, 0, 0, -0x7F00000000000000),
					make_call_statistics(0, 0, 0, 0, 0, 0, -10),
					make_call_statistics(0, 0, 0, 0, 0, 0, 11),
					make_call_statistics(0, 0, 0, 0, 0, 0, 10000000000),
					make_call_statistics(0, 0, 0, 0, 0, 0, 10000000001),
					make_call_statistics(0, 0, 0, 0, 0, 0, 0x7F00000000000000),
				};

				// ACT / ASSERT
				assert_comparison_valid(c_statistics_columns[main_columns::exclusive], data);
			}

		end_test_suite
	}
}
