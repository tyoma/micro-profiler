#include <collector/statistics_bridge.h>

#include "mocks.h"

#include <algorithm>
//#include <collector/calibration.h>
#include <collector/module_tracker.h>
#include <common/path.h>
#include <common/time.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const overhead c_overhead(17, 0);

			template <typename ContainerT>
			const symbol_info *get_symbol_by_name(const ContainerT &symbols, const char *name)
			{
				for (typename ContainerT::const_iterator i = symbols.begin(); i != symbols.end(); ++i)
					if (string::npos != i->name.find(name))
						return &*i;
				assert_is_true(false);
				return 0;
			}

			template <typename ContainerT>
			string get_file(const ContainerT &files, unsigned id)
			{
				for (typename ContainerT::const_iterator i = files.begin(); i != files.end(); ++i)
					if (id == i->first)
						return i->second;
				assert_is_true(false);
				return 0;
			}
		}

		begin_test_suite( StatisticsBridgeTests )
			vector<image> images;
			shared_ptr<mocks::Tracer> cc;
			shared_ptr<mocks::frontend_state> state;
			shared_ptr<ipc::channel> frontend;
			vector<mocks::statistics_map_detailed> update_log;
			shared_ptr<module_tracker> mtracker;
			unsigned int ref_count;

			init( CreateQueue )
			{
				image images_[] = {
					image("symbol_container_1"),
					image("symbol_container_2"),
					image("symbol_container_3_nosymbols"),
				};

				images.assign(images_, array_end(images_));
				mtracker.reset(new module_tracker);
			}


			init( InitializeFrontendMock )
			{
				ref_count = 0;
				cc.reset(new mocks::Tracer);
				state.reset(new mocks::frontend_state(cc));
				state->updated = [this] (const mocks::statistics_map_detailed &u) { update_log.push_back(u); };
				frontend = state->create();
			}


			test( FrontendIsInitializedAtBridgeConstruction )
			{
				// INIT
				initialization_data process_init;

				state->initialized = [&] (const initialization_data &id) { process_init = id; };

				// ACT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);

				// ASSERT
				assert_equal(get_current_process_executable(), process_init.executable);
				assert_approx_equal(ticks_per_second(), process_init.ticks_per_second, 0.01);
			}


			test( FrontendUpdateIsNotCalledIfNoUpdates )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);

				// ACT
				b.analyze();
				b.update_frontend();

				// ASSERT
				assert_is_empty(update_log);
			}


			test( FrontendUpdateIsNotCalledIfNoAnalysisInvoked )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + c_overhead.inner, (void *)(0)	},
				};

				cc->Add(mt::thread::id(), trace);

				// ACT
				b.update_frontend();

				// ASSERT
				assert_is_empty(update_log);
			}


			test( FrontendUpdateClearsTheAnalyzer )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + c_overhead.inner, (void *)(0)	},
				};

				cc->Add(mt::thread::id(), trace);

				b.analyze();
				b.update_frontend();

				// ACT
				b.update_frontend();

				// ASSERT
				assert_equal(1u, update_log.size());
			}


			test( CollectedCallsArePassedToFrontend )
			{
				// INIT
				vector<mocks::statistics_map_detailed> update_log1, update_log2;
				mocks::Tracer cc1, cc2;
				overhead o1(23, 0), o2(31, 0);
				shared_ptr<mocks::frontend_state> state1(state),
					state2(new mocks::frontend_state(shared_ptr<void>()));
				shared_ptr<ipc::channel> frontend2 = state2->create();
				statistics_bridge b1(cc1, o1, *frontend, mtracker), b2(cc2, o2, *frontend2, mtracker);
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	10 + o1.inner, (void *)(0)	},
					{	1000, (void *)0x1223	},
					{	1029 + o1.inner, (void *)(0)	},
				};
				call_record trace2[] = {
					{	0, (void *)0x2223	},
					{	13 + o2.inner, (void *)(0)	},
					{	1000, (void *)0x3223	},
					{	1017 + o2.inner, (void *)(0)	},
					{	2000, (void *)0x4223	},
					{	2019 + o2.inner, (void *)(0)	},
				};
				state1->updated = [&] (const mocks::statistics_map_detailed &u) { update_log1.push_back(u); };
				state2->updated = [&] (const mocks::statistics_map_detailed &u) { update_log2.push_back(u); };

				cc1.Add(mt::thread::id(), trace1);
				cc2.Add(mt::thread::id(), trace2);

				// ACT
				b1.analyze();
				b1.update_frontend();
				b2.analyze();
				b2.update_frontend();

				// ASSERT
				assert_equal(1u, update_log1.size());
				assert_equal(1u, update_log1[0].size());
				assert_equal(2u, update_log1[0][0x1223].times_called);
				assert_equal(39, update_log1[0][0x1223].exclusive_time);
				assert_equal(39, update_log1[0][0x1223].inclusive_time);

				assert_equal(1u, update_log2.size());
				assert_equal(3u, update_log2[0].size());
				assert_equal(1u, update_log2[0][0x2223].times_called);
				assert_equal(13, update_log2[0][0x2223].exclusive_time);
				assert_equal(13, update_log2[0][0x2223].inclusive_time);
				assert_equal(1u, update_log2[0][0x3223].times_called);
				assert_equal(17, update_log2[0][0x3223].exclusive_time);
				assert_equal(17, update_log2[0][0x3223].inclusive_time);
				assert_equal(1u, update_log2[0][0x4223].times_called);
				assert_equal(19, update_log2[0][0x4223].exclusive_time);
				assert_equal(19, update_log2[0][0x4223].inclusive_time);
			}


			test( LoadedModulesAreReportedOnUpdate )
			{
				// INIT
				vector<loaded_modules> loads;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);

				state->modules_loaded = [&] (const loaded_modules &m) { loads.push_back(m); };

				// ACT
				mtracker->load(images[0].get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, loads.size());
				assert_equal(1u, loads[0].size());

				assert_equal(0u, loads[0][0].instance_id);

				// ACT
				mtracker->load(images[1].get_symbol_address("get_function_addresses_2"));
				mtracker->load(images[2].get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, loads.size());
				assert_equal(2u, loads[1].size());

				assert_equal(1u, loads[1][0].instance_id);
				assert_equal(2u, loads[1][1].instance_id);
			}


			test( UnloadedModulesAreReportedOnUpdate )
			{
				// INIT
				vector<unloaded_modules> unloads;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);

				mtracker->load(images[0].get_symbol_address("get_function_addresses_1"));
				mtracker->load(images[1].get_symbol_address("get_function_addresses_2"));
				mtracker->load(images[2].get_symbol_address("get_function_addresses_3"));
				b.update_frontend();
				state->modules_unloaded = [&] (const unloaded_modules &m) { unloads.push_back(m); };

				// ACT
				mtracker->unload(images[0].get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, unloads.size());
				assert_equal(1u, unloads[0].size());

				assert_equal(0u, unloads[0][0]);

				// ACT
				mtracker->unload(images[1].get_symbol_address("get_function_addresses_2"));
				mtracker->unload(images[2].get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, unloads.size());
				assert_equal(2u, unloads[1].size());

				assert_equal(1u, unloads[1][0]);
				assert_equal(2u, unloads[1][1]);
			}


			test( EventsAreReportedInLoadsUpdatesUnloadsOrder )
			{
				// INIT
				vector<int> order;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);
				call_record trace[] = {
					{	0, (void *)0x2223	},
					{	2019, (void *)0	},
				};

				mtracker->load(images[0].get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				state->modules_loaded = [&] (const loaded_modules &) { order.push_back(0); };
				state->updated = [&] (const mocks::statistics_map_detailed &) { order.push_back(1); };
				state->modules_unloaded = [&] (const unloaded_modules &) { order.push_back(2); };

				cc->Add(mt::thread::id(), trace);
				b.analyze();

				// ACT
				mtracker->load(images[1].get_symbol_address("get_function_addresses_2"));
				mtracker->unload(images[0].get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				int reference1[] = { 0, 1, 2, };

				assert_equal(reference1, order);
			}


			test( ModuleMetadataIsSerializedOnRequest )
			{
				// INIT
				mapped_module mb;
				module_info_metadata md;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);

				state->metadata_received = [&] (const mapped_module &mb_, const module_info_metadata &md_) {
					mb = mb_, md = md_;
				};

				mtracker->load(images[0].get_symbol_address("get_function_addresses_1"));
				mtracker->load(images[1].get_symbol_address("get_function_addresses_2"));

				// ACT
				b.send_module_metadata(1);

				// ASSERT
				assert_equal(1u, mb.instance_id);
				assert_equal(images[1].load_address(), mb.load_address);
				assert_not_equal(string::npos, mb.path.find("symbol_container_2"));

				const symbol_info *symbol = get_symbol_by_name(md.symbols, "get_function_addresses_2");

				assert_not_null(symbol);
				assert_equal(images[1].get_symbol_address("get_function_addresses_2"),
					images[1].load_address_ptr() + symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_2.cpp", *get_file(md.source_files, symbol->file_id));
#endif

				symbol = get_symbol_by_name(md.symbols, "guinea_snprintf");

				assert_not_null(symbol);
				assert_equal(images[1].get_symbol_address("guinea_snprintf"),
					images[1].load_address_ptr() + symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_2.cpp", *get_file(md.source_files, symbol->file_id));
#endif

				// ACT
				b.send_module_metadata(0);

				// ASSERT
				assert_equal(0u, mb.instance_id);
				assert_equal(images[0].load_address(), mb.load_address);
				assert_not_equal(string::npos, mb.path.find("symbol_container_1"));

				symbol = get_symbol_by_name(md.symbols, "get_function_addresses_1");

				assert_not_null(symbol);
				assert_equal(images[0].get_symbol_address("get_function_addresses_1"),
					images[0].load_address_ptr() + symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_1.cpp", *get_file(md.source_files, symbol->file_id));
#endif
			}
		end_test_suite
	}
}
