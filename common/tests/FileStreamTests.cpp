#include <common/file_stream.h>

#include <common/types.h>
#include <cstdio>
#include <memory>
#include <test-helpers/constants.h>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int dummy;

			size_t get_file_length(const string &path)
			{
				auto raw = fopen(path.c_str(), "rb");

				assert_not_null(raw);

				shared_ptr<FILE> f(raw, &fclose);

				fseek(f.get(), 0, SEEK_END);
				return ftell(f.get());
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


			test( OpeningAMissingFileThrowsFileNotFoundException )
			{
				// ACT / ACT
				assert_throws(read_file_stream("wjkwjkwjrr.wddd"), runtime_error);
				assert_throws(read_file_stream("wjkwjkwjrr.wddd"), file_not_found_exception);
			}

		end_test_suite


		begin_test_suite( WriteFileStreamTests )
			temporary_directory dir;


			test( ConstructionCreatesAFile )
			{
				// INIT / ACT
				const auto f1 = dir.track_file("test1-abcdef.dll");
				const auto f2 = dir.track_file("test2-abcdef.so");
				write_file_stream s1(f1);
				write_file_stream s2(f2);

				// ASSERT
				read_file_stream r1(f1);
				read_file_stream r2(f2);
			}


			test( WritingToAFileCanBeRead )
			{
				// INIT
				const auto f1 = dir.track_file("test1-abcdef.dll");
				const auto f2 = dir.track_file("test2-abcdef.so");

				unique_ptr<write_file_stream> s1(new write_file_stream(f1));
				unique_ptr<write_file_stream> s2(new write_file_stream(f2));
				char data1[] = "Lorem Ipsum";
				char data2[] = "zz";
				char data3[] = "Lorem Ipsum Amet Dolor";

				// ACT
				s1->write(data1, sizeof data1);
				s1->write(data2, sizeof data2);
				s1.reset();
				s2->write(data3, sizeof data3);
				s2.reset();

				// ASSERT
				char buffer1[sizeof data1 + sizeof data2] = { 0 };
				read_file_stream r1(f1);

				char buffer2[sizeof data3] = { 0 };
				read_file_stream r2(f2);

				r1.read(buffer1, sizeof buffer1);
				r2.read(buffer2, sizeof buffer2);

				const char reference1[] = "Lorem Ipsum\0zz";

				assert_equal(reference1, mkvector(buffer1));
				assert_equal(mkvector(data3), mkvector(buffer2));

				// ACT / ASSERT
				assert_equal(0u, r1.read_l(buffer1, 1));
				assert_equal(0u, r2.read_l(buffer1, 1));
			}
		end_test_suite

	}
}
