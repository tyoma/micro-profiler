#include <common/formatting.h>

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		[TestClass]
		public ref class TextFormattingServicesTests
		{
			static wstring format_interval_proxy(double value)
			{
				wstring result;

				format_interval(result, value);
				return result;
			}

		public:
			[TestMethod]
			void EscapeZeroSecondsOnZero()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"0s" == format_interval_proxy(0));
			}


			[TestMethod]
			void EscapeNumbersUnderPrecision()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"0s" == format_interval_proxy(0.00000000049));
				Assert::IsTrue(L"0s" == format_interval_proxy(-0.00000000049));
			}


			[TestMethod]
			void EscapeNonZeroSecondValues()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"1s" == format_interval_proxy(0.9995));
				Assert::IsTrue(L"1s" == format_interval_proxy(1));
				Assert::IsTrue(L"1.01s" == format_interval_proxy(1.01));
				Assert::IsTrue(L"3.79s" == format_interval_proxy(3.79));
				Assert::IsTrue(L"13.6s" == format_interval_proxy(13.64555));
				Assert::IsTrue(L"117s" == format_interval_proxy(117.489));
				Assert::IsTrue(L"999s" == format_interval_proxy(999.45555));
				Assert::IsTrue(L"-1s" == format_interval_proxy(-0.9995));
				Assert::IsTrue(L"-2s" == format_interval_proxy(-1.995));
				Assert::IsTrue(L"-2s" == format_interval_proxy(-2));
				Assert::IsTrue(L"-5s" == format_interval_proxy(-5));
				Assert::IsTrue(L"-17s" == format_interval_proxy(-17));
				Assert::IsTrue(L"-231s" == format_interval_proxy(-231));
				Assert::IsTrue(L"-999s" == format_interval_proxy(-999));
			}


			[TestMethod]
			void EscapeNonZeroSecondValues2()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"999.5s" == format_interval_proxy(999.5));
				Assert::IsTrue(L"1000s" == format_interval_proxy(1000));
				Assert::IsTrue(L"5123s" == format_interval_proxy(5122.9));
				Assert::IsTrue(L"9999s" == format_interval_proxy(9999.4));
				Assert::IsTrue(L"1e+004s" == format_interval_proxy(10000));
				Assert::IsTrue(L"-7.23e+004s" == format_interval_proxy(-72300));
			}


			[TestMethod]
			void EscapeNonZeroMillisecondValues()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"1ms" == format_interval_proxy(0.001));
				Assert::IsTrue(L"1.01ms" == format_interval_proxy(0.00101));
				Assert::IsTrue(L"3.79ms" == format_interval_proxy(0.00379));
				Assert::IsTrue(L"13.6ms" == format_interval_proxy(0.01364555));
				Assert::IsTrue(L"117ms" == format_interval_proxy(0.117489));
				Assert::IsTrue(L"999ms" == format_interval_proxy(0.99945555));
				Assert::IsTrue(L"-2ms" == format_interval_proxy(-0.002));
				Assert::IsTrue(L"-5ms" == format_interval_proxy(-0.005));
				Assert::IsTrue(L"-17ms" == format_interval_proxy(-0.017));
				Assert::IsTrue(L"-231ms" == format_interval_proxy(-0.231));
				Assert::IsTrue(L"-999ms" == format_interval_proxy(-0.999));
			}


			[TestMethod]
			void EscapeNonZeroMicrosecondValues()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"3us" == format_interval_proxy(0.000003));
				Assert::IsTrue(L"2.09us" == format_interval_proxy(0.0000020949));
				Assert::IsTrue(L"1.97us" == format_interval_proxy(0.00000197));
				Assert::IsTrue(L"13.6us" == format_interval_proxy(0.00001364555));
				Assert::IsTrue(L"117us" == format_interval_proxy(0.000117489));
				Assert::IsTrue(L"999us" == format_interval_proxy(0.00099945555));
				Assert::IsTrue(L"-7.91us" == format_interval_proxy(-0.000007914));
				Assert::IsTrue(L"-5.09us" == format_interval_proxy(-0.000005094));
				Assert::IsTrue(L"-17us" == format_interval_proxy(-0.000017));
				Assert::IsTrue(L"-231us" == format_interval_proxy(-0.000231));
				Assert::IsTrue(L"-999us" == format_interval_proxy(-0.000999));
			}


			[TestMethod]
			void EscapeNonZeroNanosecondValues()
			{
				// INIT / ACT / ASSERT
				Assert::IsTrue(L"3ns" == format_interval_proxy(0.000000003));
				Assert::IsTrue(L"2.09ns" == format_interval_proxy(0.0000000020949));
				Assert::IsTrue(L"1.97ns" == format_interval_proxy(0.00000000197));
				Assert::IsTrue(L"13.6ns" == format_interval_proxy(0.00000001364555));
				Assert::IsTrue(L"117ns" == format_interval_proxy(0.000000117489));
				Assert::IsTrue(L"999ns" == format_interval_proxy(0.00000099945555));
				Assert::IsTrue(L"-7.91ns" == format_interval_proxy(-0.000000007914));
				Assert::IsTrue(L"-5.09ns" == format_interval_proxy(-0.000000005094));
				Assert::IsTrue(L"-17ns" == format_interval_proxy(-0.000000017));
				Assert::IsTrue(L"-231ns" == format_interval_proxy(-0.000000231));
				Assert::IsTrue(L"-999ns" == format_interval_proxy(-0.000000999));
			}
		};
	}
}
