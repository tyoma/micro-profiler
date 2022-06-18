#include <views/transforms.h>

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
			namespace
			{
				typedef pair<int, int> A;

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
					int operator ()(const another_sneaky_type &v) const
					{	return v.a;	}
				};

				struct sneaky_keyer_b
				{
					int operator ()(const another_sneaky_type &v) const
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

			begin_test_suite( AggregationTests )
				test( RecordsAddedToUnderlyingAppearInAnUngrouppedAggregatedTable )
				{
					// INIT
					table<A, ctor> u;

					// INIT / ACT
					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					// ACT
					add_record(u, A(1, 3));
					add_record(u, A(2, 141));

					// ACT / ASSERT
					A reference1[] = {	A(1, 3), A(2, 141),	};

					assert_equivalent(reference1, *a);

					// ACT
					add_record(u, A(3, 5926));

					// ACT / ASSERT
					A reference2[] = {	A(1, 3), A(2, 141), A(3, 5926),	};

					assert_equivalent(reference2, *a);
				}


				test( OnlyAggregatedRecordsAppearInAggregatedTable )
				{
					// INIT
					table<A> u;

					// INIT / ACT
					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					// ACT
					add_record(u, A(3, 9));
					add_record(u, A(1, 3));
					add_record(u, A(2, 141));
					add_record(u, A(1, 7));
					add_record(u, A(3, 1));
					add_record(u, A(3, 5));

					// ACT / ASSERT
					A reference2[] = {	A(1, 10), A(2, 141), A(3, 15),	};

					assert_equivalent(reference2, *a);
				}


				test( ClearingAnUnderlyingClearsAggregation )
				{
					// INIT
					table<A> u;

					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					add_record(u, A(3, 9));
					add_record(u, A(1, 3));

					// ACT
					u.clear();

					// ASSERT
					assert_equal(a->end(), a->begin());
				}


				test( ClearNotificationIsSentUponUnderlyingClear )
				{
					// INIT
					table<A> u;

					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					add_record(u, A(3, 9));
					add_record(u, A(1, 3));

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
					table<A> u;

					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					add_record(u, A(3, 9));
					add_record(u, A(1, 3));

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
					table<A> u;
					vector<bool> sequence;
					vector<A> sequence_items;
					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					auto c1 = a->created += [&] (table<A>::const_iterator i) {
						sequence.push_back(true);
						sequence_items.push_back(*i);
					};
					auto c2 = a->modified += [&] (table<A>::const_iterator i) {
						sequence.push_back(false);
						sequence_items.push_back(*i);
					};

					// ACT
					add_record(u, A(3, 9));
					add_record(u, A(1, 3));
					add_record(u, A(3, 5));

					// ASSERT
					bool reference1[] = {	true, true, false,	};
					A reference1_items[] = {	A(3, 9), A(1, 3), A(3, 14),	};

					assert_equal(reference1, sequence);
					assert_equal(reference1_items, sequence_items);

					// ACT
					add_record(u, A(5, 19));
					add_record(u, A(5, 1));

					// ASSERT
					bool reference2[] = {	true, true, false, true, false,	};
					A reference2_items[] = {	A(3, 9), A(1, 3), A(3, 14), A(5, 19), A(5, 20),	};

					assert_equal(reference2, sequence);
					assert_equal(reference2_items, sequence_items);
				}


				test( AggregatedTableSupportsIndexing )
				{
					// INIT
					table<A> u;
					auto a = group_by(u, key_factory<key_first>(), aggregate_second());

					// INIT / ACT
					const auto &idx = unique_index<key_second>(*a);

					add_record(u, A(3, 9));
					add_record(u, A(1, 3));
					add_record(u, A(4, 5));
					add_record(u, A(2, 7));

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

					add_records(u, data);

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


				struct assymetrical
				{
					template <typename I>
					void operator ()(A& aggregated, I group_begin, I group_end) const
					{
						aggregated.second = 0;
						for (auto i = group_begin; i != group_end; ++i)
							aggregated.second += i->c;
					}

					template <typename T>
					sneaky_keyer_a operator ()(T &, underlying_key_tag) const
					{	return sneaky_keyer_a();	}

					template <typename T>
					key_first operator ()(T &, aggregated_key_tag) const
					{	return key_first();	}
				};

				test( AssymetricalAggregationCombinesRecordsIntoADifferentRecordType )
				{
					// INIT
					table<another_sneaky_type> u;
					another_sneaky_type data[] = {
						{	3, 2, 13	},
						{	1, 4, 2	},
						{	2, 6, 3	},
						{	1, 7, 5	},
						{	3, 9, 7	},
						{	91, 9, 7	},
					};

					add_records(u, data);

					// ACT
					auto a = group_by<A>(u, assymetrical(), default_constructor<A>(), assymetrical());

					// ASSERT
					A reference1[] = {	A(1, 7), A(2, 3), A(3, 20), A(91, 7),	};

					assert_equivalent(reference1, *a);

					// INIT
					another_sneaky_type v = {	91, 0, 93	};

					// ACT
					add_record(u, v);

					// ASSERT
					A reference2[] = {	A(1, 7), A(2, 3), A(3, 20), A(91, 100),	};

					assert_equivalent(reference2, *a);
				}


				test( ModificationOfUnderlyingRecordsUpdatesAggregatedOnes )
				{
					// INIT
					table<another_sneaky_type> u;
					another_sneaky_type data[] = {
						{	3, 2, 13	},
						{	1, 4, 2	},
						{	2, 6, 3	},
						{	1, 7, 5	},
						{	3, 9, 7	},
						{	91, 9, 7	},
					};
					auto a = group_by<A>(u, assymetrical(), default_constructor<A>(), assymetrical());
					const auto iterators = add_records(u, data);

					// ACT
					auto r0 = u.modify(iterators[0]);

					(*r0).c = 10;
					r0.commit();

					// ASSERT
					A reference1[] = {	A(1, 7), A(2, 3), A(3, 17), A(91, 7),	};

					assert_equivalent(reference1, *a);

					// ACT
					auto r1 = u.modify(iterators[1]);

					(*r1).c = 5;
					r1.commit();

					// ASSERT
					A reference2[] = {	A(1, 10), A(2, 3), A(3, 17), A(91, 7),	};

					assert_equivalent(reference2, *a);
				}


				test( RemovalOfUnderlyingRecordsUpdatesAggregatedOnes )
				{
					// INIT
					table<another_sneaky_type> u;
					another_sneaky_type data[] = {
						{	3, 2, 13	},
						{	1, 4, 2	},
						{	2, 6, 3	},
						{	1, 7, 5	},
						{	3, 9, 7	},
						{	91, 9, 7	},
					};
					auto a = group_by<A>(u, assymetrical(), default_constructor<A>(), assymetrical());
					const auto iterators = add_records(u, data);

					// ACT
					auto r0 = u.modify(iterators[0]);
					r0.remove();

					// ASSERT
					A reference1[] = {	A(1, 7), A(2, 3), A(3, 7), A(91, 7),	};

					assert_equivalent(reference1, *a);

					// ACT
					auto r3 = u.modify(iterators[3]);
					r3.remove();

					// ASSERT
					A reference2[] = {	A(1, 2), A(2, 3), A(3, 7), A(91, 7),	};

					assert_equivalent(reference2, *a);
				}


				test( CompleteRemovalOfUnderlyingRecordsRemovesAggregatedOnes )
				{
					// INIT
					table<another_sneaky_type> u;
					another_sneaky_type data[] = {
						{	3, 2, 13	},
						{	1, 4, 2	},
						{	2, 6, 3	},
						{	1, 7, 5	},
						{	3, 9, 7	},
						{	91, 9, 7	},
					};
					auto a = group_by<A>(u, assymetrical(), default_constructor<A>(), assymetrical());
					const auto iterators = add_records(u, data);

					// ACT
					auto r0 = u.modify(iterators[0]);
					r0.remove();
					auto r3 = u.modify(iterators[3]);
					r3.remove();
					auto r4 = u.modify(iterators[4]);
					r4.remove();

					// ASSERT
					A reference1[] = {	A(1, 2), A(2, 3), A(91, 7),	};

					assert_equivalent(reference1, *a);

					// ACT
					auto r1 = u.modify(iterators[1]);
					r1.remove();

					// ASSERT
					A reference2[] = {	A(2, 3), A(91, 7),	};

					assert_equivalent(reference2, *a);
				}
			end_test_suite
		}
	}
}
