#include <mt/thread.h>
#include <mt/tls.h>

#include <memory>
#include <ut/assert.h>
#include <ut/test.h>
#include <windows.h>

using namespace std;

namespace mt
{
	namespace tests
	{
		namespace
		{
			void do_nothing()
			{	}

			void threadid_capture(thread::id *thread_id, unsigned int wait_ms)
			{
				if (wait_ms > 0)
					this_thread::sleep_for(milliseconds(wait_ms));
				*thread_id = this_thread::get_id();
			}

			void thread_handle_capture_and_wait(shared_ptr<void> &hthread, shared_ptr<void> ready, shared_ptr<void> go)
			{
				HANDLE h;

				::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &h, 0, FALSE,
					DUPLICATE_SAME_ACCESS);
				hthread.reset(h, &::CloseHandle);
				::SetEvent(ready.get());
				::WaitForSingleObject(go.get(), INFINITE);
			}

			template <typename T>
			void get_from_tls(const tls<T> *ptls, T **result)
			{	*result = ptls->get();	}

			template <typename T>
			void set_get_tls(tls<T> *ptls, T *value, T **result)
			{
				ptls->set(value);
				*result = ptls->get();
			}
		}

		begin_test_suite( ThreadTests )
			test( ThreadCtorStartsNewThread )
			{
				// INIT
				thread::id new_thread_id;
				{

					// ACT
					thread t(bind(&threadid_capture, &new_thread_id, 0));
				}

				// ASSERT
				assert_not_equal(this_thread::get_id(), new_thread_id);
			}


			test( ThreadDtorWaitsForExecution )
			{
				// INIT
				thread::id new_thread_id = this_thread::get_id();
				{

					// ACT
					thread t(bind(&threadid_capture, &new_thread_id, 100));

					// ASSERT
					assert_equal(this_thread::get_id(), new_thread_id);

					// ACT
				}

				// ASSERT
				assert_not_equal(this_thread::get_id(), new_thread_id);
			}


			test( ThreadIdIsNonZero )
			{
				// INIT
				thread t(&do_nothing);

				// ACT / ASSERT
				assert_not_equal(mt::thread::id(), t.get_id());
			}


			test( ThreadIdEqualsOSIdValue )
			{
				// INIT
				thread::id new_thread_id = this_thread::get_id();

				// ACT
				thread::id id = thread(bind(&threadid_capture, &new_thread_id, 0)).get_id();

				// ASSERT
				assert_equal(new_thread_id, id);
			}


			test( ThreadIdsAreUnique )
			{
				// INIT / ACT
				thread t1(&do_nothing), t2(&do_nothing), t3(&do_nothing);

				// ACT / ASSERT
				assert_not_equal(t1.get_id(), t2.get_id());
				assert_not_equal(t2.get_id(), t3.get_id());
				assert_not_equal(t3.get_id(), t1.get_id());
			}


			test( DetachedThreadContinuesExecution )
			{
				// INIT
				shared_ptr<void> hthread;
				shared_ptr<void> ready(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle);
				shared_ptr<void> go(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle);
				auto_ptr<thread> t(new thread(bind(&thread_handle_capture_and_wait, ref(hthread), ready, go)));
				DWORD exit_code = 0;

				::WaitForSingleObject(ready.get(), INFINITE);

				// ACT
				t->detach();
				t.reset();

				// ASSERT
				::GetExitCodeThread(hthread.get(), &exit_code);

				assert_equal(STILL_ACTIVE, exit_code);

				// ACT
				::SetEvent(go.get());
				::WaitForSingleObject(hthread.get(), INFINITE);

				// ASSERT
				::GetExitCodeThread(hthread.get(), &exit_code);

				assert_equal(0u, exit_code);
			}


			test( JoinThreadGuaranteesItsCompletion )
			{
				// INIT
				shared_ptr<void> hthread;
				shared_ptr<void> ready(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle);
				shared_ptr<void> go(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle);
				auto_ptr<thread> t(new thread(bind(&thread_handle_capture_and_wait, ref(hthread), ready, go)));
				DWORD exit_code = STILL_ACTIVE;

				::WaitForSingleObject(ready.get(), INFINITE);
				::SetEvent(go.get());

				// ACT
				t->join();

				// ASSERT
				::GetExitCodeThread(hthread.get(), &exit_code);

				assert_equal(0u, exit_code);
			}


			test( TlsValueIsNullAfterInitialization )
			{
				// INIT / ACT
				tls<int> tls_int;
				tls<double> tls_dbl;

				// ACT / ASSERT
				assert_null(tls_int.get());
				assert_null(tls_dbl.get());
			}


			test( TlsReturnsSameObjectInHostThread )
			{
				// INIT
				int a = 123;
				double b = 12.3;

				// ACT
				tls<int> tls_int;
				tls<double> tls_dbl;

				tls_int.set(&a);
				tls_dbl.set(&b);

				int *ptr_a = tls_int.get();
				double *ptr_b = tls_dbl.get();

				// ASSERT
				assert_equal(ptr_a, &a);
				assert_equal(ptr_b, &b);
			}


			test( TlsReturnsNullWhenValueGotFromAnotherThread )
			{
				// INIT
				tls<int> tls_int;
				tls<double> tls_dbl;
				int a = 123;
				int *ptr_a = &a;
				double b = 12.3;
				double *ptr_b = &b;

				tls_int.set(&a);
				tls_dbl.set(&b);

				// ACT
				thread(bind(&get_from_tls<int>, &tls_int, &ptr_a));
				thread(bind(&get_from_tls<double>, &tls_dbl, &ptr_b));

				// ASSERT
				assert_null(ptr_a);
				assert_null(ptr_b);
			}


			test( TlsReturnsValueSetInSpecificThread )
			{
				// INIT
				tls<int> tls_int;
				tls<double> tls_dbl;
				int a = 123;
				int a2 = 234;
				int *ptr_a = &a;
				double b = 12.3;
				double b2 = 23.4;
				double *ptr_b = &b;

				tls_int.set(&a);
				tls_dbl.set(&b);

				// ACT
				thread t1(bind(&set_get_tls<int>, &tls_int, &a2, &ptr_a));
				thread t2(bind(&set_get_tls<double>, &tls_dbl, &b2, &ptr_b));

				// ASSERT
				t1.join();
				t2.join();
				assert_equal(ptr_a, &a2);
				assert_equal(ptr_b, &b2);
				assert_equal(tls_int.get(), &a);
				assert_equal(tls_dbl.get(), &b);
			}
		end_test_suite
	}
}

