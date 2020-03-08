#include <frontend/serialization.h>

#include "helpers.h"

#include <test-helpers/comparisons.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>

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
			typedef std::pair<address_t, statistic_types_t<address_t>::function_detailed> threaded_addressed_statistics;
		}

		begin_test_suite( SerializationTests )
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
				deserialization_context context = { };
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
				typedef std::pair<unsigned, statistic_types_t<unsigned>::function> addressed_statistics;
				typedef std::pair<address_t, statistic_types_t<address_t>::function> threaded_addressed_statistics;

				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				addressed_statistics batch1[] = {
					make_statistics_base(123441u, 17, 2012, 123123123, 32123, 2213),
					make_statistics_base(7741u, 1117, 212, 1231123, 3213, 112213),
				};
				addressed_statistics batch2[] = {
					make_statistics_base(141u, 17, 12012, 11293123, 132123, 12213),
					make_statistics_base(7341u, 21117, 2212, 21231123, 23213, 2112213),
					make_statistics_base(7741u, 31117, 3212, 31231123, 33213, 3112213),
				};
				statistic_types::map s;
				deserialization_context context = { };

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				context.threadid = 1;
				dser(s, context);
				dser(s, context);

				// ASSERT
				threaded_addressed_statistics reference1[] = {
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
				threaded_addressed_statistics reference2[] = {
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
				deserialization_context context = { &s, 0, 1 };

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
				deserialization_context context = { &s, 0, 1 };

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
				deserialization_context context = { &s, };

				ser(mkvector(batch1));
				ser(mkvector(batch2));

				// ACT
				context.threadid = 101;
				dser(s, context);

				// ASSERT
				pair<address_t, count_t> reference_1221[] = {
					make_pair(addr(1221, 101), 17),
					make_pair(addr(1222, 101), 8),
				};
				pair<address_t, count_t> reference_1231[] = {
					make_pair(addr(1221, 101), 18),
				};
				pair<address_t, count_t> reference_1241[] = {
					make_pair(addr(1221, 101), 19),
				};
				pair<address_t, count_t> reference_1251[] = {
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
				pair<address_t, count_t> reference_12211_110[] = {
					make_pair(addr(12210, 110), 107),
				};
				pair<address_t, count_t> reference_1221_110[] = {
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

		end_test_suite
	}
}
