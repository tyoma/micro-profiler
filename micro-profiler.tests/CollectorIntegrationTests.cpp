#include <micro-profiler.tests/guineapigs/guinea_runner.h>

#include "mock_frontend.h"

#include <collector/serialization.h>
#include <common/constants.h>
#include <ipc/endpoint.h>
#include <ipc/misc.h>
#include <mt/atomic.h>
#include <mt/event.h>
#include <stdlib.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>
#include <test-helpers/process.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		string format_endpoint_id()
		{
			static mt::atomic<int> port(6110);

			return ipc::sockets_endpoint_id(ipc::localhost, static_cast<unsigned short>(port.fetch_add(1)));
		}

		struct guinea_session : controllee_session
		{
			guinea_session(ipc::channel &outbound)
				: controllee_session(outbound)
			{	}

			void load_module(const string &module)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter> s(b);

				s(tests::load_module);
				s(module);
				send(mkrange<byte>(b.buffer));
			}

			void unload_module(const string &module)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter> s(b);

				s(tests::unload_module);
				s(module);
				send(mkrange<byte>(b.buffer));
			}

			void execute_function(const string &module, const string &fn)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter> s(b);

				s(tests::execute_function);
				s(module);
				s(fn);
				send(mkrange<byte>(b.buffer));
			}
		};

		begin_test_suite( CollectorIntegrationTests )

			struct frontend_factory : mocks::frontend_state, ipc::server
			{
				frontend_factory()
					: ready(false, false)
				{	}

				virtual shared_ptr<ipc::channel> create_session(ipc::channel &outbound_)
				{
					outbound = &outbound_;
					ready.set();
					return create();
				}

				template <typename FormatterT>
				void request(const FormatterT &formatter)
				{
					vector_adapter message_buffer;
					strmd::serializer<vector_adapter, packer> ser(message_buffer);

					formatter(ser);
					ready.wait();
					outbound->message(const_byte_range(&message_buffer.buffer[0], message_buffer.buffer.size()));
				}

				mt::event ready;
				ipc::channel *outbound;
			};

			shared_ptr<frontend_factory> frontend_state;
			shared_ptr< runner_controller<guinea_session> > controller;
			shared_ptr<void> hserver, hrunner;

			init( PrepareFrontend )
			{
				string frontend_id(format_endpoint_id()), runner_id(format_endpoint_id());

				setenv(constants::frontend_id_ev, frontend_id.c_str(), 1);

				frontend_state.reset(new frontend_factory);
				hserver = ipc::run_server(frontend_id, frontend_state);
				
				controller.reset(new runner_controller<guinea_session>);
				hrunner= ipc::run_server(runner_id, controller);
#ifdef _WIN32
				system(("start guinea_runner \"" + runner_id + "\"").c_str());
#else
				system(("./guinea_runner \"" + runner_id + "\" &").c_str());
#endif
				assert_is_true(controller->wait_connection());
			}


			test( ModuleLoadedIsPostOnProfileeLoad )
			{
				// INIT
				mt::event ready;

				frontend_state->modules_loaded = bind(&mt::event::set, &ready);

				// ACT
				controller->sessions[0]->load_module("symbol_container_2_instrumented");
				frontend_state->request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_update), ser(0u);
				});

				// ASSERT
				ready.wait();
			}


			test( ModuleUnloadedIsPostOnProfileeUnload )
			{
				// INIT
				mt::event ready, ready2;

				frontend_state->modules_loaded = bind(&mt::event::set, &ready);
				frontend_state->modules_unloaded = bind(&mt::event::set, &ready2);
				controller->sessions[0]->load_module("symbol_container_2_instrumented");
				frontend_state->request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_update), ser(0u);
				});

				ready.wait();

				// ACT
				controller->sessions[0]->unload_module("symbol_container_2_instrumented");
				frontend_state->request([&] (strmd::serializer<vector_adapter, packer> &ser) {
					ser(request_update), ser(0u);
				});

				// ASSERT
				ready2.wait();
			}


			// TODO: restore after CMakeLists.txt refactoring
			ignored_test( StatisticsIsReceivedFromProfillee )
			{
				// INIT
				typedef void (f21_t)(int * volatile begin, int * volatile end);
				typedef int (f22_t)(char *buffer, size_t count, const char *format, ...);
				typedef void (f2F_t)(void (*&f)(int * volatile begin, int * volatile end));

				mt::event ready;
				unique_ptr<image> guineapig;
				shared_ptr< vector<mocks::thread_statistics_map> >
					statistics(new vector<mocks::thread_statistics_map>);
				char buffer[100];
				int data[10];

				frontend_state->updated = [&, statistics] (unsigned, const mocks::thread_statistics_map &u) {
					statistics->push_back(u);
					ready.set();
				};
				
				guineapig.reset(new image("symbol_container_2_instrumented"));

				f22_t *f22 = guineapig->get_symbol<f22_t>("guinea_snprintf");
				f2F_t *f2F = guineapig->get_symbol<f2F_t>("bubble_sort_expose");
				f21_t *f21;

				// ACT
				f22(buffer, sizeof(buffer), "testtest %d %d", 10, 11);
				ready.wait();

				// ASSERT
				assert_equal(1u, statistics->size()); // single update
				assert_equal(1u, (*statistics)[0].size()); // single thread
				assert_equal(1u, (*statistics)[0].begin()->second.size()); // single function

				// INIT
				f2F(f21);

				// ACT
				f21(data, data + 10);
				ready.wait();

				// ASSERT
				assert_equal(2u, statistics->size()); // two updates
				assert_equal(1u, (*statistics)[1].size()); // single thread
				assert_equal(1u, (*statistics)[1].begin()->second.size()); // single function
			}

		end_test_suite
	}
}
