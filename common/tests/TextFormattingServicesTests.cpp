#include <common/formatting.h>

#include <limits>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( TextFormattingServicesTests )
			static wstring format_interval_proxy(double value)
			{
				wstring result;

				format_interval(result, value);
				return result;
			}


			test( EscapeZeroSecondsOnZero )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"0s", format_interval_proxy(0));
			}


			test( EscapeNumbersUnderPrecisionAsZeroes )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"0s", format_interval_proxy(0.00000000049));
				assert_equal(L"0s", format_interval_proxy(-0.00000000049));
				assert_equal(L"0s", format_interval_proxy(-0.00000000000049));
				assert_equal(L"0s", format_interval_proxy(numeric_limits<double>::min()));
			}


			test( EscapeNonZeroSecondValues )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"1s", format_interval_proxy(0.9995));
				assert_equal(L"1s", format_interval_proxy(1));
				assert_equal(L"1.01s", format_interval_proxy(1.01));
				assert_equal(L"3.79s", format_interval_proxy(3.79));
				assert_equal(L"13.6s", format_interval_proxy(13.64555));
				assert_equal(L"117s", format_interval_proxy(117.489));
				assert_equal(L"999s", format_interval_proxy(999.45555));
				assert_equal(L"-1s", format_interval_proxy(-0.9995));
				assert_equal(L"-2s", format_interval_proxy(-1.995));
				assert_equal(L"-2s", format_interval_proxy(-2));
				assert_equal(L"-5s", format_interval_proxy(-5));
				assert_equal(L"-17s", format_interval_proxy(-17));
				assert_equal(L"-231s", format_interval_proxy(-231));
				assert_equal(L"-999s", format_interval_proxy(-999));
			}


			test( EscapeNonZeroSecondValues2 )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"999.5s", format_interval_proxy(999.5));
				assert_equal(L"1000s", format_interval_proxy(1000));
				assert_equal(L"5123s", format_interval_proxy(5122.9));
				assert_equal(L"9999s", format_interval_proxy(9999.4));
				assert_equal(L"1e+04s", format_interval_proxy(10000));
				assert_equal(L"-7.23e+04s", format_interval_proxy(-72300));
			}


			test( EscapeNonZeroMillisecondValues )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"1ms", format_interval_proxy(0.0009995000000000001));
				assert_equal(L"1ms", format_interval_proxy(0.001));
				assert_equal(L"1.01ms", format_interval_proxy(0.00101));
				assert_equal(L"3.79ms", format_interval_proxy(0.00379));
				assert_equal(L"13.6ms", format_interval_proxy(0.01364555));
				assert_equal(L"117ms", format_interval_proxy(0.117489));
				assert_equal(L"999ms", format_interval_proxy(0.99945555));
				assert_equal(L"-2ms", format_interval_proxy(-0.002));
				assert_equal(L"-5ms", format_interval_proxy(-0.005));
				assert_equal(L"-17ms", format_interval_proxy(-0.017));
				assert_equal(L"-231ms", format_interval_proxy(-0.231));
				assert_equal(L"-999ms", format_interval_proxy(-0.999));
			}


			test( EscapeNonZeroMicrosecondValues )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"3\x03bcs", format_interval_proxy(0.000003));
				assert_equal(L"2.09\x03bcs", format_interval_proxy(0.0000020949));
				assert_equal(L"1.97\x03bcs", format_interval_proxy(0.00000197));
				assert_equal(L"13.6\x03bcs", format_interval_proxy(0.00001364555));
				assert_equal(L"117\x03bcs", format_interval_proxy(0.000117489));
				assert_equal(L"999\x03bcs", format_interval_proxy(0.00099945555));
				assert_equal(L"-7.91\x03bcs", format_interval_proxy(-0.000007914));
				assert_equal(L"-5.09\x03bcs", format_interval_proxy(-0.000005094));
				assert_equal(L"-17\x03bcs", format_interval_proxy(-0.000017));
				assert_equal(L"-231\x03bcs", format_interval_proxy(-0.000231));
				assert_equal(L"-999\x03bcs", format_interval_proxy(-0.000999));
			}


			test( EscapeNonZeroNanosecondValues )
			{
				// INIT / ACT / ASSERT
				assert_equal(L"3ns", format_interval_proxy(0.000000003));
				assert_equal(L"2.09ns", format_interval_proxy(0.0000000020949));
				assert_equal(L"1.97ns", format_interval_proxy(0.00000000197));
				assert_equal(L"13.6ns", format_interval_proxy(0.00000001364555));
				assert_equal(L"117ns", format_interval_proxy(0.000000117489));
				assert_equal(L"999ns", format_interval_proxy(0.00000099945555));
				assert_equal(L"-7.91ns", format_interval_proxy(-0.000000007914));
				assert_equal(L"-5.09ns", format_interval_proxy(-0.000000005094));
				assert_equal(L"-17ns", format_interval_proxy(-0.000000017));
				assert_equal(L"-231ns", format_interval_proxy(-0.000000231));
				assert_equal(L"-999ns", format_interval_proxy(-0.000000999));
			}
		end_test_suite
	}
}
