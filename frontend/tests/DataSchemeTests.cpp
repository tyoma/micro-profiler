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

			struct empty_index
			{
				const fake_call &operator [](id_t) const
				{
					assert_is_false(true);
					throw 0;
				}

				int operator [](id_t); // Shall not be accessed altogether.
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
				empty_index ei;
				const callstack_keyer<empty_index> keyer(ei);

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
				views::immutable_unique_index<views::table<fake_call>, id_keyer> by_id(tbl, id_keyer());
				const callstack_keyer< views::immutable_unique_index<views::table<fake_call>, id_keyer> > keyer(by_id);

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
				primary_id_index by_id(tbl, id_keyer());

				for (auto i = begin(data_); i != end(data_); ++i)
					add(tbl, *i);

				// INIT / ACT
				callstack_index by_callstack(tbl, callstack_keyer<primary_id_index>(by_id));

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
	}
}
