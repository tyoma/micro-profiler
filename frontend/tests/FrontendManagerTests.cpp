#include <frontend/frontend_manager.h>

#include <frontend/function_list.h>

#include "helpers.h"
#include "mocks.h"
#include "mock_channel.h"

#include <algorithm>
#include <common/serialization.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef statistic_types_t<unsigned> unthreaded_statistic_types;

			frontend_ui::ptr dummy_ui_factory(const frontend_ui_context &)
			{	return frontend_ui::ptr();	}

			frontend_ui_context create_ui_context(string executable, shared_ptr<functions_list> model)
			{
				frontend_ui_context ctx = {	{	executable, 123	}, model	};

				return ctx;
			}


			template <typename CommandDataT>
			void message(ipc::channel &channel, messages_id c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}

			initialization_data make_initialization_data(const string &executable, timestamp_t ticks_per_second)
			{
				initialization_data idata = {	executable, ticks_per_second };
				return idata;
			}

			template <typename T, size_t n>
			void reset_all(T (&a)[n])
			{	fill_n(a, n, T());	}
		}

		namespace mocks
		{
			class frontend_ui : public micro_profiler::frontend_ui
			{		
			public:
				frontend_ui(const frontend_ui_context &ui_context_)
					: ui_context(ui_context_), close_on_destroy(true)
				{	}

				~frontend_ui()
				{
					if (close_on_destroy)
						closed(); // this is here only to make frontend_manager's life harder - it should ignore this signal.
				}

				void emulate_close()
				{	closed();	}

			public:
				frontend_ui_context ui_context;
				bool close_on_destroy;

			private:
				virtual void activate() {	}
			};
		}


		begin_test_suite( FrontendManagerTests )

			mocks::outbound_channel outbound;
			shared_ptr<tables::statistics> statistics;
			shared_ptr<tables::modules> modules;
			shared_ptr<tables::module_mappings> mappings;

			init( Init )
			{
				statistics = make_shared<tables::statistics>();
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
			}


			test( OpeningFrontendChannelIncrementsInstanceCount )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory);

				// ACT / ASSERT (must not throw)
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);

				// ACT / ASSERT
				assert_equal(1u, m.instances_count());

				// ACT
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				// ACT / ASSERT
				assert_equal(2u, m.instances_count());
			}


			test( ClosingFrontendChannelsDecrementsInstanceCount )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory);
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				// ACT
				c2.reset();

				// ACT / ASSERT
				assert_equal(1u, m.instances_count());
			}


			vector< shared_ptr<mocks::frontend_ui> > _ui_creation_log;

			frontend_ui::ptr log_ui_creation(const frontend_ui_context &ui_context)
			{
				shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(ui_context));

				_ui_creation_log.push_back(ui);
				return ui;
			}

			test( FrontendInitializaionLeadsToUICreation )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				// ACT
				message(*c1, init, make_initialization_data("c:\\test\\some.exe", 12332));

				// ASSERT
				assert_equal(1u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[0]->ui_context.model);
				assert_not_null(_ui_creation_log[0]->ui_context.model->resolver);
				assert_not_null(_ui_creation_log[0]->ui_context.model->threads);
				assert_equal("c:\\test\\some.exe", _ui_creation_log[0]->ui_context.process_info.executable);

				// ACT
				message(*c2, init, make_initialization_data("kernel.exe", 12332));

				// ASSERT
				assert_equal(2u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[1]->ui_context.model);
				assert_not_null(_ui_creation_log[1]->ui_context.model->resolver);
				assert_not_null(_ui_creation_log[1]->ui_context.model->threads);
				assert_equal("kernel.exe", _ui_creation_log[1]->ui_context.process_info.executable);
				assert_not_equal(_ui_creation_log[0]->ui_context.model, _ui_creation_log[1]->ui_context.model);
				assert_not_equal(_ui_creation_log[0]->ui_context.model->resolver, _ui_creation_log[1]->ui_context.model->resolver);
				assert_not_equal(_ui_creation_log[0]->ui_context.model->threads, _ui_creation_log[1]->ui_context.model->threads);
			}


			obsolete_test( FrontendManagerIsHeldByFrontendsAlive )


			test( ThereAreNoInstancesAfterCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<mocks::symbol_resolver> sr(new mocks::symbol_resolver(modules, mappings));
				shared_ptr<mocks::threads_model> tmodel(new mocks::threads_model);
				auto s1 = make_shared<tables::statistics>();
				auto fl1 = make_shared<functions_list>(statistics, 1.0 / 123, sr, tmodel);
				auto s2 = make_shared<tables::statistics>();
				auto fl2 = make_shared<functions_list>(statistics, 1.0 / 123, sr, tmodel);

				m.load_session(create_ui_context("somefile.exe", fl1));
				m.load_session(create_ui_context("other", fl1));

				// ACT
				m.close_all();

				// ASSERT
				assert_equal(0u, m.instances_count());
				assert_null(m.get_active());
			}


			test( UIEventsAreIgnoredAfterCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				message(*c1, init, make_initialization_data("", 10));
				message(*c2, init, make_initialization_data("", 15));

				// ACT
				m.close_all();

				// ASSERT
				assert_null(m.get_active());

				// ACT
				_ui_creation_log[0]->activated();

				// ASSERT
				assert_null(m.get_active());

			}


			test( FrontendsAreDisconnectedWhenManagerIsReleased )
			{
				// INIT
				mocks::outbound_channel outbound_channels[2];
				shared_ptr<frontend_manager> m(new frontend_manager(&dummy_ui_factory));
				shared_ptr<ipc::channel> c1 = m->create_session(outbound_channels[0]);
				shared_ptr<ipc::channel> c2 = m->create_session(outbound_channels[1]);

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(outbound_channels[0].disconnected);
				assert_is_true(outbound_channels[1].disconnected);
			}


			test( FrontendForClosedUIIsDisconnected )
			{
				// INIT
				mocks::outbound_channel outbound_channels[3];
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound_channels[0]),
					m.create_session(outbound_channels[1]),
					m.create_session(outbound_channels[2]),
				};

				message(*c[0], init, make_initialization_data("", 1));
				message(*c[1], init, make_initialization_data("", 1));
				message(*c[2], init, make_initialization_data("", 1));

				// ACT
				_ui_creation_log[0]->emulate_close();
				_ui_creation_log[2]->emulate_close();

				// ASSERT
				assert_is_true(outbound_channels[0].disconnected);
				assert_is_true(outbound_channels[2].disconnected);

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ASSERT
				assert_is_true(outbound_channels[1].disconnected);
			}


			vector< weak_ptr<mocks::frontend_ui> > _ui_creation_log_w;

			frontend_ui::ptr log_ui_creation_w(const frontend_ui_context &ui_context)
			{
				shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(ui_context));

				_ui_creation_log_w.push_back(ui);
				return ui;
			}

			test( FrontendUIIsHeldAfterCreation )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1));
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				// ACT
				message(*c, init, initialization_data());

				// ASSERT
				assert_equal(1u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
			}


			test( UIIsHeldEvenAfterFrontendReleased )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());

				// ACT
				c[0].reset();

				// ASSERT
				assert_equal(2u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
				assert_is_false(_ui_creation_log_w[1].expired());
			}


			test( InstancesAreManagedByOpenedUIsAfterFrontendsReleased )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				message(*c[2], init, initialization_data());

				// ACT
				reset_all(c);

				// ASSERT
				assert_equal(3u, m.instances_count());

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(2u, m.instances_count());

				// ACT
				_ui_creation_log[2]->emulate_close();
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_equal(0u, m.instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferenced )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1));
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				message(*c, init, initialization_data());

				// ACT
				_ui_creation_log_w[0].lock()->emulate_close();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m.instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferencedNoExternalUIReference )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1));
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				message(*c, init, initialization_data());

				// ACT (must not crash - make sure even doubled close doesn't blow things up)
				m.get_instance(0)->ui->closed();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m.instances_count());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsHeld )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1)));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				message(*c, init, initialization_data());

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsReleased )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1)));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				message(*c, init, initialization_data());
				c.reset();

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( OnlyExternallyHeldUISurvivesManagerDestruction )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1)));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				message(*c[2], init, initialization_data());
				reset_all(c);

				auto ui = _ui_creation_log_w[2].lock();

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_is_true(_ui_creation_log_w[1].expired());
				assert_is_false(_ui_creation_log_w[2].expired());
			}


			obsolete_test( ManagerIsHeldByOpenedUI )


			test( NoInstanceIsReturnedForEmptyManager )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory);

				// ACT / ASSERT
				assert_null(m.get_instance(0));
				assert_null(m.get_instance(10));
			}


			test( InstancesAreReturned )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory);

				// ACT
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);
				shared_ptr<ipc::channel> c3 = m.create_session(outbound);

				// ACT / ASSERT
				assert_not_null(m.get_instance(0));
				assert_is_empty(m.get_instance(0)->process_info.executable);
				assert_null(m.get_instance(0)->model);
				assert_not_null(m.get_instance(1));
				assert_not_null(m.get_instance(2));
				assert_null(m.get_instance(3));
			}


			test( InstanceContainsExecutableNameModelAndUIAfterInitialization )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				// ACT
				message(*c, init, make_initialization_data("c:\\dev\\micro-profiler", 1));

				// ACT / ASSERT
				assert_not_null(m.get_instance(0));
				assert_equal("c:\\dev\\micro-profiler", m.get_instance(0)->process_info.executable);
				assert_equal(_ui_creation_log[0]->ui_context.model, m.get_instance(0)->model);
				assert_equal(_ui_creation_log[0], m.get_instance(0)->ui);
			}


			test( ClosingAllInstancesDisconnectsAllFrontendsAndDestroysAllUI )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1));
				mocks::outbound_channel outbound_channels[3];
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound_channels[0]), m.create_session(outbound_channels[1]), m.create_session(outbound_channels[2]),
				};

				message(*c[0], init, make_initialization_data("", 1));
				message(*c[1], init, make_initialization_data("", 1));
				message(*c[2], init, make_initialization_data("", 1));
				
				// ACT
				m.close_all();

				// ASSERT
				assert_is_true(outbound_channels[0].disconnected);
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_is_true(outbound_channels[1].disconnected);
				assert_is_true(_ui_creation_log_w[1].expired());
				assert_is_true(outbound_channels[2].disconnected);
				assert_is_true(_ui_creation_log_w[2].expired());

				// ACT
				reset_all(c);

				// ASSERT
				assert_equal(0u, m.instances_count());
			}


			test( NoActiveInstanceInEmptyManager )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( NoInstanceConsideredActiveIfNoUICreated )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				// ACT
				message(*c[0], init, initialization_data());

				// ASSERT
				assert_null(m.get_active());
			}


			test( LastInitializedInstanceConsideredActive )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				// ACT
				message(*c[0], init, initialization_data());

				// ASSERT
				assert_not_null(m.get_active());
				assert_equal(m.get_instance(0), m.get_active());

				// ACT
				message(*c[1], init, initialization_data());

				// ASSERT
				assert_equal(m.get_instance(1), m.get_active());
			}


			test( ActiveInstanceIsResetOnFrontendReleaseEvenIfUIDidNotSignalledOfClose )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				message(*c, init, initialization_data()); // make it active
				_ui_creation_log[0]->close_on_destroy = false;

				// ACT
				m.close_all();
				c.reset();

				// ASSERT
				assert_null(m.get_active());
			}


			test( ActiveInstanceIsNotResetOnFrontendReleaseWhenOtherAliveInstanceIsActive )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);

				message(*c1, init, initialization_data());
				_ui_creation_log[0]->close_on_destroy = false;

				// ACT
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);
				message(*c2, init, initialization_data());
				c1.reset();

				// ASSERT
				assert_not_null(m.get_active());
			}


			test( UIActivationSwitchesActiveInstanceInManager )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				message(*c[2], init, initialization_data());

				// ACT
				_ui_creation_log[1]->activated();

				// ASSERT
				assert_not_null(m.get_active());
				assert_equal(m.get_instance(1), m.get_active());

				// ACT
				_ui_creation_log[0]->activated();

				// ASSERT
				assert_equal(m.get_instance(0), m.get_active());

				// ACT
				_ui_creation_log[2]->activated();

				// ASSERT
				assert_equal(m.get_instance(2), m.get_active());
			}


			test( NoInstanceIsActiveAfterCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				_ui_creation_log_w[0].lock()->activated();

				// ACT
				m.close_all();

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( NoInstanceIsActiveAfterActiveInstanceIsClosed )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( ActiveInstancetIsIntactAfterInactiveInstanceIsClosed )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				message(*c[0], init, initialization_data());
				message(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(m.get_instance(0), m.get_active());
			}


			test( CreatingInstanceFromModelAddsNewInstanceToTheListAndConstructsUI )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<mocks::symbol_resolver> sr(new mocks::symbol_resolver(modules, mappings));
				shared_ptr<mocks::threads_model> tmodel(new mocks::threads_model);
				auto s1 = make_shared<tables::statistics>();
				auto fl1 = make_shared<functions_list>(statistics, 1.0 / 123, sr, tmodel);
				auto s2 = make_shared<tables::statistics>();
				auto fl2 = make_shared<functions_list>(statistics, 1.0 / 123, sr, tmodel);

				// ACT
				m.load_session(create_ui_context("somefile.exe", fl1));

				// ASSERT
				assert_equal(1u, m.instances_count());
				assert_equal("somefile.exe", m.get_instance(0)->process_info.executable);
				assert_equal(fl1, m.get_instance(0)->model);
				assert_equal(_ui_creation_log[0], m.get_instance(0)->ui);

				// ACT
				m.load_session(create_ui_context("jump.exe", fl2));

				// ASSERT
				assert_equal(2u, m.instances_count());
				assert_equal("jump.exe", m.get_instance(1)->process_info.executable);
				assert_equal(fl2, m.get_instance(1)->model);
				assert_equal(_ui_creation_log[1], m.get_instance(1)->ui);
			}


			test( NoAttemptToDisconnectFrontendIsMadeIfNoFrontendExistsOnCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1));
				shared_ptr<mocks::symbol_resolver> sr(new mocks::symbol_resolver(modules, mappings));
				shared_ptr<mocks::threads_model> tmodel(new mocks::threads_model);
				auto s1 = make_shared<tables::statistics>();
				auto fl1 = make_shared<functions_list>(statistics, 1.0 / 123, sr, tmodel);
				auto s2 = make_shared<tables::statistics>();
				auto fl2 = make_shared<functions_list>(statistics, 1.0 / 123, sr, tmodel);

				// ACT
				m.load_session(create_ui_context("somefile.exe", fl1));

				// ACT / ASSERT (does not crash)
				m.close_all();
				m.close_all();
			}

		end_test_suite
	}
}
