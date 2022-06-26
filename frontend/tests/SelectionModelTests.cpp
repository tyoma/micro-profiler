#include <frontend/selection_model.h>

#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( SelectionModelTests )
			test( NewModelHasNoSelectedItems )
			{
				// INIT
				auto scope1 = make_shared< views::table<int> >();
				auto scope2 = make_shared< views::table<string> >();
				pair<int, string> data1[] = {
					make_pair(12, "z"), make_pair(12211, "b"), make_pair(1, "a"),
				};
				pair<string, int> data2[] = {
					make_pair("foo", 1), make_pair("bar", 2),
				};

				// INIT / ACT
				auto selection1 = make_shared< selection<int> >(scope1, [&] (size_t i) {	return data1[i].first;	});
				auto selection2 = make_shared< selection<string> >(scope2, [&] (size_t i) {	return data2[i].first;	});

				// ACT / ASSERT
				assert_equal(scope1->end(), scope1->begin());
				assert_is_false(selection1->contains(0));
				assert_is_false(selection1->contains(1));
				assert_is_false(selection1->contains(2));

				assert_equal(scope2->end(), scope2->begin());
				assert_is_false(selection2->contains(0));
				assert_is_false(selection2->contains(1));
			}


			test( AdditionsRemovalsClearsModifySelection )
			{
				typedef vector< pair<int, string> > underlying_t;

				// INIT
				auto scope = make_shared< views::table<int> >();
				pair<int, string> data[] = {
					make_pair(12, "z"), make_pair(12211, "b"), make_pair(1, "a"), make_pair(192, "e"),
				};
				auto s = make_shared< selection<int> >(scope, [&] (size_t i) {	return data[i].first;	});
				wpl::dynamic_set_model &selection_ = *s;

				// ACT
				selection_.add(1);

				// ASSERT
				assert_equivalent(plural + 12211, *scope);
				assert_is_false(selection_.contains(0));
				assert_is_true(selection_.contains(1));
				assert_is_false(selection_.contains(2));
				assert_is_false(selection_.contains(3));

				// ACT
				selection_.add(3);
				selection_.add(0);

				// ASSERT
				assert_equivalent(plural + 12211 + 12 + 192, *scope);
				assert_is_true(selection_.contains(0));
				assert_is_true(selection_.contains(1));
				assert_is_true(selection_.contains(3));

				// ACT
				selection_.remove(3);

				// ASSERT
				assert_equivalent(plural + 12211 + 12, *scope);
				assert_is_true(selection_.contains(0));
				assert_is_true(selection_.contains(1));
				assert_is_false(selection_.contains(3));

				// ACT
				selection_.clear();

				// ASSERT
				assert_equal(scope->end(), scope->begin());
				assert_is_false(selection_.contains(0));
				assert_is_false(selection_.contains(1));

				// ACT
				add_records(*scope, plural + 12211);

				// ASSERT
				assert_is_true(selection_.contains(1));

				// ACT
				add_records(*scope, plural + 192);

				// ASSERT
				assert_is_true(selection_.contains(3));
			}


			test( SelectionChangesAreReportedAsInvalidations )
			{
				// INIT
				auto scope = make_shared< views::table<int> >();
				pair<int, string> data[] = {
					make_pair(11, "z"), make_pair(12, "b"), make_pair(13, "a"), make_pair(14, "e"), make_pair(15, "f"),
				};
				auto s = make_shared< selection<int> >(scope, [&] (size_t i) {	return data[i].first;	});
				vector<wpl::dynamic_set_model::index_type> log1;
				vector< vector<wpl::dynamic_set_model::index_type> > log2;
				auto conn = s->invalidate += [&] (wpl::dynamic_set_model::index_type index)	{
					log1.push_back(index);
					log2.resize(log2.size() + 1);
					for (auto i = 0u; i < 5u; ++i)
					{
						if (s->contains(i))
							log2.back().push_back(i);
					}
				};

				// ACT
				s->add(1);

				// ASSERT
				assert_equal(plural + (size_t)1u, log1);
				assert_equal(1u, log2.size());
				assert_equivalent(plural + 1u, log2.back());

				// ACT
				s->add(3);

				// ASSERT
				assert_equal(plural + (size_t)1u + (size_t)3u, log1);
				assert_equal(2u, log2.size());
				assert_equivalent(plural + 1u + 3u, log2.back());

				// ACT
				s->remove(1);

				// ASSERT
				assert_equal(plural + (size_t)1u + (size_t)3u + (size_t)1u, log1);
				assert_equivalent(plural + 3u, log2.back());

				// ACT
				s->clear();

				// ASSERT
				assert_equal(plural + (size_t)1 + (size_t)3 + (size_t)1 + wpl::index_traits::npos(), log1);
				assert_is_empty(log2.back());

				// INIT
				log1.clear();
				log2.clear();

				// ACT
				add_records(*scope, plural + 11 + 15);
				scope->invalidate();

				// ASSERT
				assert_equal(plural + wpl::index_traits::npos(), log1);
				assert_equivalent(plural + 0u + 4u, log2.back());
			}
		end_test_suite
	}
}
