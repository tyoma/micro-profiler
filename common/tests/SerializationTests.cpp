#include <common/serialization.h>

#include <test-helpers/comparisons.h>
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
	namespace tests
	{
		begin_test_suite( SerializationTests )
			test( SerializedStatisticsAreDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				function_statistics s1(17, 123123123, 32123, 2213), s2(1117, 1231123, 3213, 112213);
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


			test( SerializedSymbolMetadataIsProperlyDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				symbol_info symbols[] = {
					{ "fourrier_transform", 0x10010, 100, 11, 1001 },
					{ "build_trie", 0x8000, 301, 1171, 101 },
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
					{ "fourrier_transform", 13, 0x10010, 11, 1001 },
					{ "build_trie", 13110, 0x8000, 1171, 101 },
					{ "merge_sort", 0x8181, 274, 19, 200 },
				};
				symbol_info symbols2[] = {
					{ "fast_fourrier_transform", 0x4010, 100, 11, 1001 },
					{ "quick_sort", 0x5181, 274, 23, 200 },
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
				module_info_metadata m1 = { "kernel",  182213u, mkvector(symbols1), containers::unordered_map<unsigned int, string>(begin(files1), end(files1)) };
				module_info_metadata m2 = { "user", 10101011u, mkvector(symbols2), containers::unordered_map<unsigned int, string>(begin(files2), end(files2)) };
				module_info_metadata read;

				// ACT
				s(m1);
				ds(read);

				// ASSERT
				assert_equal("kernel", read.path);
				assert_equal(182213u, read.hash);
				assert_equal(symbols1, read.symbols);
				assert_equivalent(files1, read.source_files);

				// ACT
				s(m2);
				ds(read);

				// ASSERT
				assert_equal("user", read.path);
				assert_equal(10101011u, read.hash);
				assert_equal(symbols2, read.symbols);
				assert_equivalent(files2, read.source_files);
			}


			test( ChronoTypesAreSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				mt::milliseconds v;

				// ACT
				s(mt::milliseconds(123));
				s(mt::milliseconds(123101010101));

				// ACT / ASSERT
				ds(v);
				assert_equal(mt::milliseconds(123), v);
				ds(v);
				assert_equal(mt::milliseconds(123101010101), v);
			}


			test( ThreadInfoIsSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				thread_info ti1 = { 122, "thread 1", mt::milliseconds(123), mt::milliseconds(2345), mt::milliseconds(18), true };
				thread_info ti2 = { 27, "t #2", mt::milliseconds(1), mt::milliseconds(2), mt::milliseconds(18191716), false };
				thread_info v;

				// ACT
				s(ti1);
				s(ti2);

				// ACT / ASSERT
				ds(v);
				assert_equal(ti1, v);
				ds(v);
				assert_equal(ti2, v);
			}

		end_test_suite
	}
}
