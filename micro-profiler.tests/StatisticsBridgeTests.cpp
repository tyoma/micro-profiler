#include <collector/statistics_bridge.h>

#include "Helpers.h"
#include "Mockups.h"

#include <tchar.h>
#include <algorithm>
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
			void dummy(const void *, size_t)
			{	}

			channel_t VoidCreationFactory(bool &created)
			{
				created = true;
				return &dummy;
			}
		}

		begin_test_suite( StatisticsBridgeTests )
			vector<image> _images;
			mockups::FrontendState _state;
			shared_ptr<image_load_queue> _queue;

			init( CreateQueue )
			{
				image images[] = {
					image(L"symbol_container_1.dll"),
					image(L"symbol_container_2.dll"),
					image(L"symbol_container_3_nosymbols.dll"),
				};

				_images.assign(images, array_end(images));
				_queue.reset(new image_load_queue);
			}


			test( ConstructingBridgeInvokesFrontendFactory )
			{
				// INIT
				mockups::Tracer cc(10000);
				bool created = false;

				// ACT
				statistics_bridge b(cc, bind(&VoidCreationFactory, ref(created)), _queue);

				// ASSERT
				assert_is_true(created);
			}


			test( BridgeHoldsFrontendForALifetime )
			{
				// INIT
				mockups::Tracer cc(10000);

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
				mockups::Tracer cc(10000);

				// ACT
				statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ASSERT
				timestamp_t real_resolution = timestamp_precision();

				assert_equal(get_current_process_executable(), _state.process_init.first);
				assert_is_true(90 * real_resolution / 100
					< _state.process_init.second && _state.process_init.second
					< 110 * real_resolution / 100);
			}


			test( FrontendUpdateIsNotCalledIfNoUpdates )
			{
				// INIT
				mockups::Tracer cc(10000);
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
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(0, trace);

				// ACT
				b.update_frontend();

				// ASSERT
				assert_is_empty(_state.update_log);
			}


			test( FrontendUpdateClearsTheAnalyzer )
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);
				call_record trace[] = {
					{	0, (void *)0x1223	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(0, trace);

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
				mockups::FrontendState state2;
				mockups::Tracer cc1(10000), cc2(1000);
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

				cc1.Add(0, trace1);
				cc2.Add(0, trace2);

				// ACT
				b1.analyze();
				b1.update_frontend();
				b2.analyze();
				b2.update_frontend();

				// ASSERT
				assert_equal(1u, _state.update_log.size());
				assert_equal(1u, _state.update_log[0].update.size());
				assert_equal(2u, _state.update_log[0].update[(void*)0x1223].times_called);
				assert_equal(39, _state.update_log[0].update[(void*)0x1223].exclusive_time);
				assert_equal(39, _state.update_log[0].update[(void*)0x1223].inclusive_time);

				assert_equal(1u, state2.update_log.size());
				assert_equal(3u, state2.update_log[0].update.size());
				assert_equal(1u, state2.update_log[0].update[(void*)0x2223].times_called);
				assert_equal(13, state2.update_log[0].update[(void*)0x2223].exclusive_time);
				assert_equal(13, state2.update_log[0].update[(void*)0x2223].inclusive_time);
				assert_equal(1u, state2.update_log[0].update[(void*)0x3223].times_called);
				assert_equal(17, state2.update_log[0].update[(void*)0x3223].exclusive_time);
				assert_equal(17, state2.update_log[0].update[(void*)0x3223].inclusive_time);
				assert_equal(1u, state2.update_log[0].update[(void*)0x4223].times_called);
				assert_equal(19, state2.update_log[0].update[(void*)0x4223].exclusive_time);
				assert_equal(19, state2.update_log[0].update[(void*)0x4223].inclusive_time);
			}


			test( LoadedModulesAreReportedOnUpdate )
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);

				// ACT
				(_queue)->load(_images.at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				assert_equal(1u, _state.update_log.size());
				assert_equal(1u, _state.update_log[0].image_loads.size());

				assert_equal(_images.at(0).load_address(), _state.update_log[0].image_loads[0].load_address);
				assert_not_equal(wstring::npos, _state.update_log[0].image_loads[0].path.find(L"symbol_container_1.dll"));

				// ACT
				(_queue)->load(_images.at(1).get_symbol_address("get_function_addresses_2"));
				(_queue)->load(_images.at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				assert_equal(2u, _state.update_log.size());
				assert_equal(2u, _state.update_log[1].image_loads.size());

				assert_equal(_images.at(1).load_address(), _state.update_log[1].image_loads[0].load_address);
				assert_not_equal(wstring::npos, _state.update_log[1].image_loads[0].path.find(L"symbol_container_2.dll"));
				assert_equal(_images.at(2).load_address(), _state.update_log[1].image_loads[1].load_address);
				assert_not_equal(wstring::npos, _state.update_log[1].image_loads[1].path.find(L"symbol_container_3_nosymbols.dll"));
			}


			test( UnloadedModulesAreReportedOnUpdate )
			{
				// INIT
				mockups::Tracer cc(10000);
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
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state.MakeFactory(), _queue);
				call_record trace[] = {
					{	0, (void *)0x2223	},
					{	2019, (void *)0	},
				};

				cc.Add(0, trace);
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
