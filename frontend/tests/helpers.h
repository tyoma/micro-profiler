#pragma once

#include <frontend/primitives.h>
#include <frontend/tables.h>

#include <common/module.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <wpl/models.h>

namespace micro_profiler
{
	class symbol_resolver;
	class threads_model;

	namespace tests
	{
		struct plural_
		{
			template <typename T>
			std::vector<T> operator +(const T &rhs) const
			{	return std::vector<T>(1, rhs);	}
		} const plural;

		template <typename T>
		inline std::vector<T> operator +(std::vector<T> lhs, const T &rhs)
		{	return lhs.push_back(rhs), lhs;	}

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

		inline std::string get_text(const wpl::list_model<std::string> &m, unsigned item)
		{
			std::string text;

			m.get_value(item, text);
			return text;
		}

		inline std::string get_text(const wpl::richtext_table_model &fl, size_t row, size_t column)
		{
			agge::richtext_t text((agge::font_style_annotation()));

			return fl.get_text(static_cast<wpl::table_model_base::index_type>(row),
				static_cast<wpl::table_model_base::index_type>(column), text), text.underlying();
		}

		template <size_t columns_n>
		inline std::vector< std::vector<std::string> > get_text(const wpl::richtext_table_model &model,
			unsigned (&columns_)[columns_n])
		{
			using namespace std;

			vector< vector<string> > values;

			for (size_t i = 0, count = model.get_count(); i != count; ++i)
			{
				values.resize(values.size() + 1);
				for (auto j = begin(columns_); j != end(columns_); ++j)
					values.back().push_back(get_text(model, i, *j));
			}
			return values;
		}

		template <typename T1, size_t columns_n, typename T2, size_t rows_n>
		inline void are_rows_equal(T1 (&ordering)[columns_n], T2 (&expected)[rows_n][columns_n],
			const wpl::richtext_table_model &actual, const ut::LocationInfo &location)
		{
			ut::are_equal(rows_n, actual.get_count(), location);
			for (size_t j = 0; j != rows_n; ++j)
				for (size_t i = 0; i != columns_n; ++i)
					ut::are_equal(expected[j][i], get_text(actual, j, ordering[i]), location);
		}

		template <typename T1, size_t columns_n, typename T2, size_t rows_n>
		inline void are_rows_equivalent(T1 (&ordering)[columns_n], T2 (&expected)[rows_n][columns_n],
			const wpl::richtext_table_model &actual, const ut::LocationInfo &location)
		{
			using namespace std;

			vector< vector<string> > reference_rows(rows_n);
			vector< vector<string> > actual_rows(rows_n);

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

		inline mapped_module_identified create_mapping(unsigned instance_id, unsigned peristent_id, long_address_t base)
		{
			mapped_module_identified mmi = { instance_id, peristent_id, std::string(), base, };
			return mmi;
		}

		template <typename ArchiveT, typename DataT>
		inline void emulate_save(ArchiveT &archive, signed long long ticks_per_second, const symbol_resolver &resolver,
			const DataT &data, const threads_model &threads)
		{
			archive(ticks_per_second);
			archive(resolver);
			archive(data);
			archive(threads);
		}

		template <typename T, size_t size>
		std::vector< std::pair< unsigned /*threadid*/, std::vector<T> > > make_single_threaded(T (&array_ptr)[size],
			unsigned int threadid = 1u)
		{
			return std::vector< std::pair< unsigned /*threadid*/, std::vector<T> > >(1, std::make_pair(threadid,
				mkvector(array_ptr)));
		}

		template <typename T>
		std::vector< std::pair< unsigned /*threadid*/, std::vector<T> > > make_single_threaded(const std::vector<T> &data,
			unsigned int threadid = 1u)
		{
			return std::vector< std::pair< unsigned /*threadid*/, std::vector<T> > >(1, std::make_pair(threadid,
				data));
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

		template <typename T>
		inline T get_value(const wpl::list_model<T> &m, size_t index)
		{
			T value;

			m.get_value(index, value);
			return value;
		}
	}
}

#define assert_table_equal(ordering, expected, actual) are_rows_equal((ordering), (expected), (actual), (ut::LocationInfo(__FILE__, __LINE__)))
#define assert_table_equivalent(ordering, expected, actual) are_rows_equivalent((ordering), (expected), (actual), (ut::LocationInfo(__FILE__, __LINE__)))
