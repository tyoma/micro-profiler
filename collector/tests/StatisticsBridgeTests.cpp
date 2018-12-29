#include <collector/statistics_bridge.h>

#include "mocks.h"

#include <algorithm>
#include <collector/calibration.h>
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
			const overhead c_overhead = { 17, 0 };
		}

		begin_test_suite( StatisticsBridgeTests )
			vector<image> images;
			shared_ptr<mocks::Tracer> cc;
			shared_ptr<mocks::frontend_state> state;
			shared_ptr<ipc::channel> frontend;
			vector<mocks::statistics_map_detailed> update_log;
			shared_ptr<image_load_queue> queue;
			unsigned int ref_count;

			init( CreateQueue )
			{
				image images_[] = {
					image(L"symbol_container_1"),
					image(L"symbol_container_2"),
					image(L"symbol_container_3_nosymbols"),
				};

				images.assign(images_, array_end(images_));
				queue.reset(new image_load_queue);
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
				statistics_bridge b(*cc, c_overhead, *frontend, queue);

				// ASSERT
				assert_equal(get_current_process_executable(), process_init.executable);
				assert_approx_equal(ticks_per_second(), process_init.ticks_per_second, 0.01);
			}


			test( FrontendUpdateIsNotCalledIfNoUpdates )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, queue);

				// ACT
				b.analyze();
				b.update_frontend();

				// ASSERT
				assert_is_empty(update_log);
			}


			test( FrontendUpdateIsNotCalledIfNoAnalysisInvoked )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, queue);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + c_overhead.external, (void *)(0)	},
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
				statistics_bridge b(*cc, c_overhead, *frontend, queue);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + c_overhead.external, (void *)(0)	},
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
				overhead o1 = { 23, 0, }, o2 = { 31, 0 };
				shared_ptr<mocks::frontend_state> state1(state),
					state2(new mocks::frontend_state(shared_ptr<void>()));
				shared_ptr<ipc::channel> frontend2 = state2->create();
				statistics_bridge b1(cc1, o1, *frontend, queue), b2(cc2, o2, *frontend2, queue);
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	10 + o1.external, (void *)(0)	},
					{	1000, (void *)0x1223	},
					{	1029 + o1.external, (void *)(0)	},
				};
				call_record trace2[] = {
					{	0, (void *)0x2223	},
					{	13 + o2.external, (void *)(0)	},
					{	1000, (void *)0x3223	},
					{	1017 + o2.external, (void *)(0)	},
					{	2000, (void *)0x4223	},
					{	2019 + o2.external, (void *)(0)	},
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
				statistics_bridge b(*cc, c_overhead, *frontend, queue);

				state->modules_loaded = [&] (const loaded_modules &m) { loads.push_back(m); };

				// ACT
				(queue)->load(images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, loads.size());
				assert_equal(1u, loads[0].size());

				assert_equal(images.at(0).load_address(), loads[0][0].load_address);
				assert_not_equal(wstring::npos, loads[0][0].path.find(L"symbol_container_1"));

				// ACT
				(queue)->load(images.at(1).get_symbol_address("get_function_addresses_2"));
				(queue)->load(images.at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, loads.size());
				assert_equal(2u, loads[1].size());

				assert_equal(images.at(1).load_address(), loads[1][0].load_address);
				assert_not_equal(wstring::npos, loads[1][0].path.find(L"symbol_container_2"));
				assert_equal(images.at(2).load_address(), loads[1][1].load_address);
				assert_not_equal(wstring::npos, loads[1][1].path.find(L"symbol_container_3_nosymbols"));
			}


			test( UnloadedModulesAreReportedOnUpdate )
			{
				// INIT
				vector<unloaded_modules> unloads;
				statistics_bridge b(*cc, c_overhead, *frontend, queue);

				state->modules_unloaded = [&] (const unloaded_modules &m) { unloads.push_back(m); };

				// ACT
				(queue)->unload(images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, unloads.size());
				assert_equal(1u, unloads[0].size());

				assert_equal(images.at(0).load_address(), unloads[0][0]);

				// ACT
				(queue)->unload(images.at(1).get_symbol_address("get_function_addresses_2"));
				(queue)->unload(images.at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, unloads.size());
				assert_equal(2u, unloads[1].size());

				assert_equal(images.at(1).load_address(), unloads[1][0]);
				assert_equal(images.at(2).load_address(), unloads[1][1]);
			}


			test( EventsAreReportedInLoadsUpdatesUnloadsOrder )
			{
				// INIT
				vector<int> order;
				statistics_bridge b(*cc, c_overhead, *frontend, queue);
				call_record trace[] = {
					{	0, (void *)0x2223	},
					{	2019, (void *)0	},
				};

				state->modules_loaded = [&] (const loaded_modules &) { order.push_back(0); };
				state->updated = [&] (const mocks::statistics_map_detailed &) { order.push_back(1); };
				state->modules_unloaded = [&] (const unloaded_modules &) { order.push_back(2); };

				cc->Add(mt::thread::id(), trace);
				b.analyze();

				// ACT
				(queue)->load(images.at(1).get_symbol_address("get_function_addresses_2"));
				(queue)->unload(images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				int reference1[] = { 0, 1, 2, };

				assert_equal(reference1, order);
			}
		end_test_suite
	}
}
