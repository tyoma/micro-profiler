#include <frontend/sql_database.h>

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
					"INSERT INTO 'reordered_lorem_ipsums' VALUES (31415926, 'K', 'lorem');"

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

				template <typename BuilderT>
				void describe(BuilderT &builder, test_a &)
				{
					builder(&test_a::name, "name");
					builder(&test_a::age, "age");
				}

				template <typename BuilderT>
				void describe(BuilderT& builder, test_b&)
				{
					builder(&test_b::suspect_name, "name");
					builder(&test_b::suspect_age, "age");
					builder(&test_b::nickname, "nickname");
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
					connection conn(path);

					auto r = conn.select<test_a>("lorem_ipsums");

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
					auto r2 = conn.select<test_a>("other_lorem_ipsums");

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
					auto rb = conn.select<test_b>("reordered_lorem_ipsums");

					// ACT
					while (rb(b))
						results_b.push_back(b);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_b>("Bob", 3141, "lorem")
						+ initialize<test_b>("AJ", 314159, "Ipsum")
						+ initialize<test_b>("Liz", 314, "Lorem Ipsum Amet Dolor")
						+ initialize<test_b>("K", 31415926, "lorem"), results_b);
				}


				test( ValuesInTablesCanBeReadForOtherOrder )
				{
					// INIT / ACT
					test_a a;
					vector<test_a> results_a;
					connection conn(path);

					auto r = conn.select<test_a>("reordered_lorem_ipsums");

					// ACT
					while (r(a))
						results_a.push_back(a);

					// ASSERT
					assert_equivalent(plural
						+ initialize<test_a>("lorem", 3141)
						+ initialize<test_a>("Ipsum", 314159)
						+ initialize<test_a>("Lorem Ipsum Amet Dolor", 314)
						+ initialize<test_a>("lorem", 31415926), results_a);
				}
			end_test_suite
		}
	}
}
