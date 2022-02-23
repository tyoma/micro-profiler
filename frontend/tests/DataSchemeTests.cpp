#include <frontend/db.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct fake_call
			{
				id_t id, parent_id;
				long_address_t address;
			};

			template <typename T, typename ConstructorT>
			void add(views::table<T, ConstructorT> &table, const T &item)
			{
				auto r = table.create();

				*r = item;
				r.commit();
			}
		}

		begin_test_suite( StackIDBuilderTests )
			test( StackIDsAreBuiltForTopLevelCalls )
			{
				// INIT
				fake_call data[] = {
					{	0, 0, 0x1010000010	},
					{	0, 0, 0x7010000090	},
					{	0, 0, 0xF960000010	},
				};
				views::table<fake_call> tbl;
				const keyer::callstack< views::table<fake_call> > keyer(tbl);

				// ACT
				callstack_key key = keyer(data[0]);

				// ASSERT
				assert_equal(plural + (long_address_t)0x1010000010, key);

				// ACT
				key = keyer(data[1]);

				// ASSERT
				assert_equal(plural + (long_address_t)0x7010000090, key);

				// ACT
				key = keyer(data[2]);

				// ASSERT
				assert_equal(plural + (long_address_t)0xF960000010, key);
			}


			test( StackIDsAreBuiltForNestedCalls )
			{
				// INIT
				fake_call q[] = {
					{	0, 11, 0x1010000010	},
					{	0, 31, 0x7010000090	},
					{	0, 97, 0xF960000010	},
				};
				fake_call data_[] = {
					{	11, 0, 29	},
					{	23, 11, 31	},
					{	31, 11, 37	},
					{	97, 31, 41	},
				};
				views::table<fake_call> tbl;
				auto keyer = keyer::callstack< views::table<fake_call> >(tbl);

				for (auto i = begin(data_); i != end(data_); ++i)
					add(tbl, *i);

				// ACT
				auto key = keyer(q[0]);

				// ASSERT
				assert_equal(plural + (long_address_t)29 + (long_address_t)0x1010000010, key);

				// ACT
				key = keyer(q[1]);

				// ASSERT
				assert_equal(plural + (long_address_t)29 + (long_address_t)37 + (long_address_t)0x7010000090, key);

				// ACT
				key = keyer(q[2]);

				// ASSERT
				assert_equal(plural + (long_address_t)29 + (long_address_t)37 + (long_address_t)41 + (long_address_t)0xF960000010, key);
			}


			test( CallstackIndexIsOperable )
			{
				// INIT
				call_statistics data_[] = {
					make_call_statistics(11, 0, 0, 29, 0, 0, 0, 0, 0),
					make_call_statistics(23, 0, 11, 31, 0, 0, 0, 0, 0),
					make_call_statistics(12, 0, 0, 291, 0, 0, 0, 0, 0),
					make_call_statistics(31, 0, 11, 37, 0, 0, 0, 0, 0),
					make_call_statistics(97, 0, 31, 41, 0, 0, 0, 0, 0),
					make_call_statistics(99, 0, 11, 37, 0, 0, 0, 0, 0),
				};
				calls_statistics_table tbl;

				for (auto i = begin(data_); i != end(data_); ++i)
					add(tbl, *i);

				// INIT / ACT
				auto &by_callstack = multi_index(tbl, keyer::callstack<calls_statistics_table>(tbl));

				// ACT
				auto r = by_callstack.equal_range(plural + (long_address_t)29);

				// ASSERT
				assert_equal(1, distance(r.first, r.second));
				assert_equal(data_[0], *r.first);

				// ACT
				r = by_callstack.equal_range(plural + (long_address_t)29 + (long_address_t)31);

				// ASSERT
				assert_equal(1, distance(r.first, r.second));
				assert_equal(data_[1], *r.first);

				// ACT
				r = by_callstack.equal_range(plural + (long_address_t)291);

				// ASSERT
				assert_equal(1, distance(r.first, r.second));
				assert_equal(data_[2], *r.first);

				// ACT
				r = by_callstack.equal_range(plural + (long_address_t)29 + (long_address_t)37 + (long_address_t)41);

				// ASSERT
				assert_equal(1, distance(r.first, r.second));
				assert_equal(data_[4], *r.first);

				// ACT
				r = by_callstack.equal_range(plural + (long_address_t)29 + (long_address_t)37);

				// ASSERT
				assert_equal(2, distance(r.first, r.second));
			}

		end_test_suite


		begin_test_suite( AggregatedRepresentationTests )

			struct aggregator
			{
				template <typename I>
				void operator ()(function_statistics &record, I begin, I end) const
				{
					record = function_statistics();
					for (auto i = begin; i != end; ++i)
						record += *i;
				}
			};


			template <typename TableT>
			keyer::callstack<TableT> operator ()(const TableT &table_) const
			{	return keyer::callstack<TableT>(table_);	}


			test( ThreadAggregationWorksForPlainCalls )
			{
				typedef views::immutable_unique_index<aggregated_statistics_table, keyer::id> aggregated_primary_id_index;

				// INIT
				call_statistics data_[] = {
					make_call_statistics(1, 0, 0, 29, 10, 0, 901, 0, 0),
					make_call_statistics(2, 0, 0, 31, 11, 4, 911, 0, 0),
					make_call_statistics(3, 0, 0, 39, 15, 0, 931, 0, 0),

					make_call_statistics(4, 1, 0, 29, 03, 0, 763, 0, 0),
					make_call_statistics(5, 1, 0, 31, 07, 3, 765, 0, 0),
					make_call_statistics(6, 1, 0, 37, 90, 0, 769, 0, 0),
				};
				calls_statistics_table tbl;
				aggregated_statistics_table aggregated(tbl);

				aggregated.group_by(*this, aggregator());

				// ACT
				for (auto i = begin(data_); i != end(data_); ++i)
					add(tbl, *i);

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(1, 0, 0, 29, 13, 0, 1664, 0, 0),
					make_call_statistics(2, 0, 0, 31, 18, 4, 1676, 0, 0),
					make_call_statistics(3, 0, 0, 39, 15, 0, 931, 0, 0),
					make_call_statistics(4, 0, 0, 37, 90, 0, 769, 0, 0),
				};

				assert_equivalent(reference, aggregated);

				// ACT
				add(tbl, make_call_statistics(16, 70, 0, 37, 10, 2, 100, 3, 0));

				// ASSERT
				call_statistics reference2[] = {
					make_call_statistics(1, 0, 0, 29, 13, 0, 1664, 0, 0),
					make_call_statistics(2, 0, 0, 31, 18, 4, 1676, 0, 0),
					make_call_statistics(3, 0, 0, 39, 15, 0, 931, 0, 0),
					make_call_statistics(4, 0, 0, 37, 100, 2, 869, 3, 0),
				};

				assert_equivalent(reference2, aggregated);
			}


			test( ThreadAggregationWorksForNestedCalls )
			{
				typedef views::immutable_unique_index<aggregated_statistics_table, keyer::id> aggregated_primary_id_index;

				// INIT
				call_statistics data_[] = {
					make_call_statistics(10, 7, 00, 29, 10, 0, 901, 0, 0),
					make_call_statistics(20, 7, 10, 31, 11, 4, 911, 0, 0),
					make_call_statistics(30, 7, 00, 39, 15, 0, 931, 0, 0),

					make_call_statistics(40, 7, 10, 29, 03, 0, 763, 0, 0),
					make_call_statistics(50, 7, 20, 31, 07, 3, 765, 0, 0),
					make_call_statistics(60, 7, 30, 37, 90, 0, 769, 0, 0),

					make_call_statistics(91, 1, 00, 29, 100, 0, 1000, 0, 0),
					make_call_statistics(92, 1, 91, 31, 200, 0, 3000, 0, 0),
					make_call_statistics(95, 1, 92, 31, 300, 0, 7000, 0, 0),

				};
				calls_statistics_table tbl;
				aggregated_statistics_table aggregated(tbl);

				aggregated.group_by(*this, aggregator());

				// ACT
				for (auto i = begin(data_); i != end(data_); ++i)
					add(tbl, *i);

				// ASSERT
				call_statistics reference1[] = {
					make_call_statistics(1, 0, 0, 29, 110, 0, 1901, 0, 0),
					make_call_statistics(2, 0, 1, 31, 211, 4, 3911, 0, 0),
					make_call_statistics(3, 0, 0, 39, 15, 0, 931, 0, 0),
					make_call_statistics(4, 0, 1, 29, 03, 0, 763, 0, 0),
					make_call_statistics(5, 0, 2, 31, 307, 3, 7765, 0, 0),
					make_call_statistics(6, 0, 3, 37, 90, 0, 769, 0, 0),
				};

				assert_equivalent(reference1, aggregated);
			}
		end_test_suite
	}
}
