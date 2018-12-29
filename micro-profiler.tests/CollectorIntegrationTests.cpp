#include <collector/entry.h>

#include <micro-profiler.tests/guineapigs/guinea_runner.h>

#include <common/constants.h>
#include <ipc/endpoint.h>
#include <mt/atomic.h>
#include <mt/event.h>
#include <stdlib.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_frontend.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		string format_endpoint_id()
		{
			char buffer[100];
			static mt::atomic<int> port(6110);

			sprintf(buffer, "sockets|127.0.0.1:%d", port.fetch_add(1));
			return buffer;
		}

		class runner_controller : public ipc::server, public ipc::channel, public enable_shared_from_this<ipc::channel>
		{
		public:
			bool wait_connection()
			{	return _ready.wait(mt::milliseconds(10000));	}

			void load_module(const string &module)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter> s(b);

				s(tests::load_module);
				s(module);
				_outbound->message(mkrange<byte>(b.buffer));
			}

			void unload_module(const string &module)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter> s(b);

				s(tests::unload_module);
				s(module);
				_outbound->message(mkrange<byte>(b.buffer));
			}

			void execute_function(const string &module, const string &fn)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter> s(b);

				s(tests::execute_function);
				s(module);
				s(fn);
				_outbound->message(mkrange<byte>(b.buffer));
			}

		private:
			virtual shared_ptr<ipc::channel> create_session(ipc::channel &outbound)
			{
				_outbound = &outbound;
				_ready.set();
				return shared_from_this();
			}

			virtual void disconnect() throw()
			{	}

			virtual void message(const_byte_range /*payload*/)
			{	}

		private:
			mt::event _ready;
			ipc::channel *_outbound;
		};

		begin_test_suite( CollectorIntegrationTests )

			struct frontend_factory : mocks::frontend_state, ipc::server
			{
				virtual shared_ptr<ipc::channel> create_session(ipc::channel &/*outbound*/)
				{	return create();	}
			};

			shared_ptr<frontend_factory> frontend_state;
			shared_ptr<runner_controller> runner;
			shared_ptr<void> hserver, hrunner;

			init( PrepareFrontend )
			{
				string frontend_id(format_endpoint_id()), runner_id(format_endpoint_id());

				setenv(c_frontend_id_env, frontend_id.c_str(), 1);

				frontend_state.reset(new frontend_factory);
				hserver = ipc::run_server(frontend_id.c_str(), frontend_state);
				
				runner.reset(new runner_controller);
				hrunner= ipc::run_server(runner_id.c_str(), runner);
#ifdef _WIN32
				system(("start guinea_runner \"" + runner_id + "\"").c_str());
#else
				system(("./guinea_runner \"" + runner_id + "\" &").c_str());
#endif
				assert_is_true(runner->wait_connection());
			}


			test( ModuleLoadedIsPostOnProfileeLoad )
			{
				// INIT
				mt::event ready;

				frontend_state->modules_loaded = bind(&mt::event::set, &ready);

				// ACT
				runner->load_module("symbol_container_2_instrumented");

				// ASSERT
				ready.wait();
			}


			test( ModuleUnloadedIsPostOnProfileeUnload )
			{
				// INIT
				mt::event ready;

				frontend_state->modules_unloaded = bind(&mt::event::set, &ready);
				runner->load_module("symbol_container_2_instrumented");

				// ACT
				runner->unload_module("symbol_container_2_instrumented");

				// ASSERT
				ready.wait();
			}


			test( StatisticsIsReceivedFromProfillee )
			{
				// INIT
				typedef void (f21_t)(int * volatile begin, int * volatile end);
				typedef int (f22_t)(char *buffer, size_t count, const char *format, ...);
				typedef void (f2F_t)(void (*&f)(int * volatile begin, int * volatile end));

				mt::event ready;
				auto_ptr<image> guineapig;
				shared_ptr< vector<mocks::statistics_map_detailed> >
					statistics(new vector<mocks::statistics_map_detailed>);
				char buffer[100];
				int data[10];

				frontend_state->updated = [&, statistics] (const mocks::statistics_map_detailed &u) {
					statistics->push_back(u);
					ready.set();
				};
				
				guineapig.reset(new image(L"symbol_container_2_instrumented"));

				f22_t *f22 = guineapig->get_symbol<f22_t>("guinea_snprintf");
				f2F_t *f2F = guineapig->get_symbol<f2F_t>("bubble_sort_expose");
				f21_t *f21;

				// ACT
				f22(buffer, sizeof(buffer), "testtest %d %d", 10, 11);
				ready.wait();

				// ASSERT
				assert_equal(1u, statistics->size());
				assert_is_true((*statistics)[0].size() >= 1u);

				// INIT
				f2F(f21);

				// ACT
				f21(data, data + 10);
				ready.wait();

				// ASSERT
				assert_equal(2u, statistics->size());
				assert_is_true((*statistics)[1].size() >= 1u);
			}

		end_test_suite
	}
}
