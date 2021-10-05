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
			index_t cvt(const ScaleT &scale_, value_t value)
			{
				index_t index;

				scale_(index, value);
				return index;
			}
		}

		begin_test_suite( ScaleTests )
			test( DefaultScaleIsEmpty )
			{
				// INIT / ACT
				scale s;
				index_t index;

				// ACT / ASSERT
				assert_equal(0u, s.samples());
				assert_is_false(s(index, 0));
				assert_is_false(s(index, 0xFFFFFFF));
				assert_equal(scale(0, 0, 0), s);
			}


			test( ASingleSampledScaleAlwaysEvaluatesToZero )
			{
				// INIT / ACT
				scale s1(100, 101, 1);
				scale s2(0, 0, 1);

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
				scale s1(15901, 1000000, 3);
				scale s2(9000, 10000, 10);

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
				scale s1(1000, 3000, 3);
				scale s2(10, 710, 10);

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
				scale s1(1000, 3000, 3);
				scale s11(1000, 3000, 3);
				scale s2(10, 710, 10);
				scale s21(10, 710, 10);
				scale s3(1000, 3000, 90);
				scale s4(1000, 3050, 3);
				scale s5(1001, 3000, 3);

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
		end_test_suite
	}
}
