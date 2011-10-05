#include <pod_vector.h>

#include <list>

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

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
				v.push_back(somevalue);

				// ASSERT
				Assert::IsTrue(1 == v.size());

				// ACT
				v.push_back(somevalue);
				v.push_back(somevalue);

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
				vA.push_back(val1);
				vA.push_back(val2);
				vB.push_back(val3);
				vB.push_back(val4);
				vB.push_back(val5);

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

				v1.push_back(somevalue);
				v1.push_back(somevalue);
				v1.push_back(somevalue);
				v2.push_back(somevalue);
				v2.push_back(somevalue);
				v2.push_back(somevalue);
				v2.push_back(somevalue);

				// ASSERT
				Assert::IsTrue(3 == v1.capacity());
				Assert::IsTrue(4 == v2.capacity());

				// ACT
				v1.push_back(somevalue);
				v2.push_back(somevalue);

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

				v.push_back(val1);
				v.push_back(val2);
				v.push_back(val3);

				const B *previous_buffer = v.data();

				// ACT
				v.push_back(val4);

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

				v1.push_back(val1);
				v1.push_back(val2);
				v1.push_back(val3);
				v2.push_back(13);

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


			[TestMethod]
			void AppendingEmptyVectorWithEmptySequenceLeavesItEmpty()
			{
				// INIT
				pod_vector<B> v1(5);
				pod_vector<double> v2(7);
				B none;
				list<double> empty_list;
				
				// ACT
				v1.append(&none, &none);
				v2.append(empty_list.begin(), empty_list.end());

				// ASSERT
				Assert::IsTrue(v1.size() == 0);
				Assert::IsTrue(v2.size() == 0);
				
				// ACT
				v1.append((const B *)0, (const B *)0);

				// ASSERT
				Assert::IsTrue(v1.size() == 0);
			}


			[TestMethod]
			void AppendingEmptyVectorWithNonEmptySequenceNoGrow()
			{
				// INIT
				pod_vector<int> v1(5);
				pod_vector<double> v2(7);
				int ext1[] = {	1, 2, 3, };
				double ext2_init[] = {	2, 5, 7, 11,	};
				list<double> ext2(ext2_init, ext2_init + 4);
				
				// ACT
				v1.append(ext1, ext1 + 3);
				v2.append(ext2.begin(), ext2.end());

				// ASSERT
				Assert::IsTrue(v1.size() == 3);
				Assert::IsTrue(1 == *(v1.data() + 0));
				Assert::IsTrue(2 == *(v1.data() + 1));
				Assert::IsTrue(3 == *(v1.data() + 2));

				Assert::IsTrue(v2.size() == 4);
				Assert::IsTrue(2 == *(v2.data() + 0));
				Assert::IsTrue(5 == *(v2.data() + 1));
				Assert::IsTrue(7 == *(v2.data() + 2));
				Assert::IsTrue(11 == *(v2.data() + 3));
			}


			[TestMethod]
			void AppendingNonEmptyVectorWithNonEmptySequenceNoGrow()
			{
				// INIT
				pod_vector<int> v1(6);
				pod_vector<double> v2(7);
				int ext1[] = {	1, 2, 3, };
				int ext1b[] = {	23, 31, };
				double ext2_init[] = {	2, 5, 7, 11,	};
				list<double> ext2(ext2_init, ext2_init + 4);

				v1.append(ext1, ext1 + 3);
				v2.append(ext2.begin(), ext2.end());

				// ACT
				v1.append(ext1b, ext1b + 2);
				v2.append(ext1, ext1 + 3);

				// ASSERT
				Assert::IsTrue(v1.size() == 5);
				Assert::IsTrue(1 == *(v1.data() + 0));
				Assert::IsTrue(2 == *(v1.data() + 1));
				Assert::IsTrue(3 == *(v1.data() + 2));
				Assert::IsTrue(23 == *(v1.data() + 3));
				Assert::IsTrue(31 == *(v1.data() + 4));

				Assert::IsTrue(v2.size() == 7);
				Assert::IsTrue(2 == *(v2.data() + 0));
				Assert::IsTrue(5 == *(v2.data() + 1));
				Assert::IsTrue(7 == *(v2.data() + 2));
				Assert::IsTrue(11 == *(v2.data() + 3));
				Assert::IsTrue(1 == *(v2.data() + 4));
				Assert::IsTrue(2 == *(v2.data() + 5));
				Assert::IsTrue(3 == *(v2.data() + 6));
			}


			[TestMethod]
			void AppendingEmptyVectorWithNonEmptySequenceWithGrow()
			{
				// INIT
				pod_vector<int> v1(2);
				pod_vector<double> v2(3);
				int ext1[] = {	1, 2, 3, };
				double ext2_init[] = {	2, 5, 7, 11,	};
				list<double> ext2(ext2_init, ext2_init + 4);
				
				// ACT
				v1.append(ext1, ext1 + 3);
				v2.append(ext2.begin(), ext2.end());

				// ASSERT
				Assert::IsTrue(v1.size() == 3);
				Assert::IsTrue(1 == *(v1.data() + 0));
				Assert::IsTrue(2 == *(v1.data() + 1));
				Assert::IsTrue(3 == *(v1.data() + 2));

				Assert::IsTrue(v2.size() == 4);
				Assert::IsTrue(2 == *(v2.data() + 0));
				Assert::IsTrue(5 == *(v2.data() + 1));
				Assert::IsTrue(7 == *(v2.data() + 2));
				Assert::IsTrue(11 == *(v2.data() + 3));
			}


			[TestMethod]
			void AppendingNonEmptyVectorWithNonEmptySequenceWithGrow()
			{
				// INIT
				pod_vector<int> v1(4);
				pod_vector<double> v2(4);
				int ext1[] = {	1, 2, 3, };
				int ext1b[] = {	23, 31, };
				double ext2[] = {	2, 5, 7, 11,	};
				double ext2b[] = {	212, 215, 217, 2111, 2112,	};

				v1.append(ext1, ext1 + 3);
				v2.append(ext2, ext2 + 4);

				// ACT
				v1.append(ext1b, ext1b + 2);
				v2.append(ext2b, ext2b + 5);

				// ASSERT
				Assert::IsTrue(v1.size() == 5);
				Assert::IsTrue(1 == *(v1.data() + 0));
				Assert::IsTrue(2 == *(v1.data() + 1));
				Assert::IsTrue(3 == *(v1.data() + 2));
				Assert::IsTrue(23 == *(v1.data() + 3));
				Assert::IsTrue(31 == *(v1.data() + 4));
				Assert::IsTrue(v1.capacity() == 6);

				Assert::IsTrue(v2.size() == 9);
				Assert::IsTrue(2 == *(v2.data() + 0));
				Assert::IsTrue(5 == *(v2.data() + 1));
				Assert::IsTrue(7 == *(v2.data() + 2));
				Assert::IsTrue(11 == *(v2.data() + 3));
				Assert::IsTrue(212 == *(v2.data() + 4));
				Assert::IsTrue(215 == *(v2.data() + 5));
				Assert::IsTrue(217 == *(v2.data() + 6));
				Assert::IsTrue(2111 == *(v2.data() + 7));
				Assert::IsTrue(2112 == *(v2.data() + 8));
				Assert::IsTrue(v2.capacity() == 9);
			}
		};
	}
}