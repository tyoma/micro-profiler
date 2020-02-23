#include <common/formatting.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			template <unsigned char base, typename T>
			string itoa_s(T value)
			{
				string result(4, ' ');

				itoa<base>(result, value);
				assert_is_true(result.size() > 4);
				return result.substr(4);
			}

			template <unsigned char base, typename T>
			string itoa_v(T value)
			{
				vector<char> vresult(10);

				itoa<base>(vresult, value);
				assert_is_true(vresult.size() > 10);
				return string(vresult.begin() + 10, vresult.end());
			}
		}

		begin_test_suite( TextConversionTests )
			test( ZeroIsConvertedToZeroCharacter )
			{
				// ACT / ASSERT
				assert_equal("0", itoa_s<10>(static_cast<char>(0)));
				assert_equal("0", itoa_v<10>(static_cast<char>(0)));

				assert_equal("0", itoa_s<10>(static_cast<unsigned char>(0)));
				assert_equal("0", itoa_v<10>(static_cast<unsigned char>(0)));

				assert_equal("0", itoa_s<10>(static_cast<short>(0)));
				assert_equal("0", itoa_v<10>(static_cast<short>(0)));

				assert_equal("0", itoa_s<10>(static_cast<unsigned short>(0)));
				assert_equal("0", itoa_v<10>(static_cast<unsigned short>(0)));

				assert_equal("0", itoa_s<10>(static_cast<int>(0)));
				assert_equal("0", itoa_v<10>(static_cast<int>(0)));

				assert_equal("0", itoa_s<10>(static_cast<unsigned int>(0)));
				assert_equal("0", itoa_v<10>(static_cast<unsigned int>(0)));

				assert_equal("0", itoa_s<10>(static_cast<long>(0)));
				assert_equal("0", itoa_v<10>(static_cast<long>(0)));

				assert_equal("0", itoa_s<10>(static_cast<unsigned long>(0)));
				assert_equal("0", itoa_v<10>(static_cast<unsigned long>(0)));

				assert_equal("0", itoa_s<10>(static_cast<long long>(0)));
				assert_equal("0", itoa_v<10>(static_cast<long long>(0)));

				assert_equal("0", itoa_s<10>(static_cast<unsigned long long>(0)));
				assert_equal("0", itoa_v<10>(static_cast<unsigned long long>(0)));
			}


			test( OneIsConvertedToCharacterOne )
			{
				// ACT / ASSERT
				assert_equal("1", itoa_s<2>(1));
				assert_equal("1", itoa_v<3>(1));
				assert_equal("1", itoa_s<4>(1));
				assert_equal("1", itoa_v<5>(1));
				assert_equal("1", itoa_s<6>(1));
				assert_equal("1", itoa_v<7>(1));
				assert_equal("1", itoa_s<8>(1));
				assert_equal("1", itoa_v<9>(1));
				assert_equal("1", itoa_s<10>(1));
				assert_equal("1", itoa_v<11>(1));
				assert_equal("1", itoa_v<12>(1));
				assert_equal("1", itoa_s<13>(1));
				assert_equal("1", itoa_v<14>(1));
				assert_equal("1", itoa_s<15>(1));
				assert_equal("1", itoa_v<16>(1));
			}


			test( TenIsConvertedToOneZeroForAllBases )
			{
				// ACT / ASSERT
				assert_equal("10", itoa_s<2>(2));
				assert_equal("10", itoa_v<3>(3));
				assert_equal("10", itoa_s<4>(4));
				assert_equal("10", itoa_v<5>(5));
				assert_equal("10", itoa_s<6>(6));
				assert_equal("10", itoa_v<7>(7));
				assert_equal("10", itoa_s<8>(8));
				assert_equal("10", itoa_v<9>(9));
				assert_equal("10", itoa_s<10>(10));
				assert_equal("10", itoa_v<11>(11));
				assert_equal("10", itoa_v<12>(12));
				assert_equal("10", itoa_s<13>(13));
				assert_equal("10", itoa_v<14>(14));
				assert_equal("10", itoa_s<15>(15));
				assert_equal("10", itoa_v<16>(16));
			}


			test( RoundNumbersAreConvertedWithAccordingAmountOfZeroes )
			{
				// ACT / ASSERT
				assert_equal("100", itoa_s<2>(4));
				assert_equal("1000", itoa_s<2>(8));
				assert_equal("10000", itoa_s<2>(16));
				assert_equal("100000", itoa_s<2>(32));
				assert_equal("1000000", itoa_s<2>(64));
				assert_equal("10000000", itoa_s<2>(128));
				assert_equal("100000000", itoa_s<2>(256));
				assert_equal("100", itoa_s<3>(9));
				assert_equal("1000", itoa_s<3>(27));
			}


			test( HexDigitsAreAllConverted )
			{
				// ACT / ASSERT
				assert_equal("2", itoa_s<16>(2));
				assert_equal("3", itoa_s<16>(3));
				assert_equal("4", itoa_s<16>(4));
				assert_equal("5", itoa_s<16>(5));
				assert_equal("6", itoa_s<16>(6));
				assert_equal("7", itoa_s<16>(7));
				assert_equal("8", itoa_s<16>(8));
				assert_equal("9", itoa_s<16>(9));
				assert_equal("A", itoa_s<16>(10));
				assert_equal("B", itoa_s<16>(11));
				assert_equal("C", itoa_s<16>(12));
				assert_equal("D", itoa_s<16>(13));
				assert_equal("E", itoa_s<16>(14));
				assert_equal("F", itoa_s<16>(15));
			}


			test( ArbitraryNumbersAreConverted )
			{
				// ACT / ASSERT
				assert_equal("12272", itoa_s<8>(012272));
				assert_equal("31415926", itoa_s<10>(31415926));
				assert_equal("BAADF00D", itoa_s<16>(0xBAADF00D));
			}


			test( NegativeNumbersAreConvertedAccordinglyToTheBase )
			{
				// ACT / ASSERT
				assert_equal("-22272", itoa_s<8>(-022272));
				assert_equal("-31415926", itoa_s<10>(-31415926));
				assert_equal("-3AADF00D", itoa_s<16>(-0x3AADF00D));
			}
		end_test_suite
	}
}
