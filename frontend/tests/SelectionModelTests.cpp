#include <frontend/selection_model.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			template <typename KeyT>
			vector<KeyT> get_selected(const selection<KeyT> &selection_)
			{	return vector<KeyT>(selection_.begin(), selection_.end());	}
		}

		begin_test_suite( SelectionModelTests )
			test( NewModelHasNoSelectedItems )
			{
				typedef vector< pair<int, string> > u1_t;
				typedef vector< pair<string, int> > u2_t;

				// INIT
				pair<int, string> data1_[] = {
					make_pair(12, "z"), make_pair(12211, "b"), make_pair(1, "a"),
				};
				const auto data1 = mkvector(data1_);
				pair<string, int> data2_[] = {
					make_pair("foo", 1), make_pair("bar", 2),
				};
				const auto data2 = mkvector(data2_);

				// INIT / ACT
				selection_model<u1_t> selection1_(data1);
				selection<int> &selection1 = selection1_;
				selection_model<u2_t> selection2_(data2);
				selection<string> &selection2 = selection2_;

				// ACT / ASSERT
				assert_is_empty(get_selected(selection1));
				assert_is_false(selection1.contains(0));
				assert_is_false(selection1.contains(1));
				assert_is_false(selection1.contains(2));

				assert_is_empty(get_selected(selection2));
				assert_is_false(selection2.contains(0));
				assert_is_false(selection2.contains(1));
			}


			test( AdditionsRemovalsClearsModifySelection )
			{
				typedef vector< pair<int, string> > underlying_t;

				// INIT
				pair<int, string> data_[] = {
					make_pair(12, "z"), make_pair(12211, "b"), make_pair(1, "a"), make_pair(192, "e"),
				};
				auto data = mkvector(data_);
				selection_model<underlying_t> selection_impl(data);
				wpl::dynamic_set_model &selection_ = selection_impl;

				// ACT
				selection_.add(1);

				// ASSERT
				int reference1[] = {	12211,	};

				assert_equivalent(reference1, get_selected(selection_impl));
				assert_is_true(selection_.contains(1));

				// ACT
				selection_.add(3);
				selection_.add(0);

				// ASSERT
				int reference2[] = {	12, 12211, 192,	};

				assert_equivalent(reference2, get_selected(selection_impl));
				assert_is_true(selection_.contains(0));
				assert_is_true(selection_.contains(1));
				assert_is_true(selection_.contains(3));

				// ACT
				selection_.remove(3);

				// ASSERT
				int reference3[] = {	12, 12211,	};

				assert_equivalent(reference3, get_selected(selection_impl));
				assert_is_true(selection_.contains(0));
				assert_is_true(selection_.contains(1));
				assert_is_false(selection_.contains(3));

				// ACT
				selection_.clear();

				// ASSERT
				assert_is_empty(get_selected(selection_impl));
				assert_is_false(selection_.contains(0));
				assert_is_false(selection_.contains(1));

				// ACT
				selection_impl.add_key(12211);

				// ASSERT
				int reference4[] = {	12211,	};

				assert_equivalent(reference4, get_selected(selection_impl));

				// ACT
				selection_impl.add_key(192);

				// ASSERT
				int reference5[] = {	12211, 192,	};

				assert_equivalent(reference5, get_selected(selection_impl));
			}


			test( SelectionChangesAreReportedAsInvalidations )
			{
				typedef vector< pair<int, string> > underlying_t;

				// INIT
				pair<int, string> data_[] = {
					make_pair(11, "z"), make_pair(12, "b"), make_pair(13, "a"), make_pair(14, "e"),
				};
				auto data = mkvector(data_);
				selection_model<underlying_t> selection_(data);
				vector<selection_model<underlying_t>::index_type> log1;
				vector< vector<int> > log2;
				auto conn = selection_.invalidate += [&] (selection_model<underlying_t>::index_type key)	{
					log1.push_back(key);
					log2.push_back(get_selected(selection_));
				};

				// ACT
				selection_.add(1);

				// ASSERT
				selection_model<underlying_t>::index_type reference11[] = {	1,	};
				int reference12[] = {	12,	};

				assert_equivalent(reference11, log1);
				assert_equivalent(reference12, log2.back());

				// ACT
				selection_.add(3);

				// ASSERT
				selection_model<underlying_t>::index_type reference21[] = {	1, 3,	};
				int reference22[] = {	12, 14,	};

				assert_equivalent(reference21, log1);
				assert_equivalent(reference22, log2.back());

				// ACT
				selection_.remove(1);

				// ASSERT
				selection_model<underlying_t>::index_type reference31[] = {	1, 3, 1,	};
				int reference32[] = {	14,	};

				assert_equivalent(reference31, log1);
				assert_equivalent(reference32, log2.back());

				// ACT
				selection_.clear();

				// ASSERT
				selection_model<underlying_t>::index_type reference4[] = {	1, 3, 1, wpl::index_traits::npos(),	};

				assert_equivalent(reference4, log1);
				assert_is_empty(log2.back());

				// INIT
				log1.clear();
				log2.clear();

				// ACT
				static_cast<selection<int> &>(selection_).add_key(12);

				// ASSERT
				selection_model<underlying_t>::index_type reference51[] = {	wpl::index_traits::npos(),	};

				assert_equivalent(reference51, log1);
				assert_equal(1u, log2.size());
				assert_equivalent(reference12, log2.back());
			}
		end_test_suite
	}
}
