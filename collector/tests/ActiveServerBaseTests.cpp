#include <collector/active_server_base.h>

#include <ipc/client_session.h>
#include <mt/event.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef ipc::client_session::deserializer deserializer;

			class test_active_server : public active_server_base
			{
			public:
				~test_active_server()
				{	stop(0);	}

				using active_server_base::start;
				using active_server_base::stop;

			public:
				function<bool ()> exiting;

			private:
				virtual void initialize_session(ipc::server_session &/*session*/) override
				{	}

				virtual bool /*wait for disconnection*/ on_exiting() override
				{	return exiting ? exiting() : false;	}
			};
		}

		begin_test_suite( ActiveServerBaseTests )
			active_server_base::frontend_factory_t factory;
			shared_ptr<ipc::client_session> client;
			function<void (ipc::client_session &client_)> initialize_client;
			mt::event client_ready;
			vector< shared_ptr<void> > subscriptions;

			shared_ptr<void> &new_subscription()
			{	return *subscriptions.insert(subscriptions.end(), shared_ptr<void>());	}

			init( Init )
			{
				factory = [this] (ipc::channel &outbound) -> shared_ptr<ipc::channel> {
					client = make_shared<ipc::client_session>(outbound);
					auto p = client.get();
					client->subscribe(new_subscription(), exiting, [p] (deserializer &) {	p->disconnect_session();	});
					if (initialize_client)
						initialize_client(*client);
					client_ready.set();
					return client;
				};
			}

			teardown( Term )
			{
				subscriptions.clear();
				client.reset();
			}


			test( FrontendIsConstructedInASeparateThreadOnStart )
			{
				// INIT
				mt::thread::id tid = mt::this_thread::get_id();
				mt::event ready;
				test_active_server app;

				initialize_client = [&] (ipc::client_session &) {
					tid = mt::this_thread::get_id();
					ready.set();
				};

				// INIT / ACT
				app.start(factory);

				// ACT / ASSERT (must not hang)
				ready.wait();

				// ASSERT
				assert_not_equal(mt::this_thread::get_id(), tid);
			}

		end_test_suite
	}
}
