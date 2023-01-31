#include <sqlite++/expression.h>
#include <sqlite++/format.h>

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
					string value;

					format_expression(non_empty, e);
					format_expression(value, e);
					assert_equal("abc abc" + value, non_empty); // format() appends.
					return value;
				}

				template <typename VisitorT>
				void describe(VisitorT &&visitor, person *)
				{
					visitor(&person::last_name, "last_name");
					visitor(&person::first_name, "FirstName");
					visitor(&person::year, "YearOfBirth");
					visitor(&person::month, "Month");
					visitor(&person::day, "Day");
				}

				template <typename VisitorT>
				void describe(VisitorT &&visitor, company *)
				{
					visitor(&company::name, "CompanyName");
					visitor(&company::year_founded, "Founded");
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
					
					// INIT / ACT
					auto parm1 = p(val1);
					auto parm2 = p(val2);
					auto parm3 = p(val3);

					// ACT / ASSERT
					assert_equal(":1", (format_expression(result, parm1), result));
					assert_equal(":1:1", (format_expression(result, parm1), result));
					assert_equal(":1:1:1", (format_expression(result, parm2), result));
					assert_equal(":1:1:1:1", (format_expression(result, parm3), result));
				}


				test( ColumnsAreFormattedBasedOnSqlDescription )
				{
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

					// ACT / ASSERT
					assert_equal(&val1, &parm1.object);
					assert_equal(&val2, &parm2.object);
					assert_equal(&val3, &parm3.object);
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
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT
					format_expression(result,
						p(val1) == p(val2) && p(val3) == p(val4)
					);

					// ASSERT
					assert_equal(":1=:2 AND :3=:4", result);

					// INIT
					result.clear();

					// ACT
					format_expression(result,
						p(val1) == p(val2) || p(val3) != p(val4)
					);

					// ASSERT
					assert_equal(":1=:2 OR :3<>:4", result);

					// INIT
					result.clear();

					// ACT
					format_expression(result,
						p(val2) == c(&person::year) || c(&person::last_name) != p(val4)
					);

					// ASSERT
					assert_equal(":1=YearOfBirth OR last_name<>:2", result);
				}

			end_test_suite
		}
	}
}
