#include <collector/frontend_controller.h>
#include <entry.h>

#include "Helpers.h"
#include "Mockups.h"

#include <atlbase.h>

#pragma warning(disable:4965)

namespace std
{
	using tr1::bind;
	using tr1::ref;
	using tr1::shared_ptr;
}

using namespace std;
using namespace wpl::mt;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			void CheckCOMInitializedFactory(bool &com_initialized)
			{
				CComPtr<IUnknown> test;

				com_initialized = S_OK == test.CoCreateInstance(CComBSTR("JobObject")) && test;
			}

			void RaiseAt(event_flag *e, volatile long *times)
			{
				if (0 == _InterlockedDecrement(times))
					e->raise();
			}

			void ValidateThread(shared_ptr<void> *hthread, event_flag *finished_event, bool *finished)
			{
				if (!*hthread)
				{
					hthread->reset(::OpenThread(THREAD_QUERY_INFORMATION, FALSE, ::GetCurrentThreadId()), &::CloseHandle);
				}
				else
				{
					DWORD status = STILL_ACTIVE;

					*finished = ::GetExitCodeThread(hthread->get(), &status) && STILL_ACTIVE != status;
					finished_event->raise();
				}
			}

			void ControlInitialization(event_flag *proceed, vector< shared_ptr<void> > *log, volatile long *times,
				event_flag *finished)
			{
				if (proceed)
					proceed->wait();
				log->push_back(shared_ptr<void>(::OpenThread(THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE,
					::GetCurrentThreadId()), &::CloseHandle));
				RaiseAt(finished, times);
			}

			class auto_frontend_controller : public frontend_controller
			{
				const auto_ptr<handle> _profiler_handle;

			public:
				auto_frontend_controller(calls_collector_i &collector, const frontend_factory& factory)
					: frontend_controller(collector, factory), _profiler_handle(profile(&CheckCOMInitializedFactory))
				{	}
			};
		}

		[TestClass]
		public ref class FrontendControllerTests
		{
		public:
			[TestMethod]
			void FactoryIsNotCalledIfCollectionHandleWasNotObtained()
			{
				// INIT
				mockups::Frontend::State state;
				mockups::Tracer tracer;

				// ACT
				{	frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));	}

				// ASSERT
				Assert::IsTrue(thread::id() == state.creator_thread_id);
			}


			[TestMethod]
			void NonNullHandleObtained()
			{
				// INIT
				mockups::Frontend::State state;
				mockups::Tracer tracer;
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));

				// ACT
				auto_ptr<handle> h(fc.profile(&CheckCOMInitializedFactory));
				
				// ASSERT
				Assert::IsTrue(NULL != h.get());
			}


			[TestMethod]
			void FrontendIsCreatedIfProfilerHandleObtained()
			{
				// INIT
				thread::id test_threadid = threadex::current_thread_id();
				event_flag initialized(false, true);
				mockups::Frontend::State state(bind(&event_flag::raise, &initialized));
				mockups::Tracer tracer;
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));

				// ACT
				auto_ptr<handle> h(fc.profile(&CheckCOMInitializedFactory));

				// ASSERT
				initialized.wait();

				Assert::IsTrue(thread::id() != state.creator_thread_id);
				Assert::IsTrue(test_threadid != state.creator_thread_id);
			}


			[TestMethod]
			void FrontendIsCreatedTwiceOnRepeatedHandleObtaining()
			{
				// INIT
				event_flag initialized(false, true);
				volatile long counter = 2;
				mockups::Frontend::State state(bind(&RaiseAt, &initialized, &counter));
				mockups::Tracer tracer;
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));
				auto_ptr<handle> h;

				// ACT
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				h.reset();
				h.reset(fc.profile(&CheckCOMInitializedFactory));

				// ASSERT
				initialized.wait();

				// INIT
				counter = 3;

				// ACT
				h.reset();
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				h.reset();
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				h.reset();
				h.reset(fc.profile(&CheckCOMInitializedFactory));

				// ASSERT
				initialized.wait();
			}


			[TestMethod]
			void FrontendCreationIsRefCounted2()
			{
				// INIT
				event_flag initialized(false, true);
				volatile long counter = 1;
				mockups::Frontend::State state(bind(&RaiseAt, &initialized, &counter));
				mockups::Tracer tracer;
				auto_ptr<frontend_controller> fc(new frontend_controller(tracer, mockups::Frontend::MakeFactory(state)));
				auto_ptr<handle> h1(fc->profile(&CheckCOMInitializedFactory));
				auto_ptr<handle> h2(fc->profile(&CheckCOMInitializedFactory));

				initialized.wait();

				// ACT
				h1.reset();
				h2.reset();
				fc.reset();

				// ASSERT
				Assert::IsTrue(0 == counter);
			}


			[TestMethod]
			void FrontendCreationIsRefCounted3()
			{
				// INIT
				event_flag initialized(false, true);
				volatile long counter = 1;
				mockups::Frontend::State state(bind(&RaiseAt, &initialized, &counter));
				mockups::Tracer tracer;
				auto_ptr<frontend_controller> fc(new frontend_controller(tracer, mockups::Frontend::MakeFactory(state)));
				auto_ptr<handle> h1(fc->profile(&CheckCOMInitializedFactory));
				auto_ptr<handle> h2(fc->profile(&CheckCOMInitializedFactory));
				auto_ptr<handle> h3(fc->profile(&CheckCOMInitializedFactory));

				initialized.wait();

				// ACT
				h1.reset();
				h2.reset();
				h3.reset();
				fc.reset();

				// ASSERT
				Assert::IsTrue(0 == counter);
			}


			[TestMethod]
			void ByTheTimeOfSecondFrontendInitializationPreviousThreadIsDead()
			{
				// INIT
				shared_ptr<void> hthread;
				bool finished = false;
				event_flag finished_event(false, true);
				mockups::Frontend::State state(bind(&ValidateThread, &hthread, &finished_event, &finished));
				mockups::Tracer tracer;
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));
				auto_ptr<handle> h;

				// ACT
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				h.reset();
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				finished_event.wait();

				// ASSERT
				Assert::IsTrue(finished);
			}


			[TestMethod]
			void AllWorkerThreadsMustExitOnHandlesClosure()
			{
				// INIT
				volatile long times = 2;
				event_flag proceed(false, false), finished(false, true);
				vector< shared_ptr<void> > htread_log;
				mockups::Frontend::State state(bind(&ControlInitialization, &proceed, &htread_log, &times, &finished));
				mockups::Tracer tracer;
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));
				auto_ptr<handle> h;

				// ACT
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				h.reset();
				h.reset(fc.profile(&CheckCOMInitializedFactory));
				h.reset();
				proceed.raise();
				finished.wait();	// wait for the second thread to initialize the frontend

				// ASSERT (must not hang)
				::WaitForSingleObject(htread_log[1].get(), INFINITE);
			}


			[TestMethod]
			void FactoryIsCalledInASeparateThread()
			{
				// INIT
				thread::id test_threadid = threadex::current_thread_id();
				mockups::Frontend::State state;
				mockups::Tracer tracer;

				// ACT
				{	auto_frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));	}

				// ASSERT
				Assert::IsTrue(thread::id() != state.creator_thread_id);
				Assert::IsTrue(test_threadid != state.creator_thread_id);
				Assert::IsTrue(state.highest_thread_priority);
			}


			[TestMethod]
			void TwoCoexistingFrontendsHasDifferentWorkerThreads()
			{
				// INIT
				mockups::Tracer tracer1, tracer2;
				mockups::Frontend::State state1, state2;

				// ACT
				{
					auto_frontend_controller fe1(tracer1, mockups::Frontend::MakeFactory(state1));
					auto_frontend_controller fe2(tracer2, mockups::Frontend::MakeFactory(state2));
				}

				// ASSERT
				Assert::IsTrue(state1.creator_thread_id != state2.creator_thread_id);
			}


			[TestMethod]
			void ProfilerHandleReleaseIsNonBlockingAndFrontendThreadIsFinishedEventually()
			{
				// INIT
				mockups::Tracer tracer;
				vector< shared_ptr<void> > log_hthread;
				volatile long times = 1;
				event_flag initialized(false, true);
				mockups::Frontend::State state(bind(&ControlInitialization, static_cast<event_flag *>(0), &log_hthread,
					&times, &initialized));
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));
				auto_ptr<handle> h(fc.profile(&RaiseAt));

				initialized.wait();

				// ACT (must not hang)
				::SuspendThread(log_hthread[0].get());
				h.reset();
				::ResumeThread(log_hthread[0].get());

				// ASSERT (must not hang)
				::WaitForSingleObject(log_hthread[0].get(), INFINITE);
			}


			[TestMethod]
			void FrontendThreadHasCOMInitialized()	// Actually this will always pass when calling from managed, since COM already initialized
			{
				// INIT
				bool com_initialized = false;
				mockups::Tracer tracer;

				// INIT / ACT
				{	auto_frontend_controller fc(tracer, bind(&CheckCOMInitializedFactory, ref(com_initialized)));	}

				// ASSERT
				Assert::IsTrue(com_initialized);
			}


			[TestMethod]
			void FrontendInterfaceReleasedAtPFDestroyed()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::Frontend::State state;
				auto_ptr<auto_frontend_controller> fc(new auto_frontend_controller(tracer, mockups::Frontend::MakeFactory(state)));

				// ACT
				fc.reset();

				// ASERT
				Assert::IsTrue(state.released);
			}


			[TestMethod]
			void FrontendInitialized()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				mockups::Frontend::State state(bind(&event_flag::raise, &initialized));
				TCHAR path[MAX_PATH + 1] = { 0 };

				// ACT
				auto_ptr<auto_frontend_controller> fc(new auto_frontend_controller(tracer, mockups::Frontend::MakeFactory(state)));
				initialized.wait();

				// ASERT
				::GetModuleFileName(NULL, path, sizeof(path) / sizeof(TCHAR));
				void *exe_module = ::GetModuleHandle(NULL);
				hyper real_resolution = timestamp_precision();

				Assert::IsTrue(CComBSTR(state.executable.c_str()) == path);
				Assert::IsTrue(reinterpret_cast<hyper>(exe_module) == state.load_address);
				Assert::IsTrue(90 * real_resolution / 100 < state.ticks_resolution && state.ticks_resolution < 110 * real_resolution / 100);
			}


			[TestMethod]
			void FrontendIsNotBotheredWithEmptyDataUpdates()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::Frontend::State state;

				auto_frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));

				// ACT / ASSERT
				Assert::IsTrue(waitable::timeout == state.updated.wait(500));
			}


			[TestMethod]
			void MakeACallAndWaitForDataPost()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::Frontend::State state;

				auto_frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));

				// ACT
				call_record trace1[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	1000, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace1);

				state.updated.wait();

				// ASERT
				Assert::AreEqual(1u, state.update_log.size());

				statistics_map_detailed::const_iterator callinfo_1 = state.update_log[0].find((void *)0x1223);

				Assert::IsTrue(state.update_log[0].end() != callinfo_1);

				Assert::IsTrue(1 == callinfo_1->second.times_called);
				Assert::IsTrue(0 == callinfo_1->second.max_reentrance);
				Assert::IsTrue(1000 == callinfo_1->second.inclusive_time);
				Assert::IsTrue(callinfo_1->second.inclusive_time == callinfo_1->second.exclusive_time);
				Assert::IsTrue(callinfo_1->second.inclusive_time == callinfo_1->second.max_call_time);

				// ACT
				call_record trace2[] = {
					{	10000, (void *)(0x31223 + 5)	},
					{	14000, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace2);

				state.updated.wait();

				// ASERT
				Assert::AreEqual(2u, state.update_log.size());

				Assert::AreEqual(1u, state.update_log[1].size());	// The new batch MUST NOT not contain previous function.

				statistics_map_detailed::const_iterator callinfo_2 = state.update_log[1].find((void *)0x31223);

				Assert::IsTrue(state.update_log[1].end() != callinfo_2);

				Assert::IsTrue(1 == callinfo_2->second.times_called);
				Assert::IsTrue(0 == callinfo_2->second.max_reentrance);
				Assert::IsTrue(4000 == callinfo_2->second.inclusive_time);
				Assert::IsTrue(callinfo_2->second.inclusive_time == callinfo_2->second.exclusive_time);
				Assert::IsTrue(callinfo_2->second.inclusive_time == callinfo_2->second.max_call_time);
			}


			[TestMethod]
			void SecondProfilerInstanceIsWorkable()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				volatile long times = 2;
				mockups::Frontend::State state(bind(&RaiseAt, &initialized, &times));
				frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));
				auto_ptr<handle> h(fc.profile(&RaiseAt));
				call_record trace[] = {
					{	10000, (void *)(0x31223 + 5)	},
					{	14000, (void *)(0)	},
				};

				h.reset();
				h.reset(fc.profile(&RaiseAt));
				initialized.wait();

				// ACT
				tracer.Add(thread::id(), trace);

				// ASSERT (must not hang)
				state.updated.wait();

				// ACT (duplicated intentionally - flush-at-exit events may count for the first wait)
				tracer.Add(thread::id(), trace);

				// ASSERT (must not hang)
				state.updated.wait();
			}


			[TestMethod]
			void PassReentranceCountToFrontend()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::Frontend::State state;

				auto_frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));

				// ACT
				call_record trace1[] = {
					{	1, (void *)(0x31000 + 5)	},
						{	2, (void *)(0x31000 + 5)	},
							{	3, (void *)(0x31000 + 5)	},
								{	4, (void *)(0x31000 + 5)	},
								{	5, (void *)(0)	},
							{	6, (void *)(0)	},
						{	7, (void *)(0)	},
					{	8, (void *)(0)	},
					{	9, (void *)(0x31000 + 5)	},
						{	10, (void *)(0x31000 + 5)	},
							{	11, (void *)(0x31000 + 5)	},
							{	12, (void *)(0)	},
						{	13, (void *)(0)	},
					{	14, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace1);

				state.updated.wait();

				// ASERT
				Assert::AreEqual(1u, state.update_log.size());

				const function_statistics_detailed callinfo_1 = state.update_log[0][(void *)0x31000];

				Assert::IsTrue(7 == callinfo_1.times_called);
				Assert::IsTrue(3 == callinfo_1.max_reentrance);

				// ACT
				call_record trace2[] = {
					{	1, (void *)(0x31000 + 5)	},
						{	2, (void *)(0x31000 + 5)	},
							{	3, (void *)(0x31000 + 5)	},
								{	4, (void *)(0x31000 + 5)	},
									{	5, (void *)(0x31000 + 5)	},
									{	6, (void *)(0)	},
								{	7, (void *)(0)	},
							{	8, (void *)(0)	},
						{	9, (void *)(0)	},
					{	10, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace2);

				state.updated.wait();

				// ASERT
				Assert::AreEqual(2u, state.update_log.size());

				const function_statistics_detailed &callinfo_2 = state.update_log[1][(void *)0x31000];

				Assert::IsTrue(5 == callinfo_2.times_called);
				Assert::IsTrue(4 == callinfo_2.max_reentrance);
			}
			

			[TestMethod]
			void PerformanceDataTakesProfilerLatencyIntoAccount()
			{
				// INIT
				mockups::Tracer tracer1(13), tracer2(29);
				mockups::Frontend::State state1, state2;

				auto_frontend_controller fe1(tracer1, mockups::Frontend::MakeFactory(state1));
				auto_frontend_controller fe2(tracer2, mockups::Frontend::MakeFactory(state2));

				// ACT
				call_record trace[] = {
					{	10000, (void *)(0x3171717 + 5)	},
					{	14000, (void *)(0)	},
				};

				tracer1.Add(thread::id(), trace);
				state1.updated.wait();

				tracer2.Add(thread::id(), trace);
				state2.updated.wait();

				// ASSERT
				Assert::IsTrue(4000 - 13 == state1.update_log[0][(void *)0x3171717].inclusive_time);
				Assert::IsTrue(4000 - 29 == state2.update_log[0][(void *)0x3171717].inclusive_time);
			}


			[TestMethod]
			void ChildrenStatisticsIsPassedAlongWithTopLevels()
			{
				// INIT
				mockups::Tracer tracer(11);
				mockups::Frontend::State state;

				auto_frontend_controller fc(tracer, mockups::Frontend::MakeFactory(state));

				// ACT
				call_record trace[] = {
					{	1, (void *)(0x31000 + 5)	},
						{	20, (void *)(0x37000 + 5)	},
						{	50, (void *)(0)	},
						{	51, (void *)(0x41000 + 5)	},
						{	70, (void *)(0)	},
						{	72, (void *)(0x37000 + 5)	},
						{	90, (void *)(0)	},
					{	120, (void *)(0)	},
					{	1000, (void *)(0x11000 + 5)	},
						{	1010, (void *)(0x13000 + 5)	},
						{	1100, (void *)(0)	},
					{	1400, (void *)(0)	},
					{	1420, (void *)(0x13000 + 5)	},
					{	1490, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace);

				state.updated.wait();

				// ASSERT
				const function_statistics_detailed &callinfo_parent1 = state.update_log[0][(void *)0x31000];
				const function_statistics_detailed &callinfo_parent2 = state.update_log[0][(void *)0x11000];
				const function_statistics_detailed &callinfo_parent3 = state.update_log[0][(void *)0x13000];

				Assert::AreEqual(2u, callinfo_parent1.callees.size());
				Assert::IsTrue(108 == callinfo_parent1.inclusive_time);
				Assert::IsTrue(8 == callinfo_parent1.exclusive_time);
				
				Assert::AreEqual(1u, callinfo_parent2.callees.size());
				Assert::IsTrue(389 == callinfo_parent2.inclusive_time);
				Assert::IsTrue(288 == callinfo_parent2.exclusive_time);

				Assert::AreEqual(0u, callinfo_parent3.callees.size());
				Assert::IsTrue(138 == callinfo_parent3.inclusive_time);
				Assert::IsTrue(138 == callinfo_parent3.exclusive_time);


				const function_statistics &callinfo_child11 = state.update_log[0][(void *)0x31000].callees[(void *)0x37000];
				const function_statistics &callinfo_child12 = state.update_log[0][(void *)0x31000].callees[(void *)0x41000];

				Assert::IsTrue(2 == callinfo_child11.times_called);
				Assert::IsTrue(26 == callinfo_child11.inclusive_time);
				Assert::IsTrue(26 == callinfo_child11.exclusive_time);

				Assert::IsTrue(1 == callinfo_child12.times_called);
				Assert::IsTrue(8 == callinfo_child12.inclusive_time);
				Assert::IsTrue(8 == callinfo_child12.exclusive_time);
			}
		};
	}
}
