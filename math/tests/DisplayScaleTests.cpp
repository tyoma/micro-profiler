#include <math/display_scale.h>

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
			struct eq
			{
				bool operator ()(float lhs, float rhs, float tolerance = 0.001) const
				{	return (!lhs && !rhs) || (fabs((lhs - rhs) / (lhs + rhs)) < tolerance);	}

				bool operator ()(pair<float, float> lhs, pair<float, float> rhs) const
				{	return (*this)(lhs.first, rhs.first) && (*this)(lhs.second, rhs.second);	}
			};
		}

		begin_test_suite( LinearDisplayScaleTests )
			test( SamplePositionsAreConvertedToDisplayRange )
			{
				// INIT
				linear_scale<int> s(110, 340, 19); // bin width: 12.(7)

				// INIT / ACT
				linear_display_scale ds1(s, 1, 100); // bin pixel width: ~5.2632

				// ACT / ASSERT
				assert_equal_pred(make_pair(0.0f, 5.2632f), ds1.at(0), eq());
				assert_equal_pred(make_pair(21.0526f, 26.3158f), ds1.at(4), eq());
				assert_equal_pred(make_pair(94.7368f, 100.0f), ds1.at(18), eq());

				// INIT / ACT
				linear_display_scale ds2(s, 1, 40); // bin pixel width: ~2.1053

				// ACT / ASSERT
				assert_equal_pred(make_pair(0.0f, 2.1053f), ds2.at(0), eq());
				assert_equal_pred(make_pair(8.4212f, 10.5265f), ds2.at(4), eq());
				assert_equal_pred(make_pair(37.8947f, 40.0f), ds2.at(18), eq());
			}


			test( ValueIsConvertedToDisplayCoordinateAccordinglyToScale )
			{
				// INIT
				linear_scale<int> s(110, 340, 19);
				linear_display_scale ds1(s, 1, 100);
				linear_display_scale ds2(s, 1, 40);

				// ACT / ASSERT
				assert_approx_equal(2.6316f, ds1[110.f], 0.001);
				assert_approx_equal(44.2334f, ds1[211.f], 0.001);
				assert_approx_equal(97.3684f, ds1[340.f], 0.001);

				assert_approx_equal(1.0526f, ds2[110.f], 0.001);
				assert_approx_equal(17.6934f, ds2[211.f], 0.001);
				assert_approx_equal(38.9474f, ds2[340.f], 0.001);
			}


			test( DivisorIsAppliedToValues )
			{
				// INIT
				linear_scale<int> s(110, 340, 19);
				linear_display_scale ds1(s, 10, 100);
				linear_display_scale ds2(s, 15, 100);

				// ACT / ASSERT
				assert_approx_equal(2.6316f, ds1[11.0f], 0.001);
				assert_approx_equal(44.2334f, ds1[21.1f], 0.001);
				assert_approx_equal(97.3684f, ds1[34.0f], 0.001);

				assert_approx_equal(2.6316f, ds2[7.3333f], 0.001);
				assert_approx_equal(44.2334f, ds2[14.067f], 0.001);
				assert_approx_equal(97.3684f, ds2[22.667f], 0.001);
			}


			test( SpecialSingleSampledScaleIsSupported )
			{
				// INIT / ACT
				linear_scale<int> s(117, 345, 1);
				linear_display_scale ds1(s, 1, 50);
				linear_display_scale ds2(s, 1, 26);

				// ACT / ASSERT
				assert_approx_equal(25.0f, ds1[0.0f], 0.001f);
				assert_approx_equal(25.0f, ds1[200.0f], 0.001f);
				assert_approx_equal(25.0f, ds1[1000.0f], 0.001f);
				assert_equal(make_pair(0.0f, 50.0f), ds1.at(0));
				assert_approx_equal(13.0f, ds2[0.0f], 0.001f);
				assert_approx_equal(13.0f, ds2[200.0f], 0.001f);
				assert_approx_equal(13.0f, ds2[1000.0f], 0.001f);
				assert_equal(make_pair(0.0f, 26.0f), ds2.at(0));
			}

		end_test_suite


		begin_test_suite( LogDisplayScaleTests )
			test( SamplePositionsAreConvertedToDisplayRange )
			{
				// INIT
				log_scale<int> s(110, 340, 19); // bin width: 12.(7)

				// INIT / ACT
				log_display_scale ds1(s, 1, 100); // bin pixel width: ~5.2632

				// ACT / ASSERT
				assert_equal_pred(make_pair(0.0f, 5.2632f), ds1.at(0), eq());
				assert_equal_pred(make_pair(21.0526f, 26.3158f), ds1.at(4), eq());
				assert_equal_pred(make_pair(94.7368f, 100.0f), ds1.at(18), eq());

				// INIT / ACT
				log_display_scale ds2(s, 1, 40); // bin pixel width: ~2.1053

				// ACT / ASSERT
				assert_equal_pred(make_pair(0.0f, 2.1053f), ds2.at(0), eq());
				assert_equal_pred(make_pair(8.4212f, 10.5265f), ds2.at(4), eq());
				assert_equal_pred(make_pair(37.8947f, 40.0f), ds2.at(18), eq());
			}


			test( ValueIsConvertedToDisplayCoordinateAccordinglyToScale )
			{
				// INIT
				log_scale<int> s1(110, 340, 19);
				log_scale<float> s2(110, 34000, 100);
				log_display_scale ds1(s1, 1, 100);
				log_display_scale ds2(s2, 1.0f, 400);

				// ACT / ASSERT
				assert_approx_equal(2.6316f, ds1[110.f], 0.001);
				assert_approx_equal(16.656f, ds1[130.f], 0.001);
				assert_approx_equal(57.316f, ds1[211.f], 0.001);
				assert_approx_equal(97.368f, ds1[340.f], 0.001);

				assert_approx_equal(2.0000f, ds2[110.f], 0.001);
				assert_approx_equal(46.988f, ds2[211.f], 0.001);
				assert_approx_equal(79.939f, ds2[340.f], 0.001);
				assert_approx_equal(258.32f, ds2[4500.f], 0.001);
				assert_approx_equal(398.00f, ds2[34000.f], 0.001);
			}


			test( DivisorIsAppliedToValues )
			{
				// INIT
				log_scale<int> s(110, 3400, 19);
				log_display_scale ds1(s, 10, 100);
				log_display_scale ds2(s, 15, 100);

				// ACT / ASSERT
				assert_approx_equal(2.6316f, ds1[11.0f], 0.001);
				assert_approx_equal(97.368f, ds1[340.0f], 0.001);

				assert_approx_equal(2.6316f, ds2[7.3333f], 0.001);
				assert_approx_equal(97.368f, ds2[226.67f], 0.001);
			}


			test( SpecialSingleSampledScaleIsSupported )
			{
				// INIT / ACT
				log_scale<int> s(117, 345, 1);
				log_display_scale ds1(s, 1, 50);
				log_display_scale ds2(s, 1, 26);

				// ACT / ASSERT
				assert_approx_equal(25.0f, ds1[0.0f], 0.001f);
				assert_approx_equal(25.0f, ds1[200.0f], 0.001f);
				assert_approx_equal(25.0f, ds1[1000.0f], 0.001f);
				assert_equal(make_pair(0.0f, 50.0f), ds1.at(0));
				assert_approx_equal(13.0f, ds2[0.0f], 0.001f);
				assert_approx_equal(13.0f, ds2[200.0f], 0.001f);
				assert_approx_equal(13.0f, ds2[1000.0f], 0.001f);
				assert_equal(make_pair(0.0f, 26.0f), ds2.at(0));
			}

		end_test_suite

	}
}
