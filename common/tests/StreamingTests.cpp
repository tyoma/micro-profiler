#include <common/stream.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( BufferReaderTests )
			test( BufferReaderReadsUnderlyingData )
			{
				// INIT
				byte b[10];
				byte data1[] = {	0x10, 0x91, 0x07, 0xF2, 0xC7, 0x87,	};
				byte data2[] = {	0x13, 0x10, 0x17, 0x91, 0x00, 0x07, 0xF2, 0xC7, 0x87,	};

				// INIT / ACT
				buffer_reader r1(mkrange(data1));
				buffer_reader r2(mkrange(data2));

				// ACT
				r1.read(b, 2);

				// ASSERT
				byte reference1[] = {	0x10, 0x91,	};

				assert_equal(reference1, vector<byte>(b, b + 2));

				// ACT
				r1.read(b, 4);

				// ASSERT
				byte reference2[] = {	0x07, 0xF2, 0xC7, 0x87,	};

				assert_equal(reference2, vector<byte>(b, b + 4));

				// ACT
				r2.read(b, 3);

				// ASSERT
				byte reference3[] = {	0x13, 0x10, 0x17,	};

				assert_equal(reference3, vector<byte>(b, b + 3));

				// ACT
				r2.read(b, 2);

				// ASSERT
				byte reference4[] = {	0x91, 0x00,	};

				assert_equal(reference4, vector<byte>(b, b + 2));

				// ACT
				r2.read(b, 4);

				// ASSERT
				byte reference5[] = {	0x07, 0xF2, 0xC7, 0x87,	};

				assert_equal(reference5, vector<byte>(b, b + 4));
			}


			test( ReadingPastEndRaisesException )
			{
				// INIT
				byte b[100];
				byte data[] = {	0x13, 0x10, 0x17, 0x91, 0x00, 0x07, 0xF2, 0xC7, 0x87,	};
				buffer_reader r(mkrange(data));

				r.read(b, 2);

				// ACT
				try
				{
					r.read(b, 70);

				// ASSERT
					assert_is_false(true);
				}
				catch (const insufficient_buffer_error &e)
				{
					assert_equal(70u, e.requested);
					assert_equal(7u, e.available);
				}

				// ACT
				r.read(b, 3);

				// ASSERT
				byte reference1[] = {	0x17, 0x91, 0x00,	};

				assert_equal(reference1, vector<byte>(b, b + 3));

				// ACT
				try
				{
					r.read(b, 5);

				// ASSERT
					assert_is_false(true);
				}
				catch (const insufficient_buffer_error &e)
				{
					assert_equal(5u, e.requested);
					assert_equal(4u, e.available);
				}
			}


			test( SkippingSeeksPointerForward )
			{
				// INIT
				byte b[100];
				byte data[] = {	0x13, 0x10, 0x17, 0x91, 0x00, 0x07, 0xF2, 0xC7, 0x87,	};
				buffer_reader r(mkrange(data));

				// ACT
				r.skip(2);

				// ASSERT
				byte reference1[] = {	0x17, 0x91, 	};

				r.read(b, 2);
				assert_equal(reference1, vector<byte>(b, b + 2));

				// ACT
				r.skip(3);

				// ASSERT
				byte reference2[] = {	0xC7, 0x87,	};

				r.read(b, 2);
				assert_equal(reference2, vector<byte>(b, b + 2));

				// ACT / ASSERT
				assert_throws(r.read(b, 1), insufficient_buffer_error);
			}


			test( SkippingPastEndRaisesException )
			{
				// INIT
				byte b[100];
				byte data[] = {	0x13, 0x10, 0x17, 0x91, 0x00, 0x07, 0xF2, 0xC7, 0x87,	};
				buffer_reader r(mkrange(data));

				r.read(b, 2);

				// ACT
				try
				{
					r.skip(70);

				// ASSERT
					assert_is_false(true);
				}
				catch (const insufficient_buffer_error &e)
				{
					assert_equal(70u, e.requested);
					assert_equal(7u, e.available);
				}

				// ACT
				r.read(b, 3);

				// ASSERT
				byte reference1[] = {	0x17, 0x91, 0x00,	};

				assert_equal(reference1, vector<byte>(b, b + 3));

				// ACT
				try
				{
					r.skip(5);

				// ASSERT
					assert_is_false(true);
				}
				catch (const insufficient_buffer_error &e)
				{
					assert_equal(5u, e.requested);
					assert_equal(4u, e.available);
				}
			}
		end_test_suite
	}
}
