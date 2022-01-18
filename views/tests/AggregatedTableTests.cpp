#include <views/aggregated_table.h>

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
				typedef pair<int, int> A;

				struct X
				{
					typedef int key_type;

					int operator ()(const A &item) const
					{	return item.first;	}

					void operator ()(A &aggregated, int key) const
					{	aggregated.first = key;	}

					template <typename I>
					void aggregate(A &aggregated, I group_begin, I group_end) const
					{
						aggregated.second = 0;
						for (auto i = group_begin; i != group_end; ++i)
							aggregated.second += i->second;
					}
				};

				struct Y
				{
					typedef int key_type;

					int operator ()(const A &item) const
					{	return item.second;	}

					void operator ()(A &aggregated, int key) const
					{	aggregated.second = key;	}

					template <typename I>
					void aggregate(A &aggregated, I group_begin, I group_end) const
					{
						aggregated.first = 0;
						for (auto i = group_begin; i != group_end; ++i)
							aggregated.first += i->first;
					}
				};

				template <typename T, typename C>
				void add(table<T, C> &table_, const T &object)
				{
					auto r = table_.create();

					*r = object;
					r.commit();
				}

				struct ctor
				{
					A operator ()() const
					{	return A(0, 0);	}
				};
			}

			begin_test_suite( AggregatedTableTests )
				test( RecordsAddedToUnderlyingAreIgnoredByUngrouppedAggregatedTable )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					aggregated_table<table<A, ctor>, ctor> a(u);

					// ACT
					add(u, A(1, 31));
					add(u, A(2, 415));

					// ASSERT
					assert_equal(a.end(), a.begin());
				}


				test( RecordsAddedToUnderlyingAppearInAnUngrouppedAggregatedTable )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					aggregated_table<table<A, ctor>, ctor> a(u);

					a.group_by(X());

					// ACT
					add(u, A(1, 3));
					add(u, A(2, 141));

					// ACT / ASSERT
					A reference1[] = {	A(1, 3), A(2, 141),	};

					assert_equivalent(reference1, a);

					// ACT
					add(u, A(3, 5926));

					// ACT / ASSERT
					A reference2[] = {	A(1, 3), A(2, 141), A(3, 5926),	};

					assert_equivalent(reference2, a);
				}


				test( OnlyAggregatedRecordsAppearInAggregatedTable )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					aggregated_table<table<A, ctor>, ctor> a(u);

					a.group_by(X());

					// ACT
					add(u, A(3, 9));
					add(u, A(1, 3));
					add(u, A(2, 141));
					add(u, A(1, 7));
					add(u, A(3, 1));
					add(u, A(3, 5));

					// ACT / ASSERT
					A reference2[] = {	A(1, 10), A(2, 141), A(3, 15),	};

					assert_equivalent(reference2, a);
				}


				test( ChangingGrouppingRereadsUnderlyingItems )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					aggregated_table<table<A, ctor>, ctor> a(u);

					a.group_by(X());

					add(u, A(3, 9));
					add(u, A(1, 3));
					add(u, A(2, 141));
					add(u, A(1, 9));
					add(u, A(3, 3));
					add(u, A(3, 5));

					// ACT
					a.group_by(Y());

					// ASSERT
					A reference[] = {	A(4, 3), A(3, 5), A(4, 9), A(2, 141),	};

					assert_equivalent(reference, a);
				}


				test( ClearingAnUnderlyingClearsAggregation )
				{
					// INIT
					table<A, ctor> u;
					aggregated_table<table<A, ctor>, ctor> a(u);

					a.group_by(X());

					add(u, A(3, 9));
					add(u, A(1, 3));

					// ACT
					u.clear();

					// ASSERT
					assert_equal(a.end(), a.begin());
				}


				test( ClearNotificationIsSentUponUnderlyingClear )
				{
					// INIT
					table<A, ctor> u;
					aggregated_table<table<A, ctor>, ctor> a(u);

					a.group_by(X());

					add(u, A(3, 9));
					add(u, A(1, 3));

					auto notified = 0;
					auto c = a.cleared += [&] {
						notified++;
						assert_equal(a.end(), a.begin());
					};

					// ACT
					u.clear();

					// ASSERT
					assert_equal(1, notified);
				}


				test( ChangedNotificationIsSentUponUnderlyingAddOrChange )
				{
					typedef aggregated_table<table<A, ctor>, ctor> aggregated_table_type;

					// INIT
					table<A, ctor> u;
					aggregated_table_type a(u);
					vector<bool> sequence;
					vector<A> sequence_items;

					a.group_by(X());

					auto c = a.changed += [&] (aggregated_table_type::const_iterator i, bool new_) {
						sequence.push_back(new_);
						sequence_items.push_back(*i);
					};

					// ACT
					add(u, A(3, 9));
					add(u, A(1, 3));
					add(u, A(3, 5));

					// ASSERT
					bool reference1[] = {	true, true, false,	};
					A reference1_items[] = {	A(3, 9), A(1, 3), A(3, 14),	};

					assert_equal(reference1, sequence);
					assert_equal(reference1_items, sequence_items);

					// ACT
					add(u, A(5, 19));
					add(u, A(5, 1));

					// ASSERT
					bool reference2[] = {	true, true, false, true, false,	};
					A reference2_items[] = {	A(3, 9), A(1, 3), A(3, 14), A(5, 19), A(5, 20),	};

					assert_equal(reference2, sequence);
					assert_equal(reference2_items, sequence_items);
				}


				test( AggregatedTableSupportsIndexing )
				{
					typedef aggregated_table<table<A, ctor>, ctor> aggregated_table_type;

					// INIT
					table<A, ctor> u;
					aggregated_table_type a(u);

					a.group_by(X());

					// INIT / ACT
					const immutable_unique_index<aggregated_table_type, Y> idx(a);

					add(u, A(3, 9));
					add(u, A(1, 3));
					add(u, A(4, 5));
					add(u, A(2, 7));

					// ACT / ASSERT
					assert_equal(A(1, 3), idx[3]);
					assert_equal(A(4, 5), idx[5]);
					assert_equal(A(2, 7), idx[7]);
					assert_equal(A(3, 9), idx[9]);
				}
			end_test_suite
		}
	}
}
