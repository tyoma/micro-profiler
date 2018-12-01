#include <collector/statistics_bridge.h>

#include "mocks.h"

#include <algorithm>
#include <common/time.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			bool dummy(const void *, size_t)
			{	return true;	}

			channel_t VoidCreationFactory(bool &created)
			{
				created = true;
				return &dummy;
			}
		}

		begin_test_suite( StatisticsBridgeTests )
			vector<image> _images;
			mocks::FrontendState _state;
			shared_ptr<image_load_queue> _queue;

			init( CreateQueue )
			{
				image images[] = {
					image(L"symbol_container_1"),
					image(L"symbol_container_2"),
					image(L"symbol_container_3_nosymbols"),
				};

				_images.assign(images, array_end(images));
				_queue.reset(new image_load_queue);
			}


			test( ConstructingBridgeInvokesFrontendFactory )
			{
				// INIT
				mocks::Tracer cc(10000);
				bool created = false;

				// ACT
				statistics_bridge b(cc, bind(&VoidCreationFactory, ref(created)), _queue);

				// ASSERT
				assert_is_true(created);
			}


			test( BridgeHoldsFrontendForALifetime )
			{
				// INIT
				mocks::Tracer cc(10000);

				// INIT / ACT
				{
					statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ASSERT
					assert_equal(1u, _state.ref_count);

				// ACT (dtor)
				}

				// ASSERT
				assert_equal(0u, _state.ref_count);
			}


			test( FrontendIsInitializedAtBridgeConstruction )
			{
				// INIT
				mocks::Tracer cc(10000);

				// ACT
				statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ASSERT
				assert_equal(get_current_process_executable(), _state.process_init.executable);
				assert_approx_equal(ticks_per_second(), _state.process_init.ticks_per_second, 0.01);
			}


			test( FrontendUpdateIsNotCalledIfNoUpdates )
			{
				// INIT
				mocks::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ACT
				b.analyze();
				b.update_frontend();

				// ASSERT
				assert_is_empty(_state.update_log);
			}


			test( FrontendUpdateIsNotCalledIfNoAnalysisInvoked )
			{
				// INIT
				mocks::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(mt::thread::id(), trace);

				// ACT
				b.update_frontend();

				// ASSERT
				assert_is_empty(_state.update_log);
			}


			test( FrontendUpdateClearsTheAnalyzer )
			{
				// INIT
				mocks::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(mt::thread::id(), trace);

				b.analyze();
				b.update_frontend();

				// ACT
				b.update_frontend();

				// ASSERT
				assert_equal(1u, _state.update_log.size());
			}


			test( CollectedCallsArePassedToFrontend )
			{
				// INIT
				mocks::FrontendState state2;
				mocks::Tracer cc1(10000), cc2(1000);
				statistics_bridge b1(cc1, _state.MakeFactory(), _queue),
					b2(cc2, state2.MakeFactory(), _queue);
				call_record trace1[] = {
					{	0, (void *)0x1223	},
					{	10 + cc1.profiler_latency(), (void *)(0)	},
					{	1000, (void *)0x1223	},
					{	1029 + cc1.profiler_latency(), (void *)(0)	},
				};
				call_record trace2[] = {
					{	0, (void *)0x2223	},
					{	13 + cc2.profiler_latency(), (void *)(0)	},
					{	1000, (void *)0x3223	},
					{	1017 + cc2.profiler_latency(), (void *)(0)	},
					{	2000, (void *)0x4223	},
					{	2019 + cc2.profiler_latency(), (void *)(0)	},
				};

				cc1.Add(mt::thread::id(), trace1);
				cc2.Add(mt::thread::id(), trace2);

				// ACT
				b1.analyze();
				b1.update_frontend();
				b2.analyze();
				b2.update_frontend();

				// ASSERT
				assert_equal(1u, _state.update_log.size());
				assert_equal(1u, _state.update_log[0].update.size());
				assert_equal(2u, _state.update_log[0].update[0x1223].times_called);
				assert_equal(39, _state.update_log[0].update[0x1223].exclusive_time);
				assert_equal(39, _state.update_log[0].update[0x1223].inclusive_time);

				assert_equal(1u, state2.update_log.size());
				assert_equal(3u, state2.update_log[0].update.size());
				assert_equal(1u, state2.update_log[0].update[0x2223].times_called);
				assert_equal(13, state2.update_log[0].update[0x2223].exclusive_time);
				assert_equal(13, state2.update_log[0].update[0x2223].inclusive_time);
				assert_equal(1u, state2.update_log[0].update[0x3223].times_called);
				assert_equal(17, state2.update_log[0].update[0x3223].exclusive_time);
				assert_equal(17, state2.update_log[0].update[0x3223].inclusive_time);
				assert_equal(1u, state2.update_log[0].update[0x4223].times_called);
				assert_equal(19, state2.update_log[0].update[0x4223].exclusive_time);
				assert_equal(19, state2.update_log[0].update[0x4223].inclusive_time);
			}


			test( LoadedModulesAreReportedOnUpdate )
			{
				// INIT
				mocks::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ACT
				(_queue)->load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, _state.update_log.size());
				assert_equal(1u, _state.update_log[0].image_loads.size());

				assert_equal(_images.at(0).load_address(), _state.update_log[0].image_loads[0].load_address);
				assert_not_equal(wstring::npos, _state.update_log[0].image_loads[0].path.find(L"symbol_container_1"));

				// ACT
				(_queue)->load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				(_queue)->load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, _state.update_log.size());
				assert_equal(2u, _state.update_log[1].image_loads.size());

				assert_equal(_images.at(1).load_address(), _state.update_log[1].image_loads[0].load_address);
				assert_not_equal(wstring::npos, _state.update_log[1].image_loads[0].path.find(L"symbol_container_2"));
				assert_equal(_images.at(2).load_address(), _state.update_log[1].image_loads[1].load_address);
				assert_not_equal(wstring::npos, _state.update_log[1].image_loads[1].path.find(L"symbol_container_3_nosymbols"));
			}


			test( UnloadedModulesAreReportedOnUpdate )
			{
				// INIT
				mocks::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ACT
				(_queue)->unload(_images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, _state.update_log.size());
				assert_equal(1u, _state.update_log[0].image_unloads.size());

				assert_equal(_images.at(0).load_address(), _state.update_log[0].image_unloads[0]);

				// ACT
				(_queue)->unload(_images.at(1).get_symbol_address("get_function_addresses_2"));
				(_queue)->unload(_images.at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, _state.update_log.size());
				assert_equal(2u, _state.update_log[1].image_unloads.size());

				assert_equal(_images.at(1).load_address(), _state.update_log[1].image_unloads[0]);
				assert_equal(_images.at(2).load_address(), _state.update_log[1].image_unloads[1]);
			}


			test( EventsAreReportedInLoadsUpdatesUnloadsOrder )
			{
				// INIT
				mocks::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);
				call_record trace[] = {
					{	0, (void *)0x2223	},
					{	2019, (void *)0	},
				};

				cc.Add(mt::thread::id(), trace);
				b.analyze();

				// ACT
				(_queue)->load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				(_queue)->unload(_images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(3u, _state.update_log.size());
				
				assert_equal(1u, _state.update_log[0].image_loads.size());
				assert_is_empty(_state.update_log[0].update);
				assert_is_empty(_state.update_log[0].image_unloads);
				
				assert_is_empty(_state.update_log[1].image_loads);
				assert_equal(1u, _state.update_log[1].update.size());
				assert_is_empty(_state.update_log[1].image_unloads);
				
				assert_is_empty(_state.update_log[2].image_loads);
				assert_is_empty(_state.update_log[2].update);
				assert_equal(1u, _state.update_log[2].image_unloads.size());
			}
		end_test_suite
	}
}
