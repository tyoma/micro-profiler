#include <frontend/frontend_manager.h>

#include <common/serialization.h>
#include <common/symbol_resolver.h>
#include <frontend/function_list.h>
#include <ipc/channel_client.h>

#include <algorithm>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>
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
			frontend_ui::ptr dummy_ui_factory(const shared_ptr<functions_list> &, const wstring &)
			{	return frontend_ui::ptr();	}

			template <typename CommandDataT>
			void write(ipc::channel &channel, commands c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				channel.message(const_byte_range(&b.buffer[0], static_cast<unsigned>(b.buffer.size())));
			}

			initialization_data make_initialization_data(const wstring &executable, timestamp_t ticks_per_second)
			{
				initialization_data idata = {	executable, ticks_per_second };
				return idata;
			}

			template <typename T, size_t n>
			void reset_all(T (&a)[n])
			{	fill_n(a, n, T());	}

			namespace mocks
			{
				class frontend_ui : public micro_profiler::frontend_ui
				{		
				public:
					frontend_ui(const shared_ptr<functions_list> &model_, const wstring &process_name_)
						: model(model_), process_name(process_name_)
					{	}

					~frontend_ui()
					{	closed();	}

					void emulate_close()
					{	closed();	}

				public:
					shared_ptr<functions_list> model;
					wstring process_name;

				private:
					virtual void activate() {	}
				};

				class outbound_channel : public ipc::channel
				{
				public:
					outbound_channel()
						: disconnected(false)
					{	}

				public:
					bool disconnected;

				private:
					virtual void disconnect() throw()
					{	disconnected = true;	}

					virtual void message(const_byte_range /*payload*/)
					{	}
				};
			}
		}

		begin_test_suite( FrontendManagerTests )

			mocks::outbound_channel outbound;


			test( OpeningFrontendChannelIncrementsInstanceCount )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);

				// ACT / ASSERT (must not throw)
				shared_ptr<ipc::channel> c1 = m->create_session(outbound);

				// ACT / ASSERT
				assert_equal(1u, m->instances_count());

				// ACT
				shared_ptr<ipc::channel> c2 = m->create_session(outbound);

				// ACT / ASSERT
				assert_equal(2u, m->instances_count());
			}


			test( ClosingFrontendChannelsDecrementsInstanceCount )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);
				shared_ptr<ipc::channel> c1 = m->create_session(outbound);
				shared_ptr<ipc::channel> c2 = m->create_session(outbound);

				// ACT
				c2.reset();

				// ACT / ASSERT
				assert_equal(1u, m->instances_count());
			}


			vector< shared_ptr<mocks::frontend_ui> > _ui_creation_log;

			shared_ptr<frontend_ui> log_ui_creation(const shared_ptr<functions_list> &model, const wstring &process_name)
			{
				shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(model, process_name));

				_ui_creation_log.push_back(ui);
				return ui;
			}

			test( FrontendInitializaionLeadsToUICreation )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c1 = m->create_session(outbound);
				shared_ptr<ipc::channel> c2 = m->create_session(outbound);

				// ACT
				write(*c1, init, make_initialization_data(L"c:\\test\\some.exe", 12332));

				// ASSERT
				assert_equal(1u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[0]->model);
				assert_equal(L"c:\\test\\some.exe", _ui_creation_log[0]->process_name);

				// ACT
				write(*c2, init, make_initialization_data(L"kernel.exe", 12332));

				// ASSERT
				assert_equal(2u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[1]->model);
				assert_equal(L"kernel.exe", _ui_creation_log[1]->process_name);
				assert_not_equal(_ui_creation_log[0]->model, _ui_creation_log[1]->model);
 			}


			test( WritingStatisticsDataFillsUpFunctionsList )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);
				pair< unsigned, function_statistics_detailed_t<unsigned> > data1[] = {
					make_pair(1321222, function_statistics_detailed_t<unsigned>()),
					make_pair(1321221, function_statistics_detailed_t<unsigned>()),
				};
				pair< unsigned, function_statistics_detailed_t<unsigned> > data2[] = {
					make_pair(13, function_statistics_detailed_t<unsigned>()),
				};

				write(*c, init, make_initialization_data(L"", 11));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(*c, update_statistics, mkvector(data1));

				// ASSERT
				assert_equal(2u, model->get_count());

				// ACT
				write(*c, update_statistics, mkvector(data2));

				// ASSERT
				assert_equal(3u, model->get_count());

				// ACT
				write(*c, update_statistics, mkvector(data1));

				// ASSERT
				assert_equal(3u, model->get_count());
			}


			test( FrontendManagerIsHeldByFrontendsAlive )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);
				frontend_manager *pm = m.get();
				shared_ptr<ipc::channel> c1 = m->create_session(outbound);
				shared_ptr<ipc::channel> c2 = m->create_session(outbound);

				m.reset();

				// ACT
				c1.reset();

				// ASSERT
				assert_equal(1u, pm->instances_count());
			}


			test( FunctionListIsInitializedWithTicksPerSecond )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c1 = m->create_session(outbound);
				shared_ptr<ipc::channel> c2 = m->create_session(outbound);
				pair< unsigned, function_statistics_detailed_t<unsigned> > data[] = {
					make_pair(1321222, function_statistics_detailed_t<unsigned>()),
				};

				data[0].second.inclusive_time = 150;

				write(*c1, init, make_initialization_data(L"", 10));
				write(*c2, init, make_initialization_data(L"", 15));

				shared_ptr<functions_list> model1 = _ui_creation_log[0]->model;
				shared_ptr<functions_list> model2 = _ui_creation_log[1]->model;

				// ACT
				write(*c1, update_statistics, mkvector(data));
				write(*c2, update_statistics, mkvector(data));

				// ASSERT
				wstring text;

				assert_equal(L"15s", (model1->get_text(0, 4, text), text));
				assert_equal(L"10s", (model2->get_text(0, 4, text), text));
			}


			test( SymbolAreLoadedWhenCorrespondingCommandIsProcessed )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);
				image images[] = { image(L"symbol_container_1"), image(L"symbol_container_2"), };
				module_info mi[] = {
					{ images[0].load_address(), images[0].absolute_path() },
					{ images[1].load_address(), images[1].absolute_path() },
				};
				pair< const void *, function_statistics_detailed_t<const void *> > data[] = {
					make_pair(images[0].get_symbol_address("get_function_addresses_1"),
						function_statistics_detailed_t<const void *>()),
					make_pair(images[1].get_symbol_address("get_function_addresses_2"),
						function_statistics_detailed_t<const void *>()),
				};

				write(*c, init, make_initialization_data(L"", 10));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(*c, modules_loaded, vector<module_info>(1, mi[0]));
				write(*c, modules_loaded, vector<module_info>(1, mi[1]));

				// ASSERT
				wstring text;

				write(*c, update_statistics, mkvector(data));
				model->set_order(1, true);

				assert_equal(L"get_function_addresses_1", (model->get_text(0, 1, text), text));
				assert_equal(L"get_function_addresses_2", (model->get_text(1, 1, text), text));
			}


			test( SymbolAreLoadedWhenCorrespondingCommandIsProcessedBatch )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);
				image images[] = {
					image(L"symbol_container_1"),
					image(L"symbol_container_2"),
					image(L"symbol_container_3_nosymbols"),
				};
				module_info mi[] = {
					{ images[0].load_address(), images[0].absolute_path() },
					{ images[1].load_address(), images[1].absolute_path() },
					{ images[2].load_address(), images[2].absolute_path() },
				};
				pair< const void *, function_statistics_detailed_t<const void *> > data[] = {
					make_pair(images[0].get_symbol_address("get_function_addresses_1"),
						function_statistics_detailed_t<const void *>()),
					make_pair(images[1].get_symbol_address("get_function_addresses_2"),
						function_statistics_detailed_t<const void *>()),
					make_pair(images[2].get_symbol_address("get_function_addresses_3"),
						function_statistics_detailed_t<const void *>()),
				};

				write(*c, init, make_initialization_data(L"", 10));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(*c, modules_loaded, mkvector(mi));

				// ASSERT
				wstring text;

				write(*c, update_statistics, mkvector(data));
				model->set_order(1, true);

				assert_equal(L"get_function_addresses_1", (model->get_text(0, 1, text), text));
				assert_equal(L"get_function_addresses_2", (model->get_text(1, 1, text), text));
				assert_equal(L"get_function_addresses_3", (model->get_text(2, 1, text), text));
			}


			test( ReleasingFrontendReleasesSymbolResolver )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				write(*c, init, make_initialization_data(L"", 10));

				shared_ptr<symbol_resolver> sr = _ui_creation_log[0]->model->get_resolver();

				// ACT
				c.reset();

				// ASSERT
				assert_is_true(sr.unique());
			}


			test( FrontendsAreDisconnectedWhenManagerIsReleased )
			{
				// INIT
				mocks::outbound_channel outbound_channels[2];
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);
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
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound_channels[0]),
					m->create_session(outbound_channels[1]),
					m->create_session(outbound_channels[2]),
				};

				write(*c[0], init, make_initialization_data(L"", 1));
				write(*c[1], init, make_initialization_data(L"", 1));
				write(*c[2], init, make_initialization_data(L"", 1));

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

			shared_ptr<frontend_ui> log_ui_creation_w(const shared_ptr<functions_list> &model, const wstring &process_name)
			{
				shared_ptr<mocks::frontend_ui> ui(new mocks::frontend_ui(model, process_name));

				_ui_creation_log_w.push_back(ui);
				return ui;
			}

			test( FrontendUIIsHeldAfterCreation )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				// ACT
				write(*c, init, initialization_data());

				// ASSERT
				assert_equal(1u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
			}


			test( UIIsHeldEvenAfterFrontendReleased )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound),
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
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				write(*c[2], init, initialization_data());

				// ACT
				reset_all(c);

				// ASSERT
				assert_equal(3u, m->instances_count());

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(2u, m->instances_count());

				// ACT
				_ui_creation_log[2]->emulate_close();
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_equal(0u, m->instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferenced )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				write(*c, init, initialization_data());

				// ACT
				_ui_creation_log_w[0].lock()->emulate_close();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m->instances_count());
			}


			test( InstanceLingersWhenUIIsClosedButFrontendIsStillReferencedNoExternalUIReference )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				write(*c, init, initialization_data());

				// ACT (must not crash - make sure even doubled close doesn't blow things up)
				m->get_instance(0)->ui->closed();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m->instances_count());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsHeld )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
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
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
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
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
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


			test( ManagerIsHeldByOpenedUI )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				frontend_manager *pm = m.get();
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				write(*c[2], init, initialization_data());
				reset_all(c);

				// ACT
				m.reset();

				// ASSERT
				assert_equal(3u, pm->instances_count());
				assert_is_true(_ui_creation_log[0].unique());
				assert_is_true(_ui_creation_log[1].unique());
				assert_is_true(_ui_creation_log[2].unique());

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ASSERT
				assert_equal(2u, pm->instances_count());

				// ACT
				_ui_creation_log[0]->emulate_close();

				// ASSERT
				assert_equal(1u, pm->instances_count());

				// ACT
				_ui_creation_log[2]->emulate_close();
			}


			test( NoInstanceIsReturnedForEmptyManager )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);

				// ACT / ASSERT
				assert_null(m->get_instance(0));
				assert_null(m->get_instance(10));
			}


			test( InstancesAreReturned )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);

				// ACT
				shared_ptr<ipc::channel> c1 = m->create_session(outbound);
				shared_ptr<ipc::channel> c2 = m->create_session(outbound);
				shared_ptr<ipc::channel> c3 = m->create_session(outbound);

				// ACT / ASSERT
				assert_not_null(m->get_instance(0));
				assert_is_empty(m->get_instance(0)->executable);
				assert_null(m->get_instance(0)->model);
				assert_not_null(m->get_instance(1));
				assert_not_null(m->get_instance(2));
				assert_null(m->get_instance(3));
			}


			test( InstanceContainsExecutableNameModelAndUIAfterInitialization )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);

				// ACT
				write(*c, init, make_initialization_data(L"c:\\dev\\micro-profiler", 1));

				// ACT / ASSERT
				assert_not_null(m->get_instance(0));
				assert_equal(L"c:\\dev\\micro-profiler", m->get_instance(0)->executable);
				assert_equal(_ui_creation_log[0]->model, m->get_instance(0)->model);
				assert_equal(_ui_creation_log[0], m->get_instance(0)->ui);
			}


			test( ClosingAllInstancesDisconnectsAllFrontendsAndDestroysAllUI )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				mocks::outbound_channel outbound_channels[3];
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound_channels[0]), m->create_session(outbound_channels[1]), m->create_session(outbound_channels[2]),
				};

				write(*c[0], init, make_initialization_data(L"", 1));
				write(*c[1], init, make_initialization_data(L"", 1));
				write(*c[2], init, make_initialization_data(L"", 1));
				
				// ACT
				m->close_all();

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
				assert_equal(0u, m->instances_count());
			}


			test( NoActiveInstanceInEmptyManager )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));

				// ACT / ASSERT
				assert_null(m->get_active());
			}


			test( NoInstanceConsideredActiveIfNoUICreated )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(&dummy_ui_factory);
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				// ACT
				write(*c[0], init, initialization_data());

				// ASSERT
				assert_null(m->get_active());
			}


			test( LastInitializedInstanceConsideredActive )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				// ACT
				write(*c[0], init, initialization_data());

				// ASSERT
				assert_not_null(m->get_active());
				assert_equal(m->get_instance(0), m->get_active());

				// ACT
				write(*c[1], init, initialization_data());

				// ASSERT
				assert_equal(m->get_instance(1), m->get_active());
			}


			test( UIActivationSwitchesActiveInstanceInManager )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				write(*c[2], init, initialization_data());

				// ACT
				_ui_creation_log[1]->activated();

				// ASSERT
				assert_not_null(m->get_active());
				assert_equal(m->get_instance(1), m->get_active());

				// ACT
				_ui_creation_log[0]->activated();

				// ASSERT
				assert_equal(m->get_instance(0), m->get_active());

				// ACT
				_ui_creation_log[2]->activated();

				// ASSERT
				assert_equal(m->get_instance(2), m->get_active());
			}


			test( NoInstanceIsActiveAfterCloseAll )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				_ui_creation_log_w[0].lock()->activated();

				// ACT
				m->close_all();

				// ACT / ASSERT
				assert_null(m->get_active());
			}


			test( NoInstanceIsActiveAfterActiveInstanceIsClosed )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[0]->emulate_close();

				// ACT / ASSERT
				assert_null(m->get_active());
			}


			test( ActiveInstancetIsIntactAfterInactiveInstanceIsClosed )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c[] = {
					m->create_session(outbound), m->create_session(outbound), m->create_session(outbound),
				};

				write(*c[0], init, initialization_data());
				write(*c[1], init, initialization_data());
				_ui_creation_log[0]->activated();

				// ACT
				_ui_creation_log[1]->emulate_close();

				// ACT / ASSERT
				assert_equal(m->get_instance(0), m->get_active());
			}


			test( UpdateModelDoesNothingIfFrontendWasNotInitialized )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<ipc::channel> c = m->create_session(outbound);
				pair< unsigned, function_statistics_detailed_t<unsigned> > data[] = {
					make_pair(1321222, function_statistics_detailed_t<unsigned>()),
					make_pair(1321221, function_statistics_detailed_t<unsigned>()),
				};

				// ACT / ASSERT (must not crash)
				write(*c, update_statistics, mkvector(data));
			}


			test( CreatingInstanceFromModelAddsNewInstanceToTheListAndConstructsUI )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				shared_ptr<symbol_resolver> sr = symbol_resolver::create();
				shared_ptr<functions_list> fl1 = functions_list::create(123, sr), fl2 = functions_list::create(123, sr);

				// ACT
				m->create_instance(L"somefile.exe", fl1);

				// ASSERT
				assert_equal(1u, m->instances_count());
				assert_equal(L"somefile.exe", m->get_instance(0)->executable);
				assert_equal(fl1, m->get_instance(0)->model);
				assert_equal(_ui_creation_log[0], m->get_instance(0)->ui);

				// ACT
				m->create_instance(L"jump.exe", fl2);

				// ASSERT
				assert_equal(2u, m->instances_count());
				assert_equal(L"jump.exe", m->get_instance(1)->executable);
				assert_equal(fl2, m->get_instance(1)->model);
				assert_equal(_ui_creation_log[1], m->get_instance(1)->ui);
			}

		end_test_suite
	}
}
