#include <frontend/display_scale.h>

#include <common/histogram.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct eq
			{
				bool operator ()(float lhs, float rhs, float tolerance = 0.001) const
				{	return !lhs && !rhs || fabs((lhs - rhs) / (lhs + rhs)) < tolerance;	}

				bool operator ()(display_scale::tick lhs, display_scale::tick rhs) const
				{	return (*this)(lhs.value, rhs.value) && lhs.type == rhs.type;	}

				bool operator ()(pair<float, float> lhs, pair<float, float> rhs) const
				{	return (*this)(lhs.first, rhs.first) && (*this)(lhs.second, rhs.second);	}
			};
		}

		begin_test_suite( DisplayScaleTests )
			test( RoundTickValueIsCalculatedForNextTickPosition )
			{
				// ACT / ASSERT
				assert_approx_equal(17200.0f, display_scale::next_tick(17123.0f, 100.0f), 0.001);
				assert_approx_equal(18000.0f, display_scale::next_tick(17123.0f, 1000.0f), 0.001);
				assert_approx_equal(71.10f, display_scale::next_tick(71.05f, 0.1f), 0.001);
				assert_approx_equal(71.20f, display_scale::next_tick(71.20f, 0.1f), 0.001);
				assert_approx_equal(-0.017f, display_scale::next_tick(-0.0171f, 0.001f), 0.001);
				assert_approx_equal(-0.016f, display_scale::next_tick(-0.0169f, 0.001f), 0.001);
				assert_approx_equal(0.0f, display_scale::next_tick(0.0f, 0.01f), 0.001);
			}


			test( MajorTickValueIsLargestPowerOfTenMetAtLeastTwiceInDelta )
			{
				// ACT / ASSERT
				assert_approx_equal(1000.0f, display_scale::major_tick(17123.0f), 0.001);
				assert_approx_equal(10000.0f, display_scale::major_tick(27123.0f), 0.001);
				assert_approx_equal(0.001f, display_scale::major_tick(0.003f), 0.001);
				assert_approx_equal(0.001f, display_scale::major_tick(0.0033f), 0.001);
			}


			test( LinearScaleTicksAreListedAccordinglyToTheRange )
			{
				// INIT
				scale s1(7, 37, 100);

				// INIT / ACT
				display_scale ds1(s1, 1, 200);

				// ACT / ASSERT
				display_scale::tick reference1[] = {
					{	7.0f, display_scale::first	},
					{	10.0f, display_scale::major	},
					{	20.0f, display_scale::major	},
					{	30.0f, display_scale::major	},
					{	37.0f, display_scale::last	},
				};

				assert_equal_pred(reference1, ds1, eq());

				// INIT
				scale s2(70030, 70650, 100);

				// INIT / ACT
				display_scale ds2(s2, 1, 1000);

				// ACT / ASSERT
				display_scale::tick reference2[] = {
					{	70030.0f, display_scale::first	},
					{	70100.0f, display_scale::major	},
					{	70200.0f, display_scale::major	},
					{	70300.0f, display_scale::major	},
					{	70400.0f, display_scale::major	},
					{	70500.0f, display_scale::major	},
					{	70600.0f, display_scale::major	},
					{	70650.0f, display_scale::last	},
				};

				assert_equal_pred(reference2, ds2, eq());
			}


			test( InvalidScaleIteratesAsEmpty )
			{
				// INIT
				scale s1(7110, 7110, 100);

				// INIT / ACT
				display_scale ds1(s1, 1, 100);

				// ACT / ASSERT
				assert_equal(ds1.end(), ds1.begin());

				// INIT
				scale s2(7110, -1, 100);

				// INIT / ACT
				display_scale ds2(s2, 1, 100);

				// ACT / ASSERT
				assert_equal(ds2.end(), ds2.begin());
			}


			test( MultiplierIsAppliedToRange )
			{
				// INIT
				scale s(110, 340, 100);

				// INIT / ACT
				display_scale ds(s, 450, 100); // [0.2(4), 0.7(5)]

				// ACT / ASSERT
				display_scale::tick reference[] = {
					{	110.0f / 450, display_scale::first	},
					{	0.3f, display_scale::major	},
					{	0.4f, display_scale::major	},
					{	0.5f, display_scale::major	},
					{	0.6f, display_scale::major	},
					{	0.7f, display_scale::major	},
					{	340.0f / 450, display_scale::last	},
				};

				assert_equal_pred(reference, ds, eq());
			}


			test( SamplePositionsAreConvertedToDisplayRange )
			{
				// INIT
				scale s(110, 340, 19); // bin width: 12.(7)

				// INIT / ACT
				display_scale ds1(s, 1, 100); // bin pixel width: ~5.2632

				// ACT / ASSERT
				assert_equal_pred(make_pair(0.0f, 5.2632f), ds1.at(0), eq());
				assert_equal_pred(make_pair(21.0526f, 26.3158f), ds1.at(4), eq());
				assert_equal_pred(make_pair(94.7368f, 100.0f), ds1.at(18), eq());

				// INIT / ACT
				display_scale ds2(s, 1, 40); // bin pixel width: ~2.1053

				// ACT / ASSERT
				assert_equal_pred(make_pair(0.0f, 2.1053f), ds2.at(0), eq());
				assert_equal_pred(make_pair(8.4212f, 10.5265f), ds2.at(4), eq());
				assert_equal_pred(make_pair(37.8947f, 40.0f), ds2.at(18), eq());
			}


			test( ValueIsConvertedToDisplayCoordinateAccordinglyToScale )
			{
				// INIT
				scale s(110, 340, 19); // bin width: 12.(7)
				display_scale ds1(s, 1, 100);
				display_scale ds2(s, 1, 40);

				// ACT / ASSERT
				assert_approx_equal(2.6316f, ds1[110.f], 0.001);
				assert_approx_equal(44.2334f, ds1[211.f], 0.001);
				assert_approx_equal(97.3684f, ds1[340.f], 0.001);

				assert_approx_equal(1.0526f, ds2[110.f], 0.001);
				assert_approx_equal(17.6934f, ds2[211.f], 0.001);
				assert_approx_equal(38.9474f, ds2[340.f], 0.001);
			}
		end_test_suite
	}
}
