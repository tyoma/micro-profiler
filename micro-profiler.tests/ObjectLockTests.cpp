#include <frontend/object_lock.h>

#include <ut/assert.h>
#include <ut/test.h>

namespace std
{
	using namespace tr1;
}

using namespace std;

namespace micro_profiler
{
	bool g_locked = false;

	void LockModule()
	{
		g_locked = true;
	}

	void UnlockModule()
	{
		g_locked = false;
	}

	namespace tests
	{
		begin_test_suite( ObjectLockTests )
			test( ObjectIsHeldByObjectLock )
			{
				// INIT
				shared_ptr<self_unlockable> su(new self_unlockable);
				weak_ptr<self_unlockable> su_weak(su);

				// ACT
				lock(su);
				su = shared_ptr<self_unlockable>();

				// ASSERT
				assert_is_false(su_weak.expired());
			}


			test( ObjectIsReleasedOnDemand )
			{
				// INIT
				shared_ptr<self_unlockable> su(new self_unlockable);
				weak_ptr<self_unlockable> su_weak(su);

				// ACT
				lock(su);
				self_unlockable *p = &*su;
				su = shared_ptr<self_unlockable>();

				p->unlock();

				// ASSERT
				assert_is_true(su_weak.expired());
			}


			test( OnlyOneObjectIsReleaseWhenSeveralAreLocked )
			{
				// INIT
				shared_ptr<self_unlockable> su1(new self_unlockable), su2(new self_unlockable);
				weak_ptr<self_unlockable> su1_weak(su1), su2_weak(su2);

				lock(su1);
				lock(su2);
				self_unlockable *p1 = &*su1;
				self_unlockable *p2 = &*su2;
				su1 = shared_ptr<self_unlockable>();
				su2 = shared_ptr<self_unlockable>();

				// ACT
				p2->unlock();

				// ASSERT
				assert_is_false(su1_weak.expired());
				assert_is_true(su2_weak.expired());

				// ACT
				p1->unlock();

				// ASSERT
				assert_is_true(su1_weak.expired());
				assert_is_true(su2_weak.expired());
			}


			test( LockedUnlockedSetModuleLock )
			{
				// INIT / ACT
				shared_ptr<self_unlockable> su(new self_unlockable);

				// INIT / ASSERT
				assert_is_false(g_locked);
				
				// ACT 
				lock(su);

				// ASSERT
				assert_is_true(g_locked);

				// ACT
				su->unlock();


				// ASSERT
				assert_is_false(g_locked);
			}
		end_test_suite
	}
}
