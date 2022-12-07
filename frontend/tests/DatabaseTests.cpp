#include <frontend/sql_database.h>

#include <frontend/sql_types.h>
#include <frontend/constructors.h>
#include <sqlite3.h>
#include <test-helpers/helpers.h>
#include <test-helpers/file_helpers.h>
#include <tuple>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace sql
	{
		namespace tests
		{
			using namespace micro_profiler::tests;

			namespace
			{
				const auto c_create_sample_1 =
					"BEGIN;"

					"CREATE TABLE 'lorem_ipsums' ('name' TEXT, 'age' INTEGER);"
					"INSERT INTO 'lorem_ipsums' VALUES ('lorem', 3141);"
					"INSERT INTO 'lorem_ipsums' VALUES ('Ipsum', 314159);"
					"INSERT INTO 'lorem_ipsums' VALUES ('Lorem Ipsum Amet Dolor', 314);"
					"INSERT INTO 'lorem_ipsums' VALUES ('lorem', 31415926);"

					"CREATE TABLE 'other_lorem_ipsums' ('name' TEXT, 'age' INTEGER);"
					"INSERT INTO 'other_lorem_ipsums' VALUES ('Lorem Ipsum', 13);"
					"INSERT INTO 'other_lorem_ipsums' VALUES ('amet dolor', 191);"

					"CREATE TABLE 'reordered_lorem_ipsums' ('age' INTEGER, 'nickname' TEXT, 'name' TEXT);"
					"INSERT INTO 'reordered_lorem_ipsums' VALUES (3141, 'Bob', 'lorem');"
					"INSERT INTO 'reordered_lorem_ipsums' VALUES (314159, 'AJ', 'Ipsum');"
					"INSERT INTO 'reordered_lorem_ipsums' VALUES (314, 'Liz', 'Lorem Ipsum Amet Dolor');"
					"INSERT INTO 'reordered_lorem_ipsums' VALUES (314, 'K', 'lorem');"
					"INSERT INTO 'reordered_lorem_ipsums' VALUES (31415926, 'K', 'lorem');"

					"CREATE TABLE 'sample_items_1' ('a' INTEGER, 'b' TEXT);"
					"CREATE TABLE 'sample_items_2' ('age' INTEGER, 'nickname' TEXT, 'name' TEXT);"
					"CREATE TABLE 'sample_items_3' ('MyID' INTEGER PRIMARY KEY ASC, 'a' INTEGER, 'b' TEXT, 'c' INTEGER, 'd' REAL, 'e' INTEGER, 'f' INTEGER);"

					"COMMIT;";

				struct test_a
				{
					string name;
					int age;

					bool operator <(const test_a &rhs) const
					{	return make_pair(name, age) < make_pair(rhs.name, rhs.age);	}
				};

				struct test_b
				{
					string nickname;
					int suspect_age;
					string suspect_name;

					bool operator <(const test_b& rhs) const
					{
						return make_tuple(nickname, suspect_age, suspect_name)
							< make_tuple(rhs.nickname, rhs.suspect_age, rhs.suspect_name);
					}
				};

				struct sample_item_1
				{
					int a;
					string b;

					bool operator <(const sample_item_1& rhs) const
					{	return make_tuple(a, b) < make_tuple(rhs.a, rhs.b);	}
				};

				struct sample_item_3
				{
					int id;
					int a;
					string b;
					int64_t c;
					double d;
					uint64_t e;
					unsigned int f;

					bool operator <(const sample_item_3& rhs) const
					{
						auto approx_less = [] (double lhs, double rhs) {
							return lhs < rhs - 0.000001 * (rhs - lhs);
						};
						auto tlhs = make_tuple(id, a, b, c, e, f);
						auto trhs = make_tuple(rhs.id, rhs.a, rhs.b, rhs.c, rhs.e, rhs.f);

						if (tlhs < trhs)
							return true;
						if (trhs < tlhs)
							return false;
						return approx_less(d, rhs.d);
					}
				};

				template <typename VisitorT>
				void describe(VisitorT &visitor, test_a *)
				{
					visitor(&test_a::name, "name");
					visitor(&test_a::age, "age");
				}

				template <typename VisitorT>
				void describe(VisitorT& visitor, test_b *)
				{
					visitor(&test_b::suspect_name, "name");
					visitor(&test_b::suspect_age, "age");
					visitor(&test_b::nickname, "nickname");
				}

				template <typename VisitorT>
				void describe(VisitorT &visitor, sample_item_1 *)
				{
					visitor(&sample_item_1::a, "a");
					visitor(&sample_item_1::b, "b");
				}

				template <typename VisitorT>
				void describe(VisitorT &visitor, sample_item_3 *)
				{
					visitor(pk(&sample_item_3::id), "MyID");
					visitor(&sample_item_3::a, "a");
					visitor(&sample_item_3::b, "b");
					visitor(&sample_item_3::c, "c");
					visitor(&sample_item_3::d, "d");
					visitor(&sample_item_3::e, "e");
					visitor(&sample_item_3::f, "f");
				}


				template <typename T>
				vector<T> read_all(string path, const char *table_name)
				{
					T item;
					vector<T> result;
					transaction c(create_conneciton(path.c_str()));
					auto r = c.select<T>(table_name);

					while (r(item))
						result.push_back(item);
					return result;
				}
			}

			begin_test_suite( DatabaseTests )
				temporary_directory dir;
				sqlite3 *database;
				string path;

				init( CreateSamples )
				{
					path = dir.track_file("sample-db.db");
					sqlite3_open_v2(path.c_str(), &database,
						SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
					sqlite3_exec(database, c_create_sample_1, nullptr, nullptr, nullptr);
				}

				teardown( Release )
				{
					sqlite3_close(database);
				}


				test( ValuesInTablesCanBeRead )
				{
					// INIT / ACT
					test_a a;
					vector<test_a> results_a;
					transaction t(create_conneciton(path.c_str()));

					auto r = t.select<test_a>("lorem_ipsums");

					// ACT
					while (r(a))
						results_a.push_back(a);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_a>("lorem", 3141)
						+ initialize<test_a>("Ipsum", 314159)
						+ initialize<test_a>("Lorem Ipsum Amet Dolor", 314)
						+ initialize<test_a>("lorem", 31415926), results_a);

					// INIT
					results_a.clear();

					// INIT / ACT
					auto r2 = t.select<test_a>("other_lorem_ipsums");

					// ACT
					while (r2(a))
						results_a.push_back(a);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_a>("Lorem Ipsum", 13)
						+ initialize<test_a>("amet dolor", 191), results_a);

					// INIT
					test_b b;
					vector<test_b> results_b;

					// INIT / ACT
					auto rb = t.select<test_b>("reordered_lorem_ipsums");

					// ACT
					while (rb(b))
						results_b.push_back(b);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_b>("Bob", 3141, "lorem")
						+ initialize<test_b>("AJ", 314159, "Ipsum")
						+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor")
						+ initialize<test_b>("K", 314, "lorem")
						+ initialize<test_b>("K", 31415926, "lorem"), results_b);
				}


				test( ValuesInTablesCanBeReadForOtherOrder )
				{
					// INIT / ACT
					test_a a;
					vector<test_a> results_a;
					transaction t(create_conneciton(path.c_str()));

					auto r = t.select<test_a>("reordered_lorem_ipsums");

					// ACT
					while (r(a))
						results_a.push_back(a);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_a>("lorem", 3141)
						+ initialize<test_a>("Ipsum", 314159)
						+ initialize<test_a>("Lorem Ipsum Amet Dolor", 314)
						+ initialize<test_a>("lorem", 314)
						+ initialize<test_a>("lorem", 31415926), results_a);
				}


				test( ValuesInTableCanBeReadWithAQuery )
				{
					// INIT / ACT
					test_b b;
					vector<test_b> results;
					transaction t(create_conneciton(path.c_str()));

					// INIT / ACT
					auto r = t.select<test_b>("reordered_lorem_ipsums", c(&test_b::suspect_age) == p<const int>(314));

					// ACT
					while (r(b))
						results.push_back(b);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_b>("K", 314, "lorem")
						+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor"), results);

					// INIT
					vector<test_b> results2;

					results.clear();

					// INIT / ACT
					auto r2 = t.select<test_b>("reordered_lorem_ipsums",
						c(&test_b::suspect_age) == p<const int>(314) || c(&test_b::suspect_name) == p<const string>("Ipsum"));
					auto r3 = t.select<test_b>("reordered_lorem_ipsums",
						c(&test_b::suspect_name) == p<const string>("Ipsum") || c(&test_b::suspect_age) == p<const int>(314));

					// ACT
					while (r2(b))
						results.push_back(b);

					while (r3(b))
						results2.push_back(b);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_b>("AJ", 314159, "Ipsum")
						+ initialize<test_b>("K", 314, "lorem")
						+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor"), results);
					assert_equivalent(results, results2);
				}


				test( InsertedRecordsAreNotVisibleBeforeCommit )
				{
					// INIT
					sample_item_1 item = {	314, "Bob Marley"	};
					transaction t(create_conneciton(path.c_str()));

					// INIT / ACT
					auto w1 = t.insert<sample_item_1>("sample_items_1");

					// ACT
					w1(item);

					// ASSERT
					assert_is_empty(read_all<sample_item_1>(path, "sample_items_1"));
				}


				test( InsertedRecordsCanBeRead )
				{
					// INIT
					sample_item_1 items1[] = {
						{	314, "Bob Marley"	},
						{	141, "Peter Tosh"	},
						{	3141, "John Zorn"	},
					};
					transaction t(create_conneciton(path.c_str()));

					// INIT / ACT
					auto w1 = t.insert<sample_item_1>("sample_items_1");

					// ACT
					for (auto i = begin(items1); i != end(items1); ++i)
						w1(static_cast<const sample_item_1 &>(*i));
					t.commit();

					// ASSERT
					assert_equivalent(items1, read_all<sample_item_1>(path, "sample_items_1"));

					// INIT
					test_b items2[] = {
						{	"Bob", 3141, "lorem"	},
						{	"AJ", 314159, "Ipsum"	},
						{	"Liz", 314, "Lorem Ipsum Amet Dolor"	},
						{	"K", 314, "lorem"	},
						{	"K", 31415926, "lorem"	},
					};

					// INIT / ACT
					transaction t2(create_conneciton(path.c_str()));
					auto w2 = t2.insert<test_b>("sample_items_2");

					// ACT
					for (auto i = begin(items2); i != end(items2); ++i)
						w2(*i);
					t2.commit();

					// ASSERT
					assert_equivalent(items2, read_all<test_b>(path, "sample_items_2"));
				}


				test( InnerTransactionsAreProhibited )
				{
					// INIT
					auto conn = create_conneciton(path.c_str());

					// INIT / ACT
					auto locker = make_shared<transaction>(conn);

					// ACT / ASSERT
					assert_throws(transaction contender(conn), sql_error);

					// ACT
					locker.reset();

					// ACT / ASSERT
					transaction noncontender(conn);
				}


				test( TransactionsCombineAccordinglyToType )
				{
					// INIT
					auto conn1 = create_conneciton(path.c_str());
					auto conn2 = create_conneciton(path.c_str());
					auto conn3 = create_conneciton(path.c_str());

					// ACT
					{
						transaction locker(conn1, transaction::immediate);

					// ACT / ASSERT
						assert_throws(transaction contender(conn2, transaction::immediate, 400), sql_error);

					// ACT / ASSERT (does not throw)
						transaction non_contended(conn2, transaction::deferred, 2000);
					}

					// ACT
					{
						transaction nonlocker(conn1, transaction::deferred);

						// ACT / ASSERT (does not throw)
						transaction non_contender1(conn2, transaction::immediate, 2000);
						transaction non_contended2(conn3, transaction::deferred, 2000);
					}
				}


				test( PrimaryKeyIsSetUponInsertion )
				{
					// INIT
					transaction t(create_conneciton(path.c_str()));
					auto w = t.insert<sample_item_3>("sample_items_3");

					// ACT
					for (int n = 1000, previous = 0; n--; )
					{
						sample_item_3 item = {};

						w(item);

					// ASSERT
						if (previous)
							assert_equal(previous + 1, item.id);

						previous = item.id;
					}
				}


				test( AllSupportedTypesCanBeSelected )
				{
					// INIT
					vector<sample_item_3> items_read;
					sample_item_3 items[] = {
						{	100, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
						{	100, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
						{	100, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
					};
					transaction t(create_conneciton(path.c_str()));
					auto w = t.insert<sample_item_3>("sample_items_3");

					for (auto i = begin(items); i != end(items); ++i)
						w(*i);

					// ACT
					auto r = t.select<sample_item_3>("sample_items_3");

					for (sample_item_3 item; r(item); )
						items_read.push_back(item);

					// ASSERT
					sample_item_3 reference1[] = {
						{	1, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
						{	2, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
						{	3, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
					};

					assert_equivalent(reference1, items_read);

					// INIT
					sample_item_3 items2[] = {
						{	100, 0, "Jimi Hendrix", 30000000001, 1.5391, 0, 0	},
						{	100, 0, "Tom Waits", 70000000002, 3.141e-8, 0, 0	},
					};

					for (auto i = begin(items2); i != end(items2); ++i)
						w(*i);
					items_read.clear();

					// ACT
					auto r2 = t.select<sample_item_3>("sample_items_3");

					for (sample_item_3 item; r2(item); )
						items_read.push_back(item);

					// ASSERT
					sample_item_3 reference2[] = {
						{	1, 0, "Bod Dylan", 10000000001, 1.5391, 0x8912323200000001, 0xB0000000	},
						{	2, 110, "Nick Cave", 10000000002, 1e-8, 0xF912323200000001, 0x00000000	},
						{	3, 13, "Robert Fripp", 10000000001, 1.5e12, 0x1912323200000001, 0x10000000	},
						{	4, 0, "Jimi Hendrix", 30000000001, 1.5391, 0, 0	},
						{	5, 0, "Tom Waits", 70000000002, 3.141e-8, 0, 0	},
					};

					assert_equivalent(reference2, items_read);
				}
			end_test_suite
		}
	}
}
