#include <frontend/object_lock.h>

namespace std
{
	using namespace tr1;
}

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
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
		[TestClass]
		public ref class ObjectLockTests
		{
		public:
			[TestMethod]
			void ObjectIsHeldByObjectLock()
			{
				// INIT
				shared_ptr<self_unlockable> su(new self_unlockable);
				weak_ptr<self_unlockable> su_weak(su);

				// ACT
				lock(su);
				su = shared_ptr<self_unlockable>();

				// ASSERT
				Assert::IsFalse(su_weak.expired());
			}


			[TestMethod]
			void ObjectIsReleasedOnDemand()
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
				Assert::IsTrue(su_weak.expired());
			}


			[TestMethod]
			void OnlyOneObjectIsReleaseWhenSeveralAreLocked()
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
				Assert::IsFalse(su1_weak.expired());
				Assert::IsTrue(su2_weak.expired());

				// ACT
				p1->unlock();

				// ASSERT
				Assert::IsTrue(su1_weak.expired());
				Assert::IsTrue(su2_weak.expired());
			}


			[TestMethod]
			void LockedUnlockedSetModuleLock()
			{
				// INIT / ACT
				shared_ptr<self_unlockable> su(new self_unlockable);

				// INIT / ASSERT
				Assert::IsFalse(g_locked);
				
				// ACT 
				lock(su);

				// ASSERT
				Assert::IsTrue(g_locked);

				// ACT
				su->unlock();


				// ASSERT
				Assert::IsFalse(g_locked);
			}
		};
	}
}
