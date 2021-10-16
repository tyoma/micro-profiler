#include <ipc/server_session.h>

#include "helpers.h"
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
				void serialize(ArchiveT &archive, sneaky_type &data)
				{	archive(data.ivalue), archive(data.svalue);	}
			}

			begin_test_suite( ServerSessionTests )
				mocks::channel outbound;
				
				test( AppropriateHandlerIsCalledOnInboundMessage )
				{
					// INIT
					server_session s(outbound);
					vector<int> log;

					// INIT / ACT
					s.add_handler(13, [&] (server_session::response &, int) {	log.push_back(113);	});
					s.add_handler(171, [&] (server_session::response &, int) {	log.push_back(1171);	});
					s.add_handler(1991, [&] (server_session::response &, int) {	log.push_back(11991);	});
					s.add_handler(19911, [&] (server_session::response &) {	log.push_back(21991);	});
					s.add_handler(19912, [&] (server_session::response &) {	log.push_back(21992);	});

					// ACT
					send_standard(s, 171, 19193, 1);

					// ASSERT
					int reference1[] = {	1171,	};

					assert_equal(reference1, log);

					// ACT
					send_standard(s, 1991, 19194, 1);
					send_standard(s, 171, 19195, 1);
					send_standard(s, 13, 19196, 1);

					// ASSERT
					int reference2[] = {	1171, 11991, 1171, 113,	};

					assert_equal(reference2, log);

					// ACT
					send_standard(s, 19911, 19196);
					send_standard(s, 19912, 19196);
				}


				test( InvocationOfUnknownRequestDoesNotThrow )
				{
					// INIT
					server_session s(outbound);
					vector<int> log;

					s.add_handler(13, [&] (server_session::response &, int) {	log.push_back(113);	});

					// ACT / ASSERT (must not throw)
					send_standard(s, 12314, 0, 1);

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
					s.add_handler(1, [&] (server_session::response &, const vector<int> &payload) {
						log1.push_back(payload);
					});
					s.add_handler(2, [&] (server_session::response &, const sneaky_type &payload) {
						log2.push_back(payload);
					});

					// ACT
					int ints1[] = {	3, 1, 4, 1, 5, 9, 26,	};

					send_standard(s, 1, 100, mkvector(ints1));

					// ASSERT
					assert_equal(1u, log1.size());
					assert_equal(ints1, log1.back());
					assert_is_empty(log2);

					// ACT
					int ints2[] = {	100, 3, 1, 4, 1, 5, 9, -26, 919191	};

					send_standard(s, 1, 101, mkvector(ints2));
					send_standard(s, 2, 102, sneaky_type::create(19, "one"));
					send_standard(s, 2, 103, sneaky_type::create(191, "two"));

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
					s.add_handler(1, [&] (server_session::response &, const vector<int> &payload) {
						log.push_back(&payload);
					});

					//ACT
					send_standard(s, 1, 100, mkvector(data));
					alloca(1000);
					send_standard(s, 1, 100, mkvector(data));

					// ASSERT
					assert_equal(log[0], log[1]);
				}


				test( SpecifiedResponseIdAndRequestTokenArePassedInResponse )
				{
					// INIT
					server_session s(outbound);
					vector< pair<int, unsigned long long> > log;

					outbound.on_message = [&] (const_byte_range r) {
						buffer_reader b(r);
						strmd::deserializer<buffer_reader, packer> d(b);
						int id;
						unsigned long long token;

						d(id), d(token);
						log.push_back(make_pair(id, token));
					};

					s.add_handler(1, [&] (server_session::response &resp, int) {

					// ACT
						resp(193817);
					});

					s.add_handler(2, [&] (server_session::response &resp, int) {

					// ACT
						resp(1311310);
					});

					// ACT
					send_standard(s, 1, 0x100010001000ull, 0);

					// ASSERT
					pair<int, unsigned long long> reference1[] = {
						make_pair(193817, 0x100010001000ull),
					};

					assert_equal(reference1, log);

					// ACT
					send_standard(s, 1, 0x10001, 0);
					send_standard(s, 2, 0xF00010001000ull, 0);

					// ASSERT
					pair<int, unsigned long long> reference2[] = {
						make_pair(193817, 0x100010001000ull),
						make_pair(193817, 0x10001),
						make_pair(1311310, 0xF00010001000ull),
					};

					assert_equal(reference2, log);
				}


				test( DataSerializedInResponseAreReadable )
				{
					// INIT
					server_session s(outbound);
					vector<string> log1;
					vector<int> log2;

					outbound.on_message = [&] (const_byte_range r) {
						buffer_reader b(r);
						strmd::deserializer<buffer_reader, packer> d(b);
						int id;
						unsigned token;
						string v1;
						int v2;

						switch (d(id), d(token), id)
						{
						case 100:
							d(v1);
							log1.push_back(v1);
							break;

						case 101:
							d(v2);
							log2.push_back(v2);
							break;

						}
					};

					s.add_handler(1, [&] (server_session::response &resp, int) {

					// ACT
						resp(100, string("Lorem ipsum amet dolor"));
					});

					s.add_handler(2, [&] (server_session::response &resp, int) {

					// ACT
						resp(100, string("Whoa!"));
					});

					s.add_handler(3, [&] (server_session::response &resp, int) {

					// ACT
						resp(101, 99183);
					});

					s.add_handler(4, [&] (server_session::response &resp, int) {

					// ACT
						resp(101, 91919191);
					});

					// ACT
					send_standard(s, 1, 12, 0);

					// ASSERT
					string reference1[] = {	"Lorem ipsum amet dolor",	};

					assert_equal(reference1, log1);
					assert_is_empty(log2);

					// ACT
					send_standard(s, 2, 12, 0);
					send_standard(s, 2, 12, 0);
					send_standard(s, 3, 12, 0);
					send_standard(s, 4, 12, 0);

					// ASSERT
					string reference2[] = {	"Lorem ipsum amet dolor", "Whoa!", "Whoa!",	};
					int reference3[] = {	99183, 91919191,	};

					assert_equal(reference2, log1);
					assert_equal(reference3, log2);
				}


				test( MessagesAreSerializedWithoutTokens )
				{
					// INIT
					server_session s(outbound);
					vector<string> log1;
					vector<int> log2;

					outbound.on_message = [&] (const_byte_range r) {
						buffer_reader b(r);
						strmd::deserializer<buffer_reader, packer> d(b);
						int id;
						string v1;
						int v2;

						switch (d(id), id)
						{
						case 100:
							d(v1);
							log1.push_back(v1);
							break;

						case 101:
							d(v2);
							log2.push_back(v2);
							break;

						}
					};

					// ACT
					s.message(101, [&] (serializer &ser) {	ser(1919191);	});

					// ASSERT
					int reference1[] = {	1919191,	};

					assert_is_empty(log1);
					assert_equal(reference1, log2);

					// ACT
					s.message(101, [&] (serializer &ser) {	ser(1);	});
					s.message(101, [&] (serializer &ser) {	ser(2);	});
					s.message(100, [&] (serializer &ser) {	ser(string("Hello!"));	});

					// ASSERT
					string reference2[] = {	"Hello!",	};
					int reference3[] = {	1919191, 1, 2,	};

					assert_equal(reference2, log1);
					assert_equal(reference3, log2);
				}


				test( DisconnectHandlerIsCalledIfPresentOnDisconnect )
				{
					// INIT
					server_session s1(outbound), s2(outbound);
					auto called = 0;

					s1.set_disconnect_handler([&] {	called++;	});

					// ASSERT
					assert_equal(0, called);

					// ACT
					static_cast<channel &>(s1).disconnect();

					// ASSERT
					assert_equal(1, called);

					// ACT / ASSERT (no exception)
					static_cast<channel &>(s2).disconnect();
				}
			end_test_suite
		}
	}
}
