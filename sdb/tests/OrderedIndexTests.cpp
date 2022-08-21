#include <sdb/index.h>

#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace ut
{
	template <typename T1, typename T2>
	inline void content_equal(const T1 &expected, const T2 &actual, const LocationInfo &location)
	{
		auto e = std::begin(expected);
		auto a = std::begin(actual);

		for (; e != std::end(expected) && a != std::end(actual); ++e, ++a)
			are_equal(*e, *a, location);
		are_equal(e, std::end(expected), location);
		are_equal(a, std::end(actual), location);
	}
}

namespace sdb
{
	namespace tests
	{
		namespace
		{
			struct local_type
			{
				local_type(int a_, double b_, string c_)
					: a(a_), b(b_), c(c_)
				{	}

				int a;
				double b;
				string c;

				bool operator ==(const local_type &rhs) const
				{	return a == rhs.a && b == rhs.b && c == rhs.c;	}

				bool operator <(const local_type &rhs) const
				{	return make_tuple(a, b, c) < make_tuple(rhs.a, rhs.b, rhs.c);	}
			};

			template <typename T, typename K>
			ordered_index<T, K> make_ordered_index(T &table, const K &keyer)
			{	return std::move(ordered_index<T, K>(table, keyer));	}
		}

		begin_test_suite( OrderedIndexTests )
			test( IndexIsBuiltOnConstruction )
			{
				// INIT
				auto data1_ = plural
					+ local_type(17, 0.932, "lorem")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(19, 1.031, "amet")
					+ local_type(13, 0.032, "dolor");
				auto data2_ = plural
					+ local_type(17, 0.932, "lorem")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(19, 1.031, "amet")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.032, "dolor");
				list<local_type> data1(begin(data1_), end(data1_));
				list<local_type> data2(begin(data2_), end(data2_));

				// INIT / ACT
				auto idx11 = make_ordered_index(data1, [] (const local_type &record) -> int {	return record.a;	});
				auto idx12 = make_ordered_index(data1, [] (const local_type &record) {	return record.c;	});
				auto idx21 = make_ordered_index(data2, [] (const local_type &record) -> double {	return record.b;	});
				auto idx22 = make_ordered_index(data2, [] (const local_type &record) {	return make_tuple(record.a, record.b);	});

				// ACT / ASSERT
				assert_content_equal(plural
					+ local_type(11, 0.132, "ipsum")
					+ local_type(13, 0.032, "dolor")
					+ local_type(17, 0.932, "lorem")
					+ local_type(19, 1.031, "amet"), idx11);
				assert_content_equal(plural
					+ local_type(19, 1.031, "amet")
					+ local_type(13, 0.032, "dolor")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(17, 0.932, "lorem"), idx12);
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(17, 0.932, "lorem")
					+ local_type(19, 1.031, "amet"), idx21);
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(17, 0.932, "lorem")
					+ local_type(19, 1.031, "amet")
					+ local_type(43, 0.122, "ipsum"), idx22);

				// ACT / ASSERT
				assert_not_equal(idx11.end(), idx11.upper_bound(10));
				assert_equal(local_type(11, 0.132, "ipsum"), *idx11.upper_bound(10));
				assert_equal(local_type(13, 0.032, "dolor"), *idx11.upper_bound(11));
				assert_equal(local_type(19, 1.031, "amet"), *idx11.upper_bound(18));
				assert_equal(idx11.end(), idx11.upper_bound(19));

				assert_not_equal(idx21.end(), idx21.upper_bound(1.03));
				assert_equal(local_type(11, 0.132, "ipsum"), *idx21.upper_bound(0.131999));
				assert_equal(local_type(11, 0.032, "dolor"), *idx21.upper_bound(0.0319999));
				assert_equal(local_type(19, 1.031, "amet"), *idx21.upper_bound(1.03));
				assert_equal(local_type(43, 0.122, "ipsum"), *idx21.upper_bound(0.121999));
				assert_equal(idx21.end(), idx21.upper_bound(1.03101));
			}


			test( TableComponentEventsAreProcessedByIndex )
			{
				// INIT
				auto data_ = plural
					+ local_type(17, 0.932, "lorem")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(19, 1.031, "amet")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.032, "dolor");
				list<local_type> data(begin(data_), end(data_));
				auto idx = make_ordered_index(data, [] (const local_type &record) -> double {	return record.b;	});
				auto &cidx = static_cast<table_component<list<local_type>::const_iterator> &>(idx);
				list<local_type>::iterator i1, i2, i3;

				// ACT
				cidx.created(i1 = data.insert(data.end(), local_type(97, 0.100, "Lorem")));
				cidx.created(i2 = data.insert(data.end(), local_type(07, 0.900, "AMET")));

				// ASSERT
				assert_equal(local_type(97, 0.100, "Lorem"), *idx.upper_bound(0.09999));
				assert_equal(local_type(07, 0.900, "AMET"), *idx.upper_bound(0.8999));
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(97, 0.100, "Lorem")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(07, 0.900, "AMET")
					+ local_type(17, 0.932, "lorem")
					+ local_type(19, 1.031, "amet"), idx);

				// ACT
				cidx.created(i3 = data.insert(data.end(), local_type(99, 0.950, "dOLOR")));

				// ASSERT
				assert_equal(local_type(99, 0.950, "dOLOR"), *idx.upper_bound(0.94999));
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(97, 0.100, "Lorem")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(07, 0.900, "AMET")
					+ local_type(17, 0.932, "lorem")
					+ local_type(99, 0.950, "dOLOR")
					+ local_type(19, 1.031, "amet"), idx);

				// ACT
				cidx.removed(i2);

				// ASSERT
				assert_equal(local_type(17, 0.932, "lorem"), *idx.upper_bound(0.8999));
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(97, 0.100, "Lorem")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(17, 0.932, "lorem")
					+ local_type(99, 0.950, "dOLOR")
					+ local_type(19, 1.031, "amet"), idx);

				// ACT
				cidx.removed(i1);

				// ASSERT
				assert_equal(local_type(11, 0.112, "ipsum"), *idx.upper_bound(0.09999));
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(17, 0.932, "lorem")
					+ local_type(99, 0.950, "dOLOR")
					+ local_type(19, 1.031, "amet"), idx);

				// ACT / ASSERT (no crash)
				cidx.removed(i1);
				cidx.removed(i2);

				// ASSERT
				assert_content_equal(plural
					+ local_type(11, 0.032, "dolor")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(17, 0.932, "lorem")
					+ local_type(99, 0.950, "dOLOR")
					+ local_type(19, 1.031, "amet"), idx);

				// ACT
				cidx.cleared();

				// ACT / ASSERT
				cidx.removed(i3);

				// ASSERT
				assert_equal(idx.end(), idx.begin());
			}


			test( EquivalentElementsCanBeIndexed )
			{
				// INIT
				auto data_ = plural
					+ local_type(17, 0.932, "lorem")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(19, 1.031, "amet")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.032, "dolor");
				list<local_type> data(begin(data_), end(data_));

				// INIT / ACT
				auto idx = make_ordered_index(data, [] (const local_type &record) -> int {	return record.a;	});

				// ACT / ASSERT
				assert_equivalent(plural
					+ local_type(17, 0.932, "lorem")
					+ local_type(11, 0.132, "ipsum")
					+ local_type(11, 0.112, "ipsum")
					+ local_type(19, 1.031, "amet")
					+ local_type(43, 0.122, "ipsum")
					+ local_type(11, 0.032, "dolor"), idx);
			}
		end_test_suite
	}
}
