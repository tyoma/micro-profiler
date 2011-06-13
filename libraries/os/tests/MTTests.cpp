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
			void do_nothing()
			{	}

			void threadid_capture(unsigned int *thread_id, unsigned int wait_ms)
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


			[TestMethod]
			void ThreadIdIsNonZero()
			{
				// INIT
				thread t(&do_nothing);

				// ACT / ASSERT
				Assert::IsTrue(0 != t.id());
			}


			[TestMethod]
			void ThreadIdEqualsOSIdValue()
			{
				// INIT
				unsigned int new_thread_id = ::GetCurrentThreadId();

				// ACT
				unsigned int id = thread(bind(&threadid_capture, &new_thread_id, 0)).id();

				// ASSERT
				Assert::IsTrue(new_thread_id == id);
			}


			[TestMethod]
			void ThreadIdsAreUnique()
			{
				// INIT / ACT
				thread t1(&do_nothing), t2(&do_nothing), t3(&do_nothing);

				// ACT / ASSERT
				Assert::IsTrue(t1.id() != t2.id());
				Assert::IsTrue(t2.id() != t3.id());
				Assert::IsTrue(t3.id() != t1.id());
			}


			[TestMethod]
			void InitializedRunReturnsValidThreads()
			{
				// INIT / ACT
				auto_ptr<thread> t1(thread::run(&do_nothing, &do_nothing));
				auto_ptr<thread> t2(thread::run(&do_nothing, &do_nothing));

				// ASSERT
				Assert::IsTrue(t1.get() != 0);
				Assert::IsTrue(t2.get() != 0);
				Assert::IsTrue(t2->id() != t1->id());
			}


			[TestMethod]
			void InitializedRunInitializerAndJobAreCalledFromNewThread()
			{
				// INIT
				unsigned int id_initializer1, id_job1, id_initializer2, id_job2;

				// ACT
				auto_ptr<thread> t1(thread::run(bind(&threadid_capture, &id_initializer1, 0), bind(&threadid_capture, &id_job1, 0)));
				auto_ptr<thread> t2(thread::run(bind(&threadid_capture, &id_initializer2, 0), bind(&threadid_capture, &id_job2, 0)));

				::Sleep(100);

				// ASSERT
				Assert::IsTrue(t1->id() == id_initializer1);
				Assert::IsTrue(t1->id() == id_job1);
				Assert::IsTrue(t2->id() == id_initializer2);
				Assert::IsTrue(t2->id() == id_job2);
			}


			[TestMethod]
			void EventFlagCreateRaised()
			{
				// INIT
				event_flag e(true, false);

				// ACT / ASSERT
				Assert::IsTrue(waitable::satisfied == e.wait(0));
				Assert::IsTrue(waitable::satisfied == e.wait(10000));
				Assert::IsTrue(waitable::satisfied == e.wait(waitable::infinite));
			}


			[TestMethod]
			void EventFlagCreateLowered()
			{
				// INIT
				event_flag e(false, false);

				// ACT / ASSERT
				Assert::IsTrue(waitable::timeout == e.wait(0));
				Assert::IsTrue(waitable::timeout == e.wait(200));
			}


			[TestMethod]
			void EventFlagCreateAutoResettable()
			{
				// INIT
				event_flag e(true, true);

				e.wait(100);

				// ACT / ASSERT
				Assert::IsTrue(waitable::timeout == e.wait(100));
			}


			[TestMethod]
			void RaisingEventFlag()
			{
				// INIT
				event_flag e(false, false);

				// ACT
				e.raise();

				// ACT / ASSERT
				Assert::IsTrue(waitable::satisfied == e.wait());
			}


			[TestMethod]
			void LoweringEventFlag()
			{
				// INIT
				event_flag e(true, false);

				// ACT
				e.lower();

				// ACT / ASSERT
				Assert::IsTrue(waitable::timeout == e.wait(0));
			}


			[TestMethod]
			void ThreadInitializerIsCalledSynchronuously()
			{
				// INIT
				unsigned int id_initializer1, id_initializer2;

				// ACT
				auto_ptr<thread> t1(thread::run(bind(&threadid_capture, &id_initializer1, 100), &do_nothing));
				auto_ptr<thread> t2(thread::run(bind(&threadid_capture, &id_initializer2, 100), &do_nothing));

				// ASSERT
				Assert::IsTrue(t1->id() == id_initializer1);
				Assert::IsTrue(t2->id() == id_initializer2);
			}
		};
	}
}
