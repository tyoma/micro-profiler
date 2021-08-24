#include <frontend/flatten_view.h>

#include <map>
#include <test-helpers/helpers.h>
#include <vector>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
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
				typedef vector<string>::const_iterator nested_const_iterator;

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
				static nested_const_iterator begin(const Type &v)
				{	return v.second.inner.begin();	}

				template <typename Type>
				static nested_const_iterator end(const Type &v)
				{	return v.second.inner.end();	}
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
				flatten_view<hierarchy1_t, access_x> v(h1);

				// ASSERT
				assert_equal(v.end(), v.begin());
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
				flatten_view<hierarchy1_t, access_x> v1(h1), v2(h2);

				// ASSERT
				assert_equal(v1.end(), v1.begin());
				assert_equal(v2.end(), v2.begin());
			}


			test( ViewIsPopulatedFromNonEmptySubcontainers )
			{
				// INIT
				hierarchy1_t h;
				vector<x_compound> vv;
				string data1[] = {	"one",	}, data2[] = {	"One", "Two",	},
					data3[] = {	"foo", "bar", "baz",	};

				h[1].group_name = "x";
				h[1].inner = mkvector(data1);
				h[3].group_name = "z";
				h[3].inner = mkvector(data2);

				// INIT / ACT
				flatten_view<hierarchy1_t, access_x> v(h);

				// ACT
				for (auto i = v.begin(); i != v.end(); ++i)
					vv.push_back(*i);

				// ACT / ASSERT
				x_compound reference1[] = {
					x_compound(1, "x", "one"),
					x_compound(3, "z", "One"), x_compound(3, "z", "Two"),
				};

				assert_equal(reference1, vv);

				// INIT
				vv.clear();
				h[2].group_name = "y";
				h[2].inner = mkvector(data3);

				// ACT
				for (auto i = v.begin(); i != v.end(); ++i)
					vv.push_back(*i);

				// ACT / ASSERT
				x_compound reference2[] = {
					x_compound(1, "x", "one"),
					x_compound(2, "y", "foo"), x_compound(2, "y", "bar"), x_compound(2, "y", "baz"),
					x_compound(3, "z", "One"), x_compound(3, "z", "Two"),
				};

				assert_equal(reference2, vv);
			}


			test( EmptySubcontainersAreSkippedOnIteration )
			{
				// INIT
				hierarchy1_t h;
				vector<x_compound> vv;
				string data1[] = {	"one",	}, data2[] = {	"One", "Two",	},
					data3[] = {	"foo", "bar", "baz",	};

				h[10].group_name = "x";
				h[10].inner = mkvector(data1);
				h[20].group_name = "y";
				h[20].inner = mkvector(data3);
				h[21];
				h[30].group_name = "z";
				h[30].inner = mkvector(data2);

				// INIT / ACT
				flatten_view<hierarchy1_t, access_x> v(h);

				// ACT
				for (auto i = v.begin(); i != v.end(); ++i)
					vv.push_back(*i);

				// ACT / ASSERT
				x_compound reference[] = {
					x_compound(10, "x", "one"),
					x_compound(20, "y", "foo"), x_compound(20, "y", "bar"), x_compound(20, "y", "baz"),
					x_compound(30, "z", "One"), x_compound(30, "z", "Two"),
				};

				assert_equal(reference, vv);

				// INIT
				h[15];
				h[25];
				h[35];
				vv.clear();

				// ACT
				for (auto i = v.begin(); i != v.end(); ++i)
					vv.push_back(*i);

				// ACT / ASSERT
				assert_equal(reference, vv);
			}

		end_test_suite
	}
}
