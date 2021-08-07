#include <ipc/server_session.h>

#include "mocks.h"

#ifndef _MSC_VER
	#include <alloca.h>
#endif
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			using namespace micro_profiler::tests;

			namespace
			{
				struct sneaky_type
				{
					static sneaky_type create(int i, string s)
					{
						sneaky_type r = {	i, s	};
						return r;
					}

					int ivalue;
					string svalue;

					bool operator ==(const sneaky_type &rhs) const
					{	return ivalue == rhs.ivalue && svalue == rhs.svalue;	}
				};

				template <typename ArchiveT>
				void serialize(ArchiveT &archive, sneaky_type &data, unsigned int /*version*/)
				{	archive(data.ivalue), archive(data.svalue);	}

				template <typename PayloadT>
				void send_request(ipc::channel &c, unsigned id, unsigned long long token, const PayloadT &payload)
				{
					pod_vector<byte> data;
					buffer_writer< pod_vector<byte> > w(data);
					server_session::serializer s(w);

					s(id);
					s(token);
					s(payload);
					c.message(const_byte_range(data.data(), data.size()));
				}
			}

			begin_test_suite( AServerSessionTests )
				mocks::channel outbound;
				
				test( AppropriateHandlerIsCalledOnInboundMessage )
				{
					// INIT
					server_session s(outbound);
					vector<int> log;

					// INIT / ACT
					s.add_handler<int>(13, [&] (server_session::request &, int) {	log.push_back(113);	});
					s.add_handler<int>(171, [&] (server_session::request &, int) {	log.push_back(1171);	});
					s.add_handler<int>(1991, [&] (server_session::request &, int) {	log.push_back(11991);	});

					// ACT
					send_request(s, 171, 19193, 1);

					// ASSERT
					int reference1[] = {	1171,	};

					assert_equal(reference1, log);

					// ACT
					send_request(s, 1991, 19194, 1);
					send_request(s, 171, 19195, 1);
					send_request(s, 13, 19196, 1);

					// ASSERT
					int reference2[] = {	1171, 11991, 1171, 113,	};

					assert_equal(reference2, log);
				}


				test( InvocationOfUnknownRequestDoesNotThrow )
				{
					// INIT
					server_session s(outbound);
					vector<int> log;

					s.add_handler<int>(13, [&] (server_session::request &, int) {	log.push_back(113);	});

					// ACT / ASSERT (must not throw)
					send_request(s, 12314, 0, 1);

					// ASSERT
					assert_is_empty(log);
				}


				test( PayloadIsDeserializedAndPassedToDataPassedToHandler )
				{
					// INIT
					server_session s(outbound);
					vector< vector<int> > log1;
					vector<sneaky_type> log2;

					// INIT / ACT
					s.add_handler< vector<int> >(1, [&] (server_session::request &, const vector<int> &payload) {
						log1.push_back(payload);
					});
					s.add_handler<sneaky_type>(2, [&] (server_session::request &, const sneaky_type &payload) {
						log2.push_back(payload);
					});

					// ACT
					int ints1[] = {	3, 1, 4, 1, 5, 9, 26,	};

					send_request(s, 1, 100, mkvector(ints1));

					// ASSERT
					assert_equal(1u, log1.size());
					assert_equal(ints1, log1.back());
					assert_is_empty(log2);

					// ACT
					int ints2[] = {	100, 3, 1, 4, 1, 5, 9, -26, 919191	};

					send_request(s, 1, 101, mkvector(ints2));
					send_request(s, 2, 102, sneaky_type::create(19, "one"));
					send_request(s, 2, 103, sneaky_type::create(191, "two"));

					// ASSERT
					sneaky_type reference[] = {
						sneaky_type::create(19, "one"), sneaky_type::create(191, "two"),
					};

					assert_equal(2u, log1.size());
					assert_equal(ints2, log1.back());
					assert_equal(reference, log2);
				}


				test( SamePayloadObjectIsPassedToAHandler )
				{
					// INIT
					server_session s(outbound);
					vector<const void *> log;
					int data[] = {	3, 1, 4, 1, 5, 9, 26,	};

					// INIT / ACT
					s.add_handler< vector<int> >(1, [&] (server_session::request &, const vector<int> &payload) {
						log.push_back(&payload);
					});

					//ACT
					send_request(s, 1, 100, mkvector(data));
					alloca(1000);
					send_request(s, 1, 100, mkvector(data));

					// ASSERT
					assert_equal(log[0], log[1]);
				}


				test( SpecifiedResponseIdAndRequestTokenArePassedInResponse )
				{
					// INIT
					server_session s(outbound);
					vector<const void *> log;

					s.add_handler<int>(1, [&] (server_session::request &/*req*/, int) {
					// ACT
//						req.respond(
					});

					send_request(s, 1, 100, 0);

				}
			end_test_suite
		}
	}
}
