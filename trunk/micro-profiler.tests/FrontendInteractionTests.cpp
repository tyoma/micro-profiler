#include "Helpers.h"

#include <entry.h>
#include <system.h>

#include "./../micro-profiler/_generated/microprofilerfrontend_i.h"

#include <wpl/mt/thread.h>
#include <wpl/mt/synchronization.h>
#include <atlbase.h>
#include <memory>
#include <vector>
#include <list>
#include <algorithm>

using namespace std;
using namespace wpl::mt;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		void sleep_20();
		void empty_call();
		void sleep_n(int n);
		void controlled_recursion(unsigned int level);
		void call_aa();
		void call_ab();
		void call_a();
		void call_ba();
		void call_b();

		void clear_collection_traces();

		namespace
		{
			unsigned int threadid;
			bool com_initialized;
			int fe_thread_priority;

			bool fe_released;
			CComBSTR fe_executable;
			hyper fe_load_address;
			hyper fe_ticks_resolution;
			event_flag fe_initialized(false, true);
			size_t fe_raise_updated_limit;
			vector<FunctionStatisticsDetailed> fe_update_statistics;
			list< vector<FunctionStatistics> > fe_children_update_statistics;
			unsigned fe_update_call_times;
			event_flag fe_stat_updated(false, true);
			hyper fe_stop_call;

			class FrontendMockup : IProfilerFrontend
			{
				unsigned _refcount;

			public:
				FrontendMockup()
					: _refcount(0)
				{	}

				~FrontendMockup()
				{	fe_released = true;	}

				STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
				{
					if (riid != IID_IUnknown && riid != __uuidof(IProfilerFrontend))
						return E_NOINTERFACE;
					*ppv = (IUnknown *)this;
					AddRef();
					return S_OK;
				}

				STDMETHODIMP_(ULONG) AddRef()
				{	return ++_refcount;	}

				STDMETHODIMP_(ULONG) Release()
				{
					if (!--_refcount)
						delete this;
					return 0;
				}

				STDMETHODIMP Initialize(BSTR executable, hyper load_address, hyper  ticks_resolution)
				{
					fe_executable = executable, fe_load_address = load_address, fe_ticks_resolution = ticks_resolution;
					fe_initialized.raise();
					return S_OK;
				}

				STDMETHODIMP UpdateStatistics(long count, FunctionStatisticsDetailed *statistics)
				{
					fe_update_statistics.insert(fe_update_statistics.end(), statistics, statistics + count);
					for (vector<FunctionStatisticsDetailed>::iterator i = fe_update_statistics.begin(); i != fe_update_statistics.begin(); ++i)
						if (i->ChildrenCount > 0)
						{
							fe_children_update_statistics.push_back(vector<FunctionStatistics>());
							
							vector<FunctionStatistics> &v = fe_children_update_statistics.back();

							v.assign(i->ChildrenStatistics, i->ChildrenStatistics + i->ChildrenCount);
							i->ChildrenStatistics = &v[0];
						}
					if (fe_raise_updated_limit && fe_update_statistics.size() >= fe_raise_updated_limit)
						fe_stat_updated.raise();
					while (count--)
						if (statistics++->Statistics.FunctionAddress == fe_stop_call)
							fe_stat_updated.raise();
					return S_OK;
				}
			};

			void factory1(IProfilerFrontend **)
			{
				threadid = ::GetCurrentThreadId();
				fe_thread_priority = ::GetThreadPriority(::GetCurrentThread());
			}

			void factory2(IProfilerFrontend **)
			{
				CComPtr<IUnknown> test;

				com_initialized = S_OK == test.CoCreateInstance(CComBSTR("JobObject")) && test;
			}

			void factory3(IProfilerFrontend **pf)
			{
				threadid = ::GetCurrentThreadId();
				(new FrontendMockup)->QueryInterface(__uuidof(IProfilerFrontend), (void **)pf);
			}

			class find_by_address
			{
				const void *_address;

			public:
				find_by_address(const void *address)
					: _address(address)
				{	}

				bool operator ()(const FunctionStatistics &s) const
				{	return reinterpret_cast<const void *>(s.FunctionAddress) == _address;	}

				bool operator ()(const FunctionStatisticsDetailed &s) const
				{	return reinterpret_cast<const void *>(s.Statistics.FunctionAddress) == _address;	}
			};
		}

		[TestClass]
		public ref class FrontendInteractionTests
		{
		public:
			[TestInitialize]
			void Init()
			{
				fe_raise_updated_limit = 0;
				fe_stop_call = 0;
				clear_collection_traces();
				fe_update_statistics.clear();
				fe_children_update_statistics.clear();
			}


			[TestMethod]
			void FactoryIsCalledInASeparateThread()
			{
				// INIT
				unsigned int test_threadid = ::GetCurrentThreadId();

				// ACT
				{	profiler_frontend fe(&factory1);	}

				// ASSERT
				Assert::IsTrue(0 != threadid);
				Assert::IsTrue(test_threadid != threadid);
				Assert::IsTrue(THREAD_PRIORITY_HIGHEST == fe_thread_priority);
			}


			[TestMethod]
			void FrontendThreadIsFinishedAfterDestroy()
			{
				// INIT / ACT
				DWORD status1 = 0, status2 = 0;
				auto_ptr<profiler_frontend> init(new profiler_frontend(&factory3));

				::Sleep(100);

				HANDLE hthread(::OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadid));

				// ASSERT (postponed)
				::GetExitCodeThread(hthread, &status1);

				// ACT
				init.reset();

				// ASSERT
				::GetExitCodeThread(hthread, &status2);
				::CloseHandle(hthread);

				Assert::IsTrue(STILL_ACTIVE == status1);
				Assert::IsTrue(0 == status2);
			}


			[TestMethod]
			void FrontendThreadHasCOMInitialized()	// Actually this will always pass when calling from managed, since COM already initialized
			{
				// INIT
				com_initialized = false;

				// INIT / ACT
				{	profiler_frontend fe(&factory2);	}

				// ASSERT
				Assert::IsTrue(com_initialized);
			}


			[TestMethod]
			void FrontendInterfaceReleasedAtPFDestroyed()
			{
				// INIT
				auto_ptr<profiler_frontend> fe(new profiler_frontend(&factory3));
				fe_released = false;

				// ACT
				fe.reset();

				// ASERT
				Assert::IsTrue(fe_released);
			}


			[TestMethod]
			void FrontendInitialized()
			{
				// INIT
				fe_executable = L"";
				fe_load_address = 0;
				TCHAR path[MAX_PATH + 1] = { 0 };

				// ACT
				auto_ptr<profiler_frontend> fe(new profiler_frontend(&factory3));
				fe_initialized.wait();

				// ASERT
				::GetModuleFileName(NULL, path, MAX_PATH + 1);
				void *exe_module = ::GetModuleHandle(NULL);
				hyper real_resolution = timestamp_precision();

				Assert::IsTrue(fe_executable == path);
				Assert::IsTrue(reinterpret_cast<hyper>(exe_module) == fe_load_address);
				Assert::IsTrue(90 * real_resolution / 100 < fe_ticks_resolution && fe_ticks_resolution < 110 * real_resolution / 100);
			}


			[TestMethod]
			void FrontendIsNotBotheredWithEmptyDataUpdates()
			{
				// INIT
				fe_update_call_times = 0;
				profiler_frontend fe(&factory3);

				// ACT / ASSERT
				Assert::IsTrue(waitable::timeout == fe_stat_updated.wait(500));

				// ASSERT
				Assert::IsTrue(fe_update_call_times == 0);
			}


			[TestMethod]
			void MakeACallAndWaitForDataPost()
			{
				// INIT
				profiler_frontend fe(&factory3);
				FunctionStatistics sleep_20_call, sleep_n_call;

				fe_raise_updated_limit = 2;
				fe_update_statistics.clear();
				fe_initialized.wait();

				// ACT
				sleep_20();
				fe_stat_updated.wait();

				// ASERT
				Assert::AreEqual(2u, fe_update_statistics.size());

				sleep_20_call = fe_update_statistics[1].Statistics;

				Assert::IsTrue(sleep_20_call.FunctionAddress == reinterpret_cast<hyper>(&sleep_20));
				Assert::IsTrue(sleep_20_call.TimesCalled == 1);
				Assert::IsTrue(sleep_20_call.MaxReentrance == 0);
				Assert::IsTrue(sleep_20_call.InclusiveTime > 0);
				Assert::IsTrue(sleep_20_call.ExclusiveTime == sleep_20_call.InclusiveTime);
				Assert::IsTrue(sleep_20_call.MaxCallTime == sleep_20_call.InclusiveTime);

				// INIT
				fe_update_statistics.clear();

				// ACT
				sleep_n(200);
				fe_stat_updated.wait(20);	// such a timeout MUST be sufficient enough

				// ASERT
				Assert::IsTrue(2 == fe_update_statistics.size());

				sleep_n_call = fe_update_statistics[1].Statistics;

				Assert::IsTrue(sleep_n_call.FunctionAddress == reinterpret_cast<hyper>(&sleep_n));
				Assert::IsTrue(sleep_n_call.TimesCalled == 1);
				Assert::IsTrue(sleep_n_call.MaxReentrance == 0);
				Assert::IsTrue(sleep_n_call.InclusiveTime > sleep_20_call.InclusiveTime);
				Assert::IsTrue(sleep_n_call.ExclusiveTime == sleep_n_call.InclusiveTime);
				Assert::IsTrue(sleep_n_call.MaxCallTime == sleep_n_call.InclusiveTime);
			}


			[TestMethod]
			void PassReentranceCountToFrontend()
			{
				// INIT
				fe_raise_updated_limit = 1;

				// ACT
				controlled_recursion(5);
				controlled_recursion(7);
				profiler_frontend fe(&factory3);
				fe_stat_updated.wait(20);	// such a timeout MUST be sufficient enough

				// ASERT
				Assert::IsTrue(1 == fe_update_statistics.size());

				FunctionStatistics recursive_call = fe_update_statistics[0].Statistics;

				Assert::IsTrue(recursive_call.FunctionAddress == reinterpret_cast<hyper>(&controlled_recursion));
				Assert::IsTrue(recursive_call.TimesCalled == 12);
				Assert::IsTrue(recursive_call.MaxReentrance == 6);
			}
			

			[TestMethod]
			void PerformanceDataTakesProfilerLatencyIntoAccount()
			{
				// INIT
				int check_amount = 90000;

				fe_stop_call = reinterpret_cast<hyper>(&sleep_20);

				// ACT
				for (int i = 0; i < check_amount; ++i)
					empty_call();
				sleep_20();
				profiler_frontend fe(&factory3);
				fe_stat_updated.wait();

				// ASERT
				Assert::IsTrue(2 == fe_update_statistics.size());

				FunctionStatistics stat = fe_update_statistics[0].Statistics.FunctionAddress == reinterpret_cast<hyper>(&empty_call) ? fe_update_statistics[0].Statistics : fe_update_statistics[1].Statistics;

				Assert::IsTrue(stat.FunctionAddress == reinterpret_cast<hyper>(&empty_call));
				Assert::IsTrue(stat.TimesCalled == check_amount);
				Assert::IsTrue(stat.InclusiveTime > 0);
				Assert::IsTrue(stat.InclusiveTime / stat.TimesCalled < 150);
				Assert::IsTrue(stat.ExclusiveTime == stat.InclusiveTime);
				Assert::IsTrue(stat.MaxCallTime > stat.InclusiveTime / check_amount);
				Assert::IsTrue(stat.MaxCallTime < stat.InclusiveTime);
			}


			[TestMethod]
			void ChildrenStatisticsIsPassedAlongWithTopLevels()
			{
				// INIT
				fe_stop_call = reinterpret_cast<hyper>(&sleep_20);

				// ACT
				call_a();
				call_b();
				sleep_20();

				profiler_frontend fe(&factory3);
				fe_stat_updated.wait();

				// ASSERT
				typedef vector<FunctionStatisticsDetailed>::const_iterator iterator_detailed;
				typedef const FunctionStatistics *iterator;
				
				iterator_detailed i1 = find_if(fe_update_statistics.begin(), fe_update_statistics.end(), find_by_address(&call_a));
				iterator_detailed i2 = find_if(fe_update_statistics.begin(), fe_update_statistics.end(), find_by_address(&call_b));
				iterator_detailed i3 = find_if(fe_update_statistics.begin(), fe_update_statistics.end(), find_by_address(&sleep_20));

				Assert::IsTrue(i1 != fe_update_statistics.end());
				Assert::IsTrue(i2 != fe_update_statistics.end());
				Assert::IsTrue(i3 != fe_update_statistics.end());

				Assert::IsTrue(2 == i1->ChildrenCount);
				Assert::IsTrue(1 == i2->ChildrenCount);
				Assert::IsTrue(0 == i3->ChildrenCount);

				iterator i11 = find_if(i1->ChildrenStatistics, i1->ChildrenStatistics + i1->ChildrenCount, find_by_address(&call_aa));
				iterator i12 = find_if(i1->ChildrenStatistics, i1->ChildrenStatistics + i1->ChildrenCount, find_by_address(&call_ab));
				iterator i21 = find_if(i2->ChildrenStatistics, i2->ChildrenStatistics + i2->ChildrenCount, find_by_address(&call_ba));

				Assert::IsTrue(i11 != i1->ChildrenStatistics + i1->ChildrenCount);
				Assert::IsTrue(i12 != i1->ChildrenStatistics + i1->ChildrenCount);
				Assert::IsTrue(i21 != i2->ChildrenStatistics + i2->ChildrenCount);

				Assert::IsTrue(2 == i11->TimesCalled);
				Assert::IsTrue(1 == i12->TimesCalled);
				Assert::IsTrue(1 == i21->TimesCalled);
			}
		};
	}
}
