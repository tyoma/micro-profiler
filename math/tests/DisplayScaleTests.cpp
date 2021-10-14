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

		begin_test_suite( DisplayScaleTests )
			test( SamplePositionsAreConvertedToDisplayRange )
			{
				// INIT
				linear_scale<int> s(110, 340, 19); // bin width: 12.(7)

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
				linear_scale<int> s(110, 340, 19); // bin width: 12.(7)
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


			test( SpecialSingleSampledScaleIsSupported )
			{
				// INIT / ACT
				linear_scale<int> s(117, 345, 1);
				display_scale ds1(s, 1, 50);
				display_scale ds2(s, 1, 26);

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


			test( DisplayScaleForEmptyScaleIsEmpty )
			{
				// INIT / ACT
				linear_scale<int> s;
				display_scale ds1(s, 1, 50);
				display_scale ds2(s, 1, 26);

				// ASSERT
				assert_equal(0.0f, ds1[0.0f]);
				assert_equal(0.0f, ds1[200.0f]);
				assert_equal(0.0f, ds1[1000.0f]);
				assert_equal(0.0f, ds2[0.0f]);
				assert_equal(0.0f, ds2[200.0f]);
				assert_equal(0.0f, ds2[1000.0f]);
			}

		end_test_suite
	}
}
