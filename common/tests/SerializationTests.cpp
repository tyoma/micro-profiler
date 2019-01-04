#include <common/serialization.h>
#include <collector/primitives.h>

#include <test-helpers/helpers.h>

#include <functional>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	inline bool operator ==(const symbol_info &lhs, const symbol_info &rhs)
	{
		return lhs.id == rhs.id && lhs.name == rhs.name && lhs.rva == rhs.rva && lhs.size == rhs.size
			&& lhs.file_id == rhs.file_id && lhs.line == rhs.line;
	}

	namespace tests
	{
		namespace
		{
			typedef function_statistics_detailed_t<const void *> function_statistics_detailed;
			typedef function_statistics_detailed_t<const void *>::callees_map statistics_map;
		}

		begin_test_suite( SerializationTests )
			test( SerializedStatisticsAreDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				function_statistics s1(17, 2012, 123123123, 32123, 2213), s2(1117, 212, 1231123, 3213, 112213);
				function_statistics ds1, ds2;

				// ACT (serialization)
				s(s1);
				s(s2);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT (deserialization)
				ds(ds1);
				ds(ds2);

				// ASSERT
				assert_equal(s1, ds1);
				assert_equal(s2, ds2);
			}

			
			test( SerializationDeserializationOfContainersOfFunctionStaticsProducesTheSameResults )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistics_map s1, s2;
				statistics_map ds1, ds2;

				s1[(void *)123441] = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1[(void *)71341] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s2[(void *)141] = function_statistics(17, 12012, 1123123123, 132123, 12213);
				s2[(void *)7341] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				s2[(void *)7741] = function_statistics(31117, 3212, 31231123, 33213, 3112213);

				// ACT
				s(s1);
				s(s2);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT (deserialization)
				ds(ds1);
				ds(ds2);

				// ASSERT
				assert_equivalent(mkvector(ds1), mkvector(s1));
				assert_equivalent(mkvector(ds2), mkvector(s2));
			}

			
			test( DeserializationIntoExistingValuesAddsValues )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistics_map s1, s2;

				s1[(void *)123441] = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1[(void *)7741] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s2[(void *)141] = function_statistics(17, 12012, 11293123, 132123, 12213);
				s2[(void *)7341] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				s2[(void *)7741] = function_statistics(31117, 3212, 31231123, 33213, 3112213);
				s(s1);
				s(s2);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT
				ds(s2);

				// ASSERT
				statistics_map reference1;

				reference1[(void *)141] = function_statistics(17, 12012, 11293123, 132123, 12213);
				reference1[(void *)7341] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				reference1[(void *)7741] = function_statistics(31117 + 1117, 3212, 31231123 + 1231123, 33213 + 3213, 3112213);
				reference1[(void *)123441] = function_statistics(17, 2012, 123123123, 32123, 2213);

				assert_equivalent(mkvector(reference1), mkvector(s2));

				// ACT
				ds(s2);

				// ASSERT
				statistics_map reference2;

				reference2[(void *)141] = function_statistics(2 * 17, 12012, 2 * 11293123, 2 * 132123, 12213);
				reference2[(void *)7341] = function_statistics(2 * 21117, 2212, 2 * 21231123, 2 * 23213, 2112213);
				reference2[(void *)7741] = function_statistics(2 * 31117 + 1117, 3212, 2 * 31231123 + 1231123, 2 * 33213 + 3213, 3112213);
				reference2[(void *)123441] = function_statistics(17, 2012, 123123123, 32123, 2213);

				assert_equivalent(mkvector(reference2), mkvector(s2));
			}


			test( DetailedStatisticsIsSerializedAsExpected )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				function_statistics_detailed s1;

				static_cast<function_statistics &>(s1) = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1.callees[(void *)7741] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s1.callees[(void *)141] = function_statistics(17, 12012, 11293123, 132123, 12213);
				s1.callers[(void *)14100] = 1232;
				s1.callers[(void *)14101] = 12322;
				s1.callers[(void *)141000] = 123221;

				// ACT
				s(s1);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				function_statistics_detailed ds1;
				vector< pair<const void *, function_statistics> > callees;
				vector< pair<const void *, count_t> > callers;

				// ACT
				ds(ds1);

				// ASSERT
				pair<const void *, function_statistics> reference[] = {
					make_pair((void *)7741, function_statistics(1117, 212, 1231123, 3213, 112213)),
					make_pair((void *)141, function_statistics(17, 12012, 11293123, 132123, 12213)),
				};

				assert_equal(s1, ds1);
				assert_equivalent(reference, mkvector(ds1.callees));
			}


			test( EntriesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistics_map_detailed ss, addition;

				static_cast<function_statistics &>(ss[(void *)1221]) = function_statistics(17, 2012, 123123123, 32124, 2213);
				static_cast<function_statistics &>(ss[(void *)1231]) = function_statistics(18, 2011, 123123122, 32125, 2211);
				static_cast<function_statistics &>(ss[(void *)1241]) = function_statistics(19, 2010, 123123121, 32126, 2209);

				static_cast<function_statistics &>(addition[(void *)1231]) = function_statistics(28, 1011, 23123122, 72125, 3211);
				static_cast<function_statistics &>(addition[(void *)1241]) = function_statistics(29, 3013, 23123121, 72126, 1209);
				static_cast<function_statistics &>(addition[(void *)12211]) = function_statistics(97, 2012, 123123123, 32124, 2213);

				s(addition);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// INIT
				statistics_map_detailed dss;

				static_cast<statistics_map_detailed &>(dss) = ss;

				// ACT
				ds(dss);

				// ASSERT
				statistics_map_detailed reference;

				static_cast<function_statistics &>(reference[(void *)1221]) = function_statistics(17, 2012, 123123123, 32124, 2213);
				static_cast<function_statistics &>(reference[(void *)1231]) = function_statistics(18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211);
				static_cast<function_statistics &>(reference[(void *)1241]) = function_statistics(19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209);
				static_cast<function_statistics &>(reference[(void *)12211]) = function_statistics(97, 2012, 123123123, 32124, 2213);

				assert_equivalent(mkvector(reference), mkvector(dss));
			}


			test( CalleesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistics_map_detailed ss, addition;

				ss[(void *)1221].callees[(void *)1221] = function_statistics(17, 2012, 123123123, 32124, 2213);
				ss[(void *)1221].callees[(void *)1231] = function_statistics(18, 2011, 123123122, 32125, 2211);
				ss[(void *)1221].callees[(void *)1241] = function_statistics(19, 2010, 123123121, 32126, 2209);

				addition[(void *)1221].callees[(void *)1231] = function_statistics(28, 1011, 23123122, 72125, 3211);
				addition[(void *)1221].callees[(void *)1241] = function_statistics(29, 3013, 23123121, 72126, 1209);
				addition[(void *)1221].callees[(void *)12211] = function_statistics(97, 2012, 123123123, 32124, 2213);

				s(addition);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// INIT
				statistics_map_detailed dss;

				static_cast<statistics_map_detailed &>(dss) = ss;

				// ACT
				ds(dss);

				// ASSERT
				statistics_map reference;

				reference[(void *)1221] = function_statistics(17, 2012, 123123123, 32124, 2213);
				reference[(void *)1231] = function_statistics(18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211);
				reference[(void *)1241] = function_statistics(19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209);
				reference[(void *)12211] = function_statistics(97, 2012, 123123123, 32124, 2213);

				assert_equivalent(mkvector(reference), mkvector(dss[(void *)1221].callees));
			}


			test( CallersInDetailedStatisticsMapUpdatedOnDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistics_map_detailed ss;
				statistics_map_detailed dss;

				ss[(void *)1221].callees[(void *)1221] = function_statistics(17, 0, 0, 0, 0);
				ss[(void *)1221].callees[(void *)1231] = function_statistics(18, 0, 0, 0, 0);
				ss[(void *)1221].callees[(void *)1241] = function_statistics(19, 0, 0, 0, 0);
				ss[(void *)1222].callees[(void *)1221] = function_statistics(8, 0, 0, 0, 0);
				ss[(void *)1222].callees[(void *)1251] = function_statistics(9, 0, 0, 0, 0);

				s(ss);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT
				ds(dss);

				// ASSERT
				pair<const void *, count_t> reference_1221[] = {
					make_pair((void *)1221, 17),
					make_pair((void *)1222, 8),
				};
				pair<const void *, count_t> reference_1231[] = {
					make_pair((void *)1221, 18),
				};
				pair<const void *, count_t> reference_1241[] = {
					make_pair((void *)1221, 19),
				};
				pair<const void *, count_t> reference_1251[] = {
					make_pair((void *)1222, 9),
				};

				assert_equivalent(reference_1221, mkvector(dss[(void *)1221].callers));
				assert_equivalent(reference_1231, mkvector(dss[(void *)1231].callers));
				assert_equivalent(reference_1241, mkvector(dss[(void *)1241].callers));
				assert_equivalent(reference_1251, mkvector(dss[(void *)1251].callers));
			}


			test( SerializedSymbolMetadataIsProperlyDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				symbol_info symbols[] = {
					{ "fourrier_transform", 0x10010, 100, 13, 11, 1001 },
					{ "build_trie", 0x8000, 301, 13110, 1171, 101 },
				};
				symbol_info read;

				// ACT
				s(symbols[0]);
				ds(read);

				// ASSERT
				assert_equal(read, symbols[0]);

				// ACT
				s(symbols[1]);
				ds(read);

				// ASSERT
				assert_equal(read, symbols[1]);
			}


			test( SerializedModuleMetadataIsProperlyDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				symbol_info symbols1[] = {
					{ "fourrier_transform", 13, 0x10010, 100, 11, 1001 },
					{ "build_trie", 13110, 0x8000, 301, 1171, 101 },
					{ "merge_sort", 0x8181, 274, 13111, 19, 200 },
				};
				symbol_info symbols2[] = {
					{ "fast_fourrier_transform", 0x4010, 100, 11, 11, 1001 },
					{ "quick_sort", 0x5181, 274, 1111, 23, 200 },
				};
				pair<unsigned, string> files1[] = {
					make_pair(11, "fourrier.cpp"),
					make_pair(1171, "trie.c"),
					make_pair(19, "sort.cpp"),
				};
				pair<unsigned, string> files2[] = {
					make_pair(11, "fourrier.cpp"),
					make_pair(23, "sort.c"),
				};
				module_info_metadata m1 = { mkvector(symbols1), mkvector(files1) };
				module_info_metadata m2 = { mkvector(symbols2), mkvector(files2) };
				module_info_metadata read;

				// ACT
				s(m1);
				ds(read);

				// ASSERT
				assert_equal(symbols1, read.symbols);
				assert_equal(files1, read.source_files);

				// ACT
				s(m2);
				ds(read);

				// ASSERT
				assert_equal(symbols2, read.symbols);
				assert_equal(files2, read.source_files);
			}

		end_test_suite
	}
}
