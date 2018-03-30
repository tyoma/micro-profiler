#include <frontend/frontend_manager.h>

#include <collector/channel_client.h>
#include <common/serialization.h>
#include <frontend/function_list.h>

#include "../Helpers.h"

#include <algorithm>
#include <strmd/serializer.h>
#include <ut/assert.h>
#include <ut/test.h>
#include <wpl/mt/synchronization.h>

using namespace std::placeholders;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			frontend_ui::ptr dummy_ui_factory(const shared_ptr<functions_list> &, const wstring &)
			{	return frontend_ui::ptr();	}

			template <typename CommandDataT>
			void write(channel_t &channel, commands c, const CommandDataT &data)
			{
				vector_adapter b;
				strmd::serializer<vector_adapter, packer> archive(b);

				archive(c);
				archive(data);
				assert_is_true(channel(b.buffer.data(), static_cast<long>(b.buffer.size())));
			}

			namespace mocks
			{
				class frontend_ui : public micro_profiler::frontend_ui
				{		
				public:
					frontend_ui(const shared_ptr<functions_list> &model_, const wstring &process_name_)
						: model(model_), process_name(process_name_), _notified(false)
					{	}

					~frontend_ui()
					{
						if (!_notified)
							closed();
					}

					void emulate_close()
					{
						_notified = true;
						closed();
					}

				public:
					shared_ptr<functions_list> model;
					wstring process_name;

				private:
					virtual void activate() {	}

				private:
					bool _notified;
				};
			}
		}

		begin_test_suite( FrontendManagerTests )

			guid_t id;
			com_event clients_ready;
			com_event server_ready;

			static guid_t generate_id()
			{
				guid_t id;

				generate_n(id.values, sizeof(id.values), rand);
				return id;
			}

			init( Initialize )
			{
				id = generate_id();
			}

			test( CanCreateManagersAtDifferentID )
			{
				// INIT
				guid_t id1 = generate_id(), id2 = generate_id();

				// ACT
				frontend_manager::ptr m1 = frontend_manager::create(id1, &dummy_ui_factory);
				frontend_manager::ptr m2 = frontend_manager::create(id2, &dummy_ui_factory);

				// ASSERT
				assert_not_null(m1);
				assert_equal(0u, m1->instances_count());
				assert_not_null(m2);
				assert_equal(0u, m2->instances_count());
			}


			test( CreationOfSecondManagerAtTheSameIDFails )
			{
				// INIT
				frontend_manager::ptr m1 = frontend_manager::create(id, &dummy_ui_factory);

				// ACT / ASSERT
				assert_throws(frontend_manager::create(id, &dummy_ui_factory), runtime_error);
			}


			test( CanCreateSecondManagerAtTheSameIDAfterFirstIsDestroyed )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, &dummy_ui_factory);

				// ACT
				m.reset();
				m = frontend_manager::create(id, &dummy_ui_factory);

				// ASSERT
				assert_not_null(m);
			}


			test( OpeningFrontendChannelIncrementsInstanceCount )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, &dummy_ui_factory);

				// ACT / ASSERT (must not throw)
				channel_t c1 = open_channel(id);

				// ACT / ASSERT
				assert_equal(1u, m->instances_count());

				// ACT
				channel_t c2 = open_channel(id);

				// ACT / ASSERT
				assert_equal(2u, m->instances_count());
			}


			test( ClosingFrontendChannelsDecrementsInstanceCount )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, &dummy_ui_factory);
				channel_t c1 = open_channel(id);
				channel_t c2 = open_channel(id);

				// ACT
				c2 = channel_t();

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
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				channel_t c1 = open_channel(id);
				channel_t c2 = open_channel(id);

				// ACT
				write(c1, init, initialization_data(L"c:\\test\\some.exe", 12332));

				// ASSERT
				assert_equal(1u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[0]->model);
				assert_equal(L"c:\\test\\some.exe", _ui_creation_log[0]->process_name);

				// ACT
				write(c2, init, initialization_data(L"kernel.exe", 12332));

				// ASSERT
				assert_equal(2u, _ui_creation_log.size());

				assert_not_null(_ui_creation_log[1]->model);
				assert_equal(L"kernel.exe", _ui_creation_log[1]->process_name);
				assert_not_equal(_ui_creation_log[0]->model, _ui_creation_log[1]->model);
 			}


			test( WritingStatisticsDataFillsUpFunctionsList )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				channel_t c = open_channel(id);
				pair< unsigned, function_statistics_detailed_t<unsigned> > data1[] = {
					make_pair(1321222, function_statistics_detailed_t<unsigned>()),
					make_pair(1321221, function_statistics_detailed_t<unsigned>()),
				};
				pair< unsigned, function_statistics_detailed_t<unsigned> > data2[] = {
					make_pair(13, function_statistics_detailed_t<unsigned>()),
				};

				write(c, init, initialization_data(L"", 11));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(c, update_statistics, mkvector(data1));

				// ASSERT
				assert_equal(2u, model->get_count());

				// ACT
				write(c, update_statistics, mkvector(data2));

				// ASSERT
				assert_equal(3u, model->get_count());

				// ACT
				write(c, update_statistics, mkvector(data1));

				// ASSERT
				assert_equal(3u, model->get_count());
			}


			test( FrontendManagerIsHeldByFrontendsAlive )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, &dummy_ui_factory);
				frontend_manager *pm = m.get();
				channel_t c1 = open_channel(id);
				channel_t c2 = open_channel(id);

				m.reset();

				// ACT
				c1 = channel_t();

				// ASSERT
				assert_equal(1u, pm->instances_count());
			}


			test( FrontendManagerInHiddenAfterReleasedButHeld )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, &dummy_ui_factory);
				channel_t c = open_channel(id);

				// ACT
				m.reset();

				// ACT / ASSERT (must not throw)
				frontend_manager::create(id, &dummy_ui_factory);
			}


			test( FunctionListIsInitializedWithTicksPerSecond )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				channel_t c1 = open_channel(id), c2 = open_channel(id);
				pair< unsigned, function_statistics_detailed_t<unsigned> > data[] = {
					make_pair(1321222, function_statistics_detailed_t<unsigned>()),
				};

				data[0].second.inclusive_time = 150;

				write(c1, init, initialization_data(L"", 10));
				write(c2, init, initialization_data(L"", 15));

				shared_ptr<functions_list> model1 = _ui_creation_log[0]->model;
				shared_ptr<functions_list> model2 = _ui_creation_log[1]->model;

				// ACT
				write(c1, update_statistics, mkvector(data));
				write(c2, update_statistics, mkvector(data));

				// ASSERT
				wstring text;

				assert_equal(L"15s", (model1->get_text(0, 4, text), text));
				assert_equal(L"10s", (model2->get_text(0, 4, text), text));
			}


			test( SymbolAreLoadedWhenCorrespondingCommandIsProcessed )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				channel_t c = open_channel(id);
				image images[] = { image(L"symbol_container_1.dll"), image(L"symbol_container_2.dll"), };
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

				write(c, init, initialization_data(L"", 10));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(c, modules_loaded, vector<module_info>(1, mi[0]));
				write(c, modules_loaded, vector<module_info>(1, mi[1]));

				// ASSERT
				wstring text;

				write(c, update_statistics, mkvector(data));
				model->set_order(1, true);

				assert_equal(L"get_function_addresses_1", (model->get_text(0, 1, text), text));
				assert_equal(L"get_function_addresses_2", (model->get_text(1, 1, text), text));
			}


			test( SymbolAreLoadedWhenCorrespondingCommandIsProcessedBatch )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				channel_t c = open_channel(id);
				image images[] = {
					image(L"symbol_container_1.dll"),
					image(L"symbol_container_2.dll"),
					image(L"symbol_container_3_nosymbols.dll"),
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

				write(c, init, initialization_data(L"", 10));

				shared_ptr<functions_list> model = _ui_creation_log[0]->model;

				// ACT
				write(c, modules_loaded, mkvector(mi));

				// ASSERT
				wstring text;

				write(c, update_statistics, mkvector(data));
				model->set_order(1, true);

				assert_equal(L"get_function_addresses_1", (model->get_text(0, 1, text), text));
				assert_equal(L"get_function_addresses_2", (model->get_text(1, 1, text), text));
				assert_equal(L"get_function_addresses_3", (model->get_text(2, 1, text), text));
			}


			vector<size_t> failed_sends;

			void try_send(unsigned n)
			{
				vector<channel_t> channels;

				while (n--)
				{
					channels.push_back(open_channel(id));
					write(channels.back(), init, initialization_data(L"", 1));
				}
 				clients_ready.signal();
				server_ready.wait();
				for (size_t i = 0; i != channels.size(); ++i)
				{
					commands data = static_cast<commands>(-1); // Unrecognized command will be just skipped.
					if (!channels[i](&data, sizeof(data)))
						failed_sends.push_back(i);
				}
				channels.clear();
 				clients_ready.signal();
			}

			test( FrontendsAreDisconnectedWhenManagerIsReleased )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, &dummy_ui_factory);
				wpl::mt::thread t(bind(&FrontendManagerTests::try_send, this, 2));

				clients_ready.wait();

				// ACT
				m.reset();
				server_ready.signal();
				clients_ready.wait();
				t.join();

				// ASSERT
				unsigned reference[] = { 0, 1, };

				assert_equal(reference, failed_sends);
			}


			test( FrontendForClosedUIIsDisconnected )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				wpl::mt::thread t(bind(&FrontendManagerTests::try_send, this, 3));

				clients_ready.wait();

				// ACT
				_ui_creation_log[0]->emulate_close();
				_ui_creation_log[2]->emulate_close();
				server_ready.signal();
				clients_ready.wait();
				t.join();

				// ASSERT
				unsigned reference[] = { 0, 2, };

				assert_equal(reference, failed_sends);
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
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				channel_t c = open_channel(id);

				// ACT
				write(c, init, initialization_data());

				// ASSERT
				assert_equal(1u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
			}


			test( UIIsHeldEvenAfterFrontendReleased )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				channel_t c[] = { open_channel(id), open_channel(id), };

				write(c[0], init, initialization_data());
				write(c[1], init, initialization_data());

				// ACT
				c[0] = channel_t();

				// ASSERT
				assert_equal(2u, _ui_creation_log_w.size());
				assert_is_false(_ui_creation_log_w[0].expired());
				assert_is_false(_ui_creation_log_w[1].expired());
			}


			test( InstancesAreManagedByOpenedUIsAfterFrontendsReleased )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				channel_t c[] = { open_channel(id), open_channel(id), open_channel(id), };

				write(c[0], init, initialization_data());
				write(c[1], init, initialization_data());
				write(c[2], init, initialization_data());

				// ACT
				fill_n(c, _countof(c), channel_t());

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
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				channel_t c = open_channel(id);

				write(c, init, initialization_data());

				// ACT
				_ui_creation_log_w[0].lock()->emulate_close();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
				assert_equal(1u, m->instances_count());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsHeld )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				channel_t c = open_channel(id);

				write(c, init, initialization_data());

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( UIIsDestroyedOnManagerDestructionWhenFrontendIsReleased )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				channel_t c = open_channel(id);

				write(c, init, initialization_data());
				c = channel_t();

				// ACT
				m.reset();

				// ASSERT
				assert_is_true(_ui_creation_log_w[0].expired());
			}


			test( OnlyExternallyHeldUISurvivesManagerDestruction )
			{
				// INIT
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation_w, this,
					_1, _2));
				channel_t c[] = { open_channel(id), open_channel(id), open_channel(id), };

				write(c[0], init, initialization_data());
				write(c[1], init, initialization_data());
				write(c[2], init, initialization_data());
				fill_n(c, _countof(c), channel_t());

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
				frontend_manager::ptr m = frontend_manager::create(id, bind(&FrontendManagerTests::log_ui_creation, this,
					_1, _2));
				frontend_manager *pm = m.get();
				channel_t c[] = { open_channel(id), open_channel(id), open_channel(id), };

				write(c[0], init, initialization_data());
				write(c[1], init, initialization_data());
				write(c[2], init, initialization_data());
				fill_n(c, _countof(c), channel_t());

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

		end_test_suite
	}
}
