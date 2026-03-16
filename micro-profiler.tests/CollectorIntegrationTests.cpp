#include <micro-profiler.tests/guineapigs/guinea_runner.h>

#include <coipc/client_session.h>
#include <coipc/misc.h>
#include <coipc/endpoint_spawn.h>
#include <common/constants.h>
#include <common/file_id.h>
#include <common/protocol.h>
#include <common/serialization.h>
#include <mt/event.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace coipc;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class server : public coipc::server
			{
			public:
				server(function<channel_ptr_t (channel &outbound)> initialize_client)
					: _initialize_client(initialize_client)
				{	}

				virtual channel_ptr_t create_session(channel &outbound) override
				{	return _initialize_client(outbound);	}

			private:
				function<channel_ptr_t (channel &outbound)> _initialize_client;
			};

			class client_session : public coipc::client_session
			{
			public:
				client_session(channel &outbound, const function<void (coipc::client_session &)> &disconnected)
					: coipc::client_session(outbound), _disconnected(disconnected)
				{	}

				virtual void disconnect() throw() override
				{	_disconnected(*this);	}

			private:
				function<void (coipc::client_session &)> _disconnected;
			};
		}

		begin_test_suite( CollectorIntegrationTests )
			function<void (client_session &client_)> disconnected;
			function<void (shared_ptr<client_session> client_)> initialize_client;
			function<void (client_session &client_)> exiting_;
			shared_ptr<void> hserver;
			list< shared_ptr<void> > subs;
			mt::event connection_ready;

			init( PrepareFrontend )
			{
				disconnected = [] (client_session &) {
					fprintf(stderr, "server disconnected...");
				};
				exiting_ = [] (client_session &client_) {	client_.disconnect_session();	};

				auto server_session = make_shared<mocks::server>([this] (channel &outbound) -> channel_ptr_t {
					auto client = make_shared<mocks::client_session>(outbound, disconnected);
					auto &r = *client;
					auto t = exiting_;

					client->subscribe(*subs.insert(subs.end(), shared_ptr<void>()), exiting, [&r, t] (deserializer &) {
						t(r);
					});
					initialize_client(client);
					return client;
				});

				for (unsigned short port = 6110; port < 6200; port++)
					try
					{
						auto frontend_id = sockets_endpoint_id(localhost, port);

						hserver = run_server(frontend_id, server_session);
						setenv(constants::frontend_id_ev, frontend_id.c_str(), 1);
						break;
					}
					catch (...)
					{	}
				assert_not_null(hserver);
			}


			shared_ptr<client_session> controller;
			shared_ptr<void> req;

			init( RunAndControlGuinea )
			{
				controller = make_shared<client_session>([] (channel &outbound) {
					return spawn::connect_client(c_guinea_runner, vector<string>(), outbound);
				});
			}


			test( ConnectionIsMadeOnRunningAProfilee )
			{
				// INIT
				shared_ptr<client_session> client;

				initialize_client = [&] (shared_ptr<client_session> c) {
					client = c;
					connection_ready.set();
				};

				// ACT
				controller->request(req, load_module, c_symbol_container_2_instrumented, 0, [] (deserializer &) {});

				// ACT / ASSERT (is satisfied)
				connection_ready.wait();
			}


			test( ConnectionIsKeptForTheLifetimeOfTheProfilee )
			{
				// INIT
				initialize_client = [&] (shared_ptr<client_session>) {	connection_ready.set();	};
				exiting_ = [&] (client_session &c) {
					c.disconnect_session();
					connection_ready.set();
				};

				controller->request(req, load_module, c_symbol_container_2_instrumented, 0, [] (deserializer &) {});
				connection_ready.wait();

				// ACT
				controller.reset();

				// ACT / ASSERT (is satisfied)
				connection_ready.wait();
			}


			test( ModuleLoadedIsPostOnProfileeLoad )
			{
				// INIT
				mt::event ready;
				client_session *client;
				loaded_modules l;

				initialize_client = [&] (shared_ptr<client_session> c) {
					client = c.get();
					connection_ready.set();
				};

				// ACT
				controller->request(req, load_module, c_symbol_container_2_instrumented, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				connection_ready.wait();
				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &dser) {	dser(l), ready.set();	});

				// ACT / ASSERT
				ready.wait();

				// ASSERT
				assert_is_true(any_of(l.begin(), l.end(), [] (const module::mapping_instance &m) {
					return file_id(m.second.path) == file_id(c_symbol_container_2_instrumented);
				}));

				// ACT
				controller->request(req, load_module, c_symbol_container_1, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				do
				{
					client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &dser) {	dser(l), ready.set();	});
					ready.wait();
				} while (l.empty() && (mt::this_thread::sleep_for(mt::milliseconds(20)), true));

				// ASSERT
				assert_is_false(any_of(l.begin(), l.end(), [] (const module::mapping_instance &m) {
					return file_id(m.second.path) == file_id(c_symbol_container_2_instrumented);
				}));
				assert_is_true(any_of(l.begin(), l.end(), [] (const module::mapping_instance &m) {
					return file_id(m.second.path) == file_id(c_symbol_container_1);
				}));
			}


			test( ModuleUnloadedIsPostOnProfileeUnload )
			{
				// INIT
				mt::event ready;
				client_session *client;
				map<id_t, module::mapping_ex> l;
				vector<id_t> u;

				initialize_client = [&] (shared_ptr<client_session> c) {
					client = c.get();
					connection_ready.set();
				};

				controller->request(req, load_module, c_symbol_container_1, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				controller->request(req, load_module, c_symbol_container_2, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				controller->request(req, load_module, c_symbol_container_2_instrumented, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				connection_ready.wait();
				client->request(req, request_update, 0, response_modules_loaded, [&] (deserializer &dser) {	dser(l), ready.set();	});
				ready.wait();

				// ACT
				controller->request(req, unload_module, c_symbol_container_1, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				do
				{
					client->request(req, request_update, 0, response_modules_unloaded, [&] (deserializer &dser) {	dser(u), ready.set();	});
					ready.wait();
				} while (u.empty() && (mt::this_thread::sleep_for(mt::milliseconds(20)), true));

				// ASSERT
				assert_equal(1u, u.size());
				assert_equal(file_id(c_symbol_container_1), file_id(l[u[0]].path));

				// ACT
				controller->request(req, unload_module, c_symbol_container_2, 1, [&] (deserializer &) {	ready.set();	});
				ready.wait();
				do
				{
					client->request(req, request_update, 0, response_modules_unloaded, [&] (deserializer &dser) {	dser(u), ready.set();	});
					ready.wait();
				} while (u.empty() && (mt::this_thread::sleep_for(mt::milliseconds(20)), true));


				// ASSERT
				assert_equal(1u, u.size());
				assert_equal(file_id(c_symbol_container_2), file_id(l[u[0]].path));
			}

		end_test_suite
	}
}
