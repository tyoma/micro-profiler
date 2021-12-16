#include <views/index.h>

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
				struct container1 : list< pair<int, string> >
				{
					typedef void transacted_record;

					wpl::signal<void (iterator record, bool new_)> changed;
				};

				template <typename T>
				struct key_first
				{
					typedef typename T::first_type key_type;

					key_type operator ()(const T &value) const
					{	return value.first;	}

					void operator ()(T &value, const key_type &key) const
					{	value.first = key;	}
				};

				template <typename T>
				struct key_second
				{
					typedef typename T::second_type key_type;

					key_type operator ()(const T &value) const
					{	return value.second;	}

					void operator ()(T &value, const key_type &key) const
					{	value.second = key;	}
				};

				template <typename T>
				struct key_first_no_new
				{
					typedef typename T::first_type key_type;

					key_type operator ()(const T &value) const
					{	return value.first;	}

					void operator ()(T &/*value*/, const key_type &/*key*/) const
					{	throw 0;	}
				};
			}

			begin_test_suite( UniqueIndexTests )
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
			end_test_suite
		}
	}
}
