#include <math/histogram.h>

#include <math/scale.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		namespace
		{
			using namespace micro_profiler::tests;

			template <typename T, typename Y>
			struct partition
			{
				Y midvalue;
				T location;
			};

			struct eq
			{
				bool operator ()(float lhs, float rhs, float tolerance = 0.001) const
				{	return (!lhs && !rhs) || (fabs((lhs - rhs) / (lhs + rhs)) < tolerance);	}

				template <typename T, typename Y>
				bool operator ()(partition<T, Y> lhs, partition<T, Y> rhs) const
				{	return lhs.midvalue == rhs.midvalue && (*this)(lhs.location, rhs.location);	}
			};
		}

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


			test( EvenPartitionsAreFoundAtExactLocations )
			{
				// INIT
				histogram<linear_scale<float>, int> h;
				partition<float, int> p;

				h.set_scale(linear_scale<float>(0.0f, 10.0f, 11));
				h.add(0.0f), h.add(1.0f), h.add(2.0f), h.add(3.0f), h.add(4.0f);
				h.add(5.0f), h.add(6.0f), h.add(7.0f), h.add(8.0f), h.add(9.0f), h.add(10.0f);

				// ACT
				p.midvalue = 1;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(0.0f, p.location, 0.001);

				// ACT
				p.midvalue = 7;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(6.0f, p.location, 0.001);

				// ACT
				p.midvalue = 11;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(10.0f, p.location, 0.001);

				// INIT
				h.add(3.0f, 5);
				h.add(5.0f, 7);

				// ACT
				p.midvalue = 9;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(3.0f, p.location, 0.001);

				// ACT
				p.midvalue = 10;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(4.0f, p.location, 0.001);

				// ACT
				p.midvalue = 18;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(5.0f, p.location, 0.001);

				// ACT
				p.midvalue = 19;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(6.0f, p.location, 0.001);
			}


			test( PartitionPositionsAreInterpolatedWhenInBetweenTwoBins )
			{
				// INIT
				histogram<linear_scale<float>, int> h;
				partition<float, int> p;

				h.set_scale(linear_scale<float>(10.0f, 100.0f, 10));

				h.add(20.0f, 14);
				h.add(30.0f, 17);

				// ACT
				p.midvalue = 15;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(20.59f, p.location, 0.001);

				// ACT
				p.midvalue = 25;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(26.47f, p.location, 0.001);

				// ACT
				p.midvalue = 30;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(29.41f, p.location, 0.001);
			}


			test( PartitionPositionsAreInterpolatedWhenInBetweenTwoDistantBins )
			{
				// INIT
				histogram<linear_scale<float>, int> h;
				partition<float, int> p;

				h.set_scale(linear_scale<float>(10.0f, 100.0f, 10));

				h.add(30.0f, 14);
				h.add(90.0f, 17);

				// ACT
				p.midvalue = 15;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(33.53f, p.location, 0.001);

				// ACT
				p.midvalue = 25;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(68.82f, p.location, 0.001);

				// ACT
				p.midvalue = 30;
				h.find_partitions(&p, &p + 1);

				// ASSERT
				assert_approx_equal(86.47f, p.location, 0.001);
			}


			test( MultipleSortedPartitionsAreProcessedDuringASingleCall )
			{
				// INIT
				histogram<linear_scale<float>, int> h;

				h.set_scale(linear_scale<float>(10.0f, 100.0f, 10));

				h.add(30.0f, 14);
				h.add(90.0f, 17);

				// ACT
				partition<float, int> p1[] = {	{	15,	}, {	25,	},	};

				h.find_partitions(p1, p1 + 2);

				// ASSERT
				partition<float, int> reference1[] = {	{	15, 33.53f	}, {	25, 68.82f	},	};

				assert_equal_pred(reference1, mkvector(p1), eq());

				// ACT
				partition<float, int> p2[] = {	{	15,	}, {	25,	}, {	30,	},	};

				h.find_partitions(p2, p2 + 3);

				// ASSERT
				partition<float, int> reference2[] = {	{	15, 33.53f	}, {	25, 68.82f	}, {	30, 86.47f	},	};

				assert_equal_pred(reference2, mkvector(p2), eq());

				// ACT
				partition<float, int> p3[] = {	{	15,	}, {	25,	}, {	25,	}, {	29,	}, {	30,	},	};

				h.find_partitions(p3, p3 + 5);

				// ASSERT
				partition<float, int> reference3[] = {	{	15, 33.53f	}, {	25, 68.82f	}, {	25, 68.82f	}, {	29, 82.94f	}, {	30, 86.47f	},	};

				assert_equal_pred(reference3, mkvector(p3), eq());
			}


			test( PartitionsOutsideRangeAreReportedOnBoundary )
			{
				// INIT
				histogram<linear_scale<float>, int> h;

				h.set_scale(linear_scale<float>(10.0f, 100.0f, 10));

				h.add(30.0f, 14);
				h.add(90.0f, 17);

				// ACT
				partition<float, int> p1[] = {	{	11,	}, {	14,	},	};

				h.find_partitions(p1, p1 + 2);

				// ASSERT
				partition<float, int> reference1[] = {	{	11, 30.00f	}, {	14, 30.00f	},	};

				assert_equal_pred(reference1, mkvector(p1), eq());

				// ACT
				partition<float, int> p2[] = {	{	11,	}, {	15,	}, {	31,	}, {	100,	},	};

				h.find_partitions(p2, p2 + 4);

				// ASSERT
				partition<float, int> reference2[] = {	{	11, 30.00f	}, {	15, 33.53f	}, {	31, 90.00f	}, {	100, 90.00f	},	};

				assert_equal_pred(reference2, mkvector(p2), eq());
			}


			test( UnsortedParitionsAreSortedAndLocationsAreReturned )
			{
				// INIT
				histogram<linear_scale<float>, int> h;

				h.set_scale(linear_scale<float>(10.0f, 100.0f, 10));

				h.add(30.0f, 14);
				h.add(90.0f, 17);

				// ACT
				partition<float, int> p[] = {	{	30,	}, {	25,	}, {	29,	}, {	25,	}, {	15,	},	};

				h.find_partitions(p, p + 5);

				// ASSERT
				partition<float, int> reference[] = {	{	15, 33.53f	}, {	25, 68.82f	}, {	25, 68.82f	}, {	29, 82.94f	}, {	30, 86.47f	},	};

				assert_equal_pred(reference, mkvector(p), eq());
			}


			test( PartitionsOfAResetHistogramAreAllEqualToFarBoundary )
			{
				// INIT
				histogram<linear_scale<float>, int> h;

				h.set_scale(linear_scale<float>(10.0f, 100.0f, 10));

				// ACT
				partition<float, int> p[] = {	{	30,	}, {	25,	},	};

				h.find_partitions(p, p + 2);

				// ASSERT
				partition<float, int> reference[] = {	{	25, 100.0f	}, {	30, 100.0f	},	};

				assert_equal_pred(reference, mkvector(p), eq());
			}
		end_test_suite
	}
}
