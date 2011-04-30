#include "Helpers.h"

#include <entry.h>

#include "./../micro-profiler-frontend/_generated/microprofilerfrontend_i.h"

#include <atlbase.h>
#include <memory>

using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			unsigned int threadid;
			bool com_initialized;
			
			bool fe_released;
			CComBSTR fe_executable;
			hyper fe_load_address;
			waitable fe_initialized;

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

			  STDMETHODIMP Initialize(BSTR executable, hyper load_address)
			  {
				  fe_executable = executable, fe_load_address = load_address;
				  fe_initialized.set();
				  return S_OK;
			  }

			  STDMETHODIMP UpdateStatistics(long count, FunctionStatistics *statistics)
			  {
				  return E_NOTIMPL;
			  }
			};

			void factory1(IProfilerFrontend **)
			{
				threadid = ::GetCurrentThreadId();
			}

			void factory2(IProfilerFrontend **)
			{
				CComPtr<IUnknown> test;

				com_initialized = S_OK == test.CoCreateInstance(CComBSTR("JobObject")) && test;
			}

			void factory3(IProfilerFrontend **pf)
			{
				(new FrontendMockup)->QueryInterface(__uuidof(IProfilerFrontend), (void **)pf);
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
				{	profiler_frontend fe(&factory1);	}

				// ASSERT
				Assert::IsTrue(0 != threadid);
				Assert::IsTrue(test_threadid != threadid);
			}


			[TestMethod]
			void FrontendThreadIsFinishedAfterDestroy()
			{
				// INIT / ACT
				DWORD status1 = 0, status2 = 0;
				auto_ptr<profiler_frontend> init(new profiler_frontend(&factory1));
				
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

				Assert::IsTrue(fe_executable == path);
				Assert::IsTrue(reinterpret_cast<hyper>(exe_module) == fe_load_address);
			}
		};
	}
}