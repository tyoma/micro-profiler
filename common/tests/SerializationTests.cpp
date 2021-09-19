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
	inline bool operator ==(const symbol_info &lhs, const symbol_info &rhs)
	{
		return lhs.name == rhs.name && lhs.rva == rhs.rva && lhs.size == rhs.size
			&& lhs.file_id == rhs.file_id && lhs.line == rhs.line;
	}

	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<const void *> statistic_types;

			const void *addr(size_t value)
			{	return reinterpret_cast<const void *>(value);	}
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
				statistic_types::map s1, s2;
				statistic_types::map ds1, ds2;

				s1[addr(123441)] = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1[addr(71341)] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s2[addr(141)] = function_statistics(17, 12012, 1123123123, 132123, 12213);
				s2[addr(7341)] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				s2[addr(7741)] = function_statistics(31117, 3212, 31231123, 33213, 3112213);

				// ACT
				s(s1);
				s(s2);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT (deserialization)
				ds(ds1);
				ds(ds2);

				// ASSERT
				assert_equivalent(ds1, s1);
				assert_equivalent(ds2, s2);
			}


			test( DetailedStatisticsIsSerializedAsExpected )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistic_types::function_detailed s1;

				static_cast<function_statistics &>(s1) = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1.callees[addr(7741)] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s1.callees[addr(141)] = function_statistics(17, 12012, 11293123, 132123, 12213);
				s1.callers[addr(14100)] = 1232;
				s1.callers[addr(14101)] = 12322;
				s1.callers[addr(141000)] = 123221;

				// ACT
				s(s1);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				statistic_types::function_detailed ds1;
				vector< pair<const void *, function_statistics> > callees;
				vector< pair<const void *, count_t> > callers;

				// ACT
				ds(ds1);

				// ASSERT
				pair<const void *, function_statistics> reference[] = {
					make_pair(addr(7741), function_statistics(1117, 212, 1231123, 3213, 112213)),
					make_pair(addr(141), function_statistics(17, 12012, 11293123, 132123, 12213)),
				};

				assert_equal(s1, ds1);
				assert_equivalent(reference, ds1.callees);
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
				module_info_metadata m1 = { "kernel",  mkvector(symbols1), containers::unordered_map<unsigned int, string>(begin(files1), end(files1)) };
				module_info_metadata m2 = { "user", mkvector(symbols2), containers::unordered_map<unsigned int, string>(begin(files2), end(files2)) };
				module_info_metadata read;

				// ACT
				s(m1);
				ds(read);

				// ASSERT
				assert_equal("kernel", read.path);
				assert_equal(symbols1, read.symbols);
				assert_equivalent(files1, read.source_files);

				// ACT
				s(m2);
				ds(read);

				// ASSERT
				assert_equal("user", read.path);
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


			test( HistogramScaleIsSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				scale scale1(10, 132112, 311);
				scale scale2(1040, 1321120033, 111);
				scale v(1, 1, 2);

				// ACT
				s(scale1);
				s(scale2);

				// ACT / ASSERT
				ds(v);
				assert_equal(scale1, v);
				ds(v);
				assert_equal(scale2, v);
			}


			test( ScaleIsFunctionalAfterDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				scale ref(10, 1710, 51);
				scale v(1, 1, 2);
				index_t index;

				// ACT
				s(ref);

				// ASSERT
				ds(v);

				assert_equal(0u, (v(index, 26), index));
				assert_equal(1u, (v(index, 27), index));
				assert_equal(5u, (v(index, 26 + 5 * 34), index));
				assert_equal(6u, (v(index, 27 + 5 * 34), index));
			}


			test( HistogramIsSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				histogram h1;
				histogram h2;
				histogram v;

				h1.set_scale(scale(100, 1100, 21));
				h1.add(640, 2);
				h1.add(140);

				h2.set_scale(scale(200, 1000, 6));
				h2.add(712);
				h2.add(512);
				h2.add(212);

				// ACT
				s(h1);
				s(h2);
				ds(v);

				// ASSERT
				unsigned reference1[] = {	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	};

				assert_equal(scale(100, 1100, 21), v.get_scale());
				assert_equal(reference1, v);

				// ACT
				ds(v);

				// ASSERT
				unsigned reference2[] = {	1, 0, 1, 1, 0, 0,	};

				assert_equal(scale(200, 1000, 6), v.get_scale());
				assert_equal(reference2, v);
			}

		end_test_suite

	}
}
