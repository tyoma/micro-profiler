#include <views/index.h>

#include "helpers.h"

#include <list>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>
#include <views/table.h>

using namespace micro_profiler::tests;
using namespace std;

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			namespace
			{
				struct container1 : list< pair<int, string> >
				{
					typedef void transacted_record;

					mutable wpl::signal<void (const_iterator record, bool new_)> changed;
					mutable wpl::signal<void (const_iterator record)> removed;
					mutable wpl::signal<void ()> cleared;
				};

				template <typename T>
				struct key_first_no_new
				{
					typedef typename T::first_type key_type;

					key_type operator ()(const T &value) const
					{	return value.first;	}

					template <typename U, typename K>
					void operator ()(immutable_unique_index<U, K> &, T &/*value*/, const key_type &/*key*/) const
					{	throw 0;	}
				};

				template <typename T, typename C, typename SrcT>
				void populate(table<T, C> &destination, const SrcT &source)
				{
					for (auto i = begin(source); i != end(source); ++i)
					{
						auto r = destination.create();

						*r = *i;
						r.commit();
					}
				}
			}


			begin_test_suite( UnorderedMapIteratorStabilityTests )
				test( UnorderedMapIteratorsAreStable )
				{
					// INIT
					unordered_map<int, string> m;
					vector<unordered_map<int, string>::const_iterator> iterators;
					const auto N = 4096;

					for (auto n = N; n--; )
						iterators.push_back(m.insert(make_pair(n, "some long-long string foobar foobar foobar")).first);

					// ACT / ASSERT
					auto i = iterators.begin();
					for (auto n = N; n--; i++)
					{
						assert_equal(n, (*i)->first);
						assert_equal("some long-long string foobar foobar foobar", (*i)->second);
					}

					// ACT
					for (auto ii = iterators.begin(); ii != iterators.end(); ++ii)
						m.erase(*ii);

					// ASSERT
					assert_equal(0u, m.size());
				}


				test( UnorderedMultiMapIteratorsAreStable )
				{
					// INIT
					unordered_multimap<int, string> m;
					vector<unordered_multimap<int, string>::const_iterator> iterators;
					const auto N = 4096;

					for (auto n = N; n--; )
						iterators.push_back(m.insert(make_pair(n, "some long-long string foobar foobar foobar")));

					// ACT / ASSERT
					auto i = iterators.begin();
					for (auto n = N; n--; i++)
					{
						assert_equal(n, (*i)->first);
						assert_equal("some long-long string foobar foobar foobar", (*i)->second);
					}

					// ACT
					for (auto ii = iterators.begin(); ii != iterators.end(); ++ii)
						m.erase(*ii);

					// ASSERT
					assert_equal(0u, m.size());
				}

			end_test_suite


			begin_test_suite( ImmutableUniqueIndexTests )
				test( IndexIsBuiltOnConstruction )
				{
					// INIT
					container1 data;

					data.push_back(make_pair(3, "lorem"));
					data.push_back(make_pair(14, "ipsum"));
					data.push_back(make_pair(159, "amet"));

					// INIT / ACT
					const immutable_unique_index< container1, key_first<container1::value_type> > idx1(data);
					const immutable_unique_index< container1, key_second<container1::value_type> > idx2(data);

					// ACT / ASSERT
					assert_equal("lorem", idx1[3].second);
					assert_equal("ipsum", idx1[14].second);
					assert_equal("amet", idx1[159].second);

					assert_equal(3, idx2["lorem"].first);
					assert_equal(14, idx2["ipsum"].first);
					assert_equal(159, idx2["amet"].first);

					// INIT
					data.push_back(make_pair(26, "dolor"));

					// ACT / ASSERT
					assert_throws(idx1[26], invalid_argument);
					assert_throws(idx2["dolor"], invalid_argument);

					assert_throws(idx1[2611], invalid_argument);
					assert_throws(idx2["amet-dolor"], invalid_argument);
				}


				test( FindingARecordReturnsPointerOrNullIfMissing )
				{
					// INIT
					container1 data;

					data.push_back(make_pair(3, "lorem"));
					data.push_back(make_pair(14, "ipsum"));
					data.push_back(make_pair(159, "amet"));

					// INIT / ACT
					const immutable_unique_index< container1, key_first<container1::value_type> > idx(data);

					// ACT / ASSERT
					assert_null(idx.find(2));
					assert_null(idx.find(4));
					assert_not_null(idx.find(3));
					assert_equal("lorem", idx.find(3)->second);
					assert_not_null(idx.find(14));
					assert_equal("ipsum", idx.find(14)->second);
				}


				test( IndexIsPopulatedOnChangedEventsForNewRecords )
				{
					// INIT
					container1 data;
					const immutable_unique_index< container1, key_first<container1::value_type> > idx(data);
					const auto i1 = data.insert(data.end(), make_pair(3, "lorem"));
					const auto i2 = data.insert(data.end(), make_pair(14, "ipsum"));
					const auto i3 = data.insert(data.end(), make_pair(159, "amet"));
					const auto i4 = data.insert(data.end(), make_pair(31, "zzzzz"));

					// ACT
					data.changed(i3, true);

					// ASSERT
					assert_throws(idx[3], invalid_argument);
					assert_throws(idx[14], invalid_argument);
					assert_equal("amet", idx[159].second);

					// ACT
					data.changed(i1, true);
					data.changed(i2, true);

					// ASSERT
					assert_equal("lorem", idx[3].second);
					assert_equal("ipsum", idx[14].second);
					assert_equal("amet", idx[159].second);

					// ACT (change notifications are ignored)
					data.changed(i4, false);

					// ASSERT
					assert_throws(idx[31], invalid_argument);
				}


				test( NewRecordIsCreatedOnAttemptToAssociateWithAMissingKey )
				{
					typedef table< pair<string, int>, function<pair<string, int> ()> > table_t;

					// INIT
					table_t data([] {	return make_pair("", 0);	});
					const table_t &cdata = data;
					immutable_unique_index< table_t, key_first<table_t::value_type> > idx1(data);
					immutable_unique_index< table_t, key_second<table_t::value_type> > idx2(data);

					// ACT
					table_t::transacted_record r1 = idx1["x"];
					table_t::transacted_record r2 = idx1["y"];
					table_t::transacted_record r3 = idx2[111];
					(*r1).second = 13211;
					(*r2).second = 132114;
					(*r3).first = "zxcv";
					r1.commit();
					r2.commit();
					r3.commit();

					// ASSERT
					pair<string, int> reference1[] = {
						make_pair("x", 13211), make_pair("y", 132114), make_pair("zxcv", 111),
					};

					assert_equal(reference1, cdata);
				}


				test( ExistingTransactedRecordIsReturned )
				{
					typedef table< pair<int, string>, function<pair<int, string> ()> > table_t;
					typedef immutable_unique_index< table_t, key_first_no_new<table_t::value_type> > index_t;

					// INIT
					auto idgen = 0;
					table_t data([&] {	return make_pair(++idgen, "");	});
					index_t idx(data);
					const index_t &cidx = idx;

					auto r1 = data.create();
					(*r1).second = "zoo";
					r1.commit();
					auto r2 = data.create();
					(*r2).second = "foo";
					r2.commit();
					auto r3 = data.create();
					(*r3).second = "bar";
					r3.commit();

					// ACT
					table_t::transacted_record rr1 = idx[1];
					auto rr2 = idx[2];
					auto rr3 = idx[3];

					// ASSERT
					assert_equal("zoo", (*rr1).second);
					assert_equal("zoo", cidx[1].second);
					assert_equal("foo", (*rr2).second);
					assert_equal("foo", cidx[2].second);
					assert_equal("bar", (*rr3).second);
					assert_equal("bar", cidx[3].second);
				}


				test( IndexIsClearedUponTableClear )
				{
					// INIT
					container1 data;

					data.push_back(make_pair(3, "lorem"));
					data.push_back(make_pair(14, "ipsum"));
					data.push_back(make_pair(159, "amet"));

					const immutable_unique_index< container1, key_first<container1::value_type> > idx(data);

					// ACT
					data.cleared();

					// ASSERT
					assert_null(idx.find(3));
					assert_throws(idx[3], invalid_argument);
					assert_null(idx.find(159));
					assert_throws(idx[159], invalid_argument);
				}


				test( RemovedItemsNoLongerAvailableFromTheIndex )
				{
					// INIT
					container1 data;

					data.insert(data.end(), make_pair(3, "lorem"));
					auto i2 = data.insert(data.end(), make_pair(14, "ipsum"));
					data.insert(data.end(), make_pair(159, "amet"));
					auto i4 = data.insert(data.end(), make_pair(17, "dolor"));

					const immutable_unique_index< container1, key_first<container1::value_type> > idx(data);

					// ACT
					data.removed(i2);

					// ASSERT
					assert_not_null(idx.find(3));
					assert_null(idx.find(14));
					assert_not_null(idx.find(159));
					assert_not_null(idx.find(17));

					// ACT
					data.removed(i4);
					data.removed(i4); // repeated removal is ignored

					// ASSERT
					assert_null(idx.find(17));
				}

			end_test_suite


			begin_test_suite( ImmutableIndexTests )
				test( IndexIsPopulatedOnConstruction )
				{
					typedef table< pair<int, string> > table_t;
					typedef immutable_index< table_t, key_first<table_t::value_type> > index1_t;
					typedef immutable_index< table_t, key_second<table_t::value_type> > index2_t;

					// INIT
					table_t t;

					populate(t, plural
						+ make_pair(11, (string)"zoo")
						+ make_pair(13, (string)"zoo")
						+ make_pair(11, (string)"foo")
						+ make_pair(11, (string)"zoo")
						+ make_pair(13, (string)"bar")
						+ make_pair(11, (string)"zoo")
						+ make_pair(19, (string)"bar")
						+ make_pair(11, (string)"zoo")
						+ make_pair(19, (string)"foo"));

					// INIT / ACT
					index1_t idx1(t);
					index2_t idx2(t);

					// ACT
					auto r1 = idx1.equal_range(11);

					// ASSERT
					assert_equivalent(plural
						+ make_pair(11, (string)"zoo")
						+ make_pair(11, (string)"foo")
						+ make_pair(11, (string)"zoo")
						+ make_pair(11, (string)"zoo")
						+ make_pair(11, (string)"zoo"), (vector< pair<int, string> >(r1.first, r1.second)));

					// ACT
					r1 = idx1.equal_range(13);

					// ASSERT
					assert_equivalent(plural
						+ make_pair(13, (string)"zoo")
						+ make_pair(13, (string)"bar"), (vector< pair<int, string> >(r1.first, r1.second)));

					// ACT
					r1 = idx1.equal_range(19);

					// ASSERT
					assert_equivalent(plural
						+ make_pair(19, (string)"bar")
						+ make_pair(19, (string)"foo"), (vector< pair<int, string> >(r1.first, r1.second)));

					// ACT
					r1 = idx1.equal_range(111);

					// ASSERT
					assert_equal(r1.first, r1.second);

					// ACT
					auto r2 = idx2.equal_range("zoo");

					// ASSERT
					assert_equivalent(plural
						+ make_pair(11, (string)"zoo")
						+ make_pair(13, (string)"zoo")
						+ make_pair(11, (string)"zoo")
						+ make_pair(11, (string)"zoo")
						+ make_pair(11, (string)"zoo"), (vector< pair<int, string> >(r2.first, r2.second)));

					// ACT
					r2 = idx2.equal_range("foo");

					// ASSERT
					assert_equivalent(plural
						+ make_pair(11, (string)"foo")
						+ make_pair(19, (string)"foo"), (vector< pair<int, string> >(r2.first, r2.second)));

					// ACT
					r2 = idx2.equal_range("bar");

					// ASSERT
					assert_equivalent(plural
						+ make_pair(13, (string)"bar")
						+ make_pair(19, (string)"bar"), (vector< pair<int, string> >(r2.first, r2.second)));

					// ACT
					r2 = idx2.equal_range("");

					// ASSERT
					assert_equal(r2.first, r2.second);
				}


				test( DynamicallyAddedRecordsBecomeIndexed )
				{
					typedef table< pair<int, string> > table_t;
					typedef immutable_index< table_t, key_first<table_t::value_type> > index_t;

					// INIT
					table_t t;
					index_t idx(t);

					// ACT
					populate(t, plural + make_pair(11, (string)"zoo"));
					auto r = idx.equal_range(11);

					// ASSERT
					assert_equivalent(plural
						+ make_pair(11, (string)"zoo"), (vector< pair<int, string> >(r.first, r.second)));
					assert_equal(11, r.first->first);
					assert_equal("zoo", r.first->second);

					// ACT
					populate(t, plural + make_pair(19, (string)"bar") + make_pair(11, (string)"foo"));
					r = idx.equal_range(11);

					// ASSERT
					assert_equivalent(plural
						+ make_pair(11, (string)"zoo")
						+ make_pair(11, (string)"foo"), (vector< pair<int, string> >(r.first, r.second)));

					// ACT
					r = idx.equal_range(19);

					// ASSERT
					assert_equivalent(plural
						+ make_pair(19, (string)"bar"), (vector< pair<int, string> >(r.first, r.second)));

					// ACT
					auto tr = t.modify(t.begin());
					(*tr).first = 1910;
					tr.commit();
					r = idx.equal_range(1910);

					// ASSERT
					assert_equal(r.first, r.second);
				}


				test( IndexIsClearedUponTableClear )
				{
					// INIT
					container1 data;

					data.push_back(make_pair(3, "lorem"));
					data.push_back(make_pair(14, "ipsum"));
					data.push_back(make_pair(159, "amet"));
					data.push_back(make_pair(3, "test"));

					const immutable_index< container1, key_first<container1::value_type> > idx(data);

					// ACT
					data.cleared();

					// ASSERT
					auto r = idx.equal_range(3);
					assert_equal(r.first, r.second);
					r = idx.equal_range(14);
					assert_equal(r.first, r.second);
				}


				test( RemovedItemsNoLongerAvailableFromTheIndex )
				{
					// INIT
					container1 data;

					auto i1 = data.insert(data.end(), make_pair(3, "lorem"));
					auto i2 = data.insert(data.end(), make_pair(14, "ipsum"));
					data.insert(data.end(), make_pair(159, "amet"));
					auto i4 = data.insert(data.end(), make_pair(3, "dolor"));
					auto i5 = data.insert(data.end(), make_pair(159, "dolor"));

					const immutable_index< container1, key_first<container1::value_type> > idx(data);

					// ACT
					data.removed(i2);

					// ASSERT
					auto r = idx.equal_range(3);
					assert_equivalent(plural
						+ make_pair(3, (string)"lorem")
						+ make_pair(3, (string)"dolor"), (vector< pair<int, string> >(r.first, r.second)));
					r = idx.equal_range(14);
					assert_equal(r.first, r.second);
					r = idx.equal_range(159);
					assert_equivalent(plural
						+ make_pair(159, (string)"amet")
						+ make_pair(159, (string)"dolor"), (vector< pair<int, string> >(r.first, r.second)));

					// ACT
					data.removed(i5);

					// ASSERT
					r = idx.equal_range(159);
					assert_equivalent(plural
						+ make_pair(159, (string)"amet"), (vector< pair<int, string> >(r.first, r.second)));

					// ACT
					data.removed(i1);
					data.removed(i4);
					data.removed(i4); // repeated removal is ignored

					// ASSERT
					r = idx.equal_range(3);
					assert_equal(r.first, r.second);
				}

			end_test_suite
		}
	}
}
