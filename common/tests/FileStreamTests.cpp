#include <common/file_stream.h>

#include <common/types.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <test-helpers/constants.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			size_t get_file_length(const string &path)
			{
				shared_ptr<FILE> f(fopen(path.c_str(), "rb"), &fclose);
				byte b[100];
				size_t length = 0;

				for (auto n = sizeof(b); n == sizeof(b); )
					length += (n = fread(b, 1, sizeof(b), f.get()));
				return length;
			}
		}

		begin_test_suite( ReadFileStreamTests )
			test( StreamsReadWithTheDifferentBlockSizesAreEqual )
			{
				// INIT
				byte b1[100], b2[113];
				vector<byte> read1, read2;
				shared_ptr<FILE> f(fopen(string(c_symbol_container_1).c_str(), "rb"), &fclose);

				// INIT / ACT
				read_file_stream s1(c_symbol_container_1);
				read_file_stream s2(c_symbol_container_1);

				// ACT
				for (size_t n = sizeof(b1); n == sizeof(b1); )
				{
					n = s1.read_l(b1, sizeof(b1));
					read1.insert(read1.end(), b1, b1 + n);
				}
				for (size_t n = sizeof(b2); n == sizeof(b2); )
				{
					n = s2.read_l(b2, sizeof(b2));
					read2.insert(read2.end(), b2, b2 + n);
				}

				// ASSERT
				assert_is_true(read1.size() > 500u);
				assert_equal(read1, read2);

				fread(read2.data(), 1, read2.size(), f.get());

				assert_equal(read1, read2);
			}


			test( SkipDuringReadingPositionsFilePointerForward )
			{
				// INIT
				vector<byte> b1(500), b2(10);
				read_file_stream s1(c_symbol_container_1);
				read_file_stream s2(c_symbol_container_1);

				s1.read(b1.data(), b1.size());

				// ACT
				s2.skip(130);
				s2.read(b2.data(), b2.size());

				// ASSERT
				assert_equal(vector<byte>(&b1[130], &b1[140]), b2);

				// ACT
				s2.skip(17);
				s2.read(b2.data(), b2.size());

				// ASSERT
				assert_equal(vector<byte>(&b1[157], &b1[167]), b2);
			}


			test( SkippingPastEndFails )
			{
				// INIT
				byte b[1];
				read_file_stream s(c_symbol_container_1);

				// ACT / ASSERT
				s.skip(1000000); // we know it's shorter
				assert_equal(0u, s.read_l(b, 1));
			}


			test( ReadingPastLimitThrowsAnException )
			{
				// INIT
				const auto l1 = get_file_length(c_symbol_container_1);
				const auto l2 = get_file_length(c_symbol_container_2);
				byte tmp[100];

				read_file_stream s1(c_symbol_container_1);
				read_file_stream s2(c_symbol_container_2);

				s1.skip(l1 - 75), s1.read(tmp, 67);
				s2.skip(l2 - 73);

				// ACT / ASSERT
				assert_throws(s1.read(tmp, 10), runtime_error);
				assert_throws(s2.read(tmp, 74), runtime_error);
			}
		end_test_suite
	}
}
