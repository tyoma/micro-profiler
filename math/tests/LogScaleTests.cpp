#include <math/scale.h>

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

		begin_test_suite( LogScaleTests )
			test( DefaultScaleIsEmpty )
			{
				// INIT / ACT
				log_scale<int> s;
				index_t index;

				// ACT / ASSERT
				assert_equal(0u, s.samples());
				assert_equal(0, s.near_value());
				assert_equal(0, s.far_value());
				assert_is_false(s(index, 0));
				assert_is_false(s(index, 0xFFFFFFF));
				assert_equal(log_scale<int>(), s);
			}


			test( ASingleSampledScaleAlwaysEvaluatesToZero )
			{
				// INIT / ACT
				log_scale<int> s1(100, 1010000, 1);
				log_scale<int> s2(1, 11, 1);

				// ACT / ASSERT
				assert_equal(1u, s1.samples());
				assert_equal(100, s1.near_value());
				assert_equal(1010000, s1.far_value());
				assert_equal(0u, cvt(s1, 1));
				assert_equal(0u, cvt(s1, 100));
				assert_equal(0u, cvt(s1, 100000000));
				assert_equal(1, s2.near_value());
				assert_equal(11, s2.far_value());
				assert_equal(0u, cvt(s2, 1));
				assert_equal(0u, cvt(s2, 100000000));
			}


			test( ScalesInitializedWithTheSameValuesAreEqual )
			{
				// INIT / ACT / ASSERT
				assert_equal(log_scale<int>(100, 1000, 3), log_scale<int>(100, 1000, 3));
				assert_equal(log_scale<int>(100, 1000, 40), log_scale<int>(100, 1000, 40));
				assert_equal(log_scale<int>(100, 1100, 40), log_scale<int>(100, 1100, 40));
				assert_equal(log_scale<int>(50, 1100, 40), log_scale<int>(50, 1100, 40));
			}


			test( ScalesInitializedWithDifferentValuesAreNotEqual )
			{
				// INIT / ACT / ASSERT
				assert_is_false(log_scale<int>(100, 1000, 3) == log_scale<int>(100, 1000, 4));
				assert_is_false(log_scale<int>(100, 1000, 3) == log_scale<int>(100, 1500, 3));
				assert_is_false(log_scale<int>(10, 1000, 7) == log_scale<int>(100, 1000, 7));
				assert_not_equal(log_scale<int>(100, 1000, 3), log_scale<int>(100, 1000, 4));
				assert_not_equal(log_scale<int>(100, 1000, 40), log_scale<int>(100, 1001, 40));
				assert_not_equal(log_scale<int>(100, 1100, 40), log_scale<int>(101, 1100, 40));
			}


			test( LogScaleGivesBoundariesForOutOfDomainValues )
			{
				// INIT
				log_scale<int> s1(15901, 1000000, 3);
				log_scale<int> s2(9000, 10000, 10);

				// ACT / ASSERT
				assert_equal(3u, s1.samples());
				assert_equal(0u, cvt(s1, 1));
				assert_equal(0u, cvt(s1, 15900));
				assert_equal(0u, cvt(s1, 15901));
				assert_equal(2u, cvt(s1, 1000000));
				assert_equal(2u, cvt(s1, 1000001));
				assert_equal(2u, cvt(s1, 2010001));

				assert_equal(10u, s2.samples());
				assert_equal(0u, cvt(s2, 1));
				assert_equal(0u, cvt(s2, 8000));
				assert_equal(0u, cvt(s2, 9000));
				assert_equal(9u, cvt(s2, 10000));
				assert_equal(9u, cvt(s2, 10001));
				assert_equal(9u, cvt(s2, 2010001));
			}


			test( LogScaleProvidesIndexForAValueWithBoundariesAtTheMiddleOfRange )
			{
				// INIT
				log_scale<int> s1(1000, 3000, 3); // 3 - 3.4771 (a segment is 0.23856)
				log_scale<int> s2(10, 710, 10); // 1 - 2.8513 (a segment is 0.2057)

				// ACT / ASSERT
				assert_equal(0u, cvt(s1, 1316));
				assert_equal(1u, cvt(s1, 1317));
				assert_equal(1u, cvt(s1, 2279));
				assert_equal(2u, cvt(s1, 2280));
			}


			test( LogScalePlaysWellWithZeroAndNegativeNumbers )
			{
				// INIT
				log_scale<int> s1(1000, 3000, 30);
				log_scale<double> s2(10, 710, 20);
				index_t index;

				// ACT / ASSERT
				assert_is_false(s1(index, 0));
				assert_is_false(s2(index, 0));
				assert_is_false(s1(index, -1000));
				assert_is_false(s2(index, -10));
			}


			test( CenterValueIsCalculatedAccordinglyToRangeAndSampling )
			{
				// INIT
				log_scale<int> s1(10, 100, 91);
				log_scale<float> s2(10.9f, 32.3f, 13);

				// ACT / ASSERT
				assert_equal(10, s1[0u]);
				assert_equal(37, s1[52u]);
				assert_equal(64, s1[73u]);
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
