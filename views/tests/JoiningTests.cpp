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
					shared_ptr< const table< joined_record<table1_t, table2_t> > > j1 = join<key_second, key_first>(t1, t2);
					shared_ptr< const table< joined_record<table2_t, table3_t> > > j2 = join<key_first, key_first>(t2, t3);
					shared_ptr< const table< joined_record<table3_t, table4_t> > > j3 = join<key_second, key_first>(t3, t4);
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
					auto j1 = join<key_n<2>, key_first>(t1, t2);

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
					auto j2 = join<key_first, key_first>(t2, t2);

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


				test( NewRecordInASourceTableGeneratesASetOfCombinationsInAJoinedTable )
				{
					typedef pair<int, string> type1_t;
					typedef tuple<string, double, int> type2_t;
					typedef table<type1_t> table1_t;
					typedef table<type2_t> table2_t;

					// INIT
					table1_t t1;
					table2_t t2;
					type1_t data1[] = {
						type1_t(9, "lorem"),
						type1_t(19, "ipsum"),
					};
					type2_t data2[] = {
						type2_t("lorem", 0.71, 4),
						type2_t("ipsum", 0.72, 3),
						type2_t("amet", 0.73, 7),
						type2_t("dolor", 0.74, 3),
						type2_t("ipsum", 0.75, 9),
					};

					add_records(t1, data1);
					add_records(t2, data2);

					auto j = join< key_second, key_n<0> >(t1, t2);

					// ACT
					add_record(t1, type1_t(7, "lorem"));

					// ASSERT
					pair<type1_t, type2_t> reference1[] = {
						make_pair(type1_t(9, "lorem"), type2_t("lorem", 0.71, 4)),
						make_pair(type1_t(7, "lorem"), type2_t("lorem", 0.71, 4)), // new
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.72, 3)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.75, 9)),
					};

					assert_equivalent(reference1, *j);

					// ACT
					add_record(t1, type1_t(7, "ipsum"));

					// ASSERT
					pair<type1_t, type2_t> reference2[] = {
						make_pair(type1_t(9, "lorem"), type2_t("lorem", 0.71, 4)),
						make_pair(type1_t(7, "lorem"), type2_t("lorem", 0.71, 4)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.72, 3)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.75, 9)),
						make_pair(type1_t(7, "ipsum"), type2_t("ipsum", 0.72, 3)), // new
						make_pair(type1_t(7, "ipsum"), type2_t("ipsum", 0.75, 9)), // new
					};

					assert_equivalent(reference2, *j);

					// ACT
					add_record(t2, type2_t("lorem", 0.92, 1));

					// ASSERT
					pair<type1_t, type2_t> reference3[] = {
						make_pair(type1_t(9, "lorem"), type2_t("lorem", 0.71, 4)),
						make_pair(type1_t(7, "lorem"), type2_t("lorem", 0.71, 4)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.72, 3)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.75, 9)),
						make_pair(type1_t(7, "ipsum"), type2_t("ipsum", 0.72, 3)),
						make_pair(type1_t(7, "ipsum"), type2_t("ipsum", 0.75, 9)),
						make_pair(type1_t(9, "lorem"), type2_t("lorem", 0.92, 1)), // new
						make_pair(type1_t(7, "lorem"), type2_t("lorem", 0.92, 1)), // new
					};

					assert_equivalent(reference3, *j);
				}


				test( ModificationOfASourceRecordDoesNotCreateNewJoinedRecords )
				{
					typedef pair<int, string> type1_t;
					typedef tuple<string, double, int> type2_t;
					typedef table<type1_t> table1_t;
					typedef table<type2_t> table2_t;

					// INIT
					table1_t t1;
					table2_t t2;
					type1_t data1[] = {
						type1_t(9, "lorem"),
						type1_t(19, "ipsum"),
					};
					type2_t data2[] = {
						type2_t("lorem", 0.71, 4),
						type2_t("ipsum", 0.72, 3),
						type2_t("amet", 0.73, 7),
						type2_t("dolor", 0.74, 3),
						type2_t("ipsum", 0.75, 9),
					};

					add_records(t1, data1);
					add_records(t2, data2);

					auto j = join< key_second, key_n<0> >(t1, t2);
					auto i1 = t1.begin();
					auto i2 = t2.begin();

					// ACT
					auto r1 = t1.modify(i1);
					auto r2 = t2.modify((++i2, i2));

					(*r1).first = 18;
					r1.commit();
					get<1>(*r2) = 1.0;
					r2.commit();

					// ASSERT
					pair<type1_t, type2_t> reference[] = {
						make_pair(type1_t(18, "lorem"), type2_t("lorem", 0.71, 4)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 1, 3)),
						make_pair(type1_t(19, "ipsum"), type2_t("ipsum", 0.75, 9)),
					};

					assert_equivalent(reference, *j);
				}


				test( RemovingItemFromEitherSourceTableRemovesCorresponsingCombinations )
				{
					typedef pair<int, string> type1_t;
					typedef tuple<string, double, int> type2_t;
					typedef table<type1_t> table1_t;
					typedef table<type2_t> table2_t;

					// INIT
					table1_t t1;
					table2_t t2;
					type1_t data1[] = {
						type1_t(1, "lorem"),
						type1_t(2, "lorem"),
						type1_t(3, "ipsum"),
						type1_t(4, "amet"),
					};
					type2_t data2[] = {
						type2_t("lorem", 0.71, 11),
						type2_t("lorem", 0.72, 12),
						type2_t("dolor", 0.73, 13),
						type2_t("ipsum", 0.74, 14),
						type2_t("ipsum", 0.75, 15),
						type2_t("ipsum", 0.76, 16),
						type2_t("amet", 0.77, 17),
					};

					add_records(t1, data1);
					add_records(t2, data2);

					auto j = join< key_second, key_n<0> >(t1, t2);

					// ACT
					auto left = t1.begin();

					++left;
					auto r1 = t1.modify(left);
					r1.remove();

					// ASSERT
					pair<type1_t, type2_t> reference1[] = {
						make_pair(type1_t(1, "lorem"), type2_t("lorem", 0.71, 11)),
						make_pair(type1_t(1, "lorem"), type2_t("lorem", 0.72, 12)),
						make_pair(type1_t(3, "ipsum"), type2_t("ipsum", 0.74, 14)),
						make_pair(type1_t(3, "ipsum"), type2_t("ipsum", 0.75, 15)),
						make_pair(type1_t(3, "ipsum"), type2_t("ipsum", 0.76, 16)),
						make_pair(type1_t(4, "amet"), type2_t("amet", 0.77, 17)),
					};

					assert_equivalent(reference1, *j);

					// ACT
					auto right = t2.begin();

					++right, ++right, ++right, ++right;
					auto r2 = t2.modify(right);
					r2.remove();

					// ASSERT
					pair<type1_t, type2_t> reference2[] = {
						make_pair(type1_t(1, "lorem"), type2_t("lorem", 0.71, 11)),
						make_pair(type1_t(1, "lorem"), type2_t("lorem", 0.72, 12)),
						make_pair(type1_t(3, "ipsum"), type2_t("ipsum", 0.74, 14)),
						make_pair(type1_t(3, "ipsum"), type2_t("ipsum", 0.76, 16)),
						make_pair(type1_t(4, "amet"), type2_t("amet", 0.77, 17)),
					};

					assert_equivalent(reference2, *j);
				}


				test( ClearingOfTheEitherUnderlyingTableClearsTheJoinedOne )
				{
					typedef pair<int, string> type1_t;
					typedef tuple<string, double, int> type2_t;
					typedef table<type1_t> table1_t;
					typedef table<type2_t> table2_t;

					// INIT
					table1_t t1;
					table2_t t2;
					type1_t data1[] = {
						type1_t(1, "lorem"),
						type1_t(3, "ipsum"),
					};
					type2_t data2[] = {
						type2_t("lorem", 0.71, 3),
						type2_t("lorem", 0.72, 3),
						type2_t("dolor", 0.73, 1),
					};
					auto j = join< key_first, key_n<2> >(t1, t2);

					add_records(t1, data1);
					add_records(t2, data2);

					// ACT
					t1.clear();

					// ASSERT
					assert_equal(j->end(), j->begin());

					// INIT
					add_records(t1, data1);

					// ASSERT
					assert_not_equal(j->end(), j->begin());

					// ACT
					t2.clear();

					// ASSERT
					assert_equal(j->end(), j->begin());
				}


				test( InvalidationsArePassedOnToAJoinedTable )
				{
					typedef tuple<string, double, int> type1_t;
					typedef pair<int, string> type2_t;
					typedef table<type1_t> table1_t;
					typedef table<type2_t> table2_t;

					// INIT
					table1_t t1;
					table2_t t2;
					auto j = join<key_n<2>, key_first>(t1, t2);
					auto invalidations = 0;
					auto conn = j->invalidate += [&] {	invalidations++;	};

					// ACT
					t1.invalidate();

					// ASSERT
					assert_equal(1, invalidations);

					// ACT
					t2.invalidate();
					t1.invalidate();

					// ASSERT
					assert_equal(3, invalidations);
				}


				test( JoinedTableValueCanBeCastedToLeftTableValue )
				{
					typedef tuple<string, double, int> type1_t;
					typedef pair<int, string> type2_t;
					typedef table<type1_t> table1_t;
					typedef table<type2_t> table2_t;

					// INIT
					table1_t t1;
					table2_t t2;
					type1_t data1[] = {
						type1_t("lorem", 0.71, 3),
						type1_t("lorem", 0.72, 3),
						type1_t("dolor", 0.73, 1),
					};
					type2_t data2[] = {
						type2_t(1, "lorem"),
						type2_t(1, "amet"),
						type2_t(3, "ipsum"),
						type2_t(0, "dolor"),
					};
					auto j1 = join<key_n<2>, key_first>(t1, t2);
					auto j2 = join< key_first, key_n<2> >(t2, t1);

					add_records(t1, data1);
					add_records(t2, data2);

					// ACT / ASSERT
					type1_t reference1[] = {
						type1_t("lorem", 0.71, 3),
						type1_t("lorem", 0.72, 3),
						type1_t("dolor", 0.73, 1),
						type1_t("dolor", 0.73, 1),
					};
					type2_t reference2[] = {
						type2_t(1, "lorem"),
						type2_t(1, "amet"),
						type2_t(3, "ipsum"),
						type2_t(3, "ipsum"),
					};

					vector<type1_t> collected1;
					for (auto i = j1->begin(); i != j1->end(); ++i)
						collected1.push_back(*i);

					assert_equivalent(reference1, collected1);
					assert_equivalent(reference2, vector<type2_t>(j2->begin(), j2->end()));
				}
			end_test_suite
		}
	}
}
