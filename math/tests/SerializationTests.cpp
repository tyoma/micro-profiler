#include <math/serialization.h>

#include <test-helpers/helpers.h>

#include <functional>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		using namespace micro_profiler::tests;

		namespace
		{
			typedef strmd::varint packer;
		}

		begin_test_suite( SerializationTests )

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
