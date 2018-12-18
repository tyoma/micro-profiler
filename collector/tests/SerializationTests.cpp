#include <collector/analyzer.h>

#include <common/serialization.h>
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
			typedef function_statistics_detailed_t<const void *> function_statistics_detailed;
			typedef function_statistics_detailed_t<const void *>::callees_map statistics_map;
		}

		begin_test_suite( SerializationTests )

			test( AnalyzerDataIsSerializable )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				analyzer a;
				map<const void *, function_statistics> m;
				call_record trace[] = {
					{	12319, (void *)1234	},
					{	12324, (void *)2234	},
					{	12326, (void *)0	},
					{	12330, (void *)0	},
				};

				a.accept_calls(mt::thread::id(), trace, array_size(trace));

				// ACT
				s(a);

				// INIT
				strmd::deserializer<vector_adapter, packer> ds(buffer);
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
