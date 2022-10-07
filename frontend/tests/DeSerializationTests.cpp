#include <frontend/serialization.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <collector/serialization.h> // TODO: remove?
#include <functional>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace strmd
{
	template <typename K>
	struct type_traits< std::map<K, micro_profiler::function_statistics> >
	{
		typedef container_type_tag category;
		struct item_reader_type : strmd::indexed_associative_container_reader
		{
			template <typename ContainerT>
			void prepare(ContainerT &, size_t /*count*/) const
			{	}
		};
	};
}

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( AdditiveDeSerializationTests )
			test( DeserializationIntoExistingValuesAddsValuesBase )
			{
				typedef std::pair<unsigned, function_statistics> addressed_statistics2;

				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				addressed_statistics2 batch1[] = {
					make_statistics(123441u, 17, 2012, 123123123, 32123),
					make_statistics(7741u, 1117, 212, 1231123, 3213),
				};
				addressed_statistics2 batch2[] = {
					make_statistics(141u, 17, 12012, 11293123, 132123),
					make_statistics(7341u, 21117, 2212, 21231123, 23213),
					make_statistics(7741u, 31117, 3212, 31231123, 33213),
				};
				map<unsigned, function_statistics> s;
				scontext::additive context;

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				dser(s, context);
				dser(s, context);

				// ASSERT
				addressed_statistics2 reference1[] = {
					make_statistics(141u, 17, 12012, 11293123, 132123),
					make_statistics(7341u, 21117, 2212, 21231123, 23213),
					make_statistics(7741u, 31117 + 1117, 3212, 31231123 + 1231123, 33213 + 3213),
					make_statistics(123441u, 17, 2012, 123123123, 32123),
				};

				assert_equivalent(reference1, s);

				// INIT
				ser(mkvector(batch2));

				// ACT
				dser(s, context);

				// ASSERT
				addressed_statistics2 reference2[] = {
					make_statistics(141u, 2 * 17, 12012, 2 * 11293123, 2 * 132123),
					make_statistics(7341u, 2 * 21117, 2212, 2 * 21231123, 2 * 23213),
					make_statistics(7741u, 2 * 31117 + 1117, 3212, 2 * 31231123 + 1231123, 2 * 33213 + 3213),
					make_statistics(123441u, 17, 2012, 123123123, 32123),
				};

				assert_equivalent(reference2, s);
			}

		end_test_suite


		begin_test_suite( FlattenDeSerializationTests )
			vector_adapter buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			scontext::additive ctx;

			FlattenDeSerializationTests()
				: ser(buffer), dser(buffer)
			{	}


			test( PlainCallsAreDeserializedFromWithinASingleThread )
			{
				// INIT
				calls_statistics_table tbl;
				auto &idx = sdb::unique_index<keyer::callnode>(tbl);

				ser(plural + make_pair(13u, plural
					+ make_statistics(1010u, 11, 0, 1002, 32)
					+ make_statistics(1110u, 110, 2, 2002, 12)));

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				call_statistics reference1[] = {
					make_call_statistics(1, 13, 0, 1010u, 11, 0, 1002, 32),
					make_call_statistics(2, 13, 0, 1110u, 110, 2, 2002, 12),
				};

				assert_equal_pred(reference1, tbl, eq());

				// INIT
				ser(plural + make_pair(17u, plural
					+ make_statistics(1u, 1, 0, 1002, 32)
					+ make_statistics(1010u, 2, 0, 1002, 32)
					+ make_statistics(1110u, 3, 2, 2002, 12)));

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				call_statistics reference2[] = {
					make_call_statistics(1, 13, 0, 1010u, 11, 0, 1002, 32),
					make_call_statistics(2, 13, 0, 1110u, 110, 2, 2002, 12),

					make_call_statistics(3, 17, 0, 1u, 1, 0, 1002, 32),
					make_call_statistics(4, 17, 0, 1010u, 2, 0, 1002, 32),
					make_call_statistics(5, 17, 0, 1110u, 3, 2, 2002, 12),
				};

				assert_equal_pred(reference2, tbl, eq());
			}


			test( PlainCallsAreDeserializedFromWithinMultipleThreads )
			{
				// INIT
				calls_statistics_table tbl;
				auto &idx = sdb::unique_index<keyer::callnode>(tbl);

				ser(plural
					+ make_pair(13u, plural
						+ make_statistics(1010u, 11, 0, 1002, 32)
						+ make_statistics(1110u, 110, 2, 2002, 12))
					+ make_pair(130u, plural
						+ make_statistics(1010u, 9, 0, 1002, 32))
					+ make_pair(131u, plural
						+ make_statistics(7u, 1, 0, 10, 11)));

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(1, 13, 0, 1010u, 11, 0, 1002, 32),
					make_call_statistics(2, 13, 0, 1110u, 110, 2, 2002, 12),
					make_call_statistics(3, 130, 0, 1010u, 9, 0, 1002, 32),
					make_call_statistics(4, 131, 0, 7u, 1, 0, 10, 11),
				};

				assert_equal_pred(reference, tbl, eq());
			}


			test( PlainCallsAreAccumulatedOnDeserialization )
			{
				// INIT
				calls_statistics_table tbl;
				auto &idx = sdb::unique_index<keyer::callnode>(tbl);

				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 100, 100, 32)
						+ make_statistics(2u, 1, 2, 101, 12)
						+ make_statistics(3u, 1, 0, 102, 32)
						+ make_statistics(5u, 2, 0, 103, 11)));
				dser(idx, scontext::root_context(ctx, idx));
				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(9u, 1, 0, 102, 32)
						+ make_statistics(1u, 10, 71, 10, 2)
						+ make_statistics(2u, 10, 72, 11, 2)
						+ make_statistics(5u, 20, 70, 13, 1)));

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(1, 7, 0, 1, 11, 100, 110, 34),
					make_call_statistics(2, 7, 0, 2, 11, 72, 112, 14),
					make_call_statistics(3, 7, 0, 3, 1, 0, 102, 32),
					make_call_statistics(4, 7, 0, 5, 22, 70, 116, 12),
					make_call_statistics(5, 7, 0, 9, 1, 0, 102, 32),
				};

				assert_equal_pred(reference, tbl, eq());
			}


			test( HierarchicalCallsAreDeserializedFromWithinASingleThread )
			{
				// INIT
				calls_statistics_table tbl;
				auto &idx = sdb::unique_index<keyer::callnode>(tbl);

				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 100, 100, 32, plural
							+ make_statistics(2u, 1, 2, 101, 12))
						+ make_statistics(3u, 1, 0, 102, 32, plural
							+ make_statistics(5u, 2, 0, 103, 11)
							+ make_statistics(9u, 1, 0, 102, 32)
							+ make_statistics(1u, 10, 71, 10, 2))
						+ make_statistics(2u, 10, 72, 11, 2, plural
							+ make_statistics(5u, 20, 70, 13, 1)
							+ make_statistics(9u, 1, 1, 1, 1))));

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(0, 7, 0, 1u, 1, 100, 100, 32),
					make_call_statistics(0, 7, 1, 2u, 1, 2, 101, 12),
					make_call_statistics(0, 7, 0, 3u, 1, 0, 102, 32),
					make_call_statistics(0, 7, 3, 5u, 2, 0, 103, 11),
					make_call_statistics(0, 7, 3, 9u, 1, 0, 102, 32),
					make_call_statistics(0, 7, 3, 1u, 10, 71, 10, 2),
					make_call_statistics(0, 7, 0, 2u, 10, 72, 11, 2),
					make_call_statistics(0, 7, 7, 5u, 20, 70, 13, 1),
					make_call_statistics(0, 7, 7, 9u, 1, 1, 1, 1),
				};

				assert_equivalent(reference, tbl);
			}


			test( ThreadsAreAccumulatedInTheContext )
			{
				// INIT
				calls_statistics_table tbl;
				auto &idx = sdb::unique_index<keyer::callnode>(tbl);

				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 0, 0, 0))
					+ make_pair(171u, plural
						+ make_statistics(1u, 1, 0, 0, 0))
					+ make_pair(733u, plural
						+ make_statistics(1u, 1, 0, 0, 0)));
				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 0, 0, 0))
					+ make_pair(3u, plural
						+ make_statistics(1u, 1, 0, 0, 0)));

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				unsigned reference1[] = {	7u, 171u, 733u,	};

				assert_equivalent(reference1, ctx.threads);

				// ACT
				dser(idx, scontext::root_context(ctx, idx));

				// ASSERT
				unsigned reference2[] = {	7u, 3u,	};

				assert_equivalent(reference2, ctx.threads);
			}
		end_test_suite
	}
}
