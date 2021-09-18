#include <common/histogram.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( ScaleTests )
			test( LinearScaleGivesBoundariesForOutOfDomainValues )
			{
				// INIT
				scale s1(15901, 1000000, 3);
				scale s2(9000, 10000, 10);

				// ACT / ASSERT
				assert_equal(3u, s1.segments());
				assert_equal(0u, s1(0));
				assert_equal(0u, s1(15900));
				assert_equal(0u, s1(15901));
				assert_equal(2u, s1(1000000));
				assert_equal(2u, s1(1000001));
				assert_equal(2u, s1(2010001));

				assert_equal(10u, s2.segments());
				assert_equal(0u, s2(0));
				assert_equal(0u, s2(8000));
				assert_equal(0u, s2(9000));
				assert_equal(9u, s2(10000));
				assert_equal(9u, s2(10001));
				assert_equal(9u, s2(2010001));
			}


			test( LinearScaleProvidesIndexForAValueWithBoundariesAtTheMiddleOfRange )
			{
				// INIT
				scale s1(1000, 3000, 3);
				scale s2(10, 710, 10);

				// ACT / ASSERT
				assert_equal(0u, s1(1499));
				assert_equal(1u, s1(1500));
				assert_equal(1u, s1(2499));
				assert_equal(2u, s1(2500));

				assert_equal(0u, s2(48));
				assert_equal(1u, s2(49));
				assert_equal(3u, s2(282));
				assert_equal(4u, s2(283));
				assert_equal(8u, s2(671));
				assert_equal(9u, s2(672));
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


		begin_test_suite( HistogramTests )
			test( HistogramIsResizedOnSettingScale )
			{
				// INIT
				histogram h;

				// INIT / ACT
				h.set_scale(scale(0, 90, 10));

				// ACT / ASSERT
				assert_equal(10u, h.size());
				assert_equal(10, distance(h.begin(), h.end()));
				assert_equal(scale(0, 90, 10), h.get_scale());

				// INIT / ACT
				h.set_scale(scale(3, 91, 17));

				// ACT / ASSERT
				assert_equal(17u, h.size());
				assert_equal(17, distance(h.begin(), h.end()));
				assert_equal(scale(3, 91, 17), h.get_scale());
			}


			test( CountsAreIncrementedAtExpectedBins )
			{
				// INIT
				histogram h;

				h.set_scale(scale(0, 90, 10));

				// ACT
				h.add(4);
				h.add(3);
				h.add(5);
				h.add(85);

				// ASSERT
				unsigned reference1[] = {	2, 1, 0, 0, 0, 0, 0, 0, 0, 1,	};

				assert_equal(reference1, h);

				// ACT
				h.add(50);
				h.add(51);
				h.add(81);
				h.add(81);
				h.add(81);

				// ASSERT
				unsigned reference2[] = {	2, 1, 0, 0, 0, 2, 0, 0, 3, 1,	};

				assert_equal(reference2, h);
			}


			test( HistogramIsResetAtRescale )
			{
				// INIT
				histogram h;
				h.set_scale(scale(0, 900, 5));

				h.add(10);
				h.add(11);
				h.add(9);
				h.add(800);

				// ACT
				h.set_scale(scale(0, 90, 7));

				// ASSERT
				unsigned reference[] = {	0, 0, 0, 0, 0, 0, 0,	};

				assert_equal(reference, h);
			}


			test( HistogramIsResetOnAddingDifferentlyScaledRhs )
			{
				// INIT
				histogram h, addition;

				h.set_scale(scale(0, 900, 5));
				addition.set_scale(scale(10, 900, 5));

				h.add(10), h.add(11), h.add(9), h.add(800);

				// ACT
				assert_equal(&h, &(h += addition));

				// ASSERT
				unsigned reference1[] = {	0, 0, 0, 0, 0,	};

				assert_equal(reference1, h);
				assert_equal(scale(10, 900, 5), h.get_scale());

				// INIT
				h.add(190);
				addition.set_scale(scale(10, 900, 12));
				addition.add(50);
				addition.add(750);

				// ACT
				h += addition;

				// ASSERT
				unsigned reference2[] = {	1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,	};

				assert_equal(reference2, h);
				assert_equal(scale(10, 900, 12), h.get_scale());
			}


			test( HistogramIsAddedToTheEquallyScaledValue )
			{
				// INIT
				histogram h, addition;

				h.set_scale(scale(0, 10, 11));
				addition.set_scale(scale(0, 10, 11));

				h.add(2);
				h.add(7);
				addition.add(1), addition.add(1);
				addition.add(7), addition.add(7), addition.add(7);
				addition.add(8);

				// ACT
				h += addition;

				// ASSERT
				unsigned reference[] = {	0, 2, 1, 0, 0, 0, 0, 4, 1, 0, 0	};

				assert_equal(reference, h);
			}


			test( DefaultConstructedHistogramIsWorkable )
			{
				// INIT / ACT
				histogram h;

				// ASSERT
				assert_equal(scale(0, 1, 2), h.get_scale());
				assert_equal(2u, h.size());
			}
		end_test_suite
	}
}
