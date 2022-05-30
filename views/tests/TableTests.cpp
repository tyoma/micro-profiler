#include <views/table.h>

#include "helpers.h"

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

				template <typename T>
				struct component_constructor
				{
					T *operator ()() const
					{	return new T;	}
				};

				enum event_type {	created_, modified_, removed_, cleared_	};

				template <typename TableT, int unique>
				class logging_component : public table_component<typename TableT::const_iterator>
				{
				public:
					explicit logging_component(vector< tuple<typename TableT::const_iterator, event_type, int> > &log)
						: _log(log)
					{	}

				private:
					virtual void created(typename TableT::const_iterator record) override
					{	_log.push_back(make_tuple(record, created_, unique));	}

					virtual void modified(typename TableT::const_iterator record) override
					{	_log.push_back(make_tuple(record, modified_, unique));	}

				private:
					vector< tuple<typename TableT::const_iterator, event_type, int> > &_log;
				};

				template <typename TableT, int unique>
				class removal_logging_component : public table_component<typename TableT::const_iterator>
				{
				public:
					explicit removal_logging_component(function<void (typename TableT::const_iterator, int)> on_removed)
						: _on_removed(on_removed)
					{	}

				private:
					virtual void removed(typename TableT::const_iterator record) override {	_on_removed(record, unique);	}

				private:
					function<void (typename TableT::const_iterator, int)> _on_removed;
				};

				template <typename TableT, int unique>
				class cleared_logging_component : public table_component<typename TableT::const_iterator>
				{
				public:
					explicit cleared_logging_component(vector<int> &log)
						: _log(log)
					{	}

				private:
					virtual void cleared() override {	_log.push_back(unique);	}

				private:
					vector<int> &_log;
				};



				template <typename T, typename CC>
				typename component_type<CC>::type &get_component(T &table_, CC factory)
				{	return table_.component(factory);	}

				template <int unique, typename T, typename OnRemovedT>
				removal_logging_component<T, unique> &get_removal_component(T &table_, OnRemovedT on_removed)
				{	return get_component(table_, [on_removed] {	return new removal_logging_component<T, unique>(on_removed);	});	}

				template <int unique, typename T>
				cleared_logging_component<T, unique> &get_cleared_component(T &table_, vector<int> &log)
				{	return get_component(table_, [&log] {	return new cleared_logging_component<T, unique>(log);	});	}
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

					auto c1 = t1.changed += [] (table_t::const_iterator, bool) {	assert_is_false(true);	};
					auto c2 = t2.changed += [] (table_t::const_iterator, bool) {	assert_is_false(true);	};

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
					auto c = t.changed += [&] (table_t::const_iterator i, bool new_) {
						cl.push_back(i);
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
					assert_equal(1u, cl.size());
					assert_equal(13, (*cl[0]).id);
					assert_equal("loop", (*cl[0]).value);

					// ACT
					r1.commit();
					r3.commit();

					// ASSERT
					assert_equal(3u, cl.size());
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
					vector<table_t::const_iterator> l;
					auto c = t.changed += [&l] (table_t::const_iterator i, bool) {	l.push_back(i);	};

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
					assert_equal(1, (*i).id);
					assert_equal("lorem", (*i).value);
					++i;
					assert_not_equal(t.end(), i);
					assert_equal(2, (*i).id);
					assert_equal("ipsum", (*i).value);
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
					i = t.begin();
					++i, ++i;
					assert_not_equal(t.end(), i);
					assert_equal(3, (*i).id);
					assert_equal("amet-dolor", (*i).value);
					++i;
					assert_equal(t.end(), i);

					ci = t.begin();
					++ci, ++ci;
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
					vector<table_t::const_iterator> l;

					auto r1= t.create();
					r1.commit();
					auto r2= t.create();
					r2.commit();
					auto r3= t.create();
					r3.commit();
					auto i = t.begin();

					auto c = t.changed += [&] (table_t::const_iterator i, bool new_) {
						l.push_back(i);
						assert_is_false(new_);
					};

					// INIT / ACT
					auto mr1 = t.modify(i);
					++i, ++i;
					auto mr3 = t.modify(i);

					// ACT
					(*mr3).value = "zubzub";
					mr3.commit();

					// ASSERT
					assert_equal(1u, l.size());
					assert_equal(i, l[0]);
					assert_equal("zubzub", (*l[0]).value);

					// ACT
					(*mr1).value = "testtest";
					mr1.commit();

					// ASSERT
					assert_equal(2u, l.size());
					assert_equal(t.begin(), l[1]);
					assert_equal("testtest", (*l[1]).value);
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
					assert_equal(1211, (table1_t::const_reference)*ci1);
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
					auto c2 = t.invalidate += [&] {
						assert_equal(1, called); // 'invalidate' signal is raised after 'cleared'
						called++;
					};

					// ACT
					t.clear();

					// ASSERT
					assert_equal(2, called);
				}


				template <typename TableT>
				struct component_a : table_component<typename TableT::const_iterator>
				{
					component_a() {	}
					component_a(const component_a &) {	assert_is_false(true);	} // A component is always created in-place.

					virtual void created(typename TableT::const_iterator) override {	}
					virtual void modified(typename TableT::const_iterator) override {	}
					virtual void removed(typename TableT::const_iterator) override {	}
					virtual void cleared() override {	}
				};

				template <typename TableT>
				struct component_b : table_component<typename TableT::const_iterator>
				{
					component_b() {	}
					component_b(const component_b &) {	assert_is_false(true);	} // A component is always created in-place.

					virtual void created(typename TableT::const_iterator) override {	}
					virtual void modified(typename TableT::const_iterator) override {	}
					virtual void removed(typename TableT::const_iterator) override {	}
					virtual void cleared() override {	}
				};

				test( TableComponentIsCreatedOnFirstAccessViaBypassConstructor )
				{
					typedef table<string> table1_t;
					typedef table<int> table2_t;

					// INIT
					table1_t t1;
					const table1_t t2;
					table2_t t3;
					const table2_t t4;

					// ACT
					component_a<table1_t> &c1 = t1.component(component_constructor< component_a<table1_t> >());
					const component_a<const table1_t> &c2 = t2.component(component_constructor< component_a<const table1_t> >());
					component_b<table1_t> &c3 = t1.component(component_constructor< component_b<table1_t> >());
					const component_b<const table1_t> &c4 = t2.component(component_constructor< component_b<const table1_t> >());
					component_a<table2_t> &c5 = t3.component(component_constructor< component_a<table2_t> >());
					const component_a<const table2_t> &c6 = t4.component(component_constructor< component_a<const table2_t> >());

					// ASSERT (basically a compile-time test)
					assert_not_null(&c1);
					assert_not_null(&c2);
					assert_not_null(&c3);
					assert_not_null(&c4);
					assert_not_null(&c5);
					assert_not_null(&c6);
				}


				test( SameComponentIsReturnedOnConsequentAccesses )
				{
					typedef table<string> table_t;

					// INIT
					table_t t1, t2;

					// ACT
					component_a<const table_t> &c1 = t1.component(component_constructor< component_a<const table_t> >());
					component_a<const table_t> &c2 = t1.component(component_constructor< component_a<const table_t> >());
					const component_a<const table_t> &c3 = static_cast<const table_t &>(t1).component(component_constructor< component_a<const table_t> >());
					const component_a<const table_t> &c4 = static_cast<const table_t &>(t2).component(component_constructor< component_a<const table_t> >());
					component_a<const table_t> &c5 = t2.component(component_constructor< component_a<const table_t> >());
					const component_a<const table_t> &c6 = static_cast<const table_t &>(t2).component(component_constructor< component_a<const table_t> >());

					// ASSERT
					assert_equal(&c1, &c2);
					assert_equal(&c1, &c3);
					assert_equal(&c4, &c5);
					assert_equal(&c4, &c6);
				}


				test( ConstructorIsOnlyCalledOnce )
				{
					typedef table<string> table_t;
					typedef component_a<const table_t> comp;

					// INIT
					vector<int> log;
					table_t t1, t2;

					// ACT
					t1.component([&] {	return log.push_back(1), new comp();	});
					static_cast<const table_t &>(t1).component([&] {	return log.push_back(1), new comp();	});
					t1.component([&] {	return log.push_back(1), new comp();	});

					// ASSERT
					int reference1[] = {	1,	};

					assert_equal(reference1, log);

					// ACT
					t2.component([&] {	return log.push_back(2), new comp();	});

					// ASSERT
					int reference2[] = {	1, 2,	};

					assert_equal(reference2, log);

					// ACT
					t2.component([&] {	return log.push_back(2), new comp();	});

					// ASSERT
					assert_equal(reference2, log);
				}


				test( RemovingItemsSkipsTheFromEnumeration )
				{
					typedef table<string> table_t;

					// INIT
					table_t t;

					add_record(t, "lorem");
					add_record(t, "ipsum");
					add_record(t, "amet");
					add_record(t, "dolor");
					add_record(t, "foo");
					add_record(t, "bar");

					auto i = t.begin();
					auto i1 = (++i, i);
					auto i3 = (++i, ++i, i);

					// ACT
					auto r3 = t.modify(i3);
					r3.remove();

					// ASSERT
					string reference1[] = {	"lorem", "ipsum", "amet", "foo", "bar",	};

					assert_equal(reference1, t);

					// ACT
					auto r1 = t.modify(i1);
					r1.remove();

					// ASSERT
					string reference2[] = {	"lorem", "amet", "foo", "bar",	};

					assert_equal(reference2, t);
				}


				test( IteratorsArePreservedAfterRemoval )
				{
					typedef table<string> table_t;
					typedef table_t::const_iterator const_iterator;

					// INIT
					table_t t;

					add_record(t, "lorem");
					add_record(t, "ipsum");
					add_record(t, "amet");
					add_record(t, "dolor");
					add_record(t, "foo");
					add_record(t, "bar");

					auto i = t.begin();
					auto i0 = i;
					auto i1 = (++i, i);
					auto i2 = (++i, i);
					auto i3 = (++i, i);
					auto i4 = (++i, i);
					auto i5 = (++i, i);

					// ACT
					auto r3 = t.modify(i3);
					r3.remove();

					// ASSERT
					assert_equal("lorem", *(const_iterator)i0);
					assert_equal("ipsum", *(const_iterator)i1);
					assert_equal("amet", *(const_iterator)i2);
					assert_equal("foo", *(const_iterator)i4);
					assert_equal("bar", *(const_iterator)i5);

					// ACT
					auto r1 = t.modify(i1);
					r1.remove();

					// ASSERT
					assert_equal("lorem", *(const_iterator)i0);
					assert_equal("amet", *(const_iterator)i2);
					assert_equal("foo", *(const_iterator)i4);
					assert_equal("bar", *(const_iterator)i5);
				}


				test( RemovalNotificationIsSentBeforeTheRecordIsRemoved )
				{
					typedef table<string> table_t;
					typedef table_t::const_iterator const_iterator;

					// INIT
					auto called = 0;
					table_t t;
					string data[] = {	"lorem", "ipsum", "amet", "dolor", "foo", "bar",	};

					add_records(t, data);

					auto i = t.begin();
					auto i2 = (++i, ++i, i);
					auto i5 = (++i, ++i, ++i, i);

					auto c = t.removed += [&] (table_t::const_iterator i) {
						assert_equal(i2, i);
						assert_equal("amet", *(const_iterator)i);
						called++;
					};

					// ACT
					auto r2 = t.modify(i2);
					r2.remove();

					// ASSERT
					assert_equal(1, called);

					// INIT
					c = t.removed += [&] (table_t::const_iterator i) {
						assert_equal(i5, i);
						assert_equal("bar", *(const_iterator)i);
						called++;
					};

					// ACT
					auto r5 = t.modify(i5);
					r5.remove();

					// ASSERT
					assert_equal(2, called);
				}


				test( ComponentsAreNotifiedOfTableEvents )
				{
					// INIT
					string data1[] = {	"lorem", "ipsum", "amet", "dolor",	};
					string data2[] = {	"foo", "bar",	};
					table<string> t;
					vector< tuple<table<string>::const_iterator, event_type, int> > log;
					
					// INIT / ACT
					get_component(t, [&log] {	return new logging_component<table<string>, 0>(log);	});

					auto iterators1 = add_records(t, data1);

					// ASSERT
					tuple<table<string>::const_iterator, event_type, int> reference1[] = {
						make_tuple(iterators1[0], created_, 0),
						make_tuple(iterators1[1], created_, 0),
						make_tuple(iterators1[2], created_, 0),
						make_tuple(iterators1[3], created_, 0),
					};

					assert_equal(reference1, log);

					// INIT
					log.clear();

					// INIT / ACT
					auto iterators2 = add_records(t, data2);

					// ASSERT
					tuple<table<string>::const_iterator, event_type, int> reference2[] = {
						make_tuple(iterators2[0], created_, 0),
						make_tuple(iterators2[1], created_, 0),
					};

					assert_equal(reference2, log);

					// INIT
					log.clear();

					// ACT
					auto r4 = t.modify(iterators2[0]);
					auto r2 = t.modify(iterators1[2]);

					*r2 = "yyy";
					*r4 = "xxx";
					r4.commit();
					r2.commit();

					// ASSERT
					tuple<table<string>::const_iterator, event_type, int> reference3[] = {
						make_tuple(iterators2[0], modified_, 0),
						make_tuple(iterators1[2], modified_, 0),
					};

					assert_equal(reference3, log);

					// INIT
					auto removals = 0;
					table<string>::const_iterator expected;

					log.clear();
					get_removal_component<0>(t, [&] (table<string>::const_iterator record, int) {
						removals++;

					// ASSERT
						assert_equal(expected, record);
					});

					// ACT
					auto r4_ = t.modify(iterators2[0]);
					auto r3 = t.modify(iterators1[3]);

					// ACT
					expected = iterators1[3];
					r3.remove();

					// ACT
					expected = iterators2[0];
					r4_.remove();

					// ASSERT
					assert_is_empty(log);
					assert_equal(2, removals);
				}


				test( CreationsAreCalledInTheOrderOfComponentAdditions )
				{
					// INIT
					table<string> t;
					vector< tuple<table<string>::const_iterator, event_type, int> > log;
					
					// INIT / ACT
					auto conn = t.changed += [&] (table<string>::const_iterator record, bool new_) {
						log.push_back(make_tuple(record, new_ ? created_ : modified_, 100));
					};

					get_component(t, [&log] {	return new logging_component<table<string>, 0>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 1>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 2>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 3>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 4>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 5>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 6>(log);	});

					// ACT
					auto i1 = add_record(t, "abc");

					// ASSERT
					tuple<table<string>::const_iterator, event_type, int> reference[] = {
						make_tuple(i1, created_, 0),
						make_tuple(i1, created_, 1),
						make_tuple(i1, created_, 2),
						make_tuple(i1, created_, 3),
						make_tuple(i1, created_, 4),
						make_tuple(i1, created_, 5),
						make_tuple(i1, created_, 6),
						make_tuple(i1, created_, 100),
					};

					assert_equal(reference, log);
				}


				test( ModificationsAreCalledInTheOrderOfComponentAdditions )
				{
					// INIT
					table<string> t;
					vector< tuple<table<string>::const_iterator, event_type, int> > log;
					auto i1 = add_record(t, "abc");

					// INIT / ACT
					auto conn = t.changed += [&] (table<string>::const_iterator record, bool new_) {
						log.push_back(make_tuple(record, new_ ? created_ : modified_, 100));
					};

					get_component(t, [&log] {	return new logging_component<table<string>, 0>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 1>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 2>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 3>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 4>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 5>(log);	});
					get_component(t, [&log] {	return new logging_component<table<string>, 6>(log);	});

					// ACT
					auto r = t.modify(i1);

					r.commit();

					// ASSERT
					tuple<table<string>::const_iterator, event_type, int> reference[] = {
						make_tuple(i1, modified_, 0),
						make_tuple(i1, modified_, 1),
						make_tuple(i1, modified_, 2),
						make_tuple(i1, modified_, 3),
						make_tuple(i1, modified_, 4),
						make_tuple(i1, modified_, 5),
						make_tuple(i1, modified_, 6),
						make_tuple(i1, modified_, 100),
					};

					assert_equal(reference, log);
				}


				test( RemovalsAreCalledInTheReversedOrderOfComponentAdditions )
				{
					// INIT
					table<string> t;
					vector< tuple<table<string>::const_iterator, event_type, int> > log;
					auto removals = 0;
					auto i1 = add_record(t, "abc");

					// INIT / ACT
					auto conn = t.removed += [&] (table<string>::const_iterator) {
						assert_equal(7, removals++);
					};

					get_removal_component<0>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(6, removals++);
					});
					get_removal_component<1>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(5, removals++);
					});
					get_removal_component<2>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(4, removals++);
					});
					get_removal_component<3>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(3, removals++);
					});
					get_removal_component<4>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(2, removals++);
					});
					get_removal_component<5>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(1, removals++);
					});
					get_removal_component<6>(t, [&] (table<string>::const_iterator, int) {
						assert_equal(0, removals++);
					});

					// ACT
					auto r = t.modify(i1);

					r.remove();

					// ASSERT
					assert_equal(8, removals);
				}


				test( ClearsAreCalledInTheReversedOrderOfComponentAdditions )
				{
					// INIT
					table<string> t;
					vector<int> log;
					
					add_record(t, "abc");

					// INIT / ACT
					auto conn = t.cleared += [&] {	log.push_back(100);	};

					get_cleared_component<0>(t, log);
					get_cleared_component<1>(t, log);
					get_cleared_component<2>(t, log);
					get_cleared_component<3>(t, log);
					get_cleared_component<4>(t, log);
					get_cleared_component<5>(t, log);

					// ACT
					t.clear();

					// ASSERT
					int reference1[] = {	100, 5, 4, 3, 2, 1, 0,	};

					assert_equal(reference1, log);

					// INIT
					get_cleared_component<6>(t, log);

					// ACT
					t.clear();

					// ASSERT
					int reference2[] = {	100, 5, 4, 3, 2, 1, 0, 100, 6, 5, 4, 3, 2, 1, 0,	};

					assert_equal(reference2, log);
				}

			end_test_suite
		}
	}
}
