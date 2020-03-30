#pragma once

#include <frontend/primitives.h>

#include <common/module.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <wpl/ui/models.h>

namespace micro_profiler
{
	class symbol_resolver;

	namespace tests
	{
		struct columns
		{
			enum main
			{
				order = 0,
				name = 1,
				threadid = 2,
				times_called = 3,
				exclusive = 4,
				inclusive = 5,
				exclusive_avg = 6,
				inclusive_avg = 7,
				max_reentrance = 8,
				max_time = 9,
			};
		};


		inline function_key addr(size_t address, unsigned int threadid = 1)
		{	return function_key(address, threadid);	}

		inline std::wstring get_text(const wpl::ui::table_model &fl, size_t row, unsigned column)
		{
			std::wstring text;

			return fl.get_text(static_cast<unsigned>(row), column, text), text;
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
			for (unsigned int j = 0; j != rows_n; ++j)
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

		template <typename ArchiveT, typename DataT>
		inline void emulate_save(ArchiveT &archive, signed long long ticks_per_second, const symbol_resolver &resolver,
			const DataT &data)
		{
			archive(ticks_per_second);
			archive(resolver);
			archive(data);
		}

		template <typename T, size_t size>
		std::vector< std::pair< unsigned /*threadid*/, std::vector<T> > > make_single_threaded(T (&array_ptr)[size],
			unsigned int threadid = 1u)
		{
			return std::vector< std::pair< unsigned /*threadid*/, std::vector<T> > >(1, std::make_pair(threadid,
				mkvector(array_ptr)));
		}

		template <typename ArchiveT, typename ContainerT>
		inline void serialize_single_threaded(ArchiveT &archive, const ContainerT &container, unsigned int threadid = 1u)
		{
			std::vector< std::pair< unsigned /*threadid*/, ContainerT > > data(1, std::make_pair(threadid, container));

			archive(data);
		}

		inline thread_info make_thread_info(unsigned native_id, std::string description, mt::milliseconds start_time,
			mt::milliseconds end_time, mt::milliseconds cpu_time, bool complete)
		{
			thread_info ti = { native_id, description, start_time, end_time, cpu_time, complete };
			return ti;
		}
	}
}

#define assert_table_equal(ordering, expected, actual) are_rows_equal((ordering), (expected), (actual), (ut::LocationInfo(__FILE__, __LINE__)))
#define assert_table_equivalent(ordering, expected, actual) are_rows_equivalent((ordering), (expected), (actual), (ut::LocationInfo(__FILE__, __LINE__)))
