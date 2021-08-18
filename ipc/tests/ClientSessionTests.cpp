#include <ipc/client_session.h>

#include "helpers.h"
#include "mocks.h"

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

				template <typename T, typename DeserializerT>
				T read(DeserializerT &dser)
				{
					T value;

					dser(value);
					return value;
				}

				template <typename ArchiveT>
				void serialize(ArchiveT &archive, sneaky_type &data, unsigned int /*version*/)
				{	archive(data.ivalue), archive(data.svalue);	}
			}

			begin_test_suite( ClientSessionTests )
				shared_ptr<mocks::channel> outbound;
				shared_ptr<void> req[10];

				test( SessionIsCreatedOnConstruction )
				{
					// INIT / ACT
					channel *inbound = nullptr;
					client_session s([&] (channel &inbound_) -> shared_ptr<channel>	{
						inbound = &inbound_;
						assert_null(outbound);
						return outbound = make_shared<mocks::channel>();
					});

					// ASSERT
					assert_equal(inbound, &s);
					assert_not_null(outbound);
				}


				test( SendingARequestSerializesRequestPayload )
				{
					// INIT / ACT
					client_session s([&] (channel &/*inbound*/)	{	return outbound = make_shared<mocks::channel>();	});
					auto messages = 0;

					// ACT
					outbound->on_message = [&] (const_byte_range payload) {
						buffer_reader br(payload); client_session::deserializer ser(br); messages++;

					// ASSERT
						assert_equal(121, read<int>(ser));
						assert_equal(1u, read<unsigned>(ser));
						assert_equal(3918344u, read<unsigned>(ser));

					};
					s.request(req[0], 121, 3918344u, 1, [] (client_session::deserializer &) {});

					// ASSERT
					assert_equal(1, messages);

					// ACT
					outbound->on_message = [&] (const_byte_range payload) {
						buffer_reader br(payload); client_session::deserializer ser(br); messages++;

					// ASSERT
						assert_equal(1001, read<int>(ser));
						assert_equal(2u, read<unsigned>(ser));
						assert_equal(sneaky_type::create(3141, "pi"), read<sneaky_type>(ser));
					};
					s.request(req[1], 1001, sneaky_type::create(3141, "pi"), 1, [] (client_session::deserializer &) {});

					// ASSERT
					assert_equal(2, messages);

					// ACT
					outbound->on_message = [&] (const_byte_range payload) {
						buffer_reader br(payload); client_session::deserializer ser(br); messages++;

					// ASSERT
						assert_equal(10001, read<int>(ser));
						assert_equal(3u, read<unsigned>(ser));
						assert_equal("Lorem ipsum", read<string>(ser));
					};
					s.request(req[2], 10001, string("Lorem ipsum"), 1, [] (client_session::deserializer &) {});

					// ASSERT
					assert_equal(3, messages);

					// ACT
					outbound->on_message = [&] (const_byte_range payload) {
						buffer_reader br(payload); client_session::deserializer ser(br); messages++;

					// ASSERT
						assert_equal(11, read<int>(ser));
						assert_equal(4u, read<unsigned>(ser));
						assert_equal(sneaky_type::create(31415926, "amet dolor"), read<sneaky_type>(ser));
					};
					s.request(req[3], 11, sneaky_type::create(31415926, "amet dolor"), 1, [] (client_session::deserializer &) {});
				}


				test( SendingARequestInPassiveSessionSerializesRequestPayload )
				{
					// INIT / ACT
					outbound = make_shared<mocks::channel>();
					client_session s(static_cast<channel &>(*outbound));
					auto messages = 0;

					// ACT
					outbound->on_message = [&] (const_byte_range payload) {
						buffer_reader br(payload); client_session::deserializer ser(br); messages++;

					// ASSERT
						assert_equal(121, read<int>(ser));
						assert_equal(1u, read<unsigned>(ser));
						assert_equal(3918344u, read<unsigned>(ser));

					};
					s.request(req[0], 121, 3918344u, 1, [] (client_session::deserializer &) {});

					// ASSERT
					assert_equal(1, messages);

					// ACT
					outbound->on_message = [&] (const_byte_range payload) {
						buffer_reader br(payload); client_session::deserializer ser(br); messages++;

					// ASSERT
						assert_equal(1001, read<int>(ser));
						assert_equal(2u, read<unsigned>(ser));
						assert_equal(sneaky_type::create(3141, "pi"), read<sneaky_type>(ser));
					};
					s.request(req[1], 1001, sneaky_type::create(3141, "pi"), 1, [] (client_session::deserializer &) {});

					// ASSERT
					assert_equal(2, messages);
				}


				test( CallbacksAreHeldAsLongAsRequestHandlesAreAlive )
				{
					// INIT
					shared_ptr<bool> alive1 = make_shared<bool>();
					shared_ptr<bool> alive2 = make_shared<bool>();
					shared_ptr<bool> alive3 = make_shared<bool>();
					shared_ptr<bool> alive4 = make_shared<bool>();
					client_session s([&] (channel &/*inbound*/)	{	return outbound = make_shared<mocks::channel>();	});

					// ACT
					s.request(req[0], 1, 1, 1, [alive1] (client_session::deserializer &) {});
					s.request(req[1], 1, 1, 1, [alive2] (client_session::deserializer &) {});
					s.request(req[2], 1, 1, 1, [alive3] (client_session::deserializer &) {});

					// ASSERT
					assert_not_null(req[0]);
					assert_not_null(req[1]);
					assert_not_null(req[2]);

					assert_not_equal(1, alive1.use_count());
					assert_not_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());

					// ACT
					req[1].reset();

					// ASSERT
					assert_not_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());

					// ACT
					s.request(req[3], 1, 1, 1, [alive4] (client_session::deserializer &) {});
					req[0].reset();

					// ASSERT
					assert_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());
					assert_not_equal(1, alive4.use_count());

					// ACT
					req[2].reset();
					req[3].reset();

					// ASSERT
					assert_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_equal(1, alive3.use_count());
					assert_equal(1, alive4.use_count());
				}


				test( AMessageIsOnlySentAfterTheRequestIsRegistered )
				{
					// INIT / ACT
					channel *inbound;
					client_session s([&] (channel &inbound_) -> shared_ptr<channel> {
						inbound = &inbound_;
						outbound = make_shared<mocks::channel>();
						return outbound;
					});
					auto called = 0;

					// ACT
					outbound->on_message = [&] (const_byte_range) {
						send_standard(*inbound, 1021, 1, 1);
					};
					s.request(req[0], 121, 3918344u, 1021, [&] (client_session::deserializer &) {
						called++;
					});

					// ASSERT
					assert_equal(1, called);
				}


				test( MessageCallbacksAreHeldAsLongAsSubscriptionHandlesAreAlive )
				{
					// INIT
					shared_ptr<bool> alive1 = make_shared<bool>();
					shared_ptr<bool> alive2 = make_shared<bool>();
					shared_ptr<bool> alive3 = make_shared<bool>();
					shared_ptr<bool> alive4 = make_shared<bool>();
					client_session s([&] (channel &/*inbound*/)	{	return outbound = make_shared<mocks::channel>();	});

					// ACT
					s.subscribe(req[0], 1, [alive1] (client_session::deserializer &) {});
					s.subscribe(req[1], 2, [alive2] (client_session::deserializer &) {});
					s.subscribe(req[2], 3, [alive3] (client_session::deserializer &) {});

					// ASSERT
					assert_not_null(req[0]);
					assert_not_null(req[1]);
					assert_not_null(req[2]);

					assert_not_equal(1, alive1.use_count());
					assert_not_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());

					// ACT
					req[1].reset();

					// ASSERT
					assert_not_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());

					// ACT
					s.subscribe(req[3], 10, [alive4] (client_session::deserializer &) {});
					req[0].reset();

					// ASSERT
					assert_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());
					assert_not_equal(1, alive4.use_count());

					// ACT
					req[2].reset();
					req[3].reset();

					// ASSERT
					assert_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_equal(1, alive3.use_count());
					assert_equal(1, alive4.use_count());
				}


				test( MessageCallbacksAreHeldAsLongAsSubscriptionHandlesAreAliveInPassiveSession )
				{
					// INIT
					outbound = make_shared<mocks::channel>();
					shared_ptr<bool> alive = make_shared<bool>();
					client_session s(static_cast<channel &>(*outbound));

					// ACT
					s.subscribe(req[0], 1, [alive] (client_session::deserializer &) {});

					// ASSERT
					assert_not_null(req[0]);

					assert_not_equal(1, alive.use_count());
				}


				test( CallbackIsSelectByResponseToken )
				{
					// INIT
					channel *inbound;
					client_session s([&] (channel &inbound_) -> shared_ptr<channel>	{
						inbound = &inbound_;
						outbound = make_shared<mocks::channel>();
						return outbound;
					});
					int calls[4] = { 0 };

					s.request(req[0], 1, 1, 1, [&] (client_session::deserializer &) {	calls[0]++;	});
					s.request(req[1], 1, 1, 1, [&] (client_session::deserializer &) {	calls[1]++;	});
					s.request(req[2], 1, 1, 1, [&] (client_session::deserializer &) {	calls[2]++;	});
					s.request(req[3], 1, 1, 1, [&] (client_session::deserializer &) {	calls[3]++;	});

					// ACT
					send_standard(*inbound, 1, 2, 1);

					// ASSERT
					int reference1[] = {	0, 1, 0, 0,	};

					assert_equal(reference1, calls);

					// ACT
					send_standard(*inbound, 1, 2, 1);
					send_standard(*inbound, 1, 3, 1);
					send_standard(*inbound, 1, 4, 1);

					// ASSERT
					int reference2[] = {	0, 2, 1, 1,	};

					assert_equal(reference2, calls);

					// ACT
					req[2].reset();
					send_standard(*inbound, 1, 1, 1);
					send_standard(*inbound, 1, 3, 1); // will not be called

					// ASSERT
					int reference3[] = {	1, 2, 1, 1,	};

					assert_equal(reference3, calls);
				}


				test( CallbackIsSelectByResponseTokenAndResponseID )
				{
					// INIT
					channel *inbound;
					client_session s([&] (channel &inbound_) -> shared_ptr<channel>	{
						inbound = &inbound_;
						outbound = make_shared<mocks::channel>();
						return outbound;
					});
					int calls[4] = { 0 };

					s.request(req[0], 1, 1, 101, [&] (client_session::deserializer &) {	calls[0]++;	});
					s.request(req[1], 1, 1, 102, [&] (client_session::deserializer &) {	calls[1]++;	});
					s.request(req[2], 1, 1, 103, [&] (client_session::deserializer &) {	calls[2]++;	});
					s.request(req[3], 1, 1, 104, [&] (client_session::deserializer &) {	calls[3]++;	});

					// ACT
					send_standard(*inbound, 104, 1, 0);
					send_standard(*inbound, 103, 2, 0);
					send_standard(*inbound, 101, 3, 0);
					send_standard(*inbound, 102, 4, 0);

					// ASSERT
					int reference1[] = {	0, 0, 0, 0,	};

					assert_equal(reference1, calls);

					// ACT
					send_standard(*inbound, 101, 1, 0);
					send_standard(*inbound, 102, 2, 0);
					send_standard(*inbound, 103, 3, 0);

					// ASSERT
					int reference2[] = {	1, 1, 1, 0,	};

					assert_equal(reference2, calls);

					// ACT
					send_standard(*inbound, 101, 1, 0);
					send_standard(*inbound, 104, 4, 0);

					// ASSERT
					int reference3[] = {	2, 1, 1, 1,	};

					assert_equal(reference3, calls);
				}


				test( MultiResponseRequestsAreHeldByASingleHandle )
				{
					// INIT
					shared_ptr<bool> alive1 = make_shared<bool>();
					shared_ptr<bool> alive2 = make_shared<bool>();
					shared_ptr<bool> alive3 = make_shared<bool>();
					shared_ptr<bool> alive4 = make_shared<bool>();
					shared_ptr<bool> alive5 = make_shared<bool>();
					pair<int, client_session::callback_t> mcb1[] = {
						make_pair(1, [alive1] (client_session::deserializer &) {	}),
						make_pair(2, [alive2] (client_session::deserializer &) {	}),
					};
					pair<int, client_session::callback_t> mcb2[] = {
						make_pair(1, [alive3] (client_session::deserializer &) {	}),
						make_pair(21, [alive4] (client_session::deserializer &) {	}),
						make_pair(30, [alive5] (client_session::deserializer &) {	}),
					};
					client_session s([&] (channel &/*inbound*/)	{	return outbound = make_shared<mocks::channel>();	});

					// ACT
					s.request(req[0], 1, 0, mcb1);
					mcb1[0].second = client_session::callback_t();
					mcb1[1].second = client_session::callback_t();

					s.request(req[1], 1, 0, mcb2);
					mcb2[0].second = client_session::callback_t();
					mcb2[1].second = client_session::callback_t();
					mcb2[2].second = client_session::callback_t();

					// ASSERT
					assert_not_null(req[0]);
					assert_not_null(req[1]);
					assert_not_equal(1, alive1.use_count());
					assert_not_equal(1, alive2.use_count());
					assert_not_equal(1, alive3.use_count());
					assert_not_equal(1, alive4.use_count());
					assert_not_equal(1, alive5.use_count());

					// ACT
					req[1].reset();

					// ASSERT
					assert_not_equal(1, alive1.use_count());
					assert_not_equal(1, alive2.use_count());
					assert_equal(1, alive3.use_count());
					assert_equal(1, alive4.use_count());
					assert_equal(1, alive5.use_count());

					// ACT
					req[0].reset();

					// ASSERT
					assert_equal(1, alive1.use_count());
					assert_equal(1, alive2.use_count());
					assert_equal(1, alive3.use_count());
					assert_equal(1, alive4.use_count());
					assert_equal(1, alive5.use_count());
				}


				test( MessagePayloadIsDeliveredToMessageCallback )
				{
					// INIT
					channel *inbound;
					client_session s([&] (channel &inbound_) -> shared_ptr<channel>	{
						inbound = &inbound_;
						outbound = make_shared<mocks::channel>();
						return outbound;
					});
					int calls[4] = { 0 };

					s.subscribe(req[0], 13, [&] (client_session::deserializer &d) {
						calls[0]++;

					// ASSERT
						assert_equal(191983, read<int>(d));
					});
					s.request(req[1], 1, 1, 13, [&] (client_session::deserializer &) {
						assert_is_false(true);
					});
					s.subscribe(req[2], 14, [&] (client_session::deserializer &d) {
						calls[2]++;

					// ASSERT
						assert_equal("moonlight", read<string>(d));
					});
					s.subscribe(req[3], 15, [&] (client_session::deserializer &d) {
						calls[3]++;

					// ASSERT
						assert_equal("A long, long road", read<string>(d));
					});

					// ACT
					send_message(*inbound, 13, 191983);

					// ASSERT
					int reference1[] = {	1, 0, 0, 0,	};

					assert_equal(reference1, calls);

					// ACT
					send_message(*inbound, 14, string("moonlight"));
					send_message(*inbound, 15, string("A long, long road"));

					// ASSERT
					int reference2[] = {	1, 0, 1, 1,	};

					assert_equal(reference2, calls);
				}


				test( DisconnectingSessionDisconnectsOutbound )
				{
					// INIT
					client_session s([&] (channel &) {
						return outbound = make_shared<mocks::channel>();
					});
					auto disconnected = 0;

					outbound->on_disconnect = [&] {	disconnected++;	};

					// ACT
					s.disconnect_session();

					// ASSERT
					assert_equal(1, disconnected);

					// ACT (to consider: maybe reject further requests, including disconnections?)
					s.disconnect_session();
					s.disconnect_session();

					// ASSERT
					assert_equal(3, disconnected);
				}

			end_test_suite
		}
	}
}
