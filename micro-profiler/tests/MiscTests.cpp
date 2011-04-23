#include <pod_vector.h>

using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct A
			{
				int a;

				bool operator ==(const A &rhs)
				{	return a == rhs.a;	}
			};

			struct B
			{
				int abc;
				char def;

				bool operator ==(const B &rhs)
				{	return abc == rhs.abc && def == rhs.def;	}
			};
		}

		[TestClass]
		public ref class MiscTests
		{
		public: 
			[TestMethod]
			void NewPodVectorHasZeroSize()
			{
				// INIT / ACT
				pod_vector<A> As;
				pod_vector<B> Bs;

				// ACT / ASSERT
				Assert::IsTrue(0 == As.size());
				Assert::IsTrue(0 == Bs.size());
			}


			[TestMethod]
			void NewPodVectorHasSpecifiedCapacity()
			{
				// INIT / ACT
				pod_vector<A> As(11);
				pod_vector<B> Bs(13);

				// ACT / ASSERT
				Assert::IsTrue(11 == As.capacity());
				Assert::IsTrue(13 == Bs.capacity());
			}


			[TestMethod]
			void AppendingIncreasesSize()
			{
				// INIT
				A somevalue;
				pod_vector<A> v;

				// ACT
				v.append(somevalue);

				// ASSERT
				Assert::IsTrue(1 == v.size());

				// ACT
				v.append(somevalue);
				v.append(somevalue);

				// ASSERT
				Assert::IsTrue(3 == v.size());
			}


			[TestMethod]
			void AppendedValuesAreCopied()
			{
				// INIT
				A val1 = {	3	}, val2 = {	5	};
				B val3 = {	1234, 1	}, val4 = {	2345, 11	}, val5 = {	34567, 13	};
				pod_vector<A> vA(4);
				pod_vector<B> vB(4);

				// ACT
				vA.append(val1);
				vA.append(val2);
				vB.append(val3);
				vB.append(val4);
				vB.append(val5);

				// ACT / ASSERT
				Assert::IsTrue(val1 == *(vA.data() + 0));
				Assert::IsTrue(val2 == *(vA.data() + 1));
				Assert::IsTrue(val3 == *(vB.data() + 0));
				Assert::IsTrue(val4 == *(vB.data() + 1));
				Assert::IsTrue(val5 == *(vB.data() + 2));
			}


			[TestMethod]
			void AppendingOverCapacityIncresesCapacityByHalfOfCurrent()
			{
				// INIT
				A somevalue;
				pod_vector<A> v1(3), v2(4);

				v1.append(somevalue);
				v1.append(somevalue);
				v1.append(somevalue);
				v2.append(somevalue);
				v2.append(somevalue);
				v2.append(somevalue);
				v2.append(somevalue);

				// ASSERT
				Assert::IsTrue(3 == v1.capacity());
				Assert::IsTrue(4 == v2.capacity());

				// ACT
				v1.append(somevalue);
				v2.append(somevalue);

				// ASSERT
				Assert::IsTrue(4 == v1.capacity());
				Assert::IsTrue(6 == v2.capacity());
			}


			[TestMethod]
			void IncresingCapacityPreservesOldValuesAndPushesNew()
			{
				// INIT
				B val1 = {	1234, 1	}, val2 = {	2345, 11	}, val3 = {	34567, 13	}, val4 = {	134567, 17	};
				pod_vector<B> v(3);

				v.append(val1);
				v.append(val2);
				v.append(val3);

				const B *previous_buffer = v.data();

				// ACT
				v.append(val4);

				// ACT / ASSERT
				Assert::IsTrue(previous_buffer != v.data());
				Assert::IsTrue(val1 == *(v.data() + 0));
				Assert::IsTrue(val2 == *(v.data() + 1));
				Assert::IsTrue(val3 == *(v.data() + 2));
				Assert::IsTrue(val4 == *(v.data() + 3));
			}


			[TestMethod]
			void CopyingVectorCopiesElements()
			{
				// INIT
				B val1 = {	1234, 1	}, val2 = {	2345, 11	}, val3 = {	34567, 13	};
				pod_vector<B> v1(3);
				pod_vector<int> v2(10);

				v1.append(val1);
				v1.append(val2);
				v1.append(val3);
				v2.append(13);

				// ACT
				pod_vector<B> copied1(v1);
				pod_vector<int> copied2(v2);

				// ACT / ASSERT
				Assert::IsTrue(copied1.capacity() == 3);
				Assert::IsTrue(copied1.size() == 3);
				Assert::IsTrue(copied1.data() != v1.data());
				Assert::IsTrue(val1 == *(copied1.data() + 0));
				Assert::IsTrue(val2 == *(copied1.data() + 1));
				Assert::IsTrue(val3 == *(copied1.data() + 2));

				Assert::IsTrue(copied2.capacity() == 10);
				Assert::IsTrue(copied2.size() == 1);
				Assert::IsTrue(copied2.data() != v2.data());
				Assert::IsTrue(13 == *(copied2.data() + 0));
			}
		};
	}
}