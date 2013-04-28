#include <collector/statistics_bridge.h>
#include <collector/calls_collector.h>

#include "Mockups.h"

#include <functional>
#include <vector>

namespace std
{
	using namespace tr1;
}

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			void factory1(vector<IProfilerFrontend *> &in_log, IProfilerFrontend **frontend)
			{
				in_log.push_back(*frontend);
				*frontend = 0;
			}

			void factory2(FrontendMockupState &state, IProfilerFrontend **frontend)
			{
				IProfilerFrontend *fe = new FrontendMockup(state);

				fe->QueryInterface(frontend);
			}

		}

		[TestClass]
		public ref class StatisticsBridgeTests
		{
		public:
			[TestMethod]
			void ConstructingBridgeInvokesFrontendFactory()
			{
				// INIT
				calls_collector cc(10000);
				vector<IProfilerFrontend *> log;

				// ACT
				statistics_bridge b(cc, bind(&factory1, ref(log), _1));

				// ASSERT
				Assert::IsTrue(1 == log.size());
				Assert::IsTrue(log[0] == 0);
			}


			[TestMethod]
			void BridgeHandlesNoncreatedFrontend()
			{
				// INIT
				calls_collector cc(10000);
				vector<IProfilerFrontend *> log;
				statistics_bridge b(cc, bind(&factory1, ref(log), _1));

				// ACT / ASSERT (must not fail)
				b.update_frontend();
			}


			[TestMethod]
			void BridgeHoldsFrontendForALifetime()
			{
				// INIT
				FrontendMockupState state;
				calls_collector cc(10000);

				// INIT / ACT
				{
					statistics_bridge b(cc, bind(&factory2, ref(state), _1));

				// ASSERT
					Assert::IsFalse(state.released);

				// ACT (dtor)
				}

				// ASSERT
				Assert::IsTrue(state.released);
			}


			[TestMethod]
			void FrontendIsInitializedAtBridgeConstruction()
			{
				// INIT
				FrontendMockupState state;
				calls_collector cc(10000);

				// ACT
				statistics_bridge b(cc, bind(&factory2, ref(state), _1));

				// ASSERT
				wchar_t path[MAX_PATH + 1] = { 0 };

				::GetModuleFileNameW(NULL, path, MAX_PATH + 1);
				void *exe_module = ::GetModuleHandle(NULL);
				hyper real_resolution = timestamp_precision();

				Assert::IsTrue(state.executable == path);
				Assert::IsTrue(reinterpret_cast<hyper>(exe_module) == state.load_address);
				Assert::IsTrue(90 * real_resolution / 100 < state.ticks_resolution && state.ticks_resolution < 110 * real_resolution / 100);
			}


			[TestMethod]
			void FrontendUpdateIsNotCalledIfNoUpdates()
			{
				// INIT
				FrontendMockupState state;
				calls_collector cc(10000);
				statistics_bridge b(cc, bind(&factory2, ref(state), _1));

				// ACT
				b.analyze();
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(0 == state.update_log.size());
			}


			[TestMethod]
			void FrontendUpdateIsNotCalledIfNoAnalysisInvoked()
			{
				// INIT
				FrontendMockupState state;
				calls_collector cc(10000);
				statistics_bridge b(cc, bind(&factory2, ref(state), _1));
				call_record trace[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.track(trace[0]);
				cc.track(trace[1]);

				// ACT
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(0 == state.update_log.size());
			}


			[TestMethod]
			void FrontendUpdateClearsTheAnalyzer()
			{
				// INIT
				FrontendMockupState state;
				calls_collector cc(10000);
				statistics_bridge b(cc, bind(&factory2, ref(state), _1));
				call_record trace[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.track(trace[0]);
				cc.track(trace[1]);

				b.analyze();
				b.update_frontend();

				// ACT
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(1 == state.update_log.size());
			}


			[TestMethod]
			void CollectedCallsArePassedToFrontend()
			{
				// INIT
				FrontendMockupState state1, state2;
				calls_collector cc1(10000), cc2(1000);
				statistics_bridge b1(cc1, bind(&factory2, ref(state1), _1)), b2(cc2, bind(&factory2, ref(state2), _1));
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

				cc1.track(trace1[0]);
				cc1.track(trace1[1]);
				cc1.track(trace1[2]);
				cc1.track(trace1[3]);
				cc2.track(trace2[0]);
				cc2.track(trace2[1]);
				cc2.track(trace2[2]);
				cc2.track(trace2[3]);
				cc2.track(trace2[4]);
				cc2.track(trace2[5]);

				// ACT
				b1.analyze();
				b1.update_frontend();
				b2.analyze();
				b2.update_frontend();

				// ASSERT
				Assert::IsTrue(1 == state1.update_log.size());
				Assert::IsTrue(1 == state1.update_log[0].size());
				Assert::IsTrue(2 == state1.update_log[0][(void*)0x1223].times_called);
				Assert::IsTrue(39 == state1.update_log[0][(void*)0x1223].exclusive_time);
				Assert::IsTrue(39 == state1.update_log[0][(void*)0x1223].inclusive_time);

				Assert::IsTrue(1 == state2.update_log.size());
				Assert::IsTrue(3 == state2.update_log[0].size());
				Assert::IsTrue(1 == state2.update_log[0][(void*)0x2223].times_called);
				Assert::IsTrue(13 == state2.update_log[0][(void*)0x2223].exclusive_time);
				Assert::IsTrue(13 == state2.update_log[0][(void*)0x2223].inclusive_time);
				Assert::IsTrue(1 == state2.update_log[0][(void*)0x3223].times_called);
				Assert::IsTrue(17 == state2.update_log[0][(void*)0x3223].exclusive_time);
				Assert::IsTrue(17 == state2.update_log[0][(void*)0x3223].inclusive_time);
				Assert::IsTrue(1 == state2.update_log[0][(void*)0x4223].times_called);
				Assert::IsTrue(19 == state2.update_log[0][(void*)0x4223].exclusive_time);
				Assert::IsTrue(19 == state2.update_log[0][(void*)0x4223].inclusive_time);
			}
		};
	}
}
