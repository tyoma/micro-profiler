#include <collector/statistics_bridge.h>

#include "helpers.h"
#include "mocks.h"

#include <algorithm>
#include <collector/module_tracker.h>
#include <common/path.h>
#include <common/time.h>
#include <test-helpers/constants.h>
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
			shared_ptr<mocks::Tracer> cc;
			shared_ptr<mocks::frontend_state> state;
			shared_ptr<ipc::channel> frontend;
			vector<mocks::statistics_map_detailed> update_log;
			shared_ptr<module_tracker> mtracker;
			unsigned int ref_count;

			init( CreateQueue )
			{
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
				image image0(c_symbol_container_1);
				b.update_frontend();

				// ASSERT
				assert_equal(1u, loads.size());
				assert_not_null(find_module(loads[0], c_symbol_container_1));

				// ACT
				image image1(c_symbol_container_2);
				image image3(c_symbol_container_3_nosymbols);
				b.update_frontend();

				// ASSERT
				assert_equal(2u, loads.size());
				assert_equal(2u, loads[1].size());
				assert_not_null(find_module(loads[1], c_symbol_container_2));
				assert_not_null(find_module(loads[1], c_symbol_container_3_nosymbols));
			}


			test( UnloadedModulesAreReportedOnUpdate )
			{
				// INIT
				loaded_modules l;
				vector<unloaded_modules> unloads;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);

				state->modules_loaded = [&] (const loaded_modules &m) { l = m; };
				state->modules_unloaded = [&] (const unloaded_modules &m) { unloads.push_back(m); };

				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				auto_ptr<image> image2(new image(c_symbol_container_3_nosymbols));
				b.update_frontend();

				const mapped_module_identified *mmi[] = {
					find_module(l, c_symbol_container_1), find_module(l, c_symbol_container_2), find_module(l, c_symbol_container_3_nosymbols),
				};

				// ACT
				image1.reset();
				b.update_frontend();

				// ASSERT
				assert_equal(1u, unloads.size());

				unsigned reference1[] = { mmi[1]->instance_id, };

				assert_equivalent(reference1, unloads[0]);

				// ACT
				image0.reset();
				image2.reset();
				b.update_frontend();

				// ASSERT
				assert_equal(2u, unloads.size());

				unsigned reference2[] = { mmi[0]->instance_id, mmi[2]->instance_id, };

				assert_equivalent(reference2, unloads[1]);
			}


			test( ModuleMetadataIsSerializedOnRequest )
			{
				// INIT
				unsigned persistent_id;
				module_info_metadata md;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker);
				loaded_modules l;
				unloaded_modules u;

				state->metadata_received = [&] (unsigned id, const module_info_metadata &md_) {
					persistent_id = id, md = md_;
				};

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);

				mtracker->get_changes(l, u);

				const mapped_module_identified *mmi[] = { find_module(l, c_symbol_container_1), find_module(l, c_symbol_container_2), };

				// ACT
				b.send_module_metadata(mmi[1]->persistent_id);

				// ASSERT
				assert_equal(persistent_id, mmi[1]->persistent_id);

				const symbol_info *symbol = get_symbol_by_name(md.symbols, "get_function_addresses_2");

				assert_not_null(symbol);
				assert_equal(image1.get_symbol_rva("get_function_addresses_2"), symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_2.cpp", *get_file(md.source_files, symbol->file_id));
#endif

				symbol = get_symbol_by_name(md.symbols, "guinea_snprintf");

				assert_not_null(symbol);
				assert_equal(image1.get_symbol_rva("guinea_snprintf"), symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_2.cpp", *get_file(md.source_files, symbol->file_id));
#endif

				// ACT
				b.send_module_metadata(mmi[0]->persistent_id);

				// ASSERT
				assert_equal(persistent_id, mmi[0]->persistent_id);

				symbol = get_symbol_by_name(md.symbols, "get_function_addresses_1");

				assert_not_null(symbol);
				assert_equal(image0.get_symbol_rva("get_function_addresses_1"), symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_1.cpp", *get_file(md.source_files, symbol->file_id));
#endif
			}
		end_test_suite
	}
}
