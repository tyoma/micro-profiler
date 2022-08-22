#include <sdb/indexed_serialization.h>

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
			struct local_type
			{
				static local_type create(string a_, int b_, int c_)
				{
					local_type v = {	a_, b_, c_	};
					return v;
				}

				string a;
				int b;
				int c;

				bool operator <(const local_type &rhs) const
				{	return make_tuple(a, b, c) < make_tuple(rhs.a, rhs.b, rhs.c);	}
			};

			struct keyer_a
			{
				string operator ()(const local_type &record) const {	return record.a;	}
				template<typename IgnoredT> void operator ()(IgnoredT &, local_type &record, string key) const {	record.a = key;	}

			};

			struct keyer_c
			{
				int operator ()(const local_type &record) const {	return record.c;	}
				template<typename IgnoredT> void operator ()(IgnoredT &, local_type &record, int key) const {	record.c = key;	}
			};

			struct skip_a_context_tag {} const skip_a_context;
			struct skip_c_context_tag {} const skip_c_context;

			template <typename ArchiveT>
			void serialize(ArchiveT &archive, local_type &data, skip_a_context_tag, unsigned int /*ver*/)
			{	archive(data.b), archive(data.c);	}

			template <typename ArchiveT>
			void serialize(ArchiveT &archive, local_type &data, scontext::indexed_by<keyer_a, void> &, unsigned int /*ver*/)
			{	archive(data.b), archive(data.c);	}

			template <typename ArchiveT>
			void serialize(ArchiveT &archive, local_type &data, skip_c_context_tag, unsigned int /*ver*/)
			{	archive(data.a), archive(data.b);	}

			template <typename ArchiveT>
			void serialize(ArchiveT &archive, local_type &data, scontext::indexed_by<keyer_c, void> &, unsigned int /*ver*/)
			{	archive(data.a), archive(data.b);	}
		}
	}
}

namespace strmd
{
	template <> struct version<sdb::tests::local_type> {	enum {	value = 1	};	};
}

namespace sdb
{
	namespace tests
	{
		begin_test_suite( IndexedSerializationTests )
			vector_adapter buffer;
			strmd::serializer<vector_adapter> ser;
			strmd::deserializer<vector_adapter> dser;

			IndexedSerializationTests()
				: ser(buffer), dser(buffer)
			{	}


			test( DeserializingViaIndexReadsPairsOfKeyValues )
			{
				// INIT
				table<local_type> t1, t2;

				// use 'c' as a key
				ser(plural
					+ make_pair(1121, local_type::create("Lorem", 312, 0))
					+ make_pair(7121, local_type::create("ipsum", 112, 0))
					+ make_pair(1021, local_type::create("AMET", 382, 0))
					+ make_pair(9121, local_type::create("DoLor", 12, 0)), skip_c_context);

				// ACT
				scontext::indexed_by<keyer_c, void> ctx1;
				dser(t1, ctx1);

				// ASSERT
				assert_equivalent(plural
					+ local_type::create("Lorem", 312, 1121)
					+ local_type::create("ipsum", 112, 7121)
					+ local_type::create("AMET", 382, 1021)
					+ local_type::create("DoLor", 12, 9121), t1);

				// INIT
				ser(plural
					+ make_pair(1121, local_type::create("foo", 11, 0))
					+ make_pair(7121, local_type::create("bar", 12, 0))
					+ make_pair(9, local_type::create("baz", 222, 0)), skip_c_context);

				// ACT (no clear, overwrite existing)
				dser(t1, ctx1);

				// ASSERT
				assert_equivalent(plural
					+ local_type::create("baz", 222, 9)
					+ local_type::create("foo", 11, 1121)
					+ local_type::create("bar", 12, 7121)
					+ local_type::create("AMET", 382, 1021)
					+ local_type::create("DoLor", 12, 9121), t1);

				// INIT
				ser(plural
					+ make_pair((string)"foo-lorem", local_type::create("", 11, 0))
					+ make_pair((string)"foo-ipsum", local_type::create("", 12, 1))
					+ make_pair((string)"bar-dolor", local_type::create("", 222, 1)), skip_a_context);

				// ACT
				scontext::indexed_by<keyer_a, void> ctx2;
				dser(t2, ctx2);

				// ASSERT
				assert_equivalent(plural
					+ local_type::create("foo-lorem", 11, 0)
					+ local_type::create("foo-ipsum", 12, 1)
					+ local_type::create("bar-dolor", 222, 1), t2);
			}


			test( TableIsInvalidatedUponIndexedDeserialization )
			{
				// INIT
				table<local_type> t;
				scontext::indexed_by<keyer_c, void> ctx;
				vector< vector<local_type> > log;
				auto c = t.invalidate += [&] {	log.push_back(vector<local_type>(t.begin(), t.end()));	};

				ser(plural
					+ make_pair(1021, local_type::create("AMET", 382, 0))
					+ make_pair(9121, local_type::create("DoLor", 12, 0)), skip_c_context);

				// ACT
				dser(t, ctx);

				// ASSERT
				assert_equal(1u, log.size());
				assert_equivalent(plural
					+ local_type::create("AMET", 382, 1021)
					+ local_type::create("DoLor", 12, 9121), log.back());
			}
		end_test_suite
	}
}
