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
			log_scale_ruler::tick_index make_tick_index(signed char order, unsigned char minor)
			{
				log_scale_ruler::tick_index v = {	order, minor	};
				return v;
			}

			struct eq
			{
				bool operator ()(float lhs, float rhs, float tolerance = 0.001) const
				{	return (!lhs && !rhs) || (fabs((lhs - rhs) / (lhs + rhs)) < tolerance);	}

				bool operator ()(ruler_tick lhs, ruler_tick rhs) const
				{	return (*this)(lhs.value, rhs.value) && lhs.type == rhs.type;	}
			};
		}

		begin_test_suite( LogScaleRulerTests )
			test( TickIndexIsRoundedToNextMajorOrMinorTickForAValue )
			{
				// ACT / ASSERT
				assert_equal(make_tick_index(4, 2), log_scale_ruler::tick_index::next(17123.0f));
				assert_equal(make_tick_index(1, 8), log_scale_ruler::tick_index::next(71.05f));
				assert_equal(make_tick_index(2, 2), log_scale_ruler::tick_index::next(150.0f));
				assert_equal(make_tick_index(-3, 4), log_scale_ruler::tick_index::next(0.003f));
				assert_equal(make_tick_index(-4, 2), log_scale_ruler::tick_index::next(0.0001f));
				assert_equal(make_tick_index(0, 2), log_scale_ruler::tick_index::next(1.0f));
				assert_equal(make_tick_index(0, 9), log_scale_ruler::tick_index::next(8.99f));
				assert_equal(make_tick_index(1, 1), log_scale_ruler::tick_index::next(9.0f));
				assert_equal(make_tick_index(2, 1), log_scale_ruler::tick_index::next(99.99f));
			}


			test( TickIndexIncrementsToTheNextMinorTick )
			{
				// INIT
				auto t1 = make_tick_index(3, 3);
				auto t2 = make_tick_index(-2, 7);

				// ACT / ASSERT
				assert_equal(make_tick_index(3, 4), ++t1);
				assert_equal(make_tick_index(3, 5), ++t1);
				assert_equal(make_tick_index(-2, 8), ++t2);
				assert_equal(make_tick_index(-2, 9), ++t2);
			}


			test( TickIndexIncrementsToTheNextMajorTick )
			{
				// INIT
				auto t1 = make_tick_index(3, 9);
				auto t2 = make_tick_index(-2, 9);

				// ACT / ASSERT
				assert_equal(make_tick_index(4, 1), ++t1);
				assert_equal(make_tick_index(4, 2), ++t1);
				assert_equal(make_tick_index(-1, 1), ++t2);
				assert_equal(make_tick_index(-1, 2), ++t2);
			}


			test( TickIndexValueIsReturned )
			{
				// ACT / ASSERT
				assert_approx_equal(9000.0f, make_tick_index(3, 9).value(), 0.0001);
				assert_approx_equal(10000.0f, make_tick_index(4, 1).value(), 0.0001);
				assert_approx_equal(0.5f, make_tick_index(-1, 5).value(), 0.0001);
				assert_approx_equal(0.1f, make_tick_index(-1, 1).value(), 0.0001);
			}


			test( OnlyTerminalTicksAreListedForLessThanOrderOfMagnituteRange )
			{
				// INIT
				log_scale<int> s(71, 79, 100);

				// INIT / ACT
				log_scale_ruler ds(s, 10);

				// ACT / ASSERT
				ruler_tick reference[] = {
					{	7.1f, ruler_tick::first	},
					{	7.9f, ruler_tick::last	},
				};

				assert_equal_pred(reference, ds, eq());
			}


			test( MinorTicksAreListedForLessThanOrderOfMagnituteRange )
			{
				// INIT
				log_scale<int> s1(7, 10, 100);

				// INIT / ACT
				log_scale_ruler ds1(s1, 1);

				// ACT / ASSERT
				ruler_tick reference1[] = {
					{	7.0f, ruler_tick::first	},
					{	8.0f, ruler_tick::minor	},
					{	9.0f, ruler_tick::minor	},
					{	10.0f, ruler_tick::last	},
				};

				assert_equal_pred(reference1, ds1, eq());

				// INIT
				log_scale<int> s2(60030, 99999, 100);

				// INIT / ACT
				log_scale_ruler ds2(s2, 100);

				// ACT / ASSERT
				ruler_tick reference2[] = {
					{	600.30f, ruler_tick::first	},
					{	700.00f, ruler_tick::minor	},
					{	800.00f, ruler_tick::minor	},
					{	900.00f, ruler_tick::minor	},
					{	999.99f, ruler_tick::last	},
				};

				assert_equal_pred(reference2, ds2, eq());
			}


			test( MinorAndMajorTicksAreListedForALongRange )
			{
				// INIT
				log_scale<int> s1(7, 25, 100);

				// INIT / ACT
				log_scale_ruler ds1(s1, 1);

				// ACT / ASSERT
				ruler_tick reference1[] = {
					{	7.0f, ruler_tick::first	},
					{	8.0f, ruler_tick::minor	},
					{	9.0f, ruler_tick::minor	},
					{	10.0f, ruler_tick::major	},
					{	20.0f, ruler_tick::minor	},
					{	25.0f, ruler_tick::last	},
				};

				assert_equal_pred(reference1, ds1, eq());

				// INIT
				log_scale<int> s2(6511, 641100, 100);

				// INIT / ACT
				log_scale_ruler ds2(s2, 100);

				// ACT / ASSERT
				ruler_tick reference2[] = {
					{	65.11f, ruler_tick::first	},
					{	70.0f, ruler_tick::minor	},
					{	80.0f, ruler_tick::minor	},
					{	90.0f, ruler_tick::minor	},
					{	100.0f, ruler_tick::major	},
					{	200.0f, ruler_tick::minor	},
					{	300.0f, ruler_tick::minor	},
					{	400.0f, ruler_tick::minor	},
					{	500.0f, ruler_tick::minor	},
					{	600.0f, ruler_tick::minor	},
					{	700.0f, ruler_tick::minor	},
					{	800.0f, ruler_tick::minor	},
					{	900.0f, ruler_tick::minor	},
					{	1000.0f, ruler_tick::major	},
					{	2000.0f, ruler_tick::minor	},
					{	3000.0f, ruler_tick::minor	},
					{	4000.0f, ruler_tick::minor	},
					{	5000.0f, ruler_tick::minor	},
					{	6000.0f, ruler_tick::minor	},
					{	6411.0f, ruler_tick::last	},
				};

				assert_equal_pred(reference2, ds2, eq());
			}

		end_test_suite
	}
}
