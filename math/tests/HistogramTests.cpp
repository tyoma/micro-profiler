#include <math/histogram.h>

#include <math/scale.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		typedef linear_scale<long long> scale_;
		typedef histogram<scale_, long long> histogram_;

		begin_test_suite( HistogramTests )
			test( HistogramIsResizedOnSettingScale )
			{
				// INIT
				histogram_ h;

				// INIT / ACT
				h.set_scale(scale_(0, 90, 10));

				// ACT / ASSERT
				assert_equal(10u, h.size());
				assert_equal(10, distance(h.begin(), h.end()));
				assert_equal(scale_(0, 90, 10), h.get_scale());

				// INIT / ACT
				h.set_scale(scale_(3, 91, 17));

				// ACT / ASSERT
				assert_equal(17u, h.size());
				assert_equal(17, distance(h.begin(), h.end()));
				assert_equal(scale_(3, 91, 17), h.get_scale());
			}


			test( CountsAreIncrementedAtExpectedBins )
			{
				// INIT
				histogram_ h;

				h.set_scale(scale_(0, 90, 10));

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
				h.add(81, 3);

				// ASSERT
				unsigned reference2[] = {	2, 1, 0, 0, 0, 2, 0, 0, 3, 1,	};

				assert_equal(reference2, h);
			}


			test( HistogramIsResetAtRescale )
			{
				// INIT
				histogram_ h;
				h.set_scale(scale_(0, 900, 5));

				h.add(10);
				h.add(11);
				h.add(9);
				h.add(800);

				// ACT
				h.set_scale(scale_(0, 90, 7));

				// ASSERT
				unsigned reference[] = {	0, 0, 0, 0, 0, 0, 0,	};

				assert_equal(reference, h);
			}


			test( HistogramIsResetOnAddingDifferentlyScaledRhs )
			{
				// INIT
				histogram_ h, addition;

				h.set_scale(scale_(0, 900, 5));
				addition.set_scale(scale_(10, 900, 5));

				h.add(10), h.add(11), h.add(9), h.add(800);

				// ACT / ASSERT
				assert_equal(&h, &(h += addition));

				// ASSERT
				unsigned reference1[] = {	0, 0, 0, 0, 0,	};

				assert_equal(reference1, h);
				assert_equal(scale_(10, 900, 5), h.get_scale());

				// INIT
				h.add(190);
				addition.set_scale(scale_(10, 900, 12));
				addition.add(50);
				addition.add(750);

				// ACT
				h += addition;

				// ASSERT
				unsigned reference2[] = {	1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,	};

				assert_equal(reference2, h);
				assert_equal(scale_(10, 900, 12), h.get_scale());
			}


			test( HistogramIsResetOnInterpolatingWithADifferentlyScaledOne )
			{
				// INIT
				histogram_ h, addition;

				h.set_scale(scale_(0, 900, 5));
				addition.set_scale(scale_(10, 900, 5));

				h.add(10), h.add(11), h.add(9), h.add(800);

				// ACT
				interpolate(h, addition, 1.0f);

				// ASSERT
				unsigned reference1[] = {	0, 0, 0, 0, 0,	};

				assert_equal(reference1, h);
				assert_equal(scale_(10, 900, 5), h.get_scale());

				// INIT
				h.add(190);
				addition.set_scale(scale_(10, 900, 12));
				addition.add(50);
				addition.add(750);

				// ACT
				interpolate(h, addition, 1.0f);

				// ASSERT
				unsigned reference2[] = {	1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,	};

				assert_equal(reference2, h);
				assert_equal(scale_(10, 900, 12), h.get_scale());
			}


			test( HistogramIsAddedToTheEquallyScaledValue )
			{
				// INIT
				histogram_ h, addition;

				h.set_scale(scale_(0, 10, 11));
				addition.set_scale(scale_(0, 10, 11));

				h.add(2);
				h.add(7);
				addition.add(1, 2);
				addition.add(7, 3);
				addition.add(8);

				// ACT
				h += addition;

				// ASSERT
				unsigned reference[] = {	0, 2, 1, 0, 0, 0, 0, 4, 1, 0, 0	};

				assert_equal(reference, h);
			}


			test( HistogramValuesAreInterpolatedAsRequested )
			{
				// INIT
				histogram_ h, addition;

				h.set_scale(scale_(0, 5, 6));
				addition.set_scale(scale_(0, 5, 6));

				addition.add(0, 100);
				addition.add(2, 51);
				addition.add(3, 117);

				// ACT
				interpolate(h, addition, 7.0f / 256);

				// ASSERT
				unsigned reference1[] = {	2, 0, 1, 3, 0, 0,	};

				assert_equal(reference1, h);

				// INIT
				const auto addition2 = h;

				// ACT
				interpolate(h, addition, 1.0f);

				// ASSERT
				unsigned reference2[] = {	100, 0, 51, 117, 0, 0,	};

				assert_equal(reference2, h);

				// ACT
				interpolate(h, addition2, 0.5f);

				// ASSERT
				unsigned reference3[] = {	51, 0, 26, 60, 0, 0,	};

				assert_equal(reference3, h);
			}


			test( DefaultConstructedHistogramEmptyButAcceptsValues )
			{
				// INIT / ACT
				histogram_ h;

				// ACT / ASSERT
				assert_equal(scale_(), h.get_scale());
				assert_equal(0u, h.size());

				// ACT / ASSERT
				h.add(0);
				h.add(1000000);
			}


			test( ResettingHistogramPreservesScaleAndSizeButSetsValuesToZeroes )
			{
				// INIT / ACT
				histogram_ h;

				h.set_scale(scale_(0, 9, 10));
				h.add(2, 9);
				h.add(7, 3);

				// ACT
				h.reset();

				// ASSERT
				unsigned reference[] = {	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	};

				assert_equal(reference, h);
			}
		end_test_suite
	}
}
