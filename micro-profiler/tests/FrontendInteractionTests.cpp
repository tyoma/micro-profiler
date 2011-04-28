#include <entry.h>

#include "./../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"

#include <atlbase.h>

using namespace micro_profiler;
using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace tests
{
	namespace
	{
		unsigned int threadid;
		bool com_initialized;

		void factory1(IProfilerSink **)
		{
			threadid = ::GetCurrentThreadId();
		}

		void factory2(IProfilerSink **)
		{
			CComPtr<IUnknown> test;

			com_initialized = S_OK == test.CoCreateInstance(CComBSTR("JobObject")) && test;
		}
	}

	[TestClass]
	public ref class FrontendInteractionTests
	{
	public: 
		[TestMethod]
		void FactoryIsCalledInASeparateThread()
		{
			// INIT
			unsigned int test_threadid = ::GetCurrentThreadId();
			
			// ACT
			initialize_frontend(&factory1);

			// ASSERT
			Assert::IsTrue(0 != threadid);
			Assert::IsTrue(test_threadid != threadid);
		}


		[TestMethod]
		void FrontendThreadIsFinishedAfterDestroy()
		{
			// INIT / ACT
			DWORD status1 = 0, status2 = 0;
			auto_ptr<destructible> init(initialize_frontend(&factory1));
			
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
			initialize_frontend(&factory2);

			// ASSERT
			Assert::IsTrue(com_initialized);
		}
	};
}
