#include <frontend/serialization.h>

#include "comparisons.h"
#include "helpers.h"
#include "primitive_helpers.h"

#include <functional>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef std::pair<unsigned, statistic_types_t<unsigned>::function_detailed> addressed_statistics;
			typedef std::pair<function_key, statistic_types_t<function_key>::function_detailed> threaded_addressed_statistics;

			const vector< pair<long_address_t, statistic_types_t<long_address_t>::function_detailed> > empty_functions;
		}

		begin_test_suite( AdditiveDeSerializationTests )
			test( ContextDeserializationFillsThreadIDFromContext )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				addressed_statistics batch1[] = {
					make_statistics(123441u, 17, 2012, 123123123, 32123, 2213),
					make_statistics(7741u, 17, 2012, 123123123, 32123, 2213),
				};
				addressed_statistics batch2[] = {
					make_statistics(141u, 17, 12012, 11293123, 132123, 12213),
					make_statistics(7341u, 21117, 2212, 21231123, 23213, 2112213),
					make_statistics(7741u, 31117, 3212, 31231123, 33213, 3112213),
				};
				statistic_types::map_detailed s1, s2;
				scontext::detailed_threaded context = { };
				strmd::deserializer<vector_adapter, packer> dser(buffer);

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				context.threadid = 13110;
				dser(s1, context);
				context.threadid = 1700;
				dser(s2, context);

				// ASSERT
				threaded_addressed_statistics reference1[] = {
					make_statistics(addr(123441u, 13110), 17, 2012, 123123123, 32123, 2213),
					make_statistics(addr(7741u, 13110), 17, 2012, 123123123, 32123, 2213),
				};
				threaded_addressed_statistics reference2[] = {
					make_statistics(addr(141u, 1700), 17, 12012, 11293123, 132123, 12213),
					make_statistics(addr(7341u, 1700), 21117, 2212, 21231123, 23213, 2112213),
					make_statistics(addr(7741u, 1700), 31117, 3212, 31231123, 33213, 3112213),
				};

				assert_equivalent(reference1, s1);
				assert_equivalent(reference2, s2);
			}


			test( DeserializationIntoExistingValuesAddsValuesBase )
			{
				typedef std::pair<unsigned, statistic_types_t<unsigned>::function> addressed_statistics2;
				typedef std::pair<function_key, statistic_types_t<function_key>::function> threaded_addressed_statistics2;

				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				addressed_statistics2 batch1[] = {
					make_statistics_base(123441u, 17, 2012, 123123123, 32123, 2213),
					make_statistics_base(7741u, 1117, 212, 1231123, 3213, 112213),
				};
				addressed_statistics2 batch2[] = {
					make_statistics_base(141u, 17, 12012, 11293123, 132123, 12213),
					make_statistics_base(7341u, 21117, 2212, 21231123, 23213, 2112213),
					make_statistics_base(7741u, 31117, 3212, 31231123, 33213, 3112213),
				};
				statistic_types::map_detailed dummy;
				statistic_types::map s;
				scontext::detailed_threaded context = { &dummy, };

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				context.threadid = 1;
				dser(s, context);
				dser(s, context);

				// ASSERT
				threaded_addressed_statistics2 reference1[] = {
					make_statistics_base(addr(141), 17, 12012, 11293123, 132123, 12213),
					make_statistics_base(addr(7341), 21117, 2212, 21231123, 23213, 2112213),
					make_statistics_base(addr(7741), 31117 + 1117, 3212, 31231123 + 1231123, 33213 + 3213, 3112213),
					make_statistics_base(addr(123441), 17, 2012, 123123123, 32123, 2213),
				};

				assert_equivalent(reference1, s);

				// INIT
				ser(mkvector(batch2));

				// ACT
				dser(s, context);

				// ASSERT
				threaded_addressed_statistics2 reference2[] = {
					make_statistics_base(addr(141), 2 * 17, 12012, 2 * 11293123, 2 * 132123, 12213),
					make_statistics_base(addr(7341), 2 * 21117, 2212, 2 * 21231123, 2 * 23213, 2112213),
					make_statistics_base(addr(7741), 2 * 31117 + 1117, 3212, 2 * 31231123 + 1231123, 2 * 33213 + 3213, 3112213),
					make_statistics_base(addr(123441), 17, 2012, 123123123, 32123, 2213),
				};

				assert_equivalent(reference2, s);
			}


			test( EntriesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				statistic_types::map_detailed s;
				scontext::detailed_threaded context = { &s, 0, 1 };

				addressed_statistics initial[] = {
					make_statistics(1221u, 17, 2012, 123123123, 32124, 2213),
					make_statistics(1231u, 18, 2011, 123123122, 32125, 2211),
					make_statistics(1241u, 19, 2010, 123123121, 32126, 2209),
				};
				addressed_statistics addition[] = {
					make_statistics(1231u, 28, 1011, 23123122, 72125, 3211),
					make_statistics(1241u, 29, 3013, 23123121, 72126, 1209),
					make_statistics(12211u, 97, 2012, 123123123, 32124, 2213),
				};

				ser(mkvector(initial));
				ser(mkvector(addition));

				dser(s, context); // read 'initial'

				// ACT
				dser(s, context); // read 'addition'

				// ASSERT
				threaded_addressed_statistics reference[] = {
					make_statistics(addr(1221), 17, 2012, 123123123, 32124, 2213),
					make_statistics(addr(1231), 18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211),
					make_statistics(addr(1241), 19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209),
					make_statistics(addr(12211), 97, 2012, 123123123, 32124, 2213),
				};

				assert_equivalent(reference, s);
			}


			test( CalleesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				statistic_types::map_detailed s;
				scontext::detailed_threaded context = { &s, 0, 1 };

				addressed_statistics initial[] = {
					make_statistics(1221u, 0, 0, 0, 0, 0,
						make_statistics_base(1221u, 17, 2012, 123123123, 32124, 2213),
						make_statistics_base(1231u, 18, 2011, 123123122, 32125, 2211),
						make_statistics_base(1241u, 19, 2010, 123123121, 32126, 2209)),
				};
				addressed_statistics addition[] = {
					make_statistics(1221u, 0, 0, 0, 0, 0,
						make_statistics_base(1231u, 28, 1011, 23123122, 72125, 3211),
						make_statistics_base(1241u, 29, 3013, 23123121, 72126, 1209),
						make_statistics_base(12211u, 97, 2012, 123123123, 32124, 2213)),
				};

				ser(mkvector(initial));
				ser(mkvector(addition));

				dser(s, context); // read 'initial'

				// ACT
				dser(s, context); // read 'addition'

				// ASSERT
				threaded_addressed_statistics reference[] = {
					make_statistics(addr(1221), 0, 0, 0, 0, 0,
						make_statistics_base(addr(1221), 17, 2012, 123123123, 32124, 2213),
						make_statistics_base(addr(1231), 18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211),
						make_statistics_base(addr(1241), 19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209),
						make_statistics_base(addr(12211), 97, 2012, 123123123, 32124, 2213)),
					make_statistics(addr(1231), 0, 0, 0, 0, 0),
					make_statistics(addr(1241), 0, 0, 0, 0, 0),
					make_statistics(addr(12211), 0, 0, 0, 0, 0),
				};

				assert_equivalent(reference, s);
			}


			test( CallersInDetailedStatisticsMapUpdatedOnDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				addressed_statistics batch1[] = {
					make_statistics(1221u, 0, 0, 0, 0, 0,
						make_statistics_base(1221u, 17, 0, 0, 0, 0),
						make_statistics_base(1231u, 18, 0, 0, 0, 0),
						make_statistics_base(1241u, 19, 0, 0, 0, 0)),
					make_statistics(1222u, 0, 0, 0, 0, 0,
						make_statistics_base(1221u, 8, 0, 0, 0, 0),
						make_statistics_base(1251u, 9, 0, 0, 0, 0)),
				};
				addressed_statistics batch2[] = {
					make_statistics(12210u, 1, 0, 0, 0, 0,
						make_statistics_base(12211u, 107, 0, 0, 0, 0),
						make_statistics_base(1221u, 8, 0, 0, 0, 0)),
				};

				statistic_types::map_detailed s;
				scontext::detailed_threaded context = { &s, };

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				context.threadid = 101;
				dser(s, context);

				// ASSERT
				pair<function_key, count_t> reference_1221[] = {
					make_pair(addr(1221, 101), 17),
					make_pair(addr(1222, 101), 8),
				};
				pair<function_key, count_t> reference_1231[] = {
					make_pair(addr(1221, 101), 18),
				};
				pair<function_key, count_t> reference_1241[] = {
					make_pair(addr(1221, 101), 19),
				};
				pair<function_key, count_t> reference_1251[] = {
					make_pair(addr(1222, 101), 9),
				};

				assert_equal(5u, s.size());
				assert_equivalent(reference_1221, s[addr(1221, 101)].callers);
				assert_is_empty(s[addr(1222, 101)].callers);
				assert_equivalent(reference_1231, s[addr(1231, 101)].callers);
				assert_equivalent(reference_1241, s[addr(1241, 101)].callers);
				assert_equivalent(reference_1251, s[addr(1251, 101)].callers);

				// ACT
				context.threadid = 110;
				dser(s, context);

				// ASSERT
				pair<function_key, count_t> reference_12211_110[] = {
					make_pair(addr(12210, 110), 107),
				};
				pair<function_key, count_t> reference_1221_110[] = {
					make_pair(addr(12210, 110), 8),
				};

				assert_equal(8u, s.size());
				assert_equivalent(reference_1221, s[addr(1221, 101)].callers);
				assert_is_empty(s[addr(1222, 101)].callers);
				assert_equivalent(reference_1231, s[addr(1231, 101)].callers);
				assert_equivalent(reference_1241, s[addr(1241, 101)].callers);
				assert_equivalent(reference_1251, s[addr(1251, 101)].callers);

				assert_equivalent(reference_12211_110, s[addr(12211, 110)].callers);
				assert_equivalent(reference_1221_110, s[addr(1221, 110)].callers);
				assert_is_empty(s[addr(12210, 110)].callers);
			}


			test( AddingParentCallsAddsNewStatisticsForParentFunctions )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				statistic_types::map_detailed m1, m2;
				threaded_addressed_statistics batch1[] = {
					make_statistics(addr(0x7011), 0, 0, 0, 0, 0,
						make_statistics_base(addr(0x0011), 10, 0, 0, 0, 0),
						make_statistics_base(addr(0x0013), 11, 0, 0, 0, 0)),
				};
				threaded_addressed_statistics batch2[] = {
					make_statistics(addr(0x5011), 0, 0, 0, 0, 0,
						make_statistics_base(addr(0x0021), 13, 0, 0, 0, 0),
						make_statistics_base(addr(0x0023), 17, 0, 0, 0, 0),
						make_statistics_base(addr(0x0027), 0x1000000000, 0, 0, 0, 0)),
				};

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				dser(m1);
				dser(m2);

				// ASSERT
				assert_equal(3u, m1.size());
				assert_is_empty(m1[addr(0x7011)].callers);
				assert_equal(1u, m1[addr(0x0011)].callers.size());
				assert_equal(1u, m1[addr(0x0013)].callers.size());
				assert_equal(10u, m1[addr(0x0011)].callers[addr(0x7011)]);
				assert_equal(11u, m1[addr(0x0013)].callers[addr(0x7011)]);

				assert_equal(4u, m2.size());
				assert_is_empty(m2[addr(0x5011)].callers);
				assert_equal(1u, m2[addr(0x0021)].callers.size());
				assert_equal(1u, m2[addr(0x0023)].callers.size());
				assert_equal(1u, m2[addr(0x0027)].callers.size());
				assert_equal(13u, m2[addr(0x0021)].callers[addr(0x5011)]);
				assert_equal(17u, m2[addr(0x0023)].callers[addr(0x5011)]);
				assert_equal(0x1000000000u, m2[addr(0x0027)].callers[addr(0x5011)]);
			}


			test( AddingParentCallsUpdatesExistingStatisticsForParentFunctions )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				statistic_types::map_detailed m;
				threaded_addressed_statistics batch[] = {
					make_statistics(addr(0x0191), 0, 0, 0, 0, 0,
						make_statistics_base(addr(0x0021), 13, 0, 0, 0, 0),
						make_statistics_base(addr(0x0023), 17, 0, 0, 0, 0),
						make_statistics_base(addr(0x0027), 0x1000000000, 0, 0, 0, 0)),
				};

				(function_statistics &)m[addr(0x0021)] = function_statistics(10, 0, 17, 11, 30);
				m[addr(0x0021)].callers[addr(0x0191)] = 123;
				m[addr(0x0023)].callers[addr(0x0791)] = 88;
				m[addr(0x0027)].callers[addr(0x0191)] = 0x0231;

				ser(mkvector(batch));

				// ACT
				dser(m);

				// ASSERT
				assert_equal(4u, m.size());
				assert_is_empty(m[addr(0x0191)].callers);
				assert_equal(1u, m[addr(0x0021)].callers.size());
				assert_equal(2u, m[addr(0x0023)].callers.size());
				assert_equal(1u, m[addr(0x0027)].callers.size());
				assert_equal(13u, m[addr(0x0021)].callers[addr(0x0191)]);
				assert_equal(17u, m[addr(0x0023)].callers[addr(0x0191)]);
				assert_equal(88u, m[addr(0x0023)].callers[addr(0x0791)]);
				assert_equal(0x1000000000u, m[addr(0x0027)].callers[addr(0x0191)]);
			}


			test( ThreadsAreAccumulatedInTheContext )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				scontext::additive collected_ids;
				statistic_types::map_detailed m;

				ser(plural + make_pair(3u, empty_functions) + make_pair(2u, empty_functions));
				ser(plural + make_pair(3u, empty_functions) + make_pair(2u, empty_functions) + make_pair(9112u, empty_functions));

				// ACT
				dser(m, collected_ids);

				// ASSERT
				unsigned reference1[] = { 2u, 3u, };

				assert_equivalent(reference1, collected_ids.threads);

				// ACT
				dser(m, collected_ids);

				// ASSERT
				unsigned reference2[] = { 2u, 3u, 9112u, };

				assert_equivalent(reference2, collected_ids.threads);
			}


			test( MappingLayoutIsUpdatedOnDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				tables::module_mappings m;

				ser(plural + make_mapping(0, 12, 10000) + make_mapping(1, 13, 1000) + make_mapping(2, 14, 200) + make_mapping(3, 19, 100000));
				ser(plural + make_mapping(7, 120, 20000) + make_mapping(8, 121, 1000) + make_mapping(9, 122, 900));

				// ACT
				dser(m);

				// ASSERT
				assert_equal(plural
					+ make_mapping(2, 14, 200)
					+ make_mapping(1, 13, 1000)
					+ make_mapping(0, 12, 10000)
					+ make_mapping(3, 19, 100000),
					m.layout);

				// ACT
				dser(m);

				// ASSERT
				assert_equal(plural
					+ make_mapping(9, 122, 900)
					+ make_mapping(8, 121, 1000)
					+ make_mapping(7, 120, 20000),
					m.layout);
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
				call_nodes_index idx(tbl);

				ser(plural + make_pair(13u, plural
					+ make_statistics(1010u, 11, 0, 1002, 32, 2000)
					+ make_statistics(1110u, 110, 2, 2002, 12, 2200)));

				// ACT
				dser(idx, ctx);

				// ASSERT
				call_statistics reference1[] = {
					make_call_statistics(1, 13, 0, 1010u, 11, 0, 1002, 32, 2000),
					make_call_statistics(2, 13, 0, 1110u, 110, 2, 2002, 12, 2200),
				};

				assert_equal_pred(reference1, tbl, eq());

				// INIT
				ser(plural + make_pair(17u, plural
					+ make_statistics(1u, 1, 0, 1002, 32, 2000)
					+ make_statistics(1010u, 2, 0, 1002, 32, 2000)
					+ make_statistics(1110u, 3, 2, 2002, 12, 2200)));

				// ACT
				dser(idx, ctx);

				// ASSERT
				call_statistics reference2[] = {
					make_call_statistics(1, 13, 0, 1010u, 11, 0, 1002, 32, 2000),
					make_call_statistics(2, 13, 0, 1110u, 110, 2, 2002, 12, 2200),

					make_call_statistics(3, 17, 0, 1u, 1, 0, 1002, 32, 2000),
					make_call_statistics(4, 17, 0, 1010u, 2, 0, 1002, 32, 2000),
					make_call_statistics(5, 17, 0, 1110u, 3, 2, 2002, 12, 2200),
				};

				assert_equal_pred(reference2, tbl, eq());
			}


			test( PlainCallsAreDeserializedFromWithinMultipleThreads )
			{
				// INIT
				calls_statistics_table tbl;
				call_nodes_index idx(tbl);

				ser(plural
					+ make_pair(13u, plural
						+ make_statistics(1010u, 11, 0, 1002, 32, 2000)
						+ make_statistics(1110u, 110, 2, 2002, 12, 2200))
					+ make_pair(130u, plural
						+ make_statistics(1010u, 9, 0, 1002, 32, 2000))
					+ make_pair(131u, plural
						+ make_statistics(7u, 1, 0, 10, 11, 12)));

				// ACT
				dser(idx, ctx);

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(1, 13, 0, 1010u, 11, 0, 1002, 32, 2000),
					make_call_statistics(2, 13, 0, 1110u, 110, 2, 2002, 12, 2200),
					make_call_statistics(3, 130, 0, 1010u, 9, 0, 1002, 32, 2000),
					make_call_statistics(4, 131, 0, 7u, 1, 0, 10, 11, 12),
				};

				assert_equal_pred(reference, tbl, eq());
			}


			test( PlainCallsAreAccumulatedOnDeserialization )
			{
				// INIT
				calls_statistics_table tbl;
				call_nodes_index idx(tbl);

				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 100, 100, 32, 2000)
						+ make_statistics(2u, 1, 2, 101, 12, 200)
						+ make_statistics(3u, 1, 0, 102, 32, 2500)
						+ make_statistics(5u, 2, 0, 103, 11, 1211)));
				dser(idx, ctx);
				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(9u, 1, 0, 102, 32, 2500)
						+ make_statistics(1u, 10, 71, 10, 2, 2)
						+ make_statistics(2u, 10, 72, 11, 2, 2)
						+ make_statistics(5u, 20, 70, 13, 1, 5)));

				// ACT
				dser(idx, ctx);

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(1, 7, 0, 1, 11, 100, 110, 34, 2000),
					make_call_statistics(2, 7, 0, 2, 11, 72, 112, 14, 200),
					make_call_statistics(3, 7, 0, 3, 1, 0, 102, 32, 2500),
					make_call_statistics(4, 7, 0, 5, 22, 70, 116, 12, 1211),
					make_call_statistics(5, 7, 0, 9, 1, 0, 102, 32, 2500),
				};

				assert_equal_pred(reference, tbl, eq());
			}


			test( HierarchicalCallsAreDeserializedFromWithinASingleThread )
			{
				// INIT
				calls_statistics_table tbl;
				call_nodes_index idx(tbl);

				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 100, 100, 32, 2000,
							make_statistics_base(2u, 1, 2, 101, 12, 200))
						+ make_statistics(3u, 1, 0, 102, 32, 2500,
							make_statistics_base(5u, 2, 0, 103, 11, 1211),
							make_statistics_base(9u, 1, 0, 102, 32, 2500),
							make_statistics_base(1u, 10, 71, 10, 2, 2))
						+ make_statistics(2u, 10, 72, 11, 2, 2,
							make_statistics_base(5u, 20, 70, 13, 1, 5),
							make_statistics_base(9u, 1, 1, 1, 1, 1))));

				// ACT
				dser(idx, ctx);

				// ASSERT
				call_statistics reference[] = {
					make_call_statistics(0, 7, 0, 1u, 1, 100, 100, 32, 2000),
					make_call_statistics(0, 7, 1, 2u, 1, 2, 101, 12, 200),
					make_call_statistics(0, 7, 0, 3u, 1, 0, 102, 32, 2500),
					make_call_statistics(0, 7, 3, 5u, 2, 0, 103, 11, 1211),
					make_call_statistics(0, 7, 3, 9u, 1, 0, 102, 32, 2500),
					make_call_statistics(0, 7, 3, 1u, 10, 71, 10, 2, 2),
					make_call_statistics(0, 7, 0, 2u, 10, 72, 11, 2, 2),
					make_call_statistics(0, 7, 7, 5u, 20, 70, 13, 1, 5),
					make_call_statistics(0, 7, 7, 9u, 1, 1, 1, 1, 1),
				};

				assert_equivalent(reference, tbl);
			}


			test( ThreadsAreAccumulatedInTheContext )
			{
				// INIT
				calls_statistics_table tbl;
				call_nodes_index idx(tbl);

				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 0, 0, 0, 0))
					+ make_pair(171u, plural
						+ make_statistics(1u, 1, 0, 0, 0, 0))
					+ make_pair(733u, plural
						+ make_statistics(1u, 1, 0, 0, 0, 0)));
				ser(plural
					+ make_pair(7u, plural
						+ make_statistics(1u, 1, 0, 0, 0, 0))
					+ make_pair(3u, plural
						+ make_statistics(1u, 1, 0, 0, 0, 0)));

				// ACT
				dser(idx, ctx);

				// ASSERT
				unsigned reference1[] = {	7u, 171u, 733u,	};

				assert_equivalent(reference1, ctx.threads);

				// ACT
				dser(idx, ctx);

				// ASSERT
				unsigned reference2[] = {	7u, 3u,	};

				assert_equivalent(reference2, ctx.threads);
			}
		end_test_suite
	}
}
