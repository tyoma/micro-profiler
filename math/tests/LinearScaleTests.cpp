#include <math/scale.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

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

				scale_(index, value);
				return index;
			}
		}

		begin_test_suite( LinearScaleTests )
			test( DefaultScaleIsEmpty )
			{
				// INIT / ACT
				linear_scale<int> s;
				index_t index;

				// ACT / ASSERT
				assert_equal(0u, s.samples());
				assert_is_false(s(index, 0));
				assert_is_false(s(index, 0xFFFFFFF));
				assert_equal(linear_scale<int>(), s); // equals to another empty scale
			}


			test( ZeroSampledScaleCannotBeCreated )
			{
				// INIT
				assert_throws(linear_scale<int>(1, 1, 10), invalid_argument);
				assert_throws(linear_scale<int>(0, 0, 10), invalid_argument);
				assert_throws(linear_scale<double>(1, -20, 9), invalid_argument);
				assert_throws(linear_scale<double>(10.0, 10.0, 91), invalid_argument);
			}


			test( ASingleSampledScaleAlwaysEvaluatesToZero )
			{
				// INIT / ACT
				linear_scale<int> s1(100, 101, 1);
				linear_scale<int> s2(0, 10, 1);

				// ACT / ASSERT
				assert_equal(0u, cvt(s1, 0));
				assert_equal(0u, cvt(s1, 100));
				assert_equal(0u, cvt(s1, 100000000));
				assert_equal(0u, cvt(s2, 0));
				assert_equal(0u, cvt(s2, 100000000));
			}


			test( LinearScaleGivesBoundariesForOutOfDomainValues )
			{
				// INIT
				linear_scale<int> s1(15901, 1000000, 3);
				linear_scale<int> s2(9000, 10000, 10);

				// ACT / ASSERT
				assert_equal(3u, s1.samples());
				assert_equal(0u, cvt(s1, 0));
				assert_equal(0u, cvt(s1, 15900));
				assert_equal(0u, cvt(s1, 15901));
				assert_equal(2u, cvt(s1, 1000000));
				assert_equal(2u, cvt(s1, 1000001));
				assert_equal(2u, cvt(s1, 2010001));

				assert_equal(10u, s2.samples());
				assert_equal(0u, cvt(s2, 0));
				assert_equal(0u, cvt(s2, 8000));
				assert_equal(0u, cvt(s2, 9000));
				assert_equal(9u, cvt(s2, 10000));
				assert_equal(9u, cvt(s2, 10001));
				assert_equal(9u, cvt(s2, 2010001));
			}


			test( LinearScaleProvidesIndexForAValueWithBoundariesAtTheMiddleOfRange )
			{
				// INIT
				linear_scale<int> s1(1000, 3000, 3);
				linear_scale<int> s2(10, 710, 10);

				// ACT / ASSERT
				assert_equal(0u, cvt(s1, 1499));
				assert_equal(1u, cvt(s1, 1500));
				assert_equal(1u, cvt(s1, 2499));
				assert_equal(2u, cvt(s1, 2500));

				assert_equal(0u, cvt(s2, 49));
				assert_equal(1u, cvt(s2, 50));
				assert_equal(3u, cvt(s2, 283));
				assert_equal(4u, cvt(s2, 284));
				assert_equal(8u, cvt(s2, 671));
				assert_equal(9u, cvt(s2, 673));
			}


			test( ScaleIsEquallyComparable )
			{
				// INIT
				linear_scale<int> s1(1000, 3000, 3);
				linear_scale<int> s11(1000, 3000, 3);
				linear_scale<int> s2(10, 710, 10);
				linear_scale<int> s21(10, 710, 10);
				linear_scale<int> s3(1000, 3000, 90);
				linear_scale<int> s4(1000, 3050, 3);
				linear_scale<int> s5(1001, 3000, 3);

				// ACT / ASSERT
				assert_is_true(s1 == s1);
				assert_is_true(s1 == s11);
				assert_is_false(s1 == s2);
				assert_is_false(s1 == s3);
				assert_is_false(s1 == s4);
				assert_is_false(s1 == s5);
				assert_is_true(s2 == s2);
				assert_is_true(s2 == s21);

				assert_is_false(s1 != s1);
				assert_is_false(s1 != s11);
				assert_is_true(s1 != s2);
				assert_is_true(s1 != s3);
				assert_is_true(s1 != s4);
				assert_is_true(s1 != s5);
				assert_is_false(s2 != s2);
				assert_is_false(s2 != s21);
			}


			test( AllZeroSampledScalesAreEqual )
			{
				// INIT / ACT / ASSERT
				assert_equal(linear_scale<int>(), linear_scale<int>(1, 100, 0));
				assert_equal(linear_scale<int>(), linear_scale<int>(-10, 0, 0));
				assert_equal(linear_scale<int>(), linear_scale<int>(0, 10, 0));
			}


			test( CenterValueIsCalculatedAccordinglyToRangeAndSampling )
			{
				// INIT
				linear_scale<int> s1(10, 100, 91);
				linear_scale<float> s2(10.9f, 32.3f, 13);

				// ACT / ASSERT
				assert_equal(10, s1[0u]);
				assert_equal(62, s1[52u]);
				assert_equal(83, s1[73u]);
				assert_equal(100, s1[90u]);

				assert_approx_equal(10.90f, s2[0u], 0.001f);
				assert_approx_equal(16.25f, s2[3u], 0.001f);
				assert_approx_equal(30.52f, s2[11u], 0.001f);
				assert_approx_equal(32.30f, s2[12u], 0.001f);
			}
		end_test_suite
	}
}
