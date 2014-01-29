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
			void CheckCOMInitialized(event_flag *e, bool *com_initialized)
			{
				CComPtr<IUnknown> test;

				*com_initialized = S_OK == test.CoCreateInstance(CComBSTR("JobObject")) && test;
				e->raise();
			}

			void RaiseAt(event_flag *e, volatile long *times_to_event)
			{
				if (0 == _InterlockedDecrement(times_to_event))
					e->raise();
			}

			void ValidateThread(shared_ptr<running_thread> *hthread, event_flag *second_initialized, bool *first_finished)
			{
				if (*hthread)
				{
					*first_finished = !(*hthread)->is_running();
					second_initialized->raise();
				}
				*hthread = this_thread::open();
			}

			void LogThread1(shared_ptr<running_thread> *hthread, event_flag *initialized)
			{
				*hthread = this_thread::open();
				initialized->raise();
			}

			void LogThreadN(vector< shared_ptr<running_thread> > *log, volatile long *times_to_initialized,
				event_flag *initialized)
			{
				log->push_back(this_thread::open());
				RaiseAt(initialized, times_to_initialized);
			}

			void ControlInitialization(event_flag *proceed, vector< shared_ptr<running_thread> > *log,
				volatile long *times_to_initialized, event_flag *initialized)
			{
				proceed->wait();
				LogThreadN(log, times_to_initialized, initialized);
			}

			handle *profile_this(frontend_controller &fc)
			{	return fc.profile(&RaiseAt);	}

			class sync_stop_frontend_controller : public frontend_controller
			{
			public:
				sync_stop_frontend_controller(calls_collector_i &collector, mockups::FrontendState &state)
					: frontend_controller(collector, state.MakeFactory())
				{	}

				~sync_stop_frontend_controller()
				{	force_stop();	}
			};

			class auto_frontend_controller : public sync_stop_frontend_controller
			{
			public:
				auto_frontend_controller(calls_collector_i &collector, mockups::FrontendState &state)
					: sync_stop_frontend_controller(collector, state), _profiler_handle(profile(&RaiseAt))
				{	}

			private:
				const auto_ptr<handle> _profiler_handle;
			};

			class scoped_thread_join
			{
				shared_ptr<running_thread> &_thread;

			public:
				scoped_thread_join(shared_ptr<running_thread> &thread)
					: _thread(thread)
				{	}

				~scoped_thread_join()
				{	_thread->join();	}
			};
		}

		[TestClass]
		[DeploymentItem("symbol_container_1.dll")]
		[DeploymentItem("symbol_container_2.dll")]
		public ref class FrontendControllerTests
		{
		public:
			[TestMethod]
			void FactoryIsNotCalledIfCollectionHandleWasNotObtained()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&event_flag::raise, &initialized));

				// ACT
				{	frontend_controller fc(tracer, state.MakeFactory());	}

				// ASSERT
				Assert::IsTrue(waitable::timeout == initialized.wait(0));
			}


			[TestMethod]
			void ForcedStopIsAllowedOnStartedController()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				frontend_controller fc(tracer, state.MakeFactory());

				// ACT / ASSERT (must not throw)
				fc.force_stop();
			}


			[TestMethod]
			void NonNullHandleObtained()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				sync_stop_frontend_controller fc(tracer, state);

				// ACT
				auto_ptr<handle> h(profile_this(fc));
				
				// ASSERT
				Assert::IsTrue(NULL != h.get());
			}


			[TestMethod]
			void FrontendIsCreatedInASeparateThreadWhenProfilerHandleObtained()
			{
				// INIT
				mockups::Tracer tracer;
				shared_ptr<running_thread> hthread;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&LogThread1, &hthread, &initialized));
				scoped_thread_join stj(hthread);
				frontend_controller fc(tracer, state.MakeFactory());

				// ACT
				auto_ptr<handle> h(profile_this(fc));

				// ASSERT
				initialized.wait();

				Assert::IsTrue(!!hthread);
				Assert::IsTrue(this_thread::get_id() != hthread->get_id());
			}


			[TestMethod]
			void TwoCoexistingFrontendsHasDifferentWorkerThreads()
			{
				// INIT
				mockups::Tracer tracer1, tracer2;
				shared_ptr<running_thread> hthread1, hthread2;
				event_flag initialized1(false, true), initialized2(false, true);
				mockups::FrontendState state1(bind(&LogThread1, &hthread1, &initialized1)),
					state2(bind(&LogThread1, &hthread2, &initialized2));
				auto_frontend_controller fc1(tracer1, state1);
				auto_frontend_controller fc2(tracer2, state2);

				// ACT
				initialized1.wait();
				initialized2.wait();

				// ASSERT
				Assert::IsTrue(hthread1->get_id() != hthread2->get_id());
			}


			[TestMethod]
			void FrontendStopsImmediatelyAtForceStopRequest()
			{
				// INIT
				mockups::Tracer tracer(11);
				shared_ptr<running_thread> hthread;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&LogThread1, &hthread, &initialized));
				frontend_controller fc(tracer, state.MakeFactory());
				auto_ptr<handle> h(profile_this(fc));

				initialized.wait();

				// ACT
				fc.force_stop();

				// ASSERT
				Assert::IsFalse(hthread->is_running());
			}


			[TestMethod]
			void FrontendIsRecreatedOnRepeatedHandleObtaining()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				volatile long counter = 2;
				mockups::FrontendState state(bind(&RaiseAt, &initialized, &counter));
				sync_stop_frontend_controller fc(tracer, state);
				auto_ptr<handle> h;

				// ACT
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));

				// ASSERT
				initialized.wait();

				// INIT
				counter = 3;

				// ACT
				h.reset();
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));

				// ASSERT
				initialized.wait();
			}


			[TestMethod]
			void FrontendCreationIsRefCounted2()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				volatile long counter = 1;
				mockups::FrontendState state(bind(&RaiseAt, &initialized, &counter));
				frontend_controller fc(tracer, state.MakeFactory());

				// ACT
				auto_ptr<handle> h1(profile_this(fc));
				auto_ptr<handle> h2(profile_this(fc));

				initialized.wait();

				h1.reset();
				h2.reset();
				fc.force_stop();

				// ASSERT
				Assert::IsTrue(0 == counter);
			}


			[TestMethod]
			void FrontendCreationIsRefCounted3()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				volatile long counter = 1;
				mockups::FrontendState state(bind(&RaiseAt, &initialized, &counter));
				frontend_controller fc(tracer, state.MakeFactory());

				// ACT
				auto_ptr<handle> h1(profile_this(fc));
				auto_ptr<handle> h2(profile_this(fc));
				auto_ptr<handle> h3(profile_this(fc));

				initialized.wait();

				h3.reset();
				h2.reset();
				h1.reset();
				fc.force_stop();

				// ASSERT
				Assert::IsTrue(0 == counter);
			}


			[TestMethod]
			void FirstThreadIsDeadByTheTimeOfTheSecondInitialization()
			{
				// INIT
				mockups::Tracer tracer;
				shared_ptr<running_thread> hthread;
				event_flag second_initialized(false, true);
				bool first_finished = false;
				mockups::FrontendState state(bind(&ValidateThread, &hthread, &second_initialized, &first_finished));
				scoped_thread_join stj(hthread);
				frontend_controller fc(tracer, state.MakeFactory());
				auto_ptr<handle> h;

				// ACT
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));
				second_initialized.wait();

				// ASSERT
				Assert::IsTrue(first_finished);
			}


			[TestMethod]
			void AllWorkerThreadsMustExitOnHandlesClosure()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag proceed(false, false), finished(false, true);
				vector< shared_ptr<running_thread> > htread_log;
				volatile long times = 2;
				mockups::FrontendState state(bind(&ControlInitialization, &proceed, &htread_log, &times, &finished));
				frontend_controller fc(tracer, state.MakeFactory());
				auto_ptr<handle> h;

				// ACT
				h.reset(profile_this(fc));
				h.reset();
				h.reset(profile_this(fc));
				h.reset();
				proceed.raise();
				finished.wait();	// wait for the second thread to initialize the frontend

				// ASSERT (must not hang)
				htread_log[1]->join();
			}


			[TestMethod]
			void ProfilerHandleReleaseIsNonBlockingAndFrontendThreadIsFinishedEventually()
			{
				// INIT
				mockups::Tracer tracer;
				shared_ptr<running_thread> hthread;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&LogThread1, &hthread, &initialized));
				scoped_thread_join stj(hthread);
				frontend_controller fc(tracer, state.MakeFactory());
				auto_ptr<handle> h(profile_this(fc));

				initialized.wait();

				// ACT (must not hang)
				hthread->suspend();
				h.reset();
				hthread->resume();

				// ASSERT (must not hang)
				hthread->join();
			}


			[TestMethod]
			void FrontendThreadHasCOMInitialized()	// Actually this will always pass when calling from managed, since COM already initialized
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				bool com_initialized = false;
				mockups::FrontendState state(bind(&CheckCOMInitialized, &initialized, &com_initialized));
				auto_frontend_controller fc(tracer, state);

				// INIT / ACT
				initialized.wait();

				// ASSERT
				Assert::IsTrue(com_initialized);
			}


			[TestMethod]
			void FrontendInterfaceReleasedAtPFDestroyed()
			{
				// INIT
				mockups::Tracer tracer;
				shared_ptr<running_thread> hthread;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&LogThread1, &hthread, &initialized));
				frontend_controller fc(tracer, state.MakeFactory());
				auto_ptr<handle> h(profile_this(fc));

				// ACT
				initialized.wait();
				h.reset();
				hthread->join();

				// ASERT
				Assert::IsTrue(state.released);
			}


			[TestMethod]
			void FrontendInitializedWithProcessIdAndTicksResolution()
			{
				// INIT
				mockups::Tracer tracer;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&event_flag::raise, &initialized));
				auto_frontend_controller fc(tracer, state);

				// ACT
				initialized.wait();

				// ASERT
				hyper real_resolution = timestamp_precision();

				Assert::IsTrue(static_cast<long>(::GetCurrentProcessId()) == state.process_id);
				Assert::IsTrue(90 * real_resolution / 100 < state.ticks_resolution && state.ticks_resolution < 110 * real_resolution / 100);
			}


			[TestMethod]
			void FrontendIsNotBotheredWithEmptyDataUpdates()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				auto_frontend_controller fc(tracer, state);

				// ACT / ASSERT
				Assert::IsTrue(waitable::timeout == state.updated.wait(500));
			}


			[TestMethod]
			void ImageLoadEventArrivesAtHandleObtaining()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				image images[] = { image(_T("symbol_container_1.dll")), image(_T("symbol_container_2.dll")), };
				sync_stop_frontend_controller fc(tracer, state);

				// ACT
				auto_ptr<handle> h1(fc.profile(images[0].get_symbol_address("get_function_addresses_1")));
				state.modules_state_updated.wait();

				// ASSERT
				Assert::AreEqual(1u, state.update_log.size());
				Assert::AreEqual(1u, state.update_log[0].image_loads.size());
				Assert::AreEqual(reinterpret_cast<uintptr_t>(images[0].load_address()),
					state.update_log[0].image_loads[0].first);
				Assert::AreNotEqual(wstring::npos, state.update_log[0].image_loads[0].second.find(L"SYMBOL_CONTAINER_1.DLL"));

				// ACT
				auto_ptr<handle> h2(fc.profile(images[1].get_symbol_address("get_function_addresses_2")));
				state.modules_state_updated.wait();

				// ASSERT
				Assert::AreEqual(2u, state.update_log.size());
				Assert::AreEqual(1u, state.update_log[1].image_loads.size());
				Assert::AreEqual(reinterpret_cast<uintptr_t>(images[1].load_address()),
					state.update_log[1].image_loads[0].first);
				Assert::AreNotEqual(wstring::npos, state.update_log[1].image_loads[0].second.find(L"SYMBOL_CONTAINER_2.DLL"));
			}


			[TestMethod]
			void ImageUnloadEventArrivesAtHandleRelease()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				image images[] = { image(_T("symbol_container_1.dll")), image(_T("symbol_container_2.dll")), };
				sync_stop_frontend_controller fc(tracer, state);

				auto_ptr<handle> h1(fc.profile(images[0].get_symbol_address("get_function_addresses_1")));
				state.modules_state_updated.wait();
				auto_ptr<handle> h2(fc.profile(images[1].get_symbol_address("get_function_addresses_2")));
				state.modules_state_updated.wait();

				// ACT
				h1.reset();
				state.modules_state_updated.wait();

				// ASSERT
				Assert::AreEqual(3u, state.update_log.size());
				Assert::AreEqual(1u, state.update_log[2].image_unloads.size());
				Assert::AreEqual(reinterpret_cast<uintptr_t>(images[0].load_address()),
					state.update_log[2].image_unloads[0]);

				// ACT
				h2.reset();
				state.modules_state_updated.wait();

				// ASSERT
				Assert::AreEqual(4u, state.update_log.size());
				Assert::AreEqual(1u, state.update_log[3].image_unloads.size());
				Assert::AreEqual(reinterpret_cast<uintptr_t>(images[1].load_address()),
					state.update_log[3].image_unloads[0]);
			}


			[TestMethod]
			void LastBatchIsReportedToFrontend()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				sync_stop_frontend_controller fc(tracer, state);
				auto_ptr<handle> h(profile_this(fc));
				call_record trace1[] = { { 0, (void *)(0x1223 + 5) }, { 1000, (void *)(0) }, };
				call_record trace2[] = { { 0, (void *)(0x12230 + 5) }, { 1000, (void *)(0) }, };

				state.modules_state_updated.wait();

				// ACT
				state.update_lock.lower();	// suspend on before exiting next update
				tracer.Add(thread::id(), trace1);
				state.updated.wait();
				h.reset();
				tracer.Add(thread::id(), trace2);
				state.update_lock.raise();	// resume
				state.modules_state_updated.wait();

				// ASSERT
				Assert::AreEqual(4u, state.update_log.size());
				Assert::AreEqual(1u, state.update_log[0].image_loads.size());
				Assert::IsTrue(state.update_log[1].update.end() != state.update_log[1].update.find((void *)0x1223));
				Assert::IsTrue(state.update_log[2].update.end() != state.update_log[2].update.find((void *)0x12230));
				Assert::AreEqual(1u, state.update_log[3].image_unloads.size());
			}


			[TestMethod]
			void MakeACallAndWaitForDataPost()
			{
				// INIT
				mockups::Tracer tracer;
				mockups::FrontendState state;
				auto_frontend_controller fc(tracer, state);

				state.modules_state_updated.wait();
				state.update_log.clear();

				// ACT
				call_record trace1[] = {
					{	0, (void *)(0x1223 + 5)	},
					{	1000, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace1);

				state.updated.wait();

				// ASERT
				Assert::AreEqual(1u, state.update_log.size());

				statistics_map_detailed::const_iterator callinfo_1 = state.update_log[0].update.find((void *)0x1223);

				Assert::IsTrue(state.update_log[0].update.end() != callinfo_1);

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

				Assert::AreEqual(1u, state.update_log[1].update.size());	// The new batch MUST NOT not contain previous function.

				statistics_map_detailed::const_iterator callinfo_2 = state.update_log[1].update.find((void *)0x31223);

				Assert::IsTrue(state.update_log[1].update.end() != callinfo_2);

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
				mockups::FrontendState state(bind(&RaiseAt, &initialized, &times));
				sync_stop_frontend_controller fc(tracer, state);
				auto_ptr<handle> h(profile_this(fc));
				call_record trace[] = {
					{	10000, (void *)(0x31223 + 5)	},
					{	14000, (void *)(0)	},
				};

				h.reset();
				h.reset(profile_this(fc));
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
				mockups::FrontendState state;
				auto_frontend_controller fc(tracer, state);

				state.modules_state_updated.wait();
				state.update_log.clear();

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

				const function_statistics_detailed callinfo_1 = state.update_log[0].update[(void *)0x31000];

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

				const function_statistics_detailed &callinfo_2 = state.update_log[1].update[(void *)0x31000];

				Assert::IsTrue(5 == callinfo_2.times_called);
				Assert::IsTrue(4 == callinfo_2.max_reentrance);
			}
			

			[TestMethod]
			void PerformanceDataTakesProfilerLatencyIntoAccount()
			{
				// INIT
				mockups::Tracer tracer1(13), tracer2(29);
				mockups::FrontendState state1, state2;

				auto_frontend_controller fe1(tracer1, state1);
				auto_frontend_controller fe2(tracer2, state2);

				state1.modules_state_updated.wait();
				state1.update_log.clear();
				state2.modules_state_updated.wait();
				state2.update_log.clear();

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
				Assert::IsTrue(4000 - 13 == state1.update_log[0].update[(void *)0x3171717].inclusive_time);
				Assert::IsTrue(4000 - 29 == state2.update_log[0].update[(void *)0x3171717].inclusive_time);
			}


			[TestMethod]
			void ChildrenStatisticsIsPassedAlongWithTopLevels()
			{
				// INIT
				mockups::Tracer tracer(11);
				mockups::FrontendState state;
				auto_frontend_controller fc(tracer, state);

				state.modules_state_updated.wait();
				state.update_log.clear();

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
				const function_statistics_detailed &callinfo_parent1 = state.update_log[0].update[(void *)0x31000];
				const function_statistics_detailed &callinfo_parent2 = state.update_log[0].update[(void *)0x11000];
				const function_statistics_detailed &callinfo_parent3 = state.update_log[0].update[(void *)0x13000];

				Assert::AreEqual(2u, callinfo_parent1.callees.size());
				Assert::IsTrue(108 == callinfo_parent1.inclusive_time);
				Assert::IsTrue(8 == callinfo_parent1.exclusive_time);
				
				Assert::AreEqual(1u, callinfo_parent2.callees.size());
				Assert::IsTrue(389 == callinfo_parent2.inclusive_time);
				Assert::IsTrue(288 == callinfo_parent2.exclusive_time);

				Assert::AreEqual(0u, callinfo_parent3.callees.size());
				Assert::IsTrue(138 == callinfo_parent3.inclusive_time);
				Assert::IsTrue(138 == callinfo_parent3.exclusive_time);


				const function_statistics &callinfo_child11 = state.update_log[0].update[(void *)0x31000].callees[(void *)0x37000];
				const function_statistics &callinfo_child12 = state.update_log[0].update[(void *)0x31000].callees[(void *)0x41000];

				Assert::IsTrue(2 == callinfo_child11.times_called);
				Assert::IsTrue(26 == callinfo_child11.inclusive_time);
				Assert::IsTrue(26 == callinfo_child11.exclusive_time);

				Assert::IsTrue(1 == callinfo_child12.times_called);
				Assert::IsTrue(8 == callinfo_child12.inclusive_time);
				Assert::IsTrue(8 == callinfo_child12.exclusive_time);
			}


			[TestMethod]
			void FrontendContinuesAfterControllerDestructionUntilAfterHandleIsReleased()
			{
				// INIT
				mockups::Tracer tracer(11);
				shared_ptr<running_thread> hthread;
				event_flag initialized(false, true);
				mockups::FrontendState state(bind(&LogThread1, &hthread, &initialized));
				scoped_thread_join stj(hthread);
				auto_ptr<frontend_controller> fc(new frontend_controller(tracer, state.MakeFactory()));
				auto_ptr<handle> h(profile_this(*fc));
				call_record trace[] = {
					{	10000, (void *)(0x31223 + 5)	},
					{	14000, (void *)(0)	},
				};

				tracer.Add(thread::id(), trace);
				state.updated.wait();

				// ACT / ASSERT (must not hang)
				fc.reset();

				// ACT
				tracer.Add(thread::id(), trace);
				
				// ASSERT
				state.updated.wait();

				// ACT (duplicated intentionally - flush-at-exit events may count for the first wait)
				tracer.Add(thread::id(), trace);

				// ASSERT (must not hang)
				state.updated.wait();
			}
		};
	}
}
