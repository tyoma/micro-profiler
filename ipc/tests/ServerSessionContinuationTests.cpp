#include <ipc/server_session.h>

#include "helpers.h"
#include "mocks.h"

#include <stdexcept>
#include <test-helpers/mock_queue.h>
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

			begin_test_suite( ServerSessionContinuationTests )
				mocks::channel outbound;
				shared_ptr<micro_profiler::tests::mocks::queue> queue;

				init( Init )
				{
					queue = make_shared<micro_profiler::tests::mocks::queue>();
				}


				test( AttemptToDeferResponseWithoutAQueueFails )
				{
					// INIT
					server_session s(outbound);

					s.add_handler<int>(13, [&] (server_session::request &req, int) {

					// ACT / ASSERT
						assert_throws(req.defer([] (server_session::request &) {}), logic_error);
					});

					// ACT / ASSERT
					send_standard(s, 13, 1, 0);
				}


				test( DeferringAResponsePlacesTasksInQueueProvided )
				{
					// INIT
					server_session s(outbound, queue);

					outbound.on_message = [] (const_byte_range) {	assert_is_false(true);	};
					s.add_handler<int>(1, [&] (server_session::request &req, int) {
						req.defer([] (server_session::request &) {});
					});

					// ACT
					send_standard(s, 1, 1, 0);

					// ASSERT
					assert_equal(1u, queue->tasks.size());

					// ACT
					send_standard(s, 1, 14, 0);
					send_standard(s, 1, 13, 0);

					// ASSERT
					assert_equal(3u, queue->tasks.size());
				}


				test( TaskIsNotQueuedUntilAfterHandlerExits )
				{
					// INIT
					server_session s(outbound, queue);

					s.add_handler<int>(1, [&] (server_session::request &req, int) {
						req.defer([] (server_session::request &) {});

					// ASSERT
						assert_is_empty(queue->tasks);
					});

					// ACT
					send_standard(s, 1, 1, 0);
				}


				test( ExecutionOfADeferredResponseTaskDoesNothingAfterSessionIsDestroyed )
				{
					// INIT
					unique_ptr<server_session> s(new server_session(outbound, queue));
					auto called = 0;
					auto cb = [&] (server_session::request &) {	called++;	};

					outbound.on_message = [] (const_byte_range) {	assert_is_false(true);	};
					s->add_handler<int>(1, [cb] (server_session::request &req, int) {
						req.defer(cb);
					});
					send_standard(*s, 1, 1, 0);

					// ACT / ASSERT
					s.reset();
					queue->run_one();

					// ASSERT
					assert_equal(0, called);
					
				}


				test( ResponsesSentFromContinuationCarryTokensOfTheOriginalRequests )
				{
					// INIT
					server_session s(outbound, queue);
					vector< pair<int, unsigned> > log;

					outbound.on_message = [&] (const_byte_range payload) {
						buffer_reader r(payload);
						strmd::deserializer<buffer_reader, strmd::varint> d(r);

						log.push_back(make_pair(0, 0u));
						d(log.back().first);
						d(log.back().second);
					};
					s.add_handler<int>(1, [] (server_session::request &req, int) {
						req.defer([] (server_session::request &req) {
							req.respond(1001, [] (server_session::serializer &) {});
						});
					});
					s.add_handler<int>(13, [] (server_session::request &req, int) {
						req.defer([] (server_session::request &req) {
							req.respond(1013, [] (server_session::serializer &) {});
						});
					});

					send_standard(s, 1, 81, 0);
					send_standard(s, 13, 810, 0);
					send_standard(s, 1, 91, 0);

					// ACT / ASSERT
					queue->run_one();

					// ASSERT
					pair<int, unsigned> reference1[] = {	make_pair(1001, 81),	};

					assert_equal(reference1, log);

					// ACT / ASSERT
					send_standard(s, 1, 90, 0);
					queue->run_till_end();

					// ASSERT
					pair<int, unsigned> reference2[] = {
						make_pair(1001, 81u), make_pair(1013, 810u), make_pair(1001, 91u), make_pair(1001, 90u),
					};

					assert_equal(reference2, log);
				}


				test( DeferralsCanBeChained )
				{
					// INIT
					server_session s(outbound, queue);
					vector< pair<int, unsigned> > log;

					outbound.on_message = [&] (const_byte_range payload) {
						buffer_reader r(payload);
						strmd::deserializer<buffer_reader, strmd::varint> d(r);

						log.push_back(make_pair(0, 0u));
						d(log.back().first);
						d(log.back().second);
					};
					s.add_handler<int>(1, [] (server_session::request &req, int) {
						req.defer([] (server_session::request &req) {
							req.defer([] (server_session::request &req) {
								req.respond(1002, [] (server_session::serializer &) {});
								req.defer([] (server_session::request &req) {
									req.respond(1003, [] (server_session::serializer &) {});
								});
							});
							req.respond(1001, [] (server_session::serializer &) {});
						});
					});

					send_standard(s, 1, 81, 0);

					// ACT
					queue->run_one();

					// ASSERT
					assert_equal(1u, queue->tasks.size());
					assert_equal(1u, log.size());

					// ACT
					queue->run_one();

					// ASSERT
					assert_equal(1u, queue->tasks.size());
					assert_equal(2u, log.size());

					// ACT
					queue->run_one();

					// ASSERT
					assert_equal(0u, queue->tasks.size());

					pair<int, unsigned> reference[] = {
						make_pair(1001, 81u), make_pair(1002, 81u), make_pair(1003, 81u),
					};

					assert_equal(reference, log);
				}
			end_test_suite
		}
	}
}
