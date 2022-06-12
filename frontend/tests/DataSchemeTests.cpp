#include <frontend/db.h>
#include <frontend/helpers.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <ut/assert.h>
#include <ut/test.h>
#include <views/transforms.h>

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
						add(record, *i);
				}
			};


			template <typename TableT>
			keyer::callstack<TableT> operator ()(const TableT &table_, views::agnostic_key_tag) const
			{	return keyer::callstack<TableT>(table_);	}


			test( ThreadAggregationWorksForPlainCalls )
			{
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
				auto aggregated = group_by(tbl, *this, aggregator());

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

				assert_equivalent(reference, *aggregated);

				// ACT
				add(tbl, make_call_statistics(16, 70, 0, 37, 10, 2, 100, 3, 0));

				// ASSERT
				call_statistics reference2[] = {
					make_call_statistics(1, 0, 0, 29, 13, 0, 1664, 0, 0),
					make_call_statistics(2, 0, 0, 31, 18, 4, 1676, 0, 0),
					make_call_statistics(3, 0, 0, 39, 15, 0, 931, 0, 0),
					make_call_statistics(4, 0, 0, 37, 100, 2, 869, 3, 0),
				};

				assert_equivalent(reference2, *aggregated);
			}


			test( ThreadAggregationWorksForNestedCalls )
			{
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
				auto aggregated = group_by(tbl, *this, aggregator());

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

				assert_equivalent(reference1, *aggregated);
			}
		end_test_suite


		begin_test_suite( DataTypesTests )
			test( CallStatisticsPathIsCalculatedUponRetrieval )
			{
				// INIT
				const call_statistics data[] = {
					make_call_statistics(1, 0, 0, 0, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 0, 0, 0, 0, 0, 0),
					make_call_statistics(3, 0, 1, 0, 0, 0, 0, 0, 0),
					make_call_statistics(4, 0, 2, 0, 0, 0, 0, 0, 0),
					make_call_statistics(5, 0, 4, 0, 0, 0, 0, 0, 0),
					make_call_statistics(6, 0, 3, 0, 0, 0, 0, 0, 0),
					make_call_statistics(7, 0, 5, 0, 0, 0, 0, 0, 0),
				};
				auto lookup = [&] (id_t id) -> const call_statistics * {
					assert_is_true(0 <= id && id < 6); // allowed range of parents
					return id ? &(data[id - 1]) : nullptr;
				};

				// ACT / ASSERT
				assert_equal(plural + 1u, data[0].path(lookup));
				assert_equal(plural + 1u + 2u, data[1].path(lookup));
				assert_equal(plural + 1u + 3u, data[2].path(lookup));
				assert_equal(plural + 1u + 2u + 4u, data[3].path(lookup));
				assert_equal(plural + 1u + 2u + 4u + 5u, data[4].path(lookup));
				assert_equal(plural + 1u + 3u + 6u, data[5].path(lookup));
				assert_equal(plural + 1u + 2u + 4u + 5u + 7u, data[6].path(lookup));
			}


			test( PathIsCachedAndNoLookupIsDone )
			{
				// INIT
				const call_statistics data[] = {
					make_call_statistics(1, 0, 0, 0, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 0, 0, 0, 0, 0, 0),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};
				auto lookup_fail = [] (id_t) -> const call_statistics * {
					assert_is_false(true);
					return nullptr;
				};

				data[0].path(lookup);
				data[1].path(lookup);

				// ACT / ASSERT
				assert_equal(1u, data[0].path(lookup_fail).size());
				assert_equal(2u, data[1].path(lookup_fail).size());

				// INIT / ACT (reference is returned)
				const auto &p1 = data[0].path(lookup);
				const auto &p2 = data[0].path(lookup);

				// ASSERT
				assert_equal(&p1, &p2);
			}


			test( AttemptToGetPathForAMissingParentAbridgesPath )
			{
				// INIT
				const call_statistics data[] = {
					make_call_statistics(1, 0, 10, 0, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 0, 0, 0, 0, 0, 0),
					make_call_statistics(3, 0, 2, 0, 0, 0, 0, 0, 0),
				};
				auto lookup = [&] (id_t id) -> const call_statistics * {
					return 1u <= id && id < 3u ? &(data[id - 1]) : nullptr;
				};

				// ACT / ASSERT
				assert_equal(plural + 1u, data[0].path(lookup));
				assert_equal(plural + 1u + 2u, data[1].path(lookup));
				assert_equal(plural + 1u + 2u + 3u, data[2].path(lookup));
			}


			test( NonRecurrentCallsHaveZeroReentranceLevel )
			{
				// INIT
				const call_statistics data[] = {
					make_call_statistics(1, 0, 0, 101, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 102, 0, 0, 0, 0, 0),
					make_call_statistics(3, 0, 1, 103, 0, 0, 0, 0, 0),
					make_call_statistics(4, 0, 2, 104, 0, 0, 0, 0, 0),
					make_call_statistics(5, 0, 4, 105, 0, 0, 0, 0, 0),
					make_call_statistics(6, 0, 3, 106, 0, 0, 0, 0, 0),
					make_call_statistics(7, 0, 5, 107, 0, 0, 0, 0, 0),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};

				// ACT / ASSERT
				assert_equal(0u, data[0].reentrance(lookup));
				assert_equal(0u, data[1].reentrance(lookup));
				assert_equal(0u, data[3].reentrance(lookup));
				assert_equal(0u, data[4].reentrance(lookup));
				assert_equal(0u, data[5].reentrance(lookup));
			}


			test( ReentranceLevelIsCalculatedBasedOnTheAddress )
			{
				// INIT
				const call_statistics data[] = {
					make_call_statistics(1, 0, 0, 101, 0, 0, 0, 0, 0),
					make_call_statistics(2, 0, 1, 101, 0, 0, 0, 0, 0),
					make_call_statistics(3, 0, 2, 102, 0, 0, 0, 0, 0),
					make_call_statistics(4, 0, 3, 102, 0, 0, 0, 0, 0),
					make_call_statistics(5, 0, 4, 101, 0, 0, 0, 0, 0),
					make_call_statistics(6, 0, 3, 102, 0, 0, 0, 0, 0),
					make_call_statistics(7, 0, 6, 101, 0, 0, 0, 0, 0),
				};
				auto lookup = [&] (id_t id) {	return id ? &(data[id - 1]) : nullptr;	};

				// ACT / ASSERT
				assert_equal(0u, data[0].reentrance(lookup));
				assert_equal(1u, data[1].reentrance(lookup));
				assert_equal(0u, data[2].reentrance(lookup));
				assert_equal(1u, data[3].reentrance(lookup));
				assert_equal(2u, data[4].reentrance(lookup));
				assert_equal(1u, data[5].reentrance(lookup));
				assert_equal(2u, data[6].reentrance(lookup));
			}
		end_test_suite
	}
}
