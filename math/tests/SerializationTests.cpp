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
			test( EmptyScalesAreSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				linear_scale<int> s1, v1(1, 100, 14);
				log_scale<int> s2, v2(1, 100, 14);

				// ACT
				s(s1);
				s(s2);

				// ASSERT
				ds(v1);
				assert_equal(linear_scale<int>(), v1);
				ds(v2);
				assert_equal(log_scale<int>(), v2);
			}

			test( LinearScaleIsSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				linear_scale<int> scale1(10, 132112, 311);
				linear_scale<int> scale2(1040, 1321120033, 111);
				linear_scale<int> v(1, 7, 2);

				// ACT
				s(scale1);
				s(scale2);

				// ACT / ASSERT
				ds(v);
				assert_equal(scale1, v);
				ds(v);
				assert_equal(scale2, v);
			}


			test( LogScaleIsSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				log_scale<int> scale1(10, 132112, 311);
				log_scale<int> scale2(1040, 1321120033, 111);
				log_scale<int> v(1, 7, 2);

				// ACT
				s(scale1);
				s(scale2);

				// ACT / ASSERT
				ds(v);
				assert_equal(scale1, v);
				ds(v);
				assert_equal(scale2, v);
			}


			test( LinearScaleIsFunctionalAfterDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				linear_scale<int> ref(10, 1710, 51);
				linear_scale<long long> v(1, 7, 2);
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


			test( LogScaleIsFunctionalAfterDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				log_scale<int> ref(10, 1710, 51);
				log_scale<long long> v(1, 7, 2);
				index_t index;

				// ACT
				s(ref);

				// ASSERT
				ds(v);

				assert_equal(0u, (v(index, 10), index));
				assert_equal(1u, (v(index, 11), index));
				assert_equal(11u, (v(index, 30), index));
				assert_equal(33u, (v(index, 300), index));
				assert_equal(45u, (v(index, 1000), index));
				assert_equal(50u, (v(index, 1710), index));
			}


			test( VariantScaleIsFunctionalAfterDeserialization )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				variant_scale<int> scale1(log_scale<int>(10, 132112, 311));
				variant_scale<long long> scale2(linear_scale<long long>(1040, 1321120033, 111));
				variant_scale<int> v1((linear_scale<int>()));
				variant_scale<long long> v2((linear_scale<long long>()));

				// ACT
				s(scale1);
				s(scale2);

				// ACT / ASSERT
				ds(v1);
				assert_equal(scale1, v1);
				ds(v2);
				assert_equal(scale2, v2);
			}


			test( HistogramIsSerialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				histogram<linear_scale<int>, int> h1;
				histogram<linear_scale<int>, int> h2;
				histogram<linear_scale<int>, int> v;

				h1.set_scale(linear_scale<int>(100, 1100, 21));
				h1.add(640, 2);
				h1.add(140);

				h2.set_scale(linear_scale<int>(200, 1000, 6));
				h2.add(712);
				h2.add(512);
				h2.add(212);

				// ACT
				s(h1);
				s(h2);
				ds(v);

				// ASSERT
				int reference1[] = {	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	};

				assert_equal(linear_scale<int>(100, 1100, 21), v.get_scale());
				assert_equal(reference1, v);

				// ACT
				ds(v);

				// ASSERT
				int reference2[] = {	1, 0, 1, 1, 0, 0,	};

				assert_equal(linear_scale<int>(200, 1000, 6), v.get_scale());
				assert_equal(reference2, v);
			}

		end_test_suite

	}
}
