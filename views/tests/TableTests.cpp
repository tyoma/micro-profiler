#include <views/table.h>

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
				struct AA
				{
					AA(int id_)
						: id(id_)
					{	}

					~AA()
					{	id = 0;	}

					int id;
					string value;
				};
			}

			begin_test_suite( TableTests )
				test( NewTableIsEmpty )
				{
					typedef table< int, function<int ()> > table_t;

					// INIT / ACT
					table_t t([] {	return 0;	});
					const table_t &ct = t;

					// ACT / ASSERT
					assert_is_true(t.end() == t.begin());
					assert_is_false(t.end() != t.begin());
					assert_is_true(ct.end() == ct.begin());
					assert_is_false(ct.end() != ct.begin());
					assert_is_true(table_t::const_iterator(t.end()) == table_t::const_iterator(t.begin()));
					assert_is_false(table_t::const_iterator(t.end()) != table_t::const_iterator(t.begin()));
				}


				test( CreatedRecordsAreInitializedWithConstructorSupplied )
				{
					typedef table< AA, function<AA ()> > table_t;

					// INIT
					table_t t1([&] {	return AA(100);	}), t2([&] {	return AA(1231);	});

					auto c1 = t1.changed += [] (table_t::iterator, bool) {	assert_is_false(true);	};
					auto c2 = t2.changed += [] (table_t::iterator, bool) {	assert_is_false(true);	};

					// ACT
					auto r1 = t1.create();
					auto r2 = t2.create();

					// ASSERT
					assert_equal(100, (*r1).id);
					assert_equal(1231, (*r2).id);
				}


				test( RecordIsAccessibleAfterCreationOfOthers )
				{
					typedef table< AA, function<AA ()> > table_t;

					// INIT
					auto idgen = 1;
					table_t t([&] {	return AA(idgen++);	});
					vector<table_t::transacted_record> l;

					// ACT
					auto r1 = t.create();
					(*r1).value = "lorem";
					for (int a = 300; a; a--)
						l.push_back(t.create());
					auto r2 = t.create();
					(*r2).value = "ipsum";
					for (int a = 5; a; a--)
						l.push_back(t.create());
					auto r3 = t.create();
					(*r3).value = "dolor";

					// ASSERT
					assert_equal(1, (*r1).id);
					assert_equal("lorem", (*r1).value);
					assert_equal(302, (*r2).id);
					assert_equal("ipsum", (*r2).value);
					assert_equal(308, (*r3).id);
					assert_equal("dolor", (*r3).value);
				}


				test( IteratorOfTheRecordCreatedIsReportedInANotification )
				{
					typedef table< AA, function<AA ()> > table_t;

					// INIT
					auto idgen = 1;
					table_t t([&] {	return AA(idgen++);	});
					vector<table_t::const_iterator> cl;
					vector<table_t::iterator> l;
					auto c = t.changed += [&] (table_t::iterator i, bool new_) {
						cl.push_back(i);
						l.push_back(i);
						assert_is_true(new_);
					};

					// ACT
					auto r1 = t.create();
					(*r1).id = 17, (*r1).value = "zooma";
					auto r2 = t.create();
					(*r2).id = 13, (*r2).value = "loop";
					auto r3 = t.create();
					(*r3).id = 19, (*r3).value = "lasso";
					r2.commit();

					// ASSERT
					assert_equal(1u, l.size());
					assert_equal(13, (*cl[0]).id);
					assert_equal("loop", (*cl[0]).value);
					assert_equal(13, (**l[0]).id);
					assert_equal("loop", (**l[0]).value);

					// ACT
					r1.commit();
					r3.commit();

					// ASSERT
					assert_equal(3u, l.size());
					assert_equal(13, (**l[0]).id);
					assert_equal("loop", (**l[0]).value);
					assert_equal(17, (**l[1]).id);
					assert_equal("zooma", (**l[1]).value);
					assert_equal(19, (**l[2]).id);
					assert_equal("lasso", (**l[2]).value);
					assert_equal(13, (*cl[0]).id);
					assert_equal("loop", (*cl[0]).value);
					assert_equal(17, (*cl[1]).id);
					assert_equal("zooma", (*cl[1]).value);
					assert_equal(19, (*cl[2]).id);
					assert_equal("lasso", (*cl[2]).value);
				}


				test( IteratorsOfTheCreatedRecordsAreUnique )
				{
					typedef table< AA, function<AA ()> > table_t;

					// INIT
					table_t t([&] {	return AA(0);	});
					vector<table_t::iterator> l;
					auto c = t.changed += [&l] (table_t::iterator i, bool) {	l.push_back(i);	};

					// ACT
					auto r1 = t.create();
					r1.commit();
					auto r2 = t.create();
					r2.commit();
					auto r3 = t.create();
					r3.commit();

					// ASSERT
					assert_is_true(l[0] == l[0]);
					assert_is_false(l[0] != l[0]);
					assert_is_true(l[1] == l[1]);
					assert_is_false(l[1] != l[1]);

					assert_is_true(l[0] != l[1]);
					assert_is_false(l[0] == l[1]);
					assert_is_true(l[1] != l[2]);
					assert_is_false(l[1] == l[2]);
					assert_is_true(l[0] != l[2]);
					assert_is_false(l[0] == l[2]);
				}


				test( ComittedRecordsAreVisisbleViaIteration )
				{
					typedef table< AA, function<AA ()> > table_t;

					// INIT
					int idgen = 1;
					table_t t([&] {	return AA(idgen++);	});

					auto r1= t.create();
					(*r1).value = "lorem";
					r1.commit();
					auto r2= t.create();
					(*r2).value = "ipsum";
					r2.commit();

					// ACT / ASSERT
					auto i = t.begin();
					auto ci = static_cast<const table_t &>(t).begin();

					assert_not_equal(t.end(), i);
					assert_equal(1, (**i).id);
					assert_equal("lorem", (**i).value);
					++i;
					assert_not_equal(t.end(), i);
					assert_equal(2, (**i).id);
					assert_equal("ipsum", (**i).value);
					++i;
					assert_equal(t.end(), i);

					assert_not_equal(static_cast<const table_t &>(t).end(), ci);
					assert_equal(1, (*ci).id);
					assert_equal("lorem", (*ci).value);
					++ci;
					assert_not_equal(static_cast<const table_t &>(t).end(), ci);
					assert_equal(2, (*ci).id);
					assert_equal("ipsum", (*ci).value);
					++ci;
					assert_equal(static_cast<const table_t &>(t).end(), ci);

					// INIT
					auto r3= t.create();
					(*r3).value = "amet-dolor";
					r3.commit();

					// ACT / ASSERT
					assert_not_equal(t.end(), i);
					assert_equal(3, (**i).id);
					assert_equal("amet-dolor", (**i).value);
					++i;
					assert_equal(t.end(), i);

					assert_not_equal(static_cast<const table_t &>(t).end(), ci);
					assert_equal(3, (*ci).id);
					assert_equal("amet-dolor", (*ci).value);
					++ci;
					assert_equal(static_cast<const table_t &>(t).end(), ci);
				}


				test( CommitingTransactedRecordFromWriteIteratorNotifiesOfUpdate )
				{
					typedef table< AA, function<AA ()> > table_t;

					// INIT
					int idgen = 1;
					table_t t([&] {	return AA(idgen++);	});
					vector<table_t::iterator> l;

					auto r1= t.create();
					r1.commit();
					auto r2= t.create();
					r2.commit();
					auto r3= t.create();
					r3.commit();
					auto i = t.begin();

					auto c = t.changed += [&] (table_t::iterator i, bool new_) {
						l.push_back(i);
						assert_is_false(new_);
					};

					// INIT / ACT
					auto mr1 = *i;
					++i, ++i;
					auto mr3 = *i;

					// ACT
					(*mr3).value = "zubzub";
					mr3.commit();

					// ASSERT
					assert_equal(1u, l.size());
					assert_equal(i, l[0]);
					assert_equal("zubzub", (**l[0]).value);
					assert_equal("zubzub", (*table_t::const_iterator(l[0])).value);

					// ACT
					(*mr1).value = "testtest";
					mr1.commit();

					// ASSERT
					assert_equal(2u, l.size());
					assert_equal(t.begin(), l[1]);
					assert_equal("testtest", (**l[1]).value);
					assert_equal("testtest", (*table_t::const_iterator(l[1])).value);
				}


				test( TableIteratorsAreAssignable )
				{
					typedef table< int, function<int ()> > table1_t;
					typedef table< string, function<string ()> > table2_t;

					// INIT
					table1_t t1([] {	return 0;	});
					table2_t t2([] {	return "";	});
					const table1_t &ct1 = t1;
					const table2_t &ct2 = t2;

					auto r1 = t1.create(); *r1 = 1211, r1.commit();
					auto r2 = t1.create(); *r2 = 1, r2.commit();
					auto r3 = t1.create(); *r3 = 17, r3.commit();
					auto r4 = t2.create(); *r4 = "1211", r4.commit();
					auto r5 = t2.create(); *r5 = "1", r5.commit();
					auto r6 = t2.create(); *r6 = "17", r6.commit();

					auto i1 = t1.begin();
					auto ci1 = ct1.begin();
					auto i2 = t2.begin();
					auto ci2 = ct2.begin();

					++i1, ++ci1;
					++i2, ++ci2;

					// ACT
					i1 = t1.begin();
					ci1 = ct1.begin();
					i2 = t2.begin();
					ci2 = ct2.begin();

					// ASSERT
					assert_equal(1211, (table1_t::reference)**i1);
					assert_equal(1211, (table1_t::const_reference)*ci1);
					assert_equal("1211", (table2_t::reference)**i2);
					assert_equal("1211", (table2_t::const_reference)*ci2);
				}


				test( TableIsEmptyAfterClear )
				{
					typedef table< string, function<string ()> > table_t;

					// INIT
					table_t t([] {	return "";	});

					auto r1 = t.create(); *r1 = "1211", r1.commit();
					auto r2 = t.create(); *r2 = "1", r2.commit();
					auto r3 = t.create(); *r3 = "17", r3.commit();

					// ACT
					t.clear();

					// ASSERT
					assert_equal(t.end(), t.begin());
				}


				test( NotificationIsSentOnClear )
				{
					typedef table< string, function<string ()> > table_t;

					// INIT
					table_t t([] {	return "";	});

					auto r1 = t.create(); *r1 = "1211", r1.commit();
					auto r2 = t.create(); *r2 = "1", r2.commit();
					auto r3 = t.create(); *r3 = "17", r3.commit();
					auto called = 0;
					auto c = t.cleared += [&] {
						assert_equal(t.end(), t.begin());
						called++;
					};

					// ACT
					t.clear();

					// ASSERT
					assert_equal(1, called);
				}
			end_test_suite
		}
	}
}
