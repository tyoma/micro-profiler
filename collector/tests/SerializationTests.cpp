#include <collector/analyzer.h>

#include <collector/serialization.h>

#include "helpers.h"

#include <common/unordered_map.h>
#include <functional>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef call_graph_types<unsigned> statistic_types;
			typedef pair<unsigned, statistic_types::node> addressed_statistics;
		}

		begin_test_suite( SerializationTests )
			test( DetailedStatisticsIsSerializedAsExpected )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				statistic_types::node s1;

				static_cast<function_statistics &>(s1) = function_statistics(17, 123123123, 32123);
				s1.callees[7741] = function_statistics(1117, 1231123, 3213);
				s1.callees[141] = function_statistics(17, 11293123, 132123);

				// ACT
				s(s1);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				statistic_types::node ds1;
				vector< pair<unsigned, function_statistics> > callees;
				vector< pair<unsigned, count_t> > callers;

				// ACT
				ds(ds1);

				// ASSERT
				addressed_statistics reference[] = {
					make_pair(7741, function_statistics(1117, 1231123, 3213)),
					make_pair(141, function_statistics(17, 11293123, 132123)),
				};

				assert_equal(s1, ds1);
				assert_equivalent(reference, ds1.callees);
			}


			test( SingleThreadedAnalyzerDataIsSerializable )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				analyzer a(overhead(0, 0));
				call_record trace[] = {
					{	12319, addr(1234)	},
						{	12324, addr(2234)	},
						{	12326, addr(0)	},
					{	12330, addr(0)	},
				};

				a.accept_calls(1717, trace, array_size(trace));

				// ACT
				s(a);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				containers::unordered_map<unsigned /*threadid*/, statistic_types::nodes_map> ss;

				// ACT
				ds(ss);

				// ASSERT
				addressed_statistics reference[] = {
					make_statistics(1234u, 1, 0, 11, 9, 11, plural
						+ make_statistics(2234u, 1, 0, 2, 2, 2)),
				};

				assert_equal(1u, ss.size());
				assert_equivalent(reference, ss.begin()->second);
			}


			test( MultiThreadedAnalyzerDataIsSerializable )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				analyzer a(overhead(0, 0));
				call_record trace1[] = {
					{	12319, addr(1234)	},
					{	12330, addr(0)	},
					{	12340, addr(2234)	},
					{	12350, addr(0)	},
				};
				call_record trace2[] = {
					{	12319, addr(1234)	},
					{	12330, addr(0)	},
				};

				a.accept_calls(1719, trace1, array_size(trace1));
				a.accept_calls(13719, trace2, array_size(trace2));

				// ACT
				s(a);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				containers::unordered_map<unsigned, statistic_types::nodes_map> ss;

				// ACT
				ds(ss);

				// ASSERT
				addressed_statistics reference1[] = {
					make_statistics(1234u, 1, 0, 11, 11, 11),
					make_statistics(2234u, 1, 0, 10, 10, 10),
				};
				addressed_statistics reference2[] = {
					make_statistics(1234u, 1, 0, 11, 11, 11),
				};

				assert_equal(2u, ss.size());
				assert_not_null(find_by_first(ss, 1719u));
				assert_not_null(find_by_first(ss, 13719u));
				assert_equivalent(reference1, *find_by_first(ss, 1719u));
				assert_equivalent(reference2, *find_by_first(ss, 13719u));

				// INIT
				a.accept_calls(11, trace2, array_size(trace2));
				ss.clear();

				// ACT
				s(a);

				ds(ss);

				// ASSERT
				assert_equal(3u, ss.size());
				assert_not_null(find_by_first(ss, 1719u));
				assert_not_null(find_by_first(ss, 13719u));
				assert_not_null(find_by_first(ss, 11u));
				assert_equivalent(reference1, *find_by_first(ss, 1719u));
				assert_equivalent(reference2, *find_by_first(ss, 13719u));
				assert_equivalent(reference2, *find_by_first(ss, 11u));
			}

		end_test_suite
	}
}
