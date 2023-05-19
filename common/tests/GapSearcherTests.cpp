#include <common/gap_searcher.h>

#include <common/types.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( GapSearcherTests )
			//test( RegionPastRequestedIsReturnedForEmptyMap )
			//{
			//	// INIT
			//	vector< pair<int, int> > m1;
			//	vector< pair<byte *, int> > m2;

			//	// ACT
			//	int r1 = gap_search(m1, make_pair(10, 100), 53 /*size*/, 13 /*max_distance_bits*/, location_length_pair_less());
			//	byte *r2 = gap_search(m2, make_pair((byte *)0x11391100, 16384), 1 << 16, 18, location_length_pair_less());

			//	// ASSERT
			//	assert_equal(110, r1);
			//	assert_equal((byte *)(0x11391100 + 16384), r2);
			//}


			test( GapSearchUpReturnsFirstMatchNoGaps )
			{
				// INIT
				int location;
				vector< pair<int, int> > m;

				// ACT / ASSERT
				location = 110;
				assert_is_true(gap_search_up(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(110, location);

				// INIT
				m.push_back(make_pair(110, 103));

				// ACT / ASSERT
				location = 110;
				assert_is_true(gap_search_up(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(213, location);

				// INIT
				m.push_back(make_pair(213, 1000));

				// ACT
				location = 110;
				assert_is_true(gap_search_up(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(1213, location);

				// ACT
				location = numeric_limits<int>::max() - 115;
				assert_is_true(gap_search_up(location, m, 116 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(numeric_limits<int>::max() - 115, location);
			}


			test( GapSearchUpFailsOnOverflow )
			{
				// ACT / ASSERT
				auto ilocation = numeric_limits<unsigned>::max() - 49;
				assert_is_false(gap_search_up(ilocation, vector< pair<unsigned, unsigned> >(), 51u /*size*/, location_length_pair_less()));

				auto plocation = (byte *)(numeric_limits<size_t>::max() - 50);
				assert_is_false(gap_search_up(plocation, vector< pair<byte *, unsigned> >(), 52u /*size*/, location_length_pair_less()));
			}


			test( GapSearchUpReturnsFirstMatchNoGapsPtr )
			{
				// INIT
				byte *location;
				vector< pair<byte *, int> > m;

				// ACT / ASSERT
				location = (byte *)110;
				assert_is_true(gap_search_up(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)110, location);

				// INIT
				m.push_back(make_pair((byte *)110, 103));

				// ACT / ASSERT
				location = (byte *)110;
				assert_is_true(gap_search_up(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)213, location);

				// INIT
				m.push_back(make_pair((byte *)213, 1000));

				// ACT
				location = (byte *)110;
				assert_is_true(gap_search_up(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)1213, location);

				// ACT
				location = (byte *)(numeric_limits<size_t>::max() - 115);
				assert_is_true(gap_search_up(location, m, 116 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)(numeric_limits<size_t>::max() - 115), location);
			}


			test( GapSearchUpReturnsFirstMatchWithGaps )
			{
				// INIT
				int location;
				auto m = plural
					+ make_pair(1000, 100)
					+ make_pair(1150, 230)
					+ make_pair(1480, 520)
					+ make_pair(2200, 230);

				// ACT / ASSERT
				location = 999;
				assert_is_true(gap_search_up(location, m, 1 /*size*/, location_length_pair_less()));
				assert_equal(999, location);
				location = 999;
				assert_equal(1100, (gap_search_up(location, m, 2 /*size*/, location_length_pair_less()), location));
				location = 999;
				assert_equal(1100, (gap_search_up(location, m, 50 /*size*/, location_length_pair_less()), location));
				location = 999;
				assert_equal(1380, (gap_search_up(location, m, 51 /*size*/, location_length_pair_less()), location));
				location = 999;
				assert_equal(1380, (gap_search_up(location, m, 100 /*size*/, location_length_pair_less()), location));
				location = 999;
				assert_equal(2000, (gap_search_up(location, m, 101 /*size*/, location_length_pair_less()), location));
				location = 999;
				assert_equal(2000, (gap_search_up(location, m, 200 /*size*/, location_length_pair_less()), location));
				location = 999;
				assert_equal(2430, (gap_search_up(location, m, 201 /*size*/, location_length_pair_less()), location));

				location = 1100;
				assert_equal(1100, (gap_search_up(location, m, 1 /*size*/, location_length_pair_less()), location));
				location = 1100;
				assert_equal(1100, (gap_search_up(location, m, 2 /*size*/, location_length_pair_less()), location));
				location = 1100;
				assert_equal(1100, (gap_search_up(location, m, 50 /*size*/, location_length_pair_less()), location));
				location = 1100;
				assert_equal(1380, (gap_search_up(location, m, 51 /*size*/, location_length_pair_less()), location));
				location = 1100;
				assert_equal(2430, (gap_search_up(location, m, 201 /*size*/, location_length_pair_less()), location));
				location = 1100;
				assert_is_true(gap_search_up(location, m, 201 /*size*/, location_length_pair_less()));
				assert_equal(2430, location);
			}


			test( GapSearchDownReturnsFirstMatchNoGaps )
			{
				// INIT
				int location;
				vector< pair<int, int> > m;

				// ACT
				location = 1000;
				assert_is_true(gap_search_down(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(947, location);

				// INIT
				m.insert(m.begin(), make_pair(910, 90));

				// ACT
				location = 1000;
				assert_is_true(gap_search_down(location, m, 73 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(837, location);

				// INIT
				m.insert(m.begin(), make_pair(760, 150));

				// ACT
				location = 1000;
				assert_is_true(gap_search_down(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(707, location);

				// ACT
				location = numeric_limits<int>::min() + 760;
				assert_is_true(gap_search_down(location, m, 760 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(numeric_limits<int>::min(), location);
			}


			test( GapSearchDownFailsOnOverflow )
			{
				// ACT / ASSERT
				auto ilocation = numeric_limits<unsigned>::min() + 50;
				assert_is_false(gap_search_down(ilocation, vector< pair<unsigned, int> >(), 51 /*size*/, location_length_pair_less()));

				auto plocation = (byte *)57;
				assert_is_false(gap_search_down(plocation, vector< pair<byte *, int> >(), 58 /*size*/, location_length_pair_less()));
			}


			test( GapSearchDownReturnsFirstMatchNoGapsPtr )
			{
				// INIT
				byte *location;
				vector< pair<byte *, int> > m;

				// ACT
				location = (byte *)1000;
				assert_is_true(gap_search_down(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)947, location);

				// INIT
				m.insert(m.begin(), make_pair((byte *)910, 90));

				// ACT
				location = (byte *)1000;
				assert_is_true(gap_search_down(location, m, 73 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)837, location);

				// INIT
				m.insert(m.begin(), make_pair((byte *)760, 150));

				// ACT
				location = (byte *)1000;
				assert_is_true(gap_search_down(location, m, 53 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal((byte *)707, location);

				// ACT
				location = (byte *)760;
				assert_is_true(gap_search_down(location, m, 760 /*size*/, location_length_pair_less()));

				// ASSERT
				assert_equal(nullptr, location);
			}


			test( GapSearchDownReturnsFirstMatchWithGaps )
			{
				// INIT
				int location;
				auto m = plural
					+ make_pair(10000, 500) // followed by a 1000-units gap
					+ make_pair(11500, 3100) // followed by a 200-units gap
					+ make_pair(14800, 7100); // followed by a 100-units gap
//					+ make_pair(22000, 230);

				// ACT / ASSERT
				location = 22000;
				assert_is_true(gap_search_down(location, m, 0 /*size*/, location_length_pair_less()));
				assert_equal(22000, location);
				location = 22000;
				assert_equal(21999, (gap_search_down(location, m, 1 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(21998, (gap_search_down(location, m, 2 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(21900, (gap_search_down(location, m, 100 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(14699, (gap_search_down(location, m, 101 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(14600, (gap_search_down(location, m, 200 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(11299, (gap_search_down(location, m, 201 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(10500, (gap_search_down(location, m, 1000 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_is_true(gap_search_down(location, m, 1001 /*size*/, location_length_pair_less()));
				assert_equal(8999, location);

				// INIT
				m = m + make_pair(22000, 230);

				// ACT / ASSERT
				location = 22000;
				assert_equal(22000, (gap_search_down(location, m, 0 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(21999, (gap_search_down(location, m, 1 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(21998, (gap_search_down(location, m, 2 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(21900, (gap_search_down(location, m, 100 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(14699, (gap_search_down(location, m, 101 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(14600, (gap_search_down(location, m, 200 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(11299, (gap_search_down(location, m, 201 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(10500, (gap_search_down(location, m, 1000 /*size*/, location_length_pair_less()), location));
				location = 22000;
				assert_equal(8999, (gap_search_down(location, m, 1001 /*size*/, location_length_pair_less()), location));

				location = 14900;
				assert_equal(14799, (gap_search_down(location, m, 1 /*size*/, location_length_pair_less()), location));
				location = 14900;
				assert_equal(14798, (gap_search_down(location, m, 2 /*size*/, location_length_pair_less()), location));
				location = 14900;
				assert_equal(14700, (gap_search_down(location, m, 100 /*size*/, location_length_pair_less()), location));
			}


//			test( SearchDownReturns )

		end_test_suite
	}

}
