#include <math/scale_ruler.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		namespace
		{
			struct eq
			{
				bool operator ()(float lhs, float rhs, float tolerance = 0.001) const
				{	return (!lhs && !rhs) || (fabs((lhs - rhs) / (lhs + rhs)) < tolerance);	}

				bool operator ()(ruler_tick lhs, ruler_tick rhs) const
				{	return (*this)(lhs.value, rhs.value) && lhs.type == rhs.type;	}
			};
		}

		begin_test_suite( LinearScaleRulerTests )
			test( RoundTickValueIsCalculatedForNextTickPosition )
			{
				// ACT / ASSERT
				assert_approx_equal(17200.0f, linear_scale_ruler::next_tick(17123.0f, 100.0f), 0.001);
				assert_approx_equal(18000.0f, linear_scale_ruler::next_tick(17123.0f, 1000.0f), 0.001);
				assert_approx_equal(71.10f, linear_scale_ruler::next_tick(71.05f, 0.1f), 0.001);
				assert_approx_equal(71.20f, linear_scale_ruler::next_tick(71.20f, 0.1f), 0.001);
				assert_approx_equal(-0.017f, linear_scale_ruler::next_tick(-0.0171f, 0.001f), 0.001);
				assert_approx_equal(-0.016f, linear_scale_ruler::next_tick(-0.0169f, 0.001f), 0.001);
				assert_approx_equal(0.0f, linear_scale_ruler::next_tick(0.0f, 0.01f), 0.001);
			}


			test( MajorTickValueIsLargestPowerOfTenMetAtLeastTwiceInDelta )
			{
				// ACT / ASSERT
				assert_approx_equal(10000.0f, linear_scale_ruler::major_tick(17123.0f), 0.001);
				assert_approx_equal(10000.0f, linear_scale_ruler::major_tick(27123.0f), 0.001);
				assert_approx_equal(0.001f, linear_scale_ruler::major_tick(0.003f), 0.001);
				assert_approx_equal(0.001f, linear_scale_ruler::major_tick(0.0033f), 0.001);
				assert_approx_equal(0.001f, linear_scale_ruler::major_tick(0.0010001f), 0.001);
				assert_approx_equal(0.01f, linear_scale_ruler::major_tick(0.099999f), 0.001);
				assert_equal(0.0f, linear_scale_ruler::major_tick(0.0f));
			}


			test( LinearScaleTicksAreListedAccordinglyToTheRange )
			{
				// INIT
				linear_scale<int> s1(7, 37, 100);

				// INIT / ACT
				linear_scale_ruler ds1(s1, 1);

				// ACT / ASSERT
				ruler_tick reference1[] = {
					{	7.0f, ruler_tick::first	},
					{	10.0f, ruler_tick::major	},
					{	20.0f, ruler_tick::major	},
					{	30.0f, ruler_tick::major	},
					{	37.0f, ruler_tick::last	},
				};

				assert_equal_pred(reference1, ds1, eq());

				// INIT
				linear_scale<int> s2(70030, 70650, 100);

				// INIT / ACT
				linear_scale_ruler ds2(s2, 1);

				// ACT / ASSERT
				ruler_tick reference2[] = {
					{	70030.0f, ruler_tick::first	},
					{	70100.0f, ruler_tick::major	},
					{	70200.0f, ruler_tick::major	},
					{	70300.0f, ruler_tick::major	},
					{	70400.0f, ruler_tick::major	},
					{	70500.0f, ruler_tick::major	},
					{	70600.0f, ruler_tick::major	},
					{	70650.0f, ruler_tick::last	},
				};

				assert_equal_pred(reference2, ds2, eq());
			}


			test( EmptyScaleIteratesAsEmpty )
			{
				// INIT
				linear_scale<int> s1;

				// INIT / ACT
				linear_scale_ruler ds1(s1, 1);

				// ACT / ASSERT
				assert_equal(ds1.end(), ds1.begin());

				// INIT
				linear_scale<int> s2(7110, 17111, 0);

				// INIT / ACT
				linear_scale_ruler ds2(s2, 1);

				// ACT / ASSERT
				assert_equal(ds2.end(), ds2.begin());
			}


			test( MultiplierIsAppliedToRange )
			{
				// INIT
				linear_scale<int> s(110, 340, 100);

				// INIT / ACT
				linear_scale_ruler ds(s, 450); // [0.2(4), 0.7(5)]

				// ACT / ASSERT
				ruler_tick reference[] = {
					{	110.0f / 450, ruler_tick::first	},
					{	0.3f, ruler_tick::major	},
					{	0.4f, ruler_tick::major	},
					{	0.5f, ruler_tick::major	},
					{	0.6f, ruler_tick::major	},
					{	0.7f, ruler_tick::major	},
					{	340.0f / 450, ruler_tick::last	},
				};

				assert_equal_pred(reference, ds, eq());
			}


			test( SpecialSingleSampledScaleIsSupported )
			{
				// INIT / ACT
				linear_scale<int> s(117, 345, 1);
				linear_scale_ruler ds1(s, 1);
				linear_scale_ruler ds2(s, 1);

				// ACT / ASSERT
				ruler_tick reference[] = {
					{	117.0f, ruler_tick::first	},
					{	200.0f, ruler_tick::major	},
					{	300.0f, ruler_tick::major	},
					{	345.0f, ruler_tick::last	},
				};

				assert_equal_pred(reference, ds1, eq());
				assert_equal_pred(reference, ds2, eq());
			}


			test( DisplayScaleForEmptyScaleIsEmpty )
			{
				// INIT / ACT
				linear_scale<int> s;
				linear_scale_ruler ds1(s, 1);
				linear_scale_ruler ds2(s, 1);

				// ASSERT
				assert_equal(ds1.end(), ds1.begin());
				assert_equal(ds2.end(), ds2.begin());
			}

		end_test_suite
	}
}
