#include <frontend/sql_expression.h>
#include <frontend/sql_format.h>

#include <cstdint>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace sql
	{
		namespace tests
		{
			namespace
			{
				struct person
				{
					string first_name, last_name;
					int year;
					int month;
					int day;
				};

				struct company
				{
					string name;
					int year_founded;
				};

				template <typename E>
				string format(const E &e)
				{
					string non_empty = "abc abc";
					format_visitor v1(non_empty);
					string value;
					format_visitor v2(value);

					e.visit(v1);
					e.visit(v2);
					assert_equal("abc abc" + value, non_empty); // format() appends.
					return value;
				}

				template <typename BuilderT>
				void describe(BuilderT &&builder, person *)
				{
					builder(&person::last_name, "last_name");
					builder(&person::first_name, "FirstName");
					builder(&person::year, "YearOfBirth");
					builder(&person::month, "Month");
					builder(&person::day, "Day");
				}

				template <typename BuilderT>
				void describe(BuilderT &&builder, company *)
				{
					builder(&company::name, "CompanyName");
					builder(&company::year_founded, "Founded");
				}
			}

			begin_test_suite( DatabaseExpressionTests )
				test( ParametersAreFormattedAsExpressions )
				{
					// INIT
					int val1 = 123;
					int val2 = 31;
					int64_t val3 = 123000000000;
					string result;
					format_visitor v(result);
					
					// INIT / ACT
					auto parm1 = p(val1);
					auto parm2 = p(val2);
					auto parm3 = p(val3);

					// ACT / ASSERT
					assert_equal(":1", (parm1.visit(v), result));
					assert_equal(":1:2", (parm1.visit(v), result));
					assert_equal(":1:2:3", (parm2.visit(v), result));
					assert_equal(":1:2:3:4", (parm3.visit(v), result));
					assert_equal(5u, v.next_index);
				}


				test( ColumnsAreFormattedBasedOnSqlDescription )
				{
					// INIT
					string result;
					format_visitor v(result);

					// INIT / ACT
					auto col1 = c(&person::first_name);
					auto col2 = c(&person::last_name);
					auto col3 = c(&person::day);
					auto col4 = c(&company::name);
					auto col5 = c(&company::year_founded);

					// ACT / ASSERT
					assert_equal("FirstName", format(col1));
					assert_equal("last_name", format(col2));
					assert_equal("Day", format(col3));
					assert_equal("CompanyName", format(col4));
					assert_equal("Founded", format(col5));

					// ACT (bound index stays intact)
					col1.visit(v);
					col3.visit(v);

					// ASSERT
					assert_equal(1u, v.next_index);
				}


				test( ParametersAreBoundAsValues )
				{
					// INIT
					int val1 = 123;
					int val2 = 31;
					int64_t val3 = 123000000000;
					auto parm1 = p(val1);
					auto parm2 = p(val2);
					auto parm3 = p(val3);
					auto called = false;

					// ACT / ASSERT
					parm1.visit([&] (const parameter<int> &v) {
						assert_equal(&val1, &v.object);
						called = true;
					});

					// ASSERT
					assert_is_true(called);

					// INIT
					called = false;

					// ACT / ASSERT
					parm2.visit([&] (const parameter<int> &v) {
						assert_equal(&val2, &v.object);
						called = true;
					});

					// ASSERT
					assert_is_true(called);

					// INIT
					called = false;

					// ACT / ASSERT
					parm3.visit([&] (const parameter<int64_t> &v) {
						assert_equal(&val3, &v.object);
						called = true;
					});

					// ASSERT
					assert_is_true(called);
				}


				test( EqualityOperatorFormatsBothExpressions )
				{
					// INIT
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT / ASSERT
					assert_equal(":1=:2", format(p(val1) == p(val2)));
					assert_equal(":1=:2", format(p(val3) == p(val4)));
				}


				test( InequalityOperatorFormatsBothExpressions )
				{
					// INIT
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT / ASSERT
					assert_equal(":1<>:2", format(p(val1) != p(val2)));
					assert_equal(":1<>:2", format(p(val3) != p(val4)));
				}


				test( LogicalOperatorsAreFormattedAppropriately )
				{
					// INIT
					auto val1 = false;
					auto val2 = true;
					auto val3 = true;
					auto val4 = false;

					// ACT / ASSERT
					assert_equal(":1 AND :2", format(p(val1) && p(val2)));
					assert_equal(":1 AND :2", format(p(val3) && p(val4)));
					assert_equal(":1 OR :2", format(p(val1) || p(val2)));
					assert_equal(":1 OR :2", format(p(val3) || p(val4)));
				}


				test( ComplexExpressionsAreFormattedAppropriately )
				{
					// INIT
					string result;
					format_visitor v(result);
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT
					(
						p(val1) == p(val2) && p(val3) == p(val4)
					).visit(v);

					// ASSERT
					assert_equal(":1=:2 AND :3=:4", result);

					// INIT
					result.clear();

					// ACT
					(
						p(val1) == p(val2) || p(val3) != p(val4)
					).visit(v);

					// ASSERT
					assert_equal(":5=:6 OR :7<>:8", result);

					// INIT
					result.clear();

					// ACT (moving visitor)
					(
						p(val2) == c(&person::year) || c(&person::last_name) != p(val4)
					).visit(format_visitor(result));

					// ASSERT
					assert_equal(":1=YearOfBirth OR last_name<>:2", result);
				}

			end_test_suite
		}
	}
}
