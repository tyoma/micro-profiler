#include <views/index.h>

#include "helpers.h"

#include <list>
#include <ut/assert.h>
#include <ut/test.h>
#include <views/table.h>

using namespace std;

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			namespace
			{
				template <typename U, typename K>
				struct index_base : views::index_base<U, K,
					std::unordered_multimap<typename result<K, typename U::value_type>::type, typename U::const_iterator> >
				{
					typedef views::index_base<U, K, std::unordered_multimap<typename result<K, typename U::value_type>::type, typename U::const_iterator> > base_t;

					index_base(const U &underlying, const K &keyer)
						: base_t(underlying, keyer)
					{	}

					using base_t::equal_range;
				};

				struct container1 : list< pair<int, string> >
				{
					typedef void transacted_record;
				};

				template <typename T, typename II>
				vector<T> range_items(const pair<II, II> &range_)
				{
					vector<T> result;

					for (auto i = range_.first; i != range_.second; ++i)
						result.push_back(*i->second);
					return result;
				}

				template <typename II>
				bool is_empty(const pair<II, II> &range_)
				{	return range_.first == range_.second;	}
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


			begin_test_suite( AAImmutableIndexBaseTests )
				test( IndexIsBuiltOnConstruction )
				{
					// INIT
					list< pair<int, string> > data;

					data.push_back(make_pair(3, "lorem"));
					data.push_back(make_pair(14, "ipsum"));
					data.push_back(make_pair(159, "amet"));
					data.push_back(make_pair(15, "ipsum"));
					data.push_back(make_pair(14, "dolor"));

					// INIT / ACT
					const index_base<list< pair<int, string> >, key_first> idx1(data, key_first());
					const index_base<list< pair<int, string> >, key_second> idx2(data, key_second());

					// ACT
					auto r1 = idx1.equal_range(3);

					// ASSERT
					pair<int, string> reference1[] = {	make_pair(3, "lorem"),	};

					assert_equivalent(reference1, (range_items< pair<int, string> >(r1)));

					// ACT
					r1 = idx1.equal_range(14);

					// ASSERT
					pair<int, string> reference2[] = {	make_pair(14, "ipsum"), make_pair(14, "dolor"),	};

					assert_equivalent(reference2, (range_items< pair<int, string> >(r1)));

					// ACT
					auto r2 = idx2.equal_range("ipsum");

					// ASSERT
					pair<int, string> reference3[] = {	make_pair(14, "ipsum"), make_pair(15, "ipsum"),	};

					assert_equivalent(reference3, (range_items< pair<int, string> >(r2)));

					// ACT
					r2 = idx2.equal_range("amet");

					// ASSERT
					pair<int, string> reference4[] = {	make_pair(159, "amet"),	};

					assert_equivalent(reference4, (range_items< pair<int, string> >(r2)));

					// ACT
					r2 = idx2.equal_range("dolor");

					// ASSERT
					pair<int, string> reference5[] = {	make_pair(14, "dolor"),	};

					assert_equivalent(reference5, (range_items< pair<int, string> >(r2)));
				}


				test( IndexIsUpdatedOnItemCreation )
				{
					// INIT
					list< pair<int, string> > data;
					index_base<list< pair<int, string> >, key_first> idx1(data, key_first());
					auto &cidx1 = static_cast<table_component<list< pair<int, string> >::const_iterator> &>(idx1);
					index_base<list< pair<int, string> >, key_second> idx2(data, key_second());
					auto &cidx2 = static_cast<table_component<list< pair<int, string> >::const_iterator> &>(idx2);
					list< pair<int, string> >::const_iterator i[] = {
						data.insert(data.end(), make_pair(3, "lorem")),
						data.insert(data.end(), make_pair(14, "ipsum")),
						data.insert(data.end(), make_pair(159, "amet")),
						data.insert(data.end(), make_pair(15, "ipsum")),
						data.insert(data.end(), make_pair(14, "dolor")),
						data.insert(data.end(), make_pair(11, "ipsum")),
					};

					// ACT
					auto r1 = idx1.equal_range(14);
					auto r2 = idx2.equal_range("ipsum");

					// ASSERT
					assert_equal(r1.first, r1.second);
					assert_equal(r2.first, r2.second);

					// INIT / ACT
					cidx1.created(i[1]);
					cidx2.created(i[1]);

					// ACT
					r1 = idx1.equal_range(14);
					r2 = idx2.equal_range("ipsum");

					// ASSERT
					pair<int, string> reference1[] = {	make_pair(14, "ipsum"),	};

					assert_equivalent(reference1, (range_items< pair<int, string> >(r1)));
					assert_equivalent(reference1, (range_items< pair<int, string> >(r2)));

					// INIT / ACT
					cidx1.created(i[3]);
					cidx2.created(i[3]);

					// ACT
					r1 = idx1.equal_range(15);
					r2 = idx2.equal_range("ipsum");

					// ASSERT
					pair<int, string> reference21[] = {	make_pair(15, "ipsum"),	};
					pair<int, string> reference22[] = {	make_pair(14, "ipsum"), make_pair(15, "ipsum"),	};

					assert_equivalent(reference21, (range_items< pair<int, string> >(r1)));
					assert_equivalent(reference22, (range_items< pair<int, string> >(r2)));
				}


				test( IndexIsUpdatedOnItemRemoval )
				{
					// INIT
					list< pair<int, string> > data;
					list< pair<int, string> >::const_iterator i[] = {
						data.insert(data.end(), make_pair(3, "lorem")),
						data.insert(data.end(), make_pair(14, "ipsum")),
						data.insert(data.end(), make_pair(159, "amet")),
						data.insert(data.end(), make_pair(15, "ipsum")),
						data.insert(data.end(), make_pair(14, "dolor")),
						data.insert(data.end(), make_pair(11, "ipsum")),
					};
					index_base<list< pair<int, string> >, key_second> idx(data, key_second());
					auto &cidx = static_cast<table_component<list< pair<int, string> >::const_iterator> &>(idx);

					// ACT
					cidx.removed(i[3]);

					// ASSERT
					pair<int, string> reference1[] = {	make_pair(14, "ipsum"), make_pair(11, "ipsum"),	};

					assert_equivalent(reference1, (range_items< pair<int, string> >(idx.equal_range("ipsum"))));

					// ACT
					cidx.removed(i[1]);
					cidx.removed(i[0]);
					cidx.removed(i[1]); // repeated removal is ignored
					cidx.removed(i[0]); // repeated removal is ignored

					// ASSERT
					pair<int, string> reference2[] = {	make_pair(11, "ipsum"),	};

					assert_equivalent(reference2, (range_items< pair<int, string> >(idx.equal_range("ipsum"))));
					assert_equal(idx.equal_range("lorem").first, idx.equal_range("lorem").second);
				}


				test( IndexIsUpdatedOnItemsClear )
				{
					// INIT
					list< pair<int, string> > data;
					list< pair<int, string> >::const_iterator i[] = {
						data.insert(data.end(), make_pair(3, "lorem")),
						data.insert(data.end(), make_pair(14, "ipsum")),
						data.insert(data.end(), make_pair(159, "amet")),
						data.insert(data.end(), make_pair(15, "ipsum")),
						data.insert(data.end(), make_pair(14, "dolor")),
						data.insert(data.end(), make_pair(11, "ipsum")),
					};
					index_base<list< pair<int, string> >, key_first> idx1(data, key_first());
					auto &cidx1 = static_cast<table_component<list< pair<int, string> >::const_iterator> &>(idx1);
					index_base<list< pair<int, string> >, key_second> idx2(data, key_second());
					auto &cidx2 = static_cast<table_component<list< pair<int, string> >::const_iterator> &>(idx2);

					// ACT
					cidx1.cleared();
					cidx2.cleared();

					// ASSERT
					assert_is_true(is_empty(idx1.equal_range(3)));
					assert_is_true(is_empty(idx1.equal_range(14)));
					assert_is_true(is_empty(idx1.equal_range(159)));
					assert_is_true(is_empty(idx2.equal_range("lorem")));
					assert_is_true(is_empty(idx2.equal_range("ipsum")));
					assert_is_true(is_empty(idx2.equal_range("amet")));

					// ACT / ASSERT (not crashing)
					cidx1.removed(i[0]);
					cidx1.removed(i[1]);
					cidx2.removed(i[0]);
					cidx2.removed(i[1]);
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
					const immutable_unique_index<container1, key_first> idx1(data);
					const immutable_unique_index<container1, key_second> idx2(data);

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
					const immutable_unique_index<container1, key_first> idx(data);

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
					immutable_unique_index<container1, key_first> idx(data);
					const auto &cidx = idx;
					auto &compidx = static_cast<table_component<container1::const_iterator> &>(idx);
					const auto i1 = data.insert(data.end(), make_pair(3, "lorem"));
					const auto i2 = data.insert(data.end(), make_pair(14, "ipsum"));
					const auto i3 = data.insert(data.end(), make_pair(159, "amet"));
					data.insert(data.end(), make_pair(31, "zzzzz"));

					// ACT
					compidx.created(i3);

					// ASSERT
					assert_throws(cidx[3], invalid_argument);
					assert_throws(cidx[14], invalid_argument);
					assert_equal("amet", cidx[159].second);

					// ACT
					compidx.created(i1);
					compidx.created(i2);

					// ASSERT
					assert_equal("lorem", cidx[3].second);
					assert_equal("ipsum", cidx[14].second);
					assert_equal("amet", cidx[159].second);
				}


				test( NewRecordIsCreatedOnAttemptToAssociateWithAMissingKey )
				{
					typedef table< pair<string, int>, function<pair<string, int> ()> > table_t;

					// INIT
					table_t data([] {	return make_pair("", 0);	});
					const table_t &cdata = data;
					immutable_unique_index<table_t, key_first> idx1(data);
					immutable_unique_index<table_t, key_second> idx2(data);

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
					typedef tuple<int, int, string, int> type_t;
					typedef table< type_t, function<type_t ()> > table_t;
					typedef immutable_unique_index< table_t, key_n_gen<1> > index_t;

					// INIT
					table_t data([] {	return type_t();	});
					type_t data_[] = {
						make_tuple(3, 1, "Lorem", 4),
						make_tuple(1, 5, "Ipsum", 9),
						make_tuple(2, 6, "Amet", 0),
					};
					auto i = add_records(data, data_);
					index_t idx(data);

					// ACT
					table_t::transacted_record rr1 = idx[1];
					auto rr2 = idx[5];
					auto rr3 = idx[6];

					// ASSERT
					assert_equal(i[0], rr1);
					assert_equal(i[1], rr2);
					assert_equal(i[2], rr3);
				}

			end_test_suite


			begin_test_suite( ImmutableIndexTests )
				test( IndexIsPopulatedOnConstruction )
				{
					typedef table< pair<int, string> > table_t;
					typedef immutable_index<table_t, key_first> index1_t;
					typedef immutable_index<table_t, key_second> index2_t;

					// INIT
					table_t t;

					add_records(t, plural
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

			end_test_suite
		}
	}
}
