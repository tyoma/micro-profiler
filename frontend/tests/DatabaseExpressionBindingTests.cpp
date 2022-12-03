#include <frontend/sql_parameters.h>

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
				string format(const E &e, unsigned int &index)
				{
					string non_empty = "abc abc";
					auto previous_index = index;
					string value;

					e.format(non_empty, index);
					e.format(value, previous_index);
					assert_equal("abc abc" + value, non_empty); // format() appends.
					return value;
				}

				template <typename BuilderT>
				void describe(BuilderT builder, person *)
				{
					builder(&person::last_name, "last_name");
					builder(&person::first_name, "FirstName");
					builder(&person::year, "YearOfBirth");
					builder(&person::month, "Month");
					builder(&person::day, "Day");
				}

				template <typename BuilderT>
				void describe(BuilderT &builder, company *)
				{
					builder(&company::name, "CompanyName");
					builder(&company::year_founded, "Founded");
				}
			}

			begin_test_suite( DatabaseExpressionBindingTests )
				test( ParametersAreFormattedAsExpressions )
				{
					// INIT
					int val1 = 123;
					int val2 = 31;
					int64_t val3 = 123000000000;
					unsigned int index = 102;
					
					// INIT / ACT
					auto parm1 = p(val1);
					auto parm2 = p(val2);
					auto parm3 = p(val3);

					// ACT / ASSERT
					assert_equal(":102", format(parm1, index));
					assert_equal(":103", format(parm1, index));
					assert_equal(":104", format(parm2, index));
					assert_equal(":105", format(parm3, index));
					assert_equal(106u, index);
				}


				test( ColumnsAreFormattedBasedOnSqlDescription )
				{
					// INIT / ACT
					unsigned int index = 213;
					auto col1 = c(&person::first_name);
					auto col2 = c(&person::last_name);
					auto col3 = c(&person::day);
					auto col4 = c(&company::name);
					auto col5 = c(&company::year_founded);

					// ACT / ASSERT
					assert_equal("FirstName", format(col1, index));
					assert_equal("last_name", format(col2, index));
					assert_equal("Day", format(col3, index));
					assert_equal("CompanyName", format(col4, index));
					assert_equal("Founded", format(col5, index));
					assert_equal(213u, index);
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
					parm1.bind([&] (const int &v) {
						assert_equal(&val1, &v);
						called = true;
					});

					// ASSERT
					assert_is_true(called);

					// INIT
					called = false;

					// ACT / ASSERT
					parm2.bind([&] (const int &v) {
						assert_equal(&val2, &v);
						called = true;
					});

					// ASSERT
					assert_is_true(called);

					// INIT
					called = false;

					// ACT / ASSERT
					parm3.bind([&] (const int64_t &v) {
						assert_equal(&val3, &v);
						called = true;
					});

					// ASSERT
					assert_is_true(called);
				}


				test( EqualityOperatorFormatsBothExpressions )
				{
					// INIT
					unsigned int index1 = 11, index2 = 100;
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT / ASSERT
					assert_equal(":11 = :12", format(p(val1) == p(val2), index1));
					assert_equal(13u, index1);
					assert_equal(":100 = :101", format(p(val3) == p(val4), index2));
					assert_equal(102u, index2);
				}


				test( InequalityOperatorFormatsBothExpressions )
				{
					// INIT
					unsigned int index1 = 11, index2 = 100;
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT / ASSERT
					assert_equal(":11 <> :12", format(p(val1) != p(val2), index1));
					assert_equal(13u, index1);
					assert_equal(":100 <> :101", format(p(val3) != p(val4), index2));
					assert_equal(102u, index2);
				}


				test( LogicalOperatorsAreFormattedAppropriately )
				{
					// INIT
					unsigned int index = 0;
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT / ASSERT
					assert_equal(":0 AND :1", format(p(val1) && p(val2), index));
					assert_equal(":2 AND :3", format(p(val3) && p(val4), index));
					assert_equal(":4 OR :5", format(p(val1) || p(val2), index));
					assert_equal(":6 OR :7", format(p(val3) || p(val4), index));
				}


				test( ComplexExpressionsAreFormattedAppropriately )
				{
					// INIT
					unsigned int index = 0;
					int val1 = 123;
					int val2 = 31;
					string val3 = "test";
					string val4 = "test2";

					// ACT / ASSERT
					assert_equal(":0 = :1 AND :2", format(p(val1) == p(val2) && p(val2), index));
					assert_equal(":3 = :4 OR :5 <> :6", format(p(val1) == p(val2) || p(val3) != p(val4), index));
					assert_equal(":7 = YearOfBirth OR last_name <> :8", format(p(val2) == c(&person::year) || c(&person::last_name) != p(val4), index));
				}

			end_test_suite
		}
	}
}
