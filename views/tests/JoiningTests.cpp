#include <views/transforms.h>

#include "comparisons.h"
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
			begin_test_suite( JoiningTests )
				test( TableOfExpectedTypeIsReturnedWhenJoining )
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
					shared_ptr< const table< joined_record<table1_t, table2_t> > > j1 = join< key_second<type1_t>, key_first<type2_t> >(t1, t2);
					shared_ptr< const table< joined_record<table2_t, table3_t> > > j2 = join< key_first<type2_t>, key_first<type3_t> >(t2, t3);
					shared_ptr< const table< joined_record<table3_t, table4_t> > > j3 = join< key_second<type3_t>, key_first<type4_t> >(t3, t4);
				}


				test( JoinedTableContainsMixOfRecordsAtConstruction )
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
					add_records(t2, data2);

					// INIT / ACT
					auto j1 = join< key_n<int, 2>, key_first<type2_t> >(t1, t2);

					// ASSERT
					pair<type1_t, type2_t> reference1[] = {
						make_pair(type1_t(0.71, "lorem", 4), type2_t(4, "Samba")), // 0; 1
						make_pair(type1_t(0.76, "dolor", 4), type2_t(4, "Samba")), // 5; 1

						make_pair(type1_t(0.72, "ipsum", 3), type2_t(3, "Jive")), // 1; 0
						make_pair(type1_t(0.74, "dolor", 3), type2_t(3, "Jive")), // 3; 0
						make_pair(type1_t(0.72, "ipsum", 3), type2_t(3, "Rumba")), // 1; 3
						make_pair(type1_t(0.74, "dolor", 3), type2_t(3, "Rumba")), // 3; 3

						make_pair(type1_t(0.75, "ipsum", 9), type2_t(9, "lorem")), // 4; 2
					};

					assert_equivalent(reference1, *j1);

					// INIT / ACT
					auto j2 = join< key_first<type2_t>, key_first<type2_t> >(t2, t2);

					// ASSERT
					pair<type2_t, type2_t> reference2[] = {
						make_pair(type2_t(3, "Jive"), type2_t(3, "Jive")),
						make_pair(type2_t(3, "Jive"), type2_t(3, "Rumba")),
						make_pair(type2_t(3, "Rumba"), type2_t(3, "Jive")),
						make_pair(type2_t(3, "Rumba"), type2_t(3, "Rumba")),
						make_pair(type2_t(4, "Samba"), type2_t(4, "Samba")),
						make_pair(type2_t(8, "Chachacha"), type2_t(8, "Chachacha")),
						make_pair(type2_t(9, "lorem"), type2_t(9, "lorem")),
					};

					assert_equivalent(reference2, *j2);
				}
			end_test_suite
		}
	}
}
