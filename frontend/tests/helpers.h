#pragma once

#include <frontend/primitives.h>

#include <common/module.h>
#include <ut/assert.h>
#include <wpl/ui/models.h>

namespace micro_profiler
{
	namespace tests
	{
		struct columns
		{
			enum main
			{
				order = 0,
				name = 1,
				times_called = 2,
				exclusive = 3,
				inclusive = 4,
				exclusive_avg = 5,
				inclusive_avg = 6,
				max_reentrance = 7,
				max_time = 8,
			};
		};


		inline function_key addr(size_t address, unsigned int threadid = 1)
		{	return function_key(address, threadid);	}

		inline std::wstring get_text(const wpl::ui::table_model &fl, unsigned row, unsigned column)
		{
			std::wstring text;

			return fl.get_text(row, column, text), text;
		}

		template <typename T1, size_t columns_n, typename T2, size_t rows_n>
		inline void are_rows_equal(T1 (&ordering)[columns_n], T2 (&expected)[rows_n][columns_n],
			const wpl::ui::table_model &actual, const ut::LocationInfo &location)
		{
			ut::are_equal(rows_n, actual.get_count(), location);
			for (size_t j = 0; j != rows_n; ++j)
				for (size_t i = 0; i != columns_n; ++i)
					ut::are_equal(expected[j][i], get_text(actual, j, ordering[i]), location);
		}

		template <typename T1, size_t columns_n, typename T2, size_t rows_n>
		inline void are_rows_equivalent(T1 (&ordering)[columns_n], T2 (&expected)[rows_n][columns_n],
			const wpl::ui::table_model &actual, const ut::LocationInfo &location)
		{
			using namespace std;

			vector< vector<wstring> > reference_rows(rows_n);
			vector< vector<wstring> > actual_rows(rows_n);

			ut::are_equal(rows_n, actual.get_count(), location);
			for (size_t j = 0; j != rows_n; ++j)
			{
				for (size_t i = 0; i != columns_n; ++i)
				{
					reference_rows[j].push_back(expected[j][i]);
					actual_rows[j].push_back(get_text(actual, j, ordering[i]));
				}
			}
			ut::are_equivalent(reference_rows, actual_rows, location);
		}

		inline mapped_module_identified create_mapping(unsigned peristent_id, long_address_t base)
		{
			mapped_module_identified mmi = { 0u, peristent_id, std::string(), base, };
			return mmi;
		}
	}
}

#define assert_table_equal(ordering, expected, actual) are_rows_equal((ordering), (expected), (actual), (ut::LocationInfo(__FILE__, __LINE__)))
#define assert_table_equivalent(ordering, expected, actual) are_rows_equivalent((ordering), (expected), (actual), (ut::LocationInfo(__FILE__, __LINE__)))
