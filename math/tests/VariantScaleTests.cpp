#include <math/variant_scale.h>

#include <ut/assert.h>
#include <ut/test.h>

namespace math
{
	namespace tests
	{
		namespace
		{
			template <typename ScaleT>
			index_t cvt(const ScaleT &scale_, typename ScaleT::value_type value)
			{
				index_t index;

				assert_is_true(scale_(index, value));
				return index;
			}
		}

		begin_test_suite( VariantScaleTests )
			test( DefaultsToEmptyLinearScale )
			{
				assert_equal(variant_scale<double>(linear_scale<double>()), variant_scale<double>());
				assert_equal(variant_scale<int>(linear_scale<int>()), variant_scale<int>());
			}

			test( HoldsTheUnderlyingScaleValue )
			{
				// INIT / ACT
				variant_scale<double> vs1(linear_scale<double>(0.0, 1.0, 11));
				variant_scale<int> vs2(log_scale<int>(1, 230, 11));

				// ACT / ASSERT
				assert_equal(variant_scale<double>(linear_scale<double>(0.0, 1.0, 11)), vs1);
				assert_not_equal(variant_scale<double>(linear_scale<double>(0.0, 1.2, 11)), vs1);
				assert_equal(variant_scale<int>(log_scale<int>(1, 230, 11)), vs2);
				assert_not_equal(variant_scale<int>(log_scale<int>(11, 230, 11)), vs2);
			}


			test( ExposesUnderlyingProperties )
			{
				// INIT / ACT
				variant_scale<double> vs1(linear_scale<double>(0.0, 1.0, 11));
				variant_scale<int> vs2(log_scale<int>(1, 230, 110));

				// ACT / ASSERT
				assert_equal(0.0, vs1.near_value());
				assert_equal(1.0, vs1.far_value());
				assert_equal(11u, vs1.samples());
				assert_equal(1, vs2.near_value());
				assert_equal(230, vs2.far_value());
				assert_equal(110u, vs2.samples());
			}


			test( ConvertsUsingUnderlying )
			{
				// INIT / ACT
				variant_scale<double> vs1(linear_scale<double>(0.0, 1.0, 11));
				variant_scale<int> vs2(log_scale<int>(1, 230, 110));
				index_t dummy;

				// ACT / ASSERT
				assert_equal(2u, cvt(vs1, 0.2));
				assert_equal(3u, cvt(vs1, 0.3));
				assert_is_false(vs2(dummy, 0));
				assert_equal(60u, cvt(vs2, 20));
				assert_equal(96u, cvt(vs2, 120));
			}


			test( CenterValueIsCalculatedAccordinglyToRangeAndSampling )
			{
				// INIT
				variant_scale<int> s1(linear_scale<int>(10, 100, 91));
				variant_scale<float> s2(log_scale<float>(10.9f, 32.3f, 13));

				// ACT / ASSERT
				assert_equal(10, s1[0u]);
				assert_equal(62, s1[52u]);
				assert_equal(83, s1[73u]);
				assert_equal(100, s1[90u]);

				assert_approx_equal(10.90f, s2[0u], 0.001f);
				assert_approx_equal(14.30f, s2[3u], 0.001f);
				assert_approx_equal(18.76f, s2[6u], 0.001f);
				assert_approx_equal(29.50f, s2[11u], 0.001f);
				assert_approx_equal(32.30f, s2[12u], 0.001f);
			}

		end_test_suite
	}
}
