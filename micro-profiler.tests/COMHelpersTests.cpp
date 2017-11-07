#include <common/fb_helpers.h>

#include "Helpers.h"

#include <algorithm>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( FlatBuffersHelpersTests )
			flatbuffers::FlatBufferBuilder fbuilder;

			test( InplaceAdditionReturnsConstRefToLHS )
			{
				// INIT
				function_statistics destination(13, 0, 50000700001, 13002, 0);
				FunctionStatistics addendum(0, 19, 0, 23, 31, 0);

				// ACT
				const function_statistics *result = &(destination += addendum);

				// ASSERT
				assert_equal(&destination, result);
			}


			test( InplaceAddingStatisticsToInternalAddsTimesCalledAndInclusiveExclusiveTimes )
			{
				// INIT
				function_statistics destination[] = {
					function_statistics(0, 0, 0, 0, 0),
					function_statistics(1, 0, 10001, 3002, 0),
					function_statistics(13, 0, 50000700001, 13002, 0),
					function_statistics(131, 0, 12345678, 345678, 0),
				};
				FunctionStatistics addendum[] = {
					// address, times called, max reentrance, exclusive time, inclusive time, max call time
					FunctionStatistics(0, 3, 0, 7, 5, 0),
					FunctionStatistics(0, 11, 0, 17, 13, 0),
					FunctionStatistics(0, 19, 0, 31, 23, 0),
					FunctionStatistics(0, 37, 0, 47, 41, 0),
				};

				// ACT
				destination[0] += addendum[0];
				destination[1] += addendum[1];
				destination[2] += addendum[2];
				destination[3] += addendum[3];

				// ASSERT
				assert_equal(function_statistics(3, 0, 7, 5, 0), destination[0]);
				assert_equal(function_statistics(12, 0, 10018, 3015, 0), destination[1]);
				assert_equal(function_statistics(32, 0, 50000700032, 13025, 0), destination[2]);
				assert_equal(function_statistics(168, 0, 12345725, 345719, 0), destination[3]);
			}


			test( InplaceAddingStatisticsToInternalUpdatesMaximumValuesForReentranceAndMaxCallTime )
			{
				// INIT
				function_statistics destination[] = {
					function_statistics(0, 0, 0, 0, 100),
					function_statistics(0, 4, 0, 0, 70),
					function_statistics(0, 9, 0, 0, 5),
					function_statistics(0, 13, 0, 0, 0),
				};
				FunctionStatistics addendum[] = {
					// address, times called, max reentrance, exclusive time, inclusive time, max call time
					FunctionStatistics(0, 1, 3, 21, 10, 0),
					FunctionStatistics(0, 2, 5, 22, 11, 70),
					FunctionStatistics(0, 3, 9, 23, 12, 6),
					FunctionStatistics(0, 4, 10, 24, 13, 9),
				};

				// ACT
				destination[0] += addendum[0];
				destination[1] += addendum[1];
				destination[2] += addendum[2];
				destination[3] += addendum[3];

				// ASSERT
				assert_equal(function_statistics(1, 3, 21, 10, 100), destination[0]);
				assert_equal(function_statistics(2, 5, 22, 11, 70), destination[1]);
				assert_equal(function_statistics(3, 9, 23, 12, 6), destination[2]);
				assert_equal(function_statistics(4, 13, 24, 13, 9), destination[3]);
			}


			test( InplaceAdditionDetailedReturnsConstRefToLHS )
			{
				// INIT
				function_statistics_detailed destination;
				const FunctionStatisticsDetailed &addendum = CreateFunctionStatisticsDetailed2(fbuilder, 0, 19, 0, 23, 31, 0);

				// ACT
				const function_statistics_detailed *result = &(destination += addendum);

				// ASSERT
				assert_equal(&destination, result);
			}


			test( InplaceAddingDetailedStatisticsToInternalNoChildrenUpdates )
			{
				// INIT
				function_statistics_detailed destination[] = {
					function_statistics_ex(0, 0, 0, 0, 100),
					function_statistics_ex(0, 4, 0, 0, 70),
					function_statistics_ex(0, 9, 0, 0, 5),
					function_statistics_ex(0, 13, 0, 0, 0),
				};
				const FunctionStatisticsDetailed *addendum[] = {
					&CreateFunctionStatisticsDetailed2(fbuilder, 0, 1, 3, 21, 10, 0),
					&CreateFunctionStatisticsDetailed2(fbuilder, 0, 2, 5, 22, 11, 70),
					&CreateFunctionStatisticsDetailed2(fbuilder, 0, 3, 9, 23, 12, 6),
					&CreateFunctionStatisticsDetailed2(fbuilder, 0, 4, 10, 24, 13, 9),
				};

				// ACT
				destination[0] += *addendum[0];
				destination[1] += *addendum[1];
				destination[2] += *addendum[2];
				destination[3] += *addendum[3];

				// ASSERT
				assert_equal(function_statistics(1, 3, 21, 10, 100), destination[0]);
				assert_equal(function_statistics(2, 5, 22, 11, 70), destination[1]);
				assert_equal(function_statistics(3, 9, 23, 12, 6), destination[2]);
				assert_equal(function_statistics(4, 13, 24, 13, 9), destination[3]);
			}


			test( InplaceAddingDetailedStatisticsToInternalWithChildrenUpdates1 )
			{
				// INIT
				function_statistics_detailed destination[2];
				FunctionStatistics children1[] = {
					FunctionStatistics(0x00001234, 1, 5, 13, 9, 0),
				};
				FunctionStatistics children2[] = {
					FunctionStatistics(0x00012340, 2, 6, 14, 10, 17),
					FunctionStatistics(0x00123400, 3, 7, 15, 11, 18),
					FunctionStatistics(0x01234000, 4, 8, 16, 12, 19),
				};
				const FunctionStatisticsDetailed *addendum[] = {
					&CreateFunctionStatisticsDetailed2(fbuilder, 0, 0, 0, 0, 0, 0, children1),
					&CreateFunctionStatisticsDetailed2(fbuilder, 0, 0, 0, 0, 0, 0, children2),
				};

				// ACT
				destination[0] += *addendum[0];
				destination[1] += *addendum[1];

				// ASSERT
				assert_equal(1u, destination[0].callees.size());
				assert_equal(function_statistics(1, 5, 13, 9, 0), destination[0].callees[(void *)0x00001234]);

				assert_equal(3u, destination[1].callees.size());
				assert_equal(function_statistics(2, 6, 14, 10, 17), destination[1].callees[(void *)0x00012340]);
				assert_equal(function_statistics(3, 7, 15, 11, 18), destination[1].callees[(void *)0x00123400]);
				assert_equal(function_statistics(4, 8, 16, 12, 19), destination[1].callees[(void *)0x01234000]);
			}


			test( InplaceAddingDetailedStatisticsToInternalWithChildrenUpdates2 )
			{
				// INIT
				function_statistics_detailed destination;
				FunctionStatistics children[] = {
					{	0x00001234, 1, 5, 13, 9, 0 },
					{	0x00012340, 2, 6, 14, 10, 29 },
				};
				const FunctionStatisticsDetailed &addendum = CreateFunctionStatisticsDetailed2(fbuilder, 0, 0, 0, 0, 0, 0, children);

				destination.callees[(void *)0x00012340] = function_statistics(3, 7, 15, 11, 18);
				destination.callees[(void *)0x00123400] = function_statistics(4, 8, 16, 12, 19);

				// ACT
				destination += addendum;

				// ASSERT
				assert_equal(3u, destination.callees.size());
				assert_equal(function_statistics(1, 5, 13, 9, 0), destination.callees[(void *)0x00001234]);
				assert_equal(function_statistics(5, 7, 29, 21, 29), destination.callees[(void *)0x00012340]);
				assert_equal(function_statistics(4, 8, 16, 12, 19), destination.callees[(void *)0x00123400]);
			}
		end_test_suite
	}
}
