#include <common/serialization.h>
#include <collector/analyzer.h>

#include "Helpers.h"

#include <strmd/strmd/serializer.h>
#include <strmd/strmd/deserializer.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class vector_adapter
			{
			public:
				vector_adapter()
					: _ptr(0)
				{	}

				void write(const void *buffer, size_t size)
				{
					const unsigned char *b = reinterpret_cast<const unsigned char *>(buffer);

					_buffer.insert(_buffer.end(), b, b + size);
				}

				void read(void *buffer, size_t size)
				{
					assert_is_true(size <= _buffer.size() - _ptr);
					memcpy(buffer, &_buffer[_ptr], size);
					_ptr += size;
				}

			private:
				size_t _ptr;
				vector<unsigned char> _buffer;
			};
		}

		begin_test_suite( SerializationHelpersTests )
			test( SerializedStatisticsAreDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter> s(buffer);
				function_statistics s1(17, 2012, 123123123, 32123, 2213), s2(1117, 212, 1231123, 3213, 112213);
				function_statistics ds1, ds2;

				// ACT (serialization)
				s(s1);
				s(s2);

				// INIT
				strmd::deserializer<vector_adapter> ds(buffer);

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
				strmd::serializer<vector_adapter> s(buffer);
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
				strmd::deserializer<vector_adapter> ds(buffer);

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
				strmd::serializer<vector_adapter> s(buffer);
				statistics_map s1, s2;

				s1[(void *)123441] = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1[(void *)7741] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s2[(void *)141] = function_statistics(17, 12012, 11293123, 132123, 12213);
				s2[(void *)7341] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				s2[(void *)7741] = function_statistics(31117, 3212, 31231123, 33213, 3112213);
				s(s1);
				s(s2);

				strmd::deserializer<vector_adapter> ds(buffer);

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
				strmd::serializer<vector_adapter> s(buffer);
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
				strmd::deserializer<vector_adapter> ds(buffer);
				function_statistics_detailed ds1;
				vector< pair<const void *, function_statistics> > callees;
				vector< pair<const void *, count_t> > callers;

				// ACT
				ds(ds1);

				// ASSERT
				pair<const void *, function_statistics> reference_callees[] = {
					make_pair((void *)7741, function_statistics(1117, 212, 1231123, 3213, 112213)),
					make_pair((void *)141, function_statistics(17, 12012, 11293123, 132123, 12213)),
				};
				pair<const void *, count_t> reference_callers[] = {
					make_pair((void *)14100, 1232),
					make_pair((void *)14101, 12322),
					make_pair((void *)141000, 123221),
				};

				assert_equal(s1, ds1);
				assert_equivalent(reference_callees, mkvector(ds1.callees));
				assert_equivalent(reference_callers, mkvector(ds1.callers));
			}


			test( EntriesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter> s(buffer);
				statistics_map_detailed ss, addition;

				static_cast<function_statistics &>(ss[(void *)1221]) = function_statistics(17, 2012, 123123123, 32124, 2213);
				static_cast<function_statistics &>(ss[(void *)1231]) = function_statistics(18, 2011, 123123122, 32125, 2211);
				static_cast<function_statistics &>(ss[(void *)1241]) = function_statistics(19, 2010, 123123121, 32126, 2209);

				static_cast<function_statistics &>(addition[(void *)1231]) = function_statistics(28, 1011, 23123122, 72125, 3211);
				static_cast<function_statistics &>(addition[(void *)1241]) = function_statistics(29, 3013, 23123121, 72126, 1209);
				static_cast<function_statistics &>(addition[(void *)12211]) = function_statistics(97, 2012, 123123123, 32124, 2213);

				s(addition);

				strmd::deserializer<vector_adapter> ds(buffer);

				// ACT
				ds(ss);

				// ASSERT
				statistics_map_detailed reference;

				static_cast<function_statistics &>(reference[(void *)1221]) = function_statistics(17, 2012, 123123123, 32124, 2213);
				static_cast<function_statistics &>(reference[(void *)1231]) = function_statistics(18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211);
				static_cast<function_statistics &>(reference[(void *)1241]) = function_statistics(19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209);
				static_cast<function_statistics &>(reference[(void *)12211]) = function_statistics(97, 2012, 123123123, 32124, 2213);

				assert_equivalent(mkvector(reference), mkvector(ss));
			}


			test( CalleesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter> s(buffer);
				statistics_map_detailed ss, addition;

				ss[(void *)1221].callees[(void *)1221] = function_statistics(17, 2012, 123123123, 32124, 2213);
				ss[(void *)1221].callees[(void *)1231] = function_statistics(18, 2011, 123123122, 32125, 2211);
				ss[(void *)1221].callees[(void *)1241] = function_statistics(19, 2010, 123123121, 32126, 2209);

				addition[(void *)1221].callees[(void *)1231] = function_statistics(28, 1011, 23123122, 72125, 3211);
				addition[(void *)1221].callees[(void *)1241] = function_statistics(29, 3013, 23123121, 72126, 1209);
				addition[(void *)1221].callees[(void *)12211] = function_statistics(97, 2012, 123123123, 32124, 2213);

				s(addition);

				strmd::deserializer<vector_adapter> ds(buffer);

				// ACT
				ds(ss);

				// ASSERT
				statistics_map reference;

				reference[(void *)1221] = function_statistics(17, 2012, 123123123, 32124, 2213);
				reference[(void *)1231] = function_statistics(18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211);
				reference[(void *)1241] = function_statistics(19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209);
				reference[(void *)12211] = function_statistics(97, 2012, 123123123, 32124, 2213);

				assert_equivalent(mkvector(reference), mkvector(ss[(void *)1221].callees));
			}


			test( CallersInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter> s(buffer);
				statistics_map_detailed ss, addition;

				ss[(void *)1221].callers[(void *)1221] = 12332;
				ss[(void *)1221].callers[(void *)1231] = 311232;
				ss[(void *)1221].callers[(void *)1241] = 9182746;

				addition[(void *)1221].callers[(void *)1231] = 12;
				addition[(void *)1221].callers[(void *)1241] = 191;
				addition[(void *)1221].callers[(void *)12211] = 723;

				s(addition);

				strmd::deserializer<vector_adapter> ds(buffer);

				// ACT
				ds(ss);

				// ASSERT
				statistics_map_callers reference;

				reference[(void *)1221] = 12332;
				reference[(void *)1231] = 311232 + 12;
				reference[(void *)1241] = 9182746 + 191;
				reference[(void *)12211] = 723;

				assert_equivalent(mkvector(reference), mkvector(ss[(void *)1221].callers));
			}



			test( AnalyzerDataIsSerializable )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter> s(buffer);
				analyzer a;
				map<const void *, function_statistics> m;
				call_record trace[] = {
					{	12319, (void *)1234	},
					{	12324, (void *)2234	},
					{	12326, (void *)0	},
					{	12330, (void *)0	},
				};

				a.accept_calls(2, trace, array_size(trace));

				// ACT
				s(a);

				// INIT
				strmd::deserializer<vector_adapter> ds(buffer);
				statistics_map_detailed ss;

				// ACT
				ds(ss);

				// ASSERT
				statistics_map_detailed reference;

				static_cast<function_statistics &>(reference[(void *)1234]) = function_statistics(1, 0, 11, 9, 11);
				static_cast<function_statistics &>(reference[(void *)2234]) = function_statistics(1, 0, 2, 2, 2);

				assert_equivalent(mkvector(reference), mkvector(ss));
			}

		end_test_suite
	}
}
