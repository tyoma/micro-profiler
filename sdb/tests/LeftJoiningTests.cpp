#include <sdb/transforms.h>

#include "comparisons.h"
#include "helpers.h"

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace sdb
{
	namespace tests
	{
		namespace
		{
			struct type_1
			{
				typedef int second_type;

				string text;
				second_type second;
			};

			struct type_2
			{
				typedef int first_type;

				first_type first;
				double period;
			};
		}

		begin_test_suite( LeftJoiningTests )
			test( LeftJoinedTableTypedAsExpected )
			{
				typedef table<type_1> table1_t;
				typedef table<type_2> table2_t;

				// INIT
				table1_t t1;
				table2_t t2;

				// INIT / ACT / ASSERT
				left_join<key_second, key_first>(t1, t2);
			}


			test( LeftJoinedTableHasSecondTableRecordAsOptional )
			{
				typedef pair<string, int> type1_t;
				typedef pair<int, string> type2_t;
				typedef pair<int, int> type3_t;
				typedef pair<int, double> type4_t;
				typedef table<type1_t> table1_t;
				typedef table<type2_t> table2_t;
				typedef table<type3_t> table3_t;
				typedef table<type4_t> table4_t;

				// INIT
				table1_t t1;
				table2_t t2;
				table3_t t3;
				table4_t t4;

				// INIT / ACT / ASSERT
				shared_ptr<const left_joined<table1_t, table2_t>::table_type> j1 = left_join<key_second, key_first>(t1, t2);
				shared_ptr<const left_joined<table2_t, table3_t>::table_type> j2 = left_join<key_first, key_first>(t2, t3);
				shared_ptr<const left_joined<table3_t, table4_t>::table_type> j3 = left_join<key_second, key_first>(t3, t4);

				// ASSERT
				assert_not_null(j1);
				assert_not_null(j2);
				assert_not_null(j3);
			}


			test( JoinedTableContainsAllLeftRecordsForEmptyRightTable )
			{
				typedef tuple<double, string, int> type1_t;
				typedef pair<int, string> type2_t;
				typedef table<type1_t> table1_t;
				typedef table<type2_t> table2_t;

				// INIT
				table1_t t1;
				table2_t t2;
				type1_t data1[] = {
					type1_t(0.71, "lorem", 4),
					type1_t(0.72, "ipsum", 3),
					type1_t(0.73, "amet", 7),
					type1_t(0.74, "dolor", 3),
					type1_t(0.75, "ipsum", 9),
					type1_t(0.76, "dolor", 4),
				};
				type2_t data2[] = {
					type2_t(3, "Jive"),
					type2_t(4, "Samba"),
					type2_t(9, "lorem"),
					type2_t(3, "Rumba"),
					type2_t(8, "Chachacha"),
				};

				add_records(t1, data1);

				// INIT / ACT
				auto j1 = left_join<key_n<2>, key_first>(t1, t2);

				// ASSERT
				assert_equivalent(plural
					+ make_pair(type1_t(0.71, "lorem", 4), nullable<type2_t>())
					+ make_pair(type1_t(0.72, "ipsum", 3), nullable<type2_t>())
					+ make_pair(type1_t(0.73, "amet", 7), nullable<type2_t>())
					+ make_pair(type1_t(0.74, "dolor", 3), nullable<type2_t>())
					+ make_pair(type1_t(0.75, "ipsum", 9), nullable<type2_t>())
					+ make_pair(type1_t(0.76, "dolor", 4), nullable<type2_t>()), *j1);

				// INIT
				add_records(t2, data2);

				// INIT / ACT
				auto j2 = left_join<key_n<2>, key_first>(t1, t2);

				// ASSERT
				assert_equivalent(plural
					+ make_pair(type1_t(0.72, "ipsum", 3), nullable<type2_t>(type2_t(3, "Jive")))
					+ make_pair(type1_t(0.72, "ipsum", 3), nullable<type2_t>(type2_t(3, "Rumba")))
					+ make_pair(type1_t(0.74, "dolor", 3), nullable<type2_t>(type2_t(3, "Jive")))
					+ make_pair(type1_t(0.74, "dolor", 3), nullable<type2_t>(type2_t(3, "Rumba")))
					+ make_pair(type1_t(0.76, "dolor", 4), nullable<type2_t>(type2_t(4, "Samba")))
					+ make_pair(type1_t(0.71, "lorem", 4), nullable<type2_t>(type2_t(4, "Samba")))
					+ make_pair(type1_t(0.73, "amet", 7), nullable<type2_t>())
					+ make_pair(type1_t(0.75, "ipsum", 9), nullable<type2_t>(type2_t(9, "lorem"))), *j2);
			}

		end_test_suite
	}
}
