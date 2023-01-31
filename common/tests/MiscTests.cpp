#include <common/path.h>
#include <common/pod_vector.h>

#include <list>
#include <ut/assert.h>
#include <ut/test.h>

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

				bool operator ==(const A &rhs) const
				{	return a == rhs.a;	}
			};

			struct B
			{
				int abc;
				char def;

				bool operator ==(const B &rhs) const
				{	return abc == rhs.abc && def == rhs.def;	}
			};
		}

		begin_test_suite( PodVectorTests )
			test( NewPodVectorHasZeroSize )
			{
				// INIT / ACT
				pod_vector<A> As;
				pod_vector<B> Bs;

				// ACT / ASSERT
				assert_equal(0u, As.size());
				assert_equal(0u, Bs.size());
			}

		
			test( NewPodVectorIsEmpty )
			{
				// INIT / ACT
				pod_vector<A> As;
				pod_vector<B> Bs;

				// ACT / ASSERT
				assert_is_true(As.empty());
				assert_is_true(Bs.empty());
			}


			test( NewPodVectorHasSpecifiedCapacity )
			{
				// INIT / ACT
				pod_vector<A> As(11);
				pod_vector<B> Bs(13);

				// ACT / ASSERT
				assert_equal(11u, As.capacity());
				assert_equal(13u, Bs.capacity());
			}


			test( AppendedVectorIsNotEmpty )
			{
				// INIT / ACT
				pod_vector<int> v;

				v.push_back(1);

				// ACT / ASSERT
				assert_is_false(v.empty());
			}


			test( AppendingIncreasesSize )
			{
				// INIT
				A somevalue;
				pod_vector<A> v;

				// ACT
				v.push_back(somevalue);

				// ASSERT
				assert_equal(1u, v.size());
				assert_equal(sizeof(A) * 1, v.byte_size());

				// ACT
				v.push_back(somevalue);
				v.push_back(somevalue);

				// ASSERT
				assert_equal(3u, v.size());
				assert_equal(sizeof(A) * 3, v.byte_size());
			}


			test( ByteSizeRespectsTypeSize )
			{
				// INIT
				A a;
				B b;
				pod_vector<A> v1;
				pod_vector<B> v2;

				// ACT
				v1.push_back(a);
				v1.push_back(a);
				v1.push_back(a);
				v1.push_back(a);
				v1.push_back(a);
				v2.push_back(b);
				v2.push_back(b);
				v2.push_back(b);

				// ASSERT
				assert_equal(sizeof(A) * 5, v1.byte_size());
				assert_equal(sizeof(B) * 3, v2.byte_size());
			}


			test( AppendedValuesAreCopied )
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
				assert_equal(val1, *(vA.data() + 0));
				assert_equal(val2, *(vA.data() + 1));
				assert_equal(val3, *(vB.data() + 0));
				assert_equal(val4, *(vB.data() + 1));
				assert_equal(val5, *(vB.data() + 2));
			}


			test( AppendingOverCapacityIncresesCapacityByHalfOfCurrent )
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
				assert_equal(3u, v1.capacity());
				assert_equal(4u, v2.capacity());

				// ACT
				v1.push_back(somevalue);
				v2.push_back(somevalue);

				// ASSERT
				assert_equal(4u, v1.capacity());
				assert_equal(6u, v2.capacity());
			}


			test( IncresingCapacityPreservesOldValuesAndPushesNew )
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
				assert_not_equal(previous_buffer, v.data());
				assert_equal(val1, *(v.data() + 0));
				assert_equal(val2, *(v.data() + 1));
				assert_equal(val3, *(v.data() + 2));
				assert_equal(val4, *(v.data() + 3));
			}


			test( CopyingVectorCopiesElements )
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
				assert_equal(3u, copied1.capacity());
				assert_equal(3u, copied1.size());
				assert_not_equal(copied1.data(), v1.data());
				assert_equal(val1, *(copied1.data() + 0));
				assert_equal(val2, *(copied1.data() + 1));
				assert_equal(val3, *(copied1.data() + 2));

				assert_equal(10u, copied2.capacity());
				assert_equal(1u, copied2.size());
				assert_not_equal(copied2.data(), v2.data());
				assert_equal(13, *(copied2.data() + 0));
			}


			test( AppendingEmptyVectorWithEmptySequenceLeavesItEmpty )
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
				assert_equal(0u, v1.size());
				assert_equal(0u, v2.size());
				
				// ACT
				v1.append((const B *)0, (const B *)0);

				// ASSERT
				assert_equal(0u, v1.size());
			}


			test( AppendingEmptyVectorWithNonEmptySequenceNoGrow )
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
				assert_equal(3u, v1.size());
				assert_equal(1, *(v1.data() + 0));
				assert_equal(2, *(v1.data() + 1));
				assert_equal(3, *(v1.data() + 2));

				assert_equal(4u, v2.size());
				assert_equal(2, *(v2.data() + 0));
				assert_equal(5, *(v2.data() + 1));
				assert_equal(7, *(v2.data() + 2));
				assert_equal(11, *(v2.data() + 3));
			}


			test( AppendingNonEmptyVectorWithNonEmptySequenceNoGrow )
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
				assert_equal(5u, v1.size());
				assert_equal(1, *(v1.data() + 0));
				assert_equal(2, *(v1.data() + 1));
				assert_equal(3, *(v1.data() + 2));
				assert_equal(23, *(v1.data() + 3));
				assert_equal(31, *(v1.data() + 4));

				assert_equal(7u, v2.size());
				assert_equal(2, *(v2.data() + 0));
				assert_equal(5, *(v2.data() + 1));
				assert_equal(7, *(v2.data() + 2));
				assert_equal(11, *(v2.data() + 3));
				assert_equal(1, *(v2.data() + 4));
				assert_equal(2, *(v2.data() + 5));
				assert_equal(3, *(v2.data() + 6));
			}


			test( AppendingEmptyVectorWithNonEmptySequenceWithGrow )
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
				assert_equal(3u, v1.size());
				assert_equal(1, *(v1.data() + 0));
				assert_equal(2, *(v1.data() + 1));
				assert_equal(3, *(v1.data() + 2));

				assert_equal(4u, v2.size());
				assert_equal(2, *(v2.data() + 0));
				assert_equal(5, *(v2.data() + 1));
				assert_equal(7, *(v2.data() + 2));
				assert_equal(11, *(v2.data() + 3));
			}


			test( AppendingNonEmptyVectorWithNonEmptySequenceWithGrow )
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
				assert_equal(5u, v1.size());
				assert_equal(1, *(v1.data() + 0));
				assert_equal(2, *(v1.data() + 1));
				assert_equal(3, *(v1.data() + 2));
				assert_equal(23, *(v1.data() + 3));
				assert_equal(31, *(v1.data() + 4));
				assert_equal(6u, v1.capacity());

				assert_equal(9u, v2.size());
				assert_equal(2, *(v2.data() + 0));
				assert_equal(5, *(v2.data() + 1));
				assert_equal(7, *(v2.data() + 2));
				assert_equal(11, *(v2.data() + 3));
				assert_equal(212, *(v2.data() + 4));
				assert_equal(215, *(v2.data() + 5));
				assert_equal(217, *(v2.data() + 6));
				assert_equal(2111, *(v2.data() + 7));
				assert_equal(2112, *(v2.data() + 8));
				assert_equal(9u, v2.capacity());
			}


			test( BackReturnsTheLastElementPushed )
			{
				// INIT
				pod_vector<int> v1(2);
				pod_vector<double> v2(3);

				// ACT
				v1.push_back(13);
				v2.push_back(3.1);

				// ACT / ASSERT
				assert_equal(13, v1.back());
				assert_equal(3.1, v2.back());

				// ACT
				v1.push_back(17);
				v1.push_back(19);
				v2.push_back(3);
				v2.push_back(3.14);
				v2.push_back(3.141);

				// ACT / ASSERT
				assert_equal(19, v1.back());
				assert_equal(3.141, v2.back());
			}


			test( ElementsAreReadInReverseOrderWhenPopping )
			{
				// INIT
				pod_vector<int> v(2);

				v.push_back(13);
				v.push_back(17);
				v.push_back(19);

				// ACT
				v.pop_back();

				// ASSERT
				assert_equal(17, v.back());

				// ACT
				v.pop_back();

				// ASSERT
				assert_equal(13, v.back());

				// ACT
				v.pop_back();

				// ASSERT
				assert_is_true(v.empty());
			}


			test( ElementsCanBeAccessedUsingIterators )
			{
				// INIT
				pod_vector<int> v1;
				pod_vector<double> v2;

				v1.push_back(13);
				v1.push_back(17);

				// INIT / ACT
				pod_vector<int>::iterator i1 = v1.begin();

				// ACT / ASSERT
				assert_not_equal(v1.end(), i1);
				assert_equal(13, *i1);
				*i1 = 31;
				assert_equal(31, *i1);

				// ACT
				++i1;

				// ASSERT
				assert_not_equal(v1.end(), i1);
				assert_equal(17, *i1);

				// ACT
				++i1;

				// ASSERT
				assert_equal(v1.end(), i1);

				// INIT
				v2.push_back(1.0);
				v2.push_back(1.01);
				v2.push_back(11.011);

				// INIT / ACT
				pod_vector<double>::iterator i2 = v2.begin();

				// ACT / ASSERT
				assert_not_equal(v2.end(), i2);
				assert_equal(1.0, *i2);

				// ACT
				++i2;

				// ASSERT
				assert_not_equal(v2.end(), i2);
				assert_equal(1.01, *i2);

				// ACT
				++i2;

				// ASSERT
				assert_not_equal(v2.end(), i2);
				assert_equal(11.011, *i2);
				*i2 = 13.24;
				assert_equal(13.24, *i2);

				// ACT
				++i2;

				// ASSERT
				assert_equal(v2.end(), i2);
			}


			test( EmptyPushReservesNewObject )
			{
				// INIT
				pod_vector<int> v1(10);
				pod_vector<double> v2(10);
				pod_vector<int>::iterator i1 = v1.begin();
				pod_vector<double>::iterator i2 = v2.begin();

				// ACT
				v1.push_back();
				v2.push_back();

				// ASSERT
				assert_equal(1, v1.end() - i1);
				assert_equal(1u, v1.size());
				assert_equal(1, v2.end() - i2);
				assert_equal(1u, v2.size());

				// ACT
				v1.push_back();
				v1.push_back();
				v2.push_back();

				// ASSERT
				assert_equal(3, v1.end() - i1);
				assert_equal(3u, v1.size());
				assert_equal(2, v2.end() - i2);
				assert_equal(2u, v2.size());
			}


			test( EmptyPushCausesSpaceGrowWhenLimitIsHit )
			{
				// INIT
				pod_vector<int> v(5);

				v.push_back(3);
				v.push_back(13);
				v.push_back(3221);
				v.push_back(332221);
				v.push_back(17);

				pod_vector<int>::iterator b = v.begin();

				// ACT
				v.push_back();

				// ASSERT
				assert_not_equal(b, v.begin());
				assert_equal(6u, v.size());
			}

		end_test_suite


		begin_test_suite( FilesystemUtilitiesTests )
			test( PathAppendConcantenatesStringsWithSlash )
			{
				// ACT / ASSERT
				assert_equal("/test/mymodule.so", string("/test") & "mymodule.so");
				assert_equal("c:\\another/mymodule2.dll", string("c:\\another") & "mymodule2.dll");
			}

		
			test( AppendingToEmptyDirectoryDoesNotPrependSlash )
			{
				// ACT / ASSERT
				assert_equal("mymodule.so", string("") & "mymodule.so");
				assert_equal("test.dll", string("") & "test.dll");
			}

		
			test( AppendingToDirectoryEndingWithSlashDoesNotAddSlash )
			{
				// ACT / ASSERT
				assert_equal("/test/mymodule.so", string("/test/") & "mymodule.so");
				assert_equal("c:\\another\\mymodule2.dll", string("c:\\another\\") & "mymodule2.dll");
			}


			test( TildeRemovesFilespec )
			{
				// ACT / ASSERT
				assert_equal("/test", ~string("/test/somemodule.so"));
				assert_equal("c:\\anotherdir", ~string("c:\\anotherdir\\testmodule.dll"));
				assert_equal("", ~string("testmodule.dll"));
			}


			test( StarLeavesOnlyFilespec )
			{
				// ACT / ASSERT
				assert_equal("somemodule.so", (string)*string("/test/somemodule.so"));
				assert_equal("testmodule.dll", (string)*string("c:\\anotherdir\\testmodule.dll"));
				assert_equal("testmodule.dll", (string)*string("testmodule.dll"));

				// INIT
				string somepath1 = "/usr/bin/profiler";
				string somepath2 = "/bin/kernel";

				assert_equal(somepath1.c_str() + 9, *somepath1);
				assert_equal(somepath2.c_str() + 5, *somepath2);
			}
		end_test_suite


		begin_test_suite( PathNormalizationTests )
			test( ExecutableNameIsAppendedWithExeExt )
			{
				// INIT / ACT / ASSERT
#if defined(_WIN32)
				assert_equal("ls.exe", normalize::exe("ls"));
				assert_equal("ls.ex.exe", normalize::exe("ls.ex"));
				assert_equal("ls.exe.test.exe", normalize::exe("ls.exe.test"));
				assert_equal("test/ls.exe", normalize::exe("test/ls"));
				assert_equal("test/test\\ls.eX.exe", normalize::exe("test/test\\ls.eX"));
				assert_equal("test\\test\\ls.eX.exe", normalize::exe("test\\test\\ls.eX"));
				assert_equal("subpath\\ls.ex.exe", normalize::exe("subpath\\ls.ex"));
#else
				assert_equal("ls", normalize::exe("ls"));
				assert_equal("ls.ex", normalize::exe("ls.ex"));
				assert_equal("ls.exe.test", normalize::exe("ls.exe.test"));
				assert_equal("test/ls", normalize::exe("test/ls"));
				assert_equal("test/test/ls.eX", normalize::exe("test/test/ls.eX"));
				assert_equal("subpath/ls.ex", normalize::exe("subpath/ls.ex"));
#endif
			}


			test( DynamicLibraryNameIsAppendedWithExeExt )
			{
				// INIT / ACT / ASSERT
#if defined(_WIN32)
				assert_equal("ls.dll", normalize::lib("ls"));
				assert_equal("ls.ex.dll", normalize::lib("ls.ex"));
				assert_equal("ls.exe.test.dll", normalize::lib("ls.exe.test"));
				assert_equal("test/ls.dll", normalize::lib("test/ls"));
				assert_equal("test/test\\ls.eX.dll", normalize::lib("test/test\\ls.eX"));
				assert_equal("test\\test\\ls.eX.dll", normalize::lib("test\\test\\ls.eX"));
				assert_equal("subpath\\ls.ex.dll", normalize::lib("subpath\\ls.ex"));
#elif defined(__APPLE__)
				assert_equal("libls.dylib", normalize::lib("ls"));
				assert_equal("libls.ex.dylib", normalize::lib("ls.ex"));
				assert_equal("libls.exe.test.dylib", normalize::lib("ls.exe.test"));
				assert_equal("test/libls.dylib", normalize::lib("test/ls"));
				assert_equal("test\\test/libls.eX.dylib", normalize::lib("test\\test/ls.eX"));
				assert_equal("subpath/libls.ex.dylib", normalize::lib("subpath/ls.ex"));
#else
				assert_equal("libls.so", normalize::lib("ls"));
				assert_equal("libls.ex.so", normalize::lib("ls.ex"));
				assert_equal("libls.exe.test.so", normalize::lib("ls.exe.test"));
				assert_equal("test/libls.so", normalize::lib("test/ls"));
				assert_equal("test\\test/libls.eX.so", normalize::lib("test\\test/ls.eX"));
				assert_equal("subpath/libls.ex.so", normalize::lib("subpath/ls.ex"));
#endif
			}
		end_test_suite

	}
}
