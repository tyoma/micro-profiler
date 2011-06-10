#include <mt.h>

#include <windows.h>
#include <functional>

namespace std
{
	using tr1::bind;
}

using namespace std;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace os
{
	namespace tests
	{
		namespace
		{
			static void threadid_capture(unsigned int *thread_id, unsigned int wait_ms)
			{
				if (wait_ms > 0)
					::Sleep(wait_ms);
				*thread_id = ::GetCurrentThreadId();
			}
		}

		[TestClass]
		public ref class MTTests
		{
		public:
			[TestMethod]
			void ThreadCtorStartsNewThread()
			{
				// INIT
				unsigned int new_thread_id;
				{

					// ACT
					thread t(bind(&threadid_capture, &new_thread_id, 0));
				}

				// ASSERT
				Assert::AreNotEqual(::GetCurrentThreadId(), new_thread_id);
			}


			[TestMethod]
			void ThreadDtorWaitsForExecution()
			{
				// INIT
				unsigned int new_thread_id = ::GetCurrentThreadId();
				{

					// ACT
					thread t(bind(&threadid_capture, &new_thread_id, 100));

					// ASSERT
					Assert::AreEqual(::GetCurrentThreadId(), new_thread_id);

					// ACT
				}

				// ASSERT
				Assert::AreNotEqual(::GetCurrentThreadId(), new_thread_id);
			}
		};
	}
}
