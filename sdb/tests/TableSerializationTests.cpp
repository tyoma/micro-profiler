#include <sdb/serialization.h>

#include <strmd/deserializer.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>
#include <sdb/table.h>

using namespace std;
using namespace micro_profiler::tests;

namespace sdb
{
	namespace tests
	{
		namespace
		{
			struct A
			{
				static A create(string a_, int b_)
				{
					A v = {	a_, b_	};
					return v;
				}

				string a;
				int b;
			};

			template <typename ArchiveT>
			void serialize(ArchiveT &archive, A &data)
			{	archive(data.a), archive(data.b);	}

			bool operator ==(const A &lhs, const A &rhs)
			{	return lhs.a == rhs.a && lhs.b == rhs.b;	}

			template <typename T, typename ConstructorT, typename T2>
			void add(table<T, ConstructorT> &table_, const T2 &data)
			{
				auto r = table_.create();

				*r = data;
				r.commit();
			}

			struct create_int
			{
				create_int(int n_ = 0)
					: n(n_)
				{	}

				int operator ()() const
				{	return n++;	}

			private:
				mutable int n;

			private:
				template <typename ArchiveT>
				friend void serialize(ArchiveT &archive, create_int &v);
			};

			template <typename ArchiveT>
			void serialize(ArchiveT &archive, create_int &v)
			{	archive(v.n);	}
		}

		begin_test_suite( TableSerializationTests )
			vector_adapter buffer;
			strmd::serializer<vector_adapter> ser;
			strmd::deserializer<vector_adapter> dser;

			TableSerializationTests()
				: ser(buffer), dser(buffer)
			{	}


			test( TableIsSerializedAsASequence )
			{
				// INIT
				table<string> t1;
				table<A> t2;

				add(t1, "foo");
				add(t1, "BAR");
				add(t1, "baz");
				add(t2, A::create("lorem", 3));
				add(t2, A::create("ipsum", 1));
				add(t2, A::create("amet", 4));
				add(t2, A::create("Dolor", 1));

				// ACT
				ser(t1);

				// ASSERT
				vector<string> read1;

				dser(read1);

				assert_equal(plural + (string)"foo" + (string)"BAR" + (string)"baz", read1);

				// INIT
				buffer = vector_adapter();

				// ACT
				ser(t2);

				// ASSERT
				vector<A> read2;

				dser(read2);

				assert_equal(plural
					+ A::create("lorem", 3)
					+ A::create("ipsum", 1)
					+ A::create("amet", 4)
					+ A::create("Dolor", 1), read2);
			}


			test( SerializedItemsAreFollowedWithSerializedConstructor )
			{
				// INIT
				table<int, create_int> t1, t2(create_int(1109));

				add(t1, 12);
				add(t1, 17);
				add(t1, 19);
				add(t2, 112);
				add(t2, 117);
				add(t2, 119);
				add(t2, 1);

				// ACT
				ser(t1);

				// ASSERT
				vector<int> read;
				create_int c(0);

				dser(read);
				dser(c);

				assert_equal(plural + 12 + 17 + 19, read);
				assert_equal(3, c());

				// INIT
				buffer = vector_adapter();

				// ACT
				ser(t2);

				// ASSERT
				dser(read);
				dser(c);

				assert_equal(plural + 112 + 117 + 119 + 1, read);
				assert_equal(1113, c());
			}
		end_test_suite
	}
}
