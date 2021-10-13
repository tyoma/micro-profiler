#include <math/serialization.h>

#include <functional>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		using namespace micro_profiler::tests;

		typedef strmd::varint packer;
		typedef linear_scale<int> linear_scale_;
		typedef histogram<linear_scale_, int> histogram_;

		begin_test_suite( AdditiveDeSerializationTests )
			test( HistogramIsDeserializedOneToOneWhenWritingOverNew )
			{
				// INIT
				scontext::additive<histogram_> w;
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				histogram_ h1;
				histogram_ h2;
				histogram_ v;

				h1.set_scale(linear_scale_(100, 1100, 21));
				h1.add(640);
				h1.add(640);
				h1.add(140);

				h2.set_scale(linear_scale_(200, 1000, 6));
				h2.add(712);
				h2.add(512);
				h2.add(212);

				// ACT
				s(h1);
				s(h2);
				ds(v, w);

				// ASSERT
				int reference1[] = {	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	};

				assert_equal(linear_scale_(100, 1100, 21), v.get_scale());
				assert_equal(reference1, v);

				// ACT
				ds(v, w);

				// ASSERT
				int reference2[] = {	1, 0, 1, 1, 0, 0,	};

				assert_equal(linear_scale_(200, 1000, 6), v.get_scale());
				assert_equal(reference2, v);
			}


			test( DeserializedValueIsAddedToAHistogram )
			{
				// INIT
				scontext::additive<histogram_> w;
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				histogram_ h1;
				histogram_ h2;
				histogram_ v;

				v.set_scale(linear_scale_(10, 20, 11));
				v.add(13, 2);
				v.add(17, 1);
				v.add(19, 3);
				h1.set_scale(linear_scale_(10, 20, 11));
				h1.add(10, 2);
				h1.add(17, 2);

				h2.set_scale(linear_scale_(10, 20, 11));
				h2.add(12);
				h2.add(14);
				h2.add(16, 2);

				// ACT
				s(h1);
				s(h2);
				ds(v, w);

				// ASSERT
				int reference1a[] = {	2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,	};
				int reference1[] = {	2, 0, 0, 2, 0, 0, 0, 3, 0, 3, 0,	};

				assert_equal(reference1a, w.buffer);
				assert_equal(reference1, v);

				// ACT
				ds(v, w);

				// ASSERT
				int reference2a[] = {	0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 0,	};
				int reference2[] = {	2, 0, 1, 2, 1, 0, 2, 3, 0, 3, 0,	};

				assert_equal(reference2a, w.buffer);
				assert_equal(reference2, v);
			}

		end_test_suite

		begin_test_suite( InterpolatingDeSerializationTests )
			test( DeserializedValueIsInterpolatedWithExisting )
			{
				// INIT
				scontext::interpolating<histogram_> w = {	75.0f / 256,	};
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				histogram_ h1;
				histogram_ h2;
				histogram_ v;

				v.set_scale(linear_scale_(10, 20, 11));
				v.add(13, 200);
				v.add(17, 100);
				v.add(19, 300);
				h1.set_scale(linear_scale_(10, 20, 11));
				h1.add(10, 240);
				h1.add(17, 1800);
				h2.set_scale(linear_scale_(10, 20, 11));
				h2.add(12, 10);
				h2.add(14, 130);
				h2.add(16, 73);

				// ACT
				s(h1);
				s(h2);
				ds(v, w);

				// ASSERT
				int reference1a[] = {	240, 0, 0, 0, 0, 0, 0, 1800, 0, 0, 0,	};
				int reference1[] = {	70, 0, 0, 142, 0, 0, 0, 598, 0, 213, 0,	};

				assert_equal(reference1a, w.buffer);
				assert_equal(reference1, v);

				// ACT
				ds(v, w);

				// ASSERT
				int reference2a[] = {	0, 0, 10, 0, 130, 0, 73, 0, 0, 0, 0,	};
				int reference2[] = {	50, 0, 2, 101, 38, 0, 21, 423, 0, 151, 0,	};

				assert_equal(reference2a, w.buffer);
				assert_equal(reference2, v);
			}

		end_test_suite
	}
}
