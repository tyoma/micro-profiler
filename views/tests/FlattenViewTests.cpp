#include <views/flatten.h>

#include "helpers.h"

#include <map>
#include <vector>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			namespace
			{
				struct X
				{
					string group_name;
					vector<string> inner;
				};

				typedef map<int, X> hierarchy1_t;

				struct access_x
				{
					typedef vector<string>::const_iterator const_iterator;

					struct const_reference
					{
						const_reference(const int &key_, const string &group_name_, const string &entry_)
							: key(key_), group_name(group_name_), inner_entry(entry_)
						{	}

						const int &key;
						const string &group_name;
						const string &inner_entry;

					private:
						void operator =(const const_reference &rhs);
					};

					typedef const_reference value_type;

					template <typename T1, typename T2>
					static const_reference get(const T1 &v1, const T2 &v2)
					{	return const_reference(v1.first, v1.second.group_name, v2);	}

					template <typename Type>
					static pair<const_iterator, const_iterator> equal_range(const Type &key)
					{
						return make_pair(key.second.inner.begin(), key.second.inner.end());
					}
				};

				struct x_compound
				{
					x_compound(int key_, string group_name_, string inner_entry_)
						: key(key_), group_name(group_name_), inner_entry(inner_entry_)
					{	}

					template <typename T>
					x_compound(const T &other)
						: key(other.key), group_name(other.group_name), inner_entry(other.inner_entry)
					{	}

					int key;
					string group_name, inner_entry;

					bool operator ==(const x_compound &rhs) const
					{	return key == rhs.key && group_name == rhs.group_name && inner_entry == rhs.inner_entry;	}
				};

			}
		
			begin_test_suite( FlattenViewTests )
				test( ViewIsEmptyForEmptyContainer )
				{
					// INIT
					hierarchy1_t h1;

					// INIT / ACT
					flatten<hierarchy1_t, access_x> v(h1);

					// ASSERT
					assert_equal(v.end(), v.begin());
				}


				test( IteratorsAreCopyable )
				{
					// INIT
					hierarchy1_t h1;
					flatten<hierarchy1_t, access_x> v(h1);

					// INIT / ACT
					auto b = v.begin();
					auto e = v.end();

					// ACT / ASSERT
					assert_equal(e, b);

					// ACT
					b = e;

					// ASSERT
					assert_equal(e, b);
				}


				test( ViewForEmptySubcontainersIsAlsoEmpty )
				{
					// INIT
					hierarchy1_t h1, h2;

					h1[10];
					h1[22];
					h1[32];

					h2[1];

					// INIT / ACT
					flatten<hierarchy1_t, access_x> v1(h1), v2(h2);

					// ASSERT
					assert_equal(v1.end(), v1.begin());
					assert_equal(v2.end(), v2.begin());
				}


				test( ViewIsPopulatedFromNonEmptySubcontainers )
				{
					// INIT
					hierarchy1_t h;

					h[1].group_name = "x";
					h[1].inner = plural + string("one");
					h[3].group_name = "z";
					h[3].inner = plural + string("One") + string("Two");

					// INIT / ACT
					flatten<hierarchy1_t, access_x> v(h);

					// ACT / ASSERT
					x_compound reference1[] = {
						x_compound(1, "x", "one"),
						x_compound(3, "z", "One"), x_compound(3, "z", "Two"),
					};

					assert_equal(reference1, v);

					// INIT
					h[2].group_name = "y";
					h[2].inner = plural + string("foo") + string("bar") + string("baz");

					// ACT / ASSERT
					x_compound reference2[] = {
						x_compound(1, "x", "one"),
						x_compound(2, "y", "foo"), x_compound(2, "y", "bar"), x_compound(2, "y", "baz"),
						x_compound(3, "z", "One"), x_compound(3, "z", "Two"),
					};

					assert_equal(reference2, v);
				}


				test( EmptySubcontainersAreSkippedOnIteration )
				{
					// INIT
					hierarchy1_t h;

					h[10].group_name = "x";
					h[10].inner = plural + string("one");
					h[20].group_name = "y";
					h[20].inner = plural + string("foo") + string("bar") + string("baz");
					h[21];
					h[30].group_name = "z";
					h[30].inner = plural + string("One") + string("Two");

					// INIT / ACT
					flatten<hierarchy1_t, access_x> v(h);

					// ACT / ASSERT
					x_compound reference[] = {
						x_compound(10, "x", "one"),
						x_compound(20, "y", "foo"), x_compound(20, "y", "bar"), x_compound(20, "y", "baz"),
						x_compound(30, "z", "One"), x_compound(30, "z", "Two"),
					};

					assert_equal(reference, v);

					// INIT
					h[15];
					h[25];
					h[35];

					// ACT / ASSERT
					assert_equal(reference, v);
				}


				struct xform_external
				{
					typedef vector<int>::const_iterator const_iterator;
					typedef vector<int>::const_reference const_reference;
					typedef int value_type;

					template <typename T1, typename T2>
					const_reference get(const T1 &/*v1*/, const T2 &v2) const
					{	return v2;	}

					template <typename Type>
					pair<const_iterator, const_iterator> equal_range(const Type &v) const
					{	return make_pair(source[v].begin(), source[v].end());	}

					map< int, vector<int> > &source;
				};

				test( ViewIsEmtpyForEmptyL1WithStatefulTransform )
				{
					// INIT / ACT
					vector<int> l1;
					map< int, vector<int> > source;
					xform_external xform = {	source	};
					flatten<vector<int>, xform_external> f(l1, move(xform));

					// ACT / ASSERT
					assert_equal(f.end(), f.begin());
				}


				test( EnumerateElementsWithStatefulTransform )
				{
					// INIT / ACT
					auto l1 = plural + 314 + 15 + 92;
					map< int, vector<int> > source;
					xform_external xform = {	source	};
					flatten<vector<int>, xform_external> f(l1, move(xform));

					source[l1[0]] = plural + 1 + 2 + 3 + 4;
					source[l1[1]] = plural + 10 + 11 + 12;
					source[l1[2]] = plural + 100 + 101 + 102 + 103;

					// ACT / ASSERT
					int reference1[] = {	1, 2, 3, 4, 10, 11, 12, 100, 101, 102, 103,	};

					assert_equal(reference1, f);

					// INIT
					l1.erase(l1.begin() + 1);

					// ACT / ASSERT
					int reference2[] = {	1, 2, 3, 4, 100, 101, 102, 103,	};

					assert_equal(reference2, f);

					// INIT
					l1.push_back(15);

					// ACT / ASSERT
					int reference3[] = {	1, 2, 3, 4, 100, 101, 102, 103, 10, 11, 12,	};

					assert_equal(reference3, f);
				}

			end_test_suite
		}
	}
}
