#include <frontend/frontend_manager.h>

#include <frontend/function_list.h>
#include <frontend/serialization.h>

#include "helpers.h"
#include "mocks.h"
#include "mock_channel.h"

#include <algorithm>
#include <strmd/deserializer.h>
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

			frontend_ui::ptr dummy_ui_factory(const shared_ptr<functions_list> &, const string &)
			{	return frontend_ui::ptr();	}

			template <typename CommandDataT>
			void write(ipc::channel &channel, messages_id c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}

			template <typename Data1T, typename Data2T>
			void write(ipc::channel &channel, messages_id c, const Data1T &data1, const Data2T &data2)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data1);
				archive(data2);
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
				frontend_ui(const shared_ptr<functions_list> &model_, const string &process_name_)
					: model(model_), process_name(process_name_), close_on_destroy(true)
				{	}

				~frontend_ui()
				{
					if (close_on_destroy)
						closed(); // this is here only to make frontend_manager's life harder - it should ignore this signal.
				}

				void emulate_close()
				{	closed();	}

			public:
				shared_ptr<functions_list> model;
				string process_name;
				bool close_on_destroy;

			private:
				virtual void activate() {	}
			};
		}


		begin_test_suite( FrontendManagerTests )

			shared_ptr<mocks::queue> queue;
			mocks::outbound_channel outbound;

			init( CreateQueue )
			{
				queue = make_shared<mocks::queue>();
			}


			test( OpeningFrontendChannelIncrementsInstanceCount )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory, queue);

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
				frontend_manager m(&dummy_ui_factory, queue);
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				// ACT
				c2.reset();

				// ACT / ASSERT
				assert_equal(1u, m.instances_count());
			}


			vector< shared_ptr<mocks::frontend_ui> > _ui_creation_log;

			shared_ptr<frontend_ui> log_ui_creation(const shared_ptr<functions_list> &model, const string &process_name)
			{
				shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(model, process_name));

				_ui_creation_log.push_back(ui);
				return ui;
			}

			test( FrontendInitializaionLeadsToUICreation )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				// ACT
				write(*c1, init, make_initialization_data("c:\\test\\some.exe", 12332));

				// ASSERT
				assert_equal(1u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[0]->model);
				assert_not_null(_ui_creation_log[0]->model->get_resolver());
				assert_not_null(_ui_creation_log[0]->model->get_threads());
				assert_equal("c:\\test\\some.exe", _ui_creation_log[0]->process_name);

				// ACT
				write(*c2, init, make_initialization_data("kernel.exe", 12332));

				// ASSERT
				assert_equal(2u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[1]->model);
				assert_not_null(_ui_creation_log[1]->model->get_resolver());
				assert_not_null(_ui_creation_log[1]->model->get_threads());
				assert_equal("kernel.exe", _ui_creation_log[1]->process_name);
				assert_not_equal(_ui_creation_log[0]->model, _ui_creation_log[1]->model);
				assert_not_equal(_ui_creation_log[0]->model->get_resolver(), _ui_creation_log[1]->model->get_resolver());
				assert_not_equal(_ui_creation_log[0]->model->get_threads(), _ui_creation_log[1]->model->get_threads());
			}


			test( WritingStatisticsDataFillsUpFunctionsList )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);
				pair< unsigned, unthreaded_statistic_types::function_detailed > data1[] = {
					make_pair(1321222, unthreaded_statistic_types::function_detailed()),
					make_pair(1321221, unthreaded_statistic_types::function_detailed()),
				};
				pair< unsigned, unthreaded_statistic_types::function_detailed > data2[] = {
					make_pair(13, unthreaded_statistic_types::function_detailed()),
				};

				write(*c, init, make_initialization_data("", 11));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(*c, response_statistics_update, make_single_threaded(data1));

				// ASSERT
				assert_equal(2u, model->get_count());

				// ACT
				write(*c, response_statistics_update, make_single_threaded(data2));

				// ASSERT
				assert_equal(3u, model->get_count());

				// ACT
				write(*c, response_statistics_update, make_single_threaded(data1));

				// ASSERT
				assert_equal(3u, model->get_count());
			}


			obsolete_test( FrontendManagerIsHeldByFrontendsAlive )


			test( ThereAreNoInstancesAfterCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<mocks::symbol_resolver> sr(new mocks::symbol_resolver);
				shared_ptr<mocks::threads_model> tmodel(new mocks::threads_model);
				shared_ptr<functions_list> fl1 = functions_list::create(123, sr, tmodel),
					fl2 = functions_list::create(123, sr, tmodel);

				m.load_session("somefile.exe", fl1);
				m.load_session("other", fl1);

				// ACT
				m.close_all();

				// ASSERT
				assert_equal(0u, m.instances_count());
				assert_null(m.get_active());
			}


			test( UIEventsAreIgnoredAfterCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);

				write(*c1, init, make_initialization_data("", 10));
				write(*c2, init, make_initialization_data("", 15));

				// ACT
				m.close_all();

				// ASSERT
				assert_null(m.get_active());

				// ACT
				_ui_creation_log[0]->activated();

				// ASSERT
				assert_null(m.get_active());

			}


			test( FunctionListIsInitializedWithTicksPerSecond )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);
				pair< unsigned, unthreaded_statistic_types::function_detailed > data[] = {
					make_pair(1321222, unthreaded_statistic_types::function_detailed()),
				};

				data[0].second.inclusive_time = 150;

				write(*c1, init, make_initialization_data("", 10));
				write(*c2, init, make_initialization_data("", 15));

				shared_ptr<functions_list> model1 = _ui_creation_log[0]->model;
				shared_ptr<functions_list> model2 = _ui_creation_log[1]->model;

				// ACT
				write(*c1, response_statistics_update, make_single_threaded(data));
				write(*c2, response_statistics_update, make_single_threaded(data));

				// ASSERT
				columns::main ordering[] = {	columns::inclusive,	};
				string reference1[][1] = {	{	"15s",	},	};
				string reference2[][1] = {	{	"10s",	},	};

				assert_table_equivalent(ordering, reference1, *model1);
				assert_table_equivalent(ordering, reference2, *model2);
			}


			test( SymbolsNamesAreLoadedUponMetadataReceival )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);
				symbol_info symbols1[] = { { "foo", 0x0100, 1 }, { "bar", 0x0200, 1 }, { "baz", 0x1100, 1 }, };
				symbol_info symbols2[] = { { "FOO", 0x0100, 1 }, { "BAR", 0x2000, 1 }, };
				mapped_module_identified basic1[] = { create_mapping(1u, 0x10000), };
				mapped_module_identified basic2[] = { create_mapping(2u, 0x100000), };
				module_info_metadata metadata[] = { { mkvector(symbols1), }, { mkvector(symbols2), }, };
				pair< unsigned, unthreaded_statistic_types::function_detailed > data1[] = {
					make_pair(0x10100, unthreaded_statistic_types::function_detailed()),
					make_pair(0x11100, unthreaded_statistic_types::function_detailed()),
				};
				pair< unsigned, unthreaded_statistic_types::function_detailed > data2[] = {
					make_pair(0x10200, unthreaded_statistic_types::function_detailed()),
					make_pair(0x102000, unthreaded_statistic_types::function_detailed()),
				};

				write(*c, init, make_initialization_data("", 10));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(*c, response_modules_loaded, mkvector(basic1));
				write(*c, response_module_metadata, basic1[0].persistent_id, metadata[0]);
				write(*c, response_statistics_update, make_single_threaded(data1));

				// ASSERT
				model->set_order(columns::name, true);

				assert_equal("baz", get_text(*model, 0, columns::name));
				assert_equal(addr(0x11100u), model->get_key(0));
				assert_equal("foo", get_text(*model, 1, columns::name));
				assert_equal(addr(0x10100), model->get_key(1));

				// ACT
				write(*c, response_modules_loaded, mkvector(basic2));
				write(*c, response_module_metadata, basic2[0].persistent_id, metadata[1]);
				write(*c, response_statistics_update, make_single_threaded(data2));

				// ASSERT
				assert_equal("BAR", get_text(*model, 0, columns::name));
				assert_equal(addr(0x102000), model->get_key(0));
				assert_equal("bar", get_text(*model, 1, columns::name));
				assert_equal(addr(0x10200), model->get_key(1));
				assert_equal("baz", get_text(*model, 2, columns::name));
				assert_equal("foo", get_text(*model, 3, columns::name));
			}


			test( FrontendsAreDisconnectedWhenManagerIsReleased )
			{
				// INIT
				mocks::outbound_channel outbound_channels[2];
				shared_ptr<frontend_manager> m(new frontend_manager(&dummy_ui_factory, queue));
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
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound_channels[0]),
					m.create_session(outbound_channels[1]),
					m.create_session(outbound_channels[2]),
				};

				write(*c[0], init, make_initialization_data("", 1));
				write(*c[1], init, make_initialization_data("", 1));
				write(*c[2], init, make_initialization_data("", 1));

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

			shared_ptr<frontend_ui> log_ui_creation_w(const shared_ptr<functions_list> &model, const string &process_name)
			{
				shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(model, process_name));

				_ui_creation_log_w.push_back(ui);
				return ui;
			}

			test( FrontendUIIsHeldAfterCreation )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				// ACT
				write(*c, init, initialization_data());

				// ASSERT
				assert_equal(1u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
			}


			test( UIIsHeldEvenAfterFrontendReleased )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());

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
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				write(*c[2], init, initialization_data());

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
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				write(*c, init, initialization_data());

				// ACT
				_ui_creation_log_w[0].lock()->emulate_close();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m.instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferencedNoExternalUIReference )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				write(*c, init, initialization_data());

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
					_1, _2), queue));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				write(*c, init, initialization_data());

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsReleased )
			{
				// INIT
				shared_ptr<frontend_manager> m(new frontend_manager(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2), queue));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				write(*c, init, initialization_data());
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
					_1, _2), queue));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				write(*c[2], init, initialization_data());
				reset_all(c);

				shared_ptr<frontend_ui> ui = _ui_creation_log_w[2].lock();

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
				frontend_manager m(&dummy_ui_factory, queue);

				// ACT / ASSERT
				assert_null(m.get_instance(0));
				assert_null(m.get_instance(10));
			}


			test( InstancesAreReturned )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory, queue);

				// ACT
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);
				shared_ptr<ipc::channel> c3 = m.create_session(outbound);

				// ACT / ASSERT
				assert_not_null(m.get_instance(0));
				assert_is_empty(m.get_instance(0)->executable);
				assert_null(m.get_instance(0)->model);
				assert_not_null(m.get_instance(1));
				assert_not_null(m.get_instance(2));
				assert_null(m.get_instance(3));
			}


			test( InstanceContainsExecutableNameModelAndUIAfterInitialization )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				// ACT
				write(*c, init, make_initialization_data("c:\\dev\\micro-profiler", 1));

				// ACT / ASSERT
				assert_not_null(m.get_instance(0));
				assert_equal("c:\\dev\\micro-profiler", m.get_instance(0)->executable);
				assert_equal(_ui_creation_log[0]->model, m.get_instance(0)->model);
				assert_equal(_ui_creation_log[0], m.get_instance(0)->ui);
			}


			test( ClosingAllInstancesDisconnectsAllFrontendsAndDestroysAllUI )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1, _2), queue);
				mocks::outbound_channel outbound_channels[3];
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound_channels[0]), m.create_session(outbound_channels[1]), m.create_session(outbound_channels[2]),
				};

				write(*c[0], init, make_initialization_data("", 1));
				write(*c[1], init, make_initialization_data("", 1));
				write(*c[2], init, make_initialization_data("", 1));
				
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
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( NoInstanceConsideredActiveIfNoUICreated )
			{
				// INIT
				frontend_manager m(&dummy_ui_factory, queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				// ACT
				write(*c[0], init, initialization_data());

				// ASSERT
				assert_null(m.get_active());
			}


			test( LastInitializedInstanceConsideredActive )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				// ACT
				write(*c[0], init, initialization_data());

				// ASSERT
				assert_not_null(m.get_active());
				assert_equal(m.get_instance(0), m.get_active());

				// ACT
				write(*c[1], init, initialization_data());

				// ASSERT
				assert_equal(m.get_instance(1), m.get_active());
			}


			test( ActiveInstanceIsResetOnFrontendReleaseEvenIfUIDidNotSignalledOfClose )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);

				write(*c, init, initialization_data()); // make it active
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
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c1 = m.create_session(outbound);

				write(*c1, init, initialization_data());
				_ui_creation_log[0]->close_on_destroy = false;

				// ACT
				shared_ptr<ipc::channel> c2 = m.create_session(outbound);
				write(*c2, init, initialization_data());
				c1.reset();

				// ASSERT
				assert_not_null(m.get_active());
			}


			test( UIActivationSwitchesActiveInstanceInManager )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				write(*c[2], init, initialization_data());

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
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation_w, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				_ui_creation_log_w[0].lock()->activated();

				// ACT
				m.close_all();

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( NoInstanceIsActiveAfterActiveInstanceIsClosed )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_null(m.get_active());
			}


			test( ActiveInstancetIsIntactAfterInactiveInstanceIsClosed )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c[] = {
					m.create_session(outbound), m.create_session(outbound), m.create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(m.get_instance(0), m.get_active());
			}


			test( UpdateModelDoesNothingIfFrontendWasNotInitialized )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);
				pair< unsigned, unthreaded_statistic_types::function_detailed > data[] = {
					make_pair(1321222, unthreaded_statistic_types::function_detailed()),
					make_pair(1321221, unthreaded_statistic_types::function_detailed()),
				};

				// ACT / ASSERT (must not crash)
				write(*c, response_statistics_update, make_single_threaded(data));
			}


			test( CreatingInstanceFromModelAddsNewInstanceToTheListAndConstructsUI )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<mocks::symbol_resolver> sr(new mocks::symbol_resolver);
				shared_ptr<mocks::threads_model> tmodel(new mocks::threads_model);
				shared_ptr<functions_list> fl1 = functions_list::create(123, sr, tmodel),
					fl2 = functions_list::create(123, sr, tmodel);

				// ACT
				m.load_session("somefile.exe", fl1);

				// ASSERT
				assert_equal(1u, m.instances_count());
				assert_equal("somefile.exe", m.get_instance(0)->executable);
				assert_equal(fl1, m.get_instance(0)->model);
				assert_equal(_ui_creation_log[0], m.get_instance(0)->ui);

				// ACT
				m.load_session("jump.exe", fl2);

				// ASSERT
				assert_equal(2u, m.instances_count());
				assert_equal("jump.exe", m.get_instance(1)->executable);
				assert_equal(fl2, m.get_instance(1)->model);
				assert_equal(_ui_creation_log[1], m.get_instance(1)->ui);
			}


			test( NoAttemptToDisconnectFrontendIsMadeIfNoFrontendExistsOnCloseAll )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<mocks::symbol_resolver> sr(new mocks::symbol_resolver);
				shared_ptr<mocks::threads_model> tmodel(new mocks::threads_model);
				shared_ptr<functions_list> fl1 = functions_list::create(123, sr, tmodel),
					fl2 = functions_list::create(123, sr, tmodel);

				// ACT
				m.load_session("somefile.exe", fl1);

				// ACT / ASSERT (does not crash)
				m.close_all();
				m.close_all();
			}


			test( MetadataRequestOnAttemptingToResolveMissingFunction )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);
				mapped_module_identified mi[] = {
					create_mapping(17u, 0u), create_mapping(99u, 0x1000), create_mapping(1000u, 0x1900),
				};
				pair< unsigned, unthreaded_statistic_types::function_detailed > data[] = {
					make_pair(0x0100, unthreaded_statistic_types::function_detailed()),
					make_pair(0x1001, unthreaded_statistic_types::function_detailed()),
					make_pair(0x1100, unthreaded_statistic_types::function_detailed()),
					make_pair(0x1910, unthreaded_statistic_types::function_detailed()),
				};

				write(*c, init, initialization_data());

				const functions_list &fl = *_ui_creation_log[0]->model;

				write(*c, response_modules_loaded, mkvector(mi));
				write(*c, response_statistics_update, make_single_threaded(data));

				// ACT
				get_text(fl, fl.get_index(addr(0x1100)), columns::name);

				// ASSERT
				unsigned int reference1[] = { 99u, };

				assert_equal(reference1, outbound.requested_metadata);

				// INIT
				outbound.requested_metadata.clear();

				// ACT
				get_text(fl, fl.get_index(addr(0x1001)), columns::name);
				get_text(fl, fl.get_index(addr(0x0100)), columns::name);
				get_text(fl, fl.get_index(addr(0x1910)), columns::name);

				// ASSERT
				unsigned int reference2[] = { 17u, 1000u, };

				assert_equal(reference2, outbound.requested_metadata);
			}


			test( MetadataIsNoLongerRequestedAfterFrontendDestruction )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);
				mapped_module_identified mi[] = {
					create_mapping(17u, 0u),
				};
				pair< unsigned, unthreaded_statistic_types::function_detailed > data[] = {
					make_pair(0x0100, unthreaded_statistic_types::function_detailed()),
				};
				string text;

				write(*c, init, initialization_data());

				const functions_list &fl = *_ui_creation_log[0]->model;

				write(*c, response_modules_loaded, mkvector(mi));
				write(*c, response_statistics_update, make_single_threaded(data));

				// ACT
				c.reset();
				get_text(fl, 0, columns::name);

				// ASSERT
				assert_is_empty(outbound.requested_metadata);
			}


			test( ThreadsModelGetsUpdatedOnThreadInfosMessage )
			{
				// INIT
				frontend_manager m(bind(&FrontendManagerTests::log_ui_creation, this, _1, _2), queue);
				shared_ptr<ipc::channel> c = m.create_session(outbound);
				pair<unsigned int, thread_info> data1[] = {
					make_pair(0, make_thread_info(1717, "thread 1", mt::milliseconds(), mt::milliseconds(),
						mt::milliseconds(), true)),
					make_pair(1, make_thread_info(11717, "thread 2", mt::milliseconds(), mt::milliseconds(),
						mt::milliseconds(), false)),
				};
				pair<unsigned int, thread_info> data2[] = {
					make_pair(1, make_thread_info(117, "", mt::milliseconds(), mt::milliseconds(),
						mt::milliseconds(), true)),
				};

				write(*c, init, initialization_data());

				const shared_ptr<threads_model> threads = _ui_creation_log[0]->model->get_threads();

				// ACT
				write(*c, response_threads_info, mkvector(data1));

				// ASSERT
				unsigned int native_id;

				assert_equal(3u, threads->get_count());
				assert_is_true(threads->get_native_id(native_id, 0));
				assert_equal(1717u, native_id);
				assert_is_true(threads->get_native_id(native_id, 1));
				assert_equal(11717u, native_id);

				// ACT
				write(*c, response_threads_info, mkvector(data2));

				// ASSERT
				assert_equal(3u, threads->get_count());
				assert_is_true(threads->get_native_id(native_id, 1));
				assert_equal(117u, native_id);
			}

		end_test_suite
	}
}
