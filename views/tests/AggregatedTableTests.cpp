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

					template <typename IndexT>
					void operator ()(IndexT &, A &aggregated, int key) const
					{	aggregated.first = key;	}
				};

				struct Y
				{
					typedef int key_type;

					int operator ()(const A &item) const
					{	return item.second;	}

					template <typename IndexT>
					void operator ()(IndexT &, A &aggregated, int key) const
					{	aggregated.second = key;	}
				};

				struct aggregate_first
				{
					template <typename I>
					void operator ()(A &aggregated, I group_begin, I group_end) const
					{
						aggregated.first = 0;
						for (auto i = group_begin; i != group_end; ++i)
							aggregated.first += i->first;
					}
				};

				struct aggregate_second
				{
					template <typename I>
					void operator ()(A& aggregated, I group_begin, I group_end) const
					{
						aggregated.second = 0;
						for (auto i = group_begin; i != group_end; ++i)
							aggregated.second += i->second;
					}
				};

				template <typename T, typename C>
				void add(table<T, C> &table_, const T &object)
				{
					auto r = table_.create();

					*r = object;
					r.commit();
				}

				template <typename T, typename C, typename T2, size_t n>
				void add(table<T, C> &table_, T2 (&objects)[n])
				{
					for (size_t i = 0; i != n; ++i)
						add(table_, objects[i]);
				}

				struct ctor
				{
					A operator ()() const
					{	return A(0, 0);	}
				};

				template <typename K>
				struct key_factory
				{
					template <typename T>
					K operator ()(const T &/*table_*/, agnostic_key_tag) const
					{	return K();	}
				};

				template <typename K1, typename K2>
				struct sided_key_factory
				{
					template <typename T>
					K1 operator ()(const T &table_, underlying_key_tag) const
					{	return logu.push_back(&table_), K1();	}

					template <typename T>
					K2 operator ()(const T &table_, aggregated_key_tag) const
					{	return loga.push_back(&table_), K2();	}

					mutable vector<const void *> logu, loga;
				};

				struct another_sneaky_type
				{
					int a, b, c;

					bool operator <(const another_sneaky_type &rhs) const
					{	return make_pair(make_pair(a, b), c) < make_pair(make_pair(rhs.a, rhs.b), rhs.c);	}
				};

				struct sneaky_keyer_a
				{
					typedef int key_type;

					key_type operator ()(const another_sneaky_type &v) const
					{	return v.a;	}
				};

				struct sneaky_keyer_b
				{
					typedef int key_type;

					key_type operator ()(const another_sneaky_type &v) const
					{	return v.b;	}

					template <typename I>
					void operator ()(I &, another_sneaky_type &v, int key) const
					{	v.b = key;	}
				};

				struct sneaky_type_aggregator
				{
					template <typename I>
					void operator ()(another_sneaky_type &aggregated, I group_begin, I group_end) const
					{
						aggregated.a = 0;
						aggregated.c = 0;
						for (auto i = group_begin; i != group_end; ++i)
							aggregated.c += i->c;
					}
				};
			}

			begin_test_suite( AggregatedTableTests )
				test( RecordsAddedToUnderlyingAppearInAnUngrouppedAggregatedTable )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					auto a = group_by(u, key_factory<X>(), aggregate_second());

					// ACT
					add(u, A(1, 3));
					add(u, A(2, 141));

					// ACT / ASSERT
					A reference1[] = {	A(1, 3), A(2, 141),	};

					assert_equivalent(reference1, *a);

					// ACT
					add(u, A(3, 5926));

					// ACT / ASSERT
					A reference2[] = {	A(1, 3), A(2, 141), A(3, 5926),	};

					assert_equivalent(reference2, *a);
				}


				test( OnlyAggregatedRecordsAppearInAggregatedTable )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					auto a = group_by(u, key_factory<X>(), aggregate_second());

					// ACT
					add(u, A(3, 9));
					add(u, A(1, 3));
					add(u, A(2, 141));
					add(u, A(1, 7));
					add(u, A(3, 1));
					add(u, A(3, 5));

					// ACT / ASSERT
					A reference2[] = {	A(1, 10), A(2, 141), A(3, 15),	};

					assert_equivalent(reference2, *a);
				}


				test( ClearingAnUnderlyingClearsAggregation )
				{
					// INIT
					table<A, ctor> u;

					auto a = group_by(u, key_factory<X>(), aggregate_second());

					add(u, A(3, 9));
					add(u, A(1, 3));

					// ACT
					u.clear();

					// ASSERT
					assert_equal(a->end(), a->begin());
				}


				test( ClearNotificationIsSentUponUnderlyingClear )
				{
					// INIT
					table<A, ctor> u;

					auto a = group_by(u, key_factory<X>(), aggregate_second());

					add(u, A(3, 9));
					add(u, A(1, 3));

					auto notified = 0;
					auto c = a->cleared += [&] {
						notified++;
						assert_equal(a->end(), a->begin());
					};

					// ACT
					u.clear();

					// ASSERT
					assert_equal(1, notified);
				}


				test( InvalidationIsPassedOnWithoutResettingTheAggregate )
				{
					// INIT
					table<A, ctor> u;

					auto a = group_by(u, key_factory<X>(), aggregate_second());

					add(u, A(3, 9));
					add(u, A(1, 3));

					auto notified = 0;
					auto c = a->invalidate += [&] {
						notified++;
					};

					// ACT
					u.invalidate();

					// ASSERT
					assert_equal(1, notified);
					assert_not_equal(a->end(), a->begin());
				}


				test( ChangedNotificationIsSentUponUnderlyingAddOrChange )
				{
					// INIT
					table<A, ctor> u;
					vector<bool> sequence;
					vector<A> sequence_items;
					auto a = group_by(u, key_factory<X>(), aggregate_second());

					auto c = a->changed += [&] (table<A, ctor>::const_iterator i, bool new_) {
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
					// INIT
					table<A, ctor> u;
					auto a = group_by(u, key_factory<X>(), aggregate_second());

					// INIT / ACT
					const immutable_unique_index<table<A, ctor>, Y> idx(*a);

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


				test( KeysAreCreatedWithADistinctiveTag )
				{
					// INIT
					table<another_sneaky_type> u;
					sided_key_factory<sneaky_keyer_a, sneaky_keyer_b> f;

					// ACT
					auto a = group_by(u, f, sneaky_type_aggregator());

					// ASSERT
					const void *reference1[] = {	&u,	};
					const void *reference2[] = {	a.get(),	};

					assert_equal(reference1, f.logu);
					assert_equal(reference2, f.loga);
				}


				test( AssymetricAggregationUsesDifferentFields )
				{
					// INIT
					table<another_sneaky_type> u;
					sided_key_factory<sneaky_keyer_a, sneaky_keyer_b> f;
					another_sneaky_type data[] = {
						{	3, 2, 13	},
						{	1, 2, 2	},
						{	2, 2, 3	},
						{	1, 2, 5	},
						{	3, 2, 7	},
					};

					add(u, data);

					// INIT / ACT
					auto a = group_by(u, f, sneaky_type_aggregator());

					// ASSERT
					another_sneaky_type reference[] = {
						{	0, 3, 20	},
						{	0, 1, 7	},
						{	0, 2, 3	},
					};

					assert_equivalent(reference, *a);
				}
			end_test_suite
		}
	}
}
