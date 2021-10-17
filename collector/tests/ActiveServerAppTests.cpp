#include <collector/active_server_app.h>

#include <ipc/client_session.h>
#include <ipc/server_session.h>
#include <mt/event.h>
#include <mt/mutex.h>
#include <test-helpers/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			using ipc::serializer;
			using ipc::deserializer;

			class test_app_events : public active_server_app::events
			{
			public:
				function<void (ipc::server_session &session)> initializing;
				function<bool (ipc::server_session &session)> finalizing;

			private:
				virtual void initialize_session(ipc::server_session &session) override
				{
					if (initializing)
						initializing(session);
				}

				virtual bool finalize_session(ipc::server_session &session) override
				{	return finalizing ? finalizing(session) : false;	}
			};

			template <typename T>
			void send(ipc::server_session &session, int code, const T &data)
			{	session.message(code, [&data] (serializer &s) {	s(data);	});	}
		}

		begin_test_suite( ActiveServerAppTests )
			active_server_app::frontend_factory_t factory;
			shared_ptr<ipc::client_session> client;
			function<void (ipc::client_session &client_)> initialize_client;
			mt::event client_ready;
			test_app_events app_events;

			init( Init )
			{
				factory = [this] (ipc::channel &outbound) -> ipc::channel_ptr_t {
					client = make_shared<ipc::client_session>(outbound);
					if (initialize_client)
						initialize_client(*client);
					client_ready.set();
					return client;
				};
			}

			teardown( Term )
			{
				client.reset();
			}


			test( FrontendIsConstructedInASeparateThreadOnStart )
			{
				// INIT
				mt::thread::id tid = mt::this_thread::get_id();
				mt::event ready;

				initialize_client = [&] (ipc::client_session &) {
					tid = mt::this_thread::get_id();
					ready.set();
				};

				// INIT / ACT
				active_server_app app(app_events, factory);

				// ACT / ASSERT (must not hang)
				ready.wait();

				// ASSERT
				assert_not_equal(mt::this_thread::get_id(), tid);
			}


			test( WorkerThreadIsStoppedOnDestruction )
			{
				// INIT
				mt::event ready;
				shared_ptr<mt::event> hthread;

				initialize_client = [&] (ipc::client_session &) {
					hthread = this_thread::open();
					ready.set();
				};

				unique_ptr<active_server_app> app(new active_server_app(app_events, factory));

				ready.wait();

				// ACT
				app.reset();

				// ASSERT
				assert_is_true(hthread->wait(mt::milliseconds(0)));
			}


			test( FrontendIsDestroyedFromTheConstructingThread )
			{
				// INIT
				auto destroyed_ok = false;
				mt::event ready;

				unique_ptr<active_server_app> app(new active_server_app(app_events,
					[&] (ipc::channel &c) -> ipc::channel_ptr_t {

					auto &destroyed_ok_ = destroyed_ok;
					auto thread_id = mt::this_thread::get_id();
					shared_ptr<ipc::client_session> client_(new ipc::client_session(c),
						[thread_id, &destroyed_ok_] (ipc::client_session *p) {
						destroyed_ok_ = thread_id == mt::this_thread::get_id();
						delete p;
					});

					ready.set();
					return client_;
				}));

				ready.wait();

				// ACT
				app.reset();

				// ASSERT
				assert_is_true(destroyed_ok);
			}


			test( TwoCoexistingAppsHaveDifferentWorkerThreads )
			{
				// INIT
				vector<mt::thread::id> tids_;
				mt::event go;
				mt::mutex mtx;

				initialize_client = [&] (ipc::client_session &) {
					mt::lock_guard<mt::mutex> l(mtx);
					tids_.push_back(mt::this_thread::get_id());
					if (2u == tids_.size())
						go.set();
				};

				// ACT
				active_server_app app1(app_events, factory);
				active_server_app app2(app_events, factory);

				go.wait();

				// ASSERT
				assert_not_equal(tids_[1], tids_[0]);
			}


			test( NotificationSentAtServerSessionInitializationIsDeliveredToTheClient )
			{
				// INIT
				mt::event received;
				vector<int> log;
				shared_ptr<void> subscription;

				initialize_client = [&] (ipc::client_session &c) {
					auto &received_ = received;
					auto &log_ = log;

					c.subscribe(subscription, 181923, [&] (deserializer &d) {
						int v;

						d(v);
						log_.push_back(v);
						received_.set();
					});
				};

				// ACT
				app_events.initializing = [] (ipc::server_session &session) {	send(session, 181923, 314159);	};
				
				active_server_app app1(app_events, factory);

				received.wait();

				// ASSERT
				int reference1[] = {	314159,	};

				assert_equal(reference1, log);

				// ACT
				app_events.initializing = [] (ipc::server_session &session) {	send(session, 181923, 27);	};
				
				active_server_app app2(app_events, factory);

				received.wait();

				// ASSERT
				int reference2[] = {	314159, 27,	};

				assert_equal(reference2, log);
			}


			test( ExistingSessionIsFinalizedOnDestruction )
			{
				// INIT
				auto finalized = false;
				mt::event ready;

				app_events.finalizing = [&finalized] (ipc::server_session &/*session*/) -> bool {
					finalized = true;
					return false;
				};

				unique_ptr<active_server_app> app(new active_server_app(app_events, factory));

				client_ready.wait();

				// ACT
				app->schedule([&ready] {	ready.set();	}, mt::milliseconds(50));

				// ASSERT
				assert_is_false(finalized);

				// ACT
				app.reset();

				// ASSERT
				assert_is_true(finalized);
			}


			test( NoFinalizationIsMadeIfSessionGetsDisconnected )
			{
				// INIT
				auto finalized = false;

				app_events.finalizing = [&finalized] (ipc::server_session &/*session*/) -> bool {
					finalized = true;
					return false;
				};

				unique_ptr<active_server_app> app(new active_server_app(app_events, factory));

				client_ready.wait();

				// ACT
				client->disconnect_session();
				app.reset();

				// ASSERT
				assert_is_false(finalized);
			}


			test( StoppingServerKeepsOperatingUntilClientDisconnectsIfFinalizationRequiresIt )
			{
				// INIT
				mt::event ready;
				shared_ptr<void> req;
				auto result = 0;

				app_events.initializing = [] (ipc::server_session &s) {
					s.add_handler(1717, [] (ipc::server_session::response &resp, int value) {	resp(1718, 17 * value);	});
				};
				app_events.finalizing = [&ready] (ipc::server_session &/*session*/) -> bool {
					ready.set();
					return true; // require session disconnection to stop
				};

				unique_ptr<active_server_app> app(new active_server_app(app_events, factory));

				client_ready.wait();

				// ACT
				mt::thread t([&app] {	app.reset();	});
				ready.wait();

				client->request(req, 1717, 19, 1718, [&] (deserializer &d) {	d(result),	ready.set();	});
				ready.wait();

				// ASSERT
				assert_equal(323, result);

				// ACT
				client->request(req, 1717, 10, 1718, [&] (deserializer &d) {	d(result),	ready.set();	});
				ready.wait();

				// ASSERT
				assert_equal(170, result);

				// ACT
				client->disconnect_session();

				// ASSERT (shall exit)
				t.join();
			}


			test( ClientGetsDestroyedOnDisconnection )
			{
				// INIT
				test_app_events events;
				mt::event client_destroyed;
				ipc::client_session *pclient = nullptr;
				const auto destroy_client = [&] (ipc::client_session *p) {
					delete p;
					client_destroyed.set();
				};

				unique_ptr<active_server_app> app(new active_server_app(app_events,
					[&] (ipc::channel &c) -> ipc::channel_ptr_t {
					shared_ptr<ipc::client_session> client_(new ipc::client_session(c), destroy_client);

					pclient = client_.get();
					client_ready.set();
					return client_;
				}));

				client_ready.wait();

				// ACT
				pclient->disconnect_session();

				// ASSERT (shall satisfy)
				client_destroyed.wait();
			}


			test( RequestsAreProcessedInWorkerThread )
			{
				// INIT
				test_app_events events;
				mt::thread::id thread_id;
				vector<mt::thread::id> log;
				mt::event ready;
				shared_ptr<void> req;

				app_events.initializing = [&] (ipc::server_session &s) {
					auto &log_ = log;
					auto &ready_ = ready;

					thread_id = mt::this_thread::get_id();
					s.add_handler(1, [&] (ipc::server_session::response &, int) {
						log_.push_back(mt::this_thread::get_id());
						ready_.set();
					});
				};

				active_server_app app(app_events, factory);

				client_ready.wait();

				// ACT
				client->request(req, 1, 0, 191919, [&] (deserializer &) {	});
				ready.wait();

				// ASSERT
				mt::thread::id reference[] = {	thread_id,	};

				assert_equal(reference, log);
			}


			test( HandlersAreDestroyedInWorkerThreadOnClientDisconnection )
			{
				// INIT
				test_app_events events;
				mt::thread::id thread_id;
				vector<mt::thread::id> log;
				mt::event ready;
				shared_ptr<void> p(new int, [&] (int *p) {
					log.push_back(mt::this_thread::get_id());
					ready.set();
					delete p;
				});

				app_events.initializing = [&] (ipc::server_session &s) {
					auto p_ = move(p);

					thread_id = mt::this_thread::get_id();
					s.add_handler(1, [p_] (ipc::server_session::response &, int) {	});
					p_.reset();
				};

				active_server_app app(app_events, factory);

				client_ready.wait();

				// ACT
				client->disconnect_session();

				// ASSERT (shall satisfy)
				ready.wait();

				// ASSERT
				mt::thread::id reference[] = {	thread_id,	};

				assert_equal(reference, log);
			}


			test( MessageSentFromFinalizationIsDeliveredToClient )
			{
				// INIT
				mt::event ready;
				shared_ptr<void> subscription;

				initialize_client = [&] (ipc::client_session &c) {
					auto &ready_ = ready;

					c.subscribe(subscription, 1122, [&] (deserializer &) {	ready_.set();	});
				};
				app_events.finalizing = [] (ipc::server_session &session) -> bool {
					send(session, 1122, 0);
					return false;
				};

				unique_ptr<active_server_app> app(new active_server_app(app_events, factory));

				client_ready.wait();

				// ACT
				mt::thread t([&app] {	app.reset();	});

				// ACT / ASSERT (shall satisfy)
				ready.wait();
				t.join();
			}

		end_test_suite
	}
}
