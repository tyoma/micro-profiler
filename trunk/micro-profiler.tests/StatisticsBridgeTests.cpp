#include <collector/statistics_bridge.h>

#include "Helpers.h"
#include "Mockups.h"

#include <tchar.h>
#include <algorithm>

namespace std
{
	using tr1::bind;
	using tr1::ref;
	using namespace tr1::placeholders;
}

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			void VoidCreationFactory(vector<IProfilerFrontend *> &in_log, IProfilerFrontend **frontend)
			{
				in_log.push_back(*frontend);
				*frontend = 0;
			}
		}

		[TestClass]
		[DeploymentItem("symbol_container_1.dll")]
		[DeploymentItem("symbol_container_2.dll")]
		[DeploymentItem("symbol_container_3_nosymbols.dll")]
		public ref class StatisticsBridgeTests
		{
			const vector<image> *_images;
			mockups::FrontendState *_state;
			shared_ptr<image_load_queue> *_queue;

		public:
			[TestInitialize]
			void CreateQueue()
			{
				shared_ptr<image_load_queue> q(new image_load_queue);
				image images[] = {
					image(_T("symbol_container_1.dll")),
					image(_T("symbol_container_2.dll")),
					image(_T("symbol_container_3_nosymbols.dll")),
				};

				_images = new vector<image>(images, images + _countof(images));
				_state = new mockups::FrontendState;
				_queue = new shared_ptr<image_load_queue>(q);
			}


			[TestCleanup]
			void DeleteQueue()
			{
				delete _queue;
				_queue = 0;
				delete _state;
				_state = 0;
				delete _images;
				_images = 0;
			}


			[TestMethod]
			void ConstructingBridgeInvokesFrontendFactory()
			{
				// INIT
				mockups::Tracer cc(10000);
				vector<IProfilerFrontend *> log;

				// ACT
				statistics_bridge b(cc, bind(&VoidCreationFactory, ref(log), _1), *_queue);

				// ASSERT
				Assert::IsTrue(1 == log.size());
				Assert::IsTrue(log[0] == 0);
			}


			[TestMethod]
			void BridgeHandlesNoncreatedFrontend()
			{
				// INIT
				mockups::Tracer cc(10000);
				vector<IProfilerFrontend *> log;
				statistics_bridge b(cc, bind(&VoidCreationFactory, ref(log), _1), *_queue);

				// ACT / ASSERT (must not fail)
				b.update_frontend();
			}


			[TestMethod]
			void BridgeHoldsFrontendForALifetime()
			{
				// INIT
				mockups::Tracer cc(10000);

				// INIT / ACT
				{
					statistics_bridge b(cc, _state->MakeFactory(), *_queue);

				// ASSERT
					Assert::IsFalse(_state->released);

				// ACT (dtor)
				}

				// ASSERT
				Assert::IsTrue(_state->released);
			}


			[TestMethod]
			void FrontendIsInitializedAtBridgeConstruction()
			{
				// INIT
				mockups::Tracer cc(10000);

				// ACT
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);

				// ASSERT
				long long real_resolution = timestamp_precision();

				Assert::IsTrue(get_current_process_id() == _state->process_id);
				Assert::IsTrue(90 * real_resolution / 100 < _state->ticks_resolution && _state->ticks_resolution < 110 * real_resolution / 100);
			}


			[TestMethod]
			void FrontendUpdateIsNotCalledIfNoUpdates()
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);

				// ACT
				b.analyze();
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(0 == _state->update_log.size());
			}


			[TestMethod]
			void FrontendUpdateIsNotCalledIfNoAnalysisInvoked()
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);
				call_record trace[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(0, trace);

				// ACT
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(0 == _state->update_log.size());
			}


			[TestMethod]
			void FrontendUpdateClearsTheAnalyzer()
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);
				call_record trace[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(0, trace);

				b.analyze();
				b.update_frontend();

				// ACT
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(1 == _state->update_log.size());
			}


			[TestMethod]
			void CollectedCallsArePassedToFrontend()
			{
				// INIT
				mockups::FrontendState state2;
				mockups::Tracer cc1(10000), cc2(1000);
				statistics_bridge b1(cc1, _state->MakeFactory(), *_queue),
					b2(cc2, state2.MakeFactory(), *_queue);
				call_record trace1[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	10 + cc1.profiler_latency(), (void *)(0)	},
					{	1000, (void *)(0x1223 + 5)	},
					{	1029 + cc1.profiler_latency(), (void *)(0)	},
				};
				call_record trace2[] = {
					{	0, (void *)(0x2223 + 5)	},
					{	13 + cc2.profiler_latency(), (void *)(0)	},
					{	1000, (void *)(0x3223 + 5)	},
					{	1017 + cc2.profiler_latency(), (void *)(0)	},
					{	2000, (void *)(0x4223 + 5)	},
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
				Assert::IsTrue(1 == _state->update_log.size());
				Assert::IsTrue(1 == _state->update_log[0].update.size());
				Assert::IsTrue(2 == _state->update_log[0].update[(void*)0x1223].times_called);
				Assert::IsTrue(39 == _state->update_log[0].update[(void*)0x1223].exclusive_time);
				Assert::IsTrue(39 == _state->update_log[0].update[(void*)0x1223].inclusive_time);

				Assert::IsTrue(1 == state2.update_log.size());
				Assert::IsTrue(3 == state2.update_log[0].update.size());
				Assert::IsTrue(1 == state2.update_log[0].update[(void*)0x2223].times_called);
				Assert::IsTrue(13 == state2.update_log[0].update[(void*)0x2223].exclusive_time);
				Assert::IsTrue(13 == state2.update_log[0].update[(void*)0x2223].inclusive_time);
				Assert::IsTrue(1 == state2.update_log[0].update[(void*)0x3223].times_called);
				Assert::IsTrue(17 == state2.update_log[0].update[(void*)0x3223].exclusive_time);
				Assert::IsTrue(17 == state2.update_log[0].update[(void*)0x3223].inclusive_time);
				Assert::IsTrue(1 == state2.update_log[0].update[(void*)0x4223].times_called);
				Assert::IsTrue(19 == state2.update_log[0].update[(void*)0x4223].exclusive_time);
				Assert::IsTrue(19 == state2.update_log[0].update[(void*)0x4223].inclusive_time);
			}


			[TestMethod]
			void LoadedModulesAreReportedOnUpdate()
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);

				// ACT
				(*_queue)->load(_images->at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(1u == _state->update_log.size());
				Assert::IsTrue(1u == _state->update_log[0].image_loads.size());

				Assert::IsTrue(reinterpret_cast<uintptr_t>(_images->at(0).load_address())
					== _state->update_log[0].image_loads[0].first);
				Assert::IsTrue(wstring::npos != _state->update_log[0].image_loads[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));

				// ACT
				(*_queue)->load(_images->at(1).get_symbol_address("get_function_addresses_2"));
				(*_queue)->load(_images->at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(2u == _state->update_log.size());
				Assert::IsTrue(2u == _state->update_log[1].image_loads.size());

				Assert::IsTrue(reinterpret_cast<uintptr_t>(_images->at(1).load_address())
					== _state->update_log[1].image_loads[0].first);
				Assert::IsTrue(wstring::npos != _state->update_log[1].image_loads[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
				Assert::IsTrue(reinterpret_cast<uintptr_t>(_images->at(2).load_address())
					== _state->update_log[1].image_loads[1].first);
				Assert::IsTrue(wstring::npos != _state->update_log[1].image_loads[1].second.find(L"SYMBOL_CONTAINER_3_NOSYMBOLS.DLL"));
			}


			[TestMethod]
			void UnloadedModulesAreReportedOnUpdate()
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);

				// ACT
				(*_queue)->unload(_images->at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(1u == _state->update_log.size());
				Assert::IsTrue(1u == _state->update_log[0].image_unloads.size());

				Assert::IsTrue(reinterpret_cast<uintptr_t>(_images->at(0).load_address())
					== _state->update_log[0].image_unloads[0]);

				// ACT
				(*_queue)->unload(_images->at(1).get_symbol_address("get_function_addresses_2"));
				(*_queue)->unload(_images->at(2).get_symbol_address("get_function_addresses_3"));
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(2u == _state->update_log.size());
				Assert::IsTrue(2u == _state->update_log[1].image_unloads.size());

				Assert::IsTrue(reinterpret_cast<uintptr_t>(_images->at(1).load_address())
					== _state->update_log[1].image_unloads[0]);
				Assert::IsTrue(reinterpret_cast<uintptr_t>(_images->at(2).load_address())
					== _state->update_log[1].image_unloads[1]);
			}


			[TestMethod]
			void EventsAreReportedInLoadsUpdatesUnloadsOrder()
			{
				// INIT
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, _state->MakeFactory(), *_queue);
				call_record trace[] = {
					{	0, (void *)(0x2223 + 5)	},
					{	2019, (void *)(0)	},
				};

				cc.Add(0, trace);
				b.analyze();

				// ACT
				(*_queue)->load(_images->at(1).get_symbol_address("get_function_addresses_2"));
				(*_queue)->unload(_images->at(0).get_symbol_address("get_function_addresses_1"));
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(3u == _state->update_log.size());
				
				Assert::IsTrue(1u == _state->update_log[0].image_loads.size());
				Assert::IsTrue(_state->update_log[0].update.empty());
				Assert::IsTrue(_state->update_log[0].image_unloads.empty());
				
				Assert::IsTrue(_state->update_log[1].image_loads.empty());
				Assert::IsTrue(1u == _state->update_log[1].update.size());
				Assert::IsTrue(_state->update_log[1].image_unloads.empty());
				
				Assert::IsTrue(_state->update_log[2].image_loads.empty());
				Assert::IsTrue(_state->update_log[2].update.empty());
				Assert::IsTrue(1u == _state->update_log[2].image_unloads.size());
			}
		};
	}
}
