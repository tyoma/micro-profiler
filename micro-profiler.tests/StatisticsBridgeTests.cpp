#include <collector/statistics_bridge.h>

#include "Mockups.h"

namespace std
{
	using tr1::bind;
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
		public ref class StatisticsBridgeTests
		{
		public:
			[TestMethod]
			void ConstructingBridgeInvokesFrontendFactory()
			{
				// INIT
				mockups::Tracer cc(10000);
				vector<IProfilerFrontend *> log;

				// ACT
				statistics_bridge b(cc, bind(&VoidCreationFactory, ref(log), _1));

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
				statistics_bridge b(cc, bind(&VoidCreationFactory, ref(log), _1));

				// ACT / ASSERT (must not fail)
				b.update_frontend();
			}


			[TestMethod]
			void BridgeHoldsFrontendForALifetime()
			{
				// INIT
				mockups::Frontend::State state;
				mockups::Tracer cc(10000);

				// INIT / ACT
				{
					statistics_bridge b(cc, mockups::Frontend::MakeFactory(state));

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
				mockups::Frontend::State state;
				mockups::Tracer cc(10000);

				// ACT
				statistics_bridge b(cc, mockups::Frontend::MakeFactory(state));

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
				mockups::Frontend::State state;
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, mockups::Frontend::MakeFactory(state));

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
				mockups::Frontend::State state;
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, mockups::Frontend::MakeFactory(state));
				call_record trace[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	10 + cc.profiler_latency(), (void *)(0)	},
				};

				cc.Add(0, trace);

				// ACT
				b.update_frontend();

				// ASSERT
				Assert::IsTrue(0 == state.update_log.size());
			}


			[TestMethod]
			void FrontendUpdateClearsTheAnalyzer()
			{
				// INIT
				mockups::Frontend::State state;
				mockups::Tracer cc(10000);
				statistics_bridge b(cc, mockups::Frontend::MakeFactory(state));
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
				Assert::IsTrue(1 == state.update_log.size());
			}


			[TestMethod]
			void CollectedCallsArePassedToFrontend()
			{
				// INIT
				mockups::Frontend::State state1, state2;
				mockups::Tracer cc1(10000), cc2(1000);
				statistics_bridge b1(cc1, mockups::Frontend::MakeFactory(state1)), b2(cc2, mockups::Frontend::MakeFactory(state2));
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
