#include <frontend/serialization.h>

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
		namespace
		{
			typedef statistic_types_t<address_t> statistic_types;
			typedef deserialization_context<address_t> test_context;

			address_t addr(size_t value)
			{	return value;	}
		}

		begin_test_suite( SerializationTests )
			test( DeserializationIntoExistingValuesAddsValues )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistic_types::map s1, s2;
				test_context context = { };

				s1[addr(123441)] = function_statistics(17, 2012, 123123123, 32123, 2213);
				s1[addr(7741)] = function_statistics(1117, 212, 1231123, 3213, 112213);
				s2[addr(141)] = function_statistics(17, 12012, 11293123, 132123, 12213);
				s2[addr(7341)] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				s2[addr(7741)] = function_statistics(31117, 3212, 31231123, 33213, 3112213);
				s(s1);
				s(s2);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT
				ds(s2, context);

				// ASSERT
				statistic_types::map reference1;

				reference1[addr(141)] = function_statistics(17, 12012, 11293123, 132123, 12213);
				reference1[addr(7341)] = function_statistics(21117, 2212, 21231123, 23213, 2112213);
				reference1[addr(7741)] = function_statistics(31117 + 1117, 3212, 31231123 + 1231123, 33213 + 3213, 3112213);
				reference1[addr(123441)] = function_statistics(17, 2012, 123123123, 32123, 2213);

				assert_equivalent(reference1, s2);

				// ACT
				ds(s2, context);

				// ASSERT
				statistic_types::map reference2;

				reference2[addr(141)] = function_statistics(2 * 17, 12012, 2 * 11293123, 2 * 132123, 12213);
				reference2[addr(7341)] = function_statistics(2 * 21117, 2212, 2 * 21231123, 2 * 23213, 2112213);
				reference2[addr(7741)] = function_statistics(2 * 31117 + 1117, 3212, 2 * 31231123 + 1231123, 2 * 33213 + 3213, 3112213);
				reference2[addr(123441)] = function_statistics(17, 2012, 123123123, 32123, 2213);

				assert_equivalent(reference2, s2);
			}


			test( EntriesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistic_types::map_detailed ss, addition;

				static_cast<function_statistics &>(ss[addr(1221)]) = function_statistics(17, 2012, 123123123, 32124, 2213);
				static_cast<function_statistics &>(ss[addr(1231)]) = function_statistics(18, 2011, 123123122, 32125, 2211);
				static_cast<function_statistics &>(ss[addr(1241)]) = function_statistics(19, 2010, 123123121, 32126, 2209);

				static_cast<function_statistics &>(addition[addr(1231)]) = function_statistics(28, 1011, 23123122, 72125, 3211);
				static_cast<function_statistics &>(addition[addr(1241)]) = function_statistics(29, 3013, 23123121, 72126, 1209);
				static_cast<function_statistics &>(addition[addr(12211)]) = function_statistics(97, 2012, 123123123, 32124, 2213);

				s(addition);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// INIT
				statistic_types::map_detailed dss;
				test_context context = { &dss, };

				dss = ss;

				// ACT
				ds(dss, context);

				// ASSERT
				statistic_types::map_detailed reference;

				static_cast<function_statistics &>(reference[addr(1221)]) = function_statistics(17, 2012, 123123123, 32124, 2213);
				static_cast<function_statistics &>(reference[addr(1231)]) = function_statistics(18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211);
				static_cast<function_statistics &>(reference[addr(1241)]) = function_statistics(19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209);
				static_cast<function_statistics &>(reference[addr(12211)]) = function_statistics(97, 2012, 123123123, 32124, 2213);

				assert_equivalent(reference, dss);
			}


			test( CalleesInDetailedStatisticsMapAppendedWithNewStatistics )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistic_types::map_detailed ss, addition;

				ss[addr(1221)].callees[addr(1221)] = function_statistics(17, 2012, 123123123, 32124, 2213);
				ss[addr(1221)].callees[addr(1231)] = function_statistics(18, 2011, 123123122, 32125, 2211);
				ss[addr(1221)].callees[addr(1241)] = function_statistics(19, 2010, 123123121, 32126, 2209);

				addition[addr(1221)].callees[addr(1231)] = function_statistics(28, 1011, 23123122, 72125, 3211);
				addition[addr(1221)].callees[addr(1241)] = function_statistics(29, 3013, 23123121, 72126, 1209);
				addition[addr(1221)].callees[addr(12211)] = function_statistics(97, 2012, 123123123, 32124, 2213);

				s(addition);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// INIT
				statistic_types::map_detailed dss;
				test_context context = { &dss, };

				dss = ss;

				// ACT
				ds(dss, context);

				// ASSERT
				statistic_types::map reference;

				reference[addr(1221)] = function_statistics(17, 2012, 123123123, 32124, 2213);
				reference[addr(1231)] = function_statistics(18 + 28, 2011, 123123122 + 23123122, 32125 + 72125, 3211);
				reference[addr(1241)] = function_statistics(19 + 29, 3013, 123123121 + 23123121, 32126 + 72126, 2209);
				reference[addr(12211)] = function_statistics(97, 2012, 123123123, 32124, 2213);

				assert_equivalent(reference, dss[addr(1221)].callees);
			}


			test( CallersInDetailedStatisticsMapUpdatedOnDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				test_context context;
				statistic_types::map_detailed ss;
				statistic_types::map_detailed dss;

				context.map = &dss;
				ss[addr(1221)].callees[addr(1221)] = function_statistics(17, 0, 0, 0, 0);
				ss[addr(1221)].callees[addr(1231)] = function_statistics(18, 0, 0, 0, 0);
				ss[addr(1221)].callees[addr(1241)] = function_statistics(19, 0, 0, 0, 0);
				ss[addr(1222)].callees[addr(1221)] = function_statistics(8, 0, 0, 0, 0);
				ss[addr(1222)].callees[addr(1251)] = function_statistics(9, 0, 0, 0, 0);

				s(ss);

				strmd::deserializer<vector_adapter, packer> ds(buffer);

				// ACT
				ds(dss, context);

				// ASSERT
				pair<address_t, count_t> reference_1221[] = {
					make_pair(addr(1221), 17),
					make_pair(addr(1222), 8),
				};
				pair<address_t, count_t> reference_1231[] = {
					make_pair(addr(1221), 18),
				};
				pair<address_t, count_t> reference_1241[] = {
					make_pair(addr(1221), 19),
				};
				pair<address_t, count_t> reference_1251[] = {
					make_pair(addr(1222), 9),
				};

				assert_equivalent(reference_1221, dss[addr(1221)].callers);
				assert_equivalent(reference_1231, dss[addr(1231)].callers);
				assert_equivalent(reference_1241, dss[addr(1241)].callers);
				assert_equivalent(reference_1251, dss[addr(1251)].callers);
			}

		end_test_suite
	}
}
