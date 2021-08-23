#include <frontend/trackables_provider.h>

#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( TrackablesProviderTests )
			test( OrderedViewProvidesTracking )
			{
				typedef vector< pair<int, string> > underlying_t;

				// INIT
				pair<int, string> values[]= {
					make_pair(17, "lorem"),
					make_pair(17230, "dolor"),
					make_pair(172311, "ipsum"),
					make_pair(17231, "amet"),
				};
				auto underlying = mkvector(values);
				trackables_provider<underlying_t> p(underlying);

				// ACT
				shared_ptr<const wpl::trackable> t1 = p.track(0);
				shared_ptr<const wpl::trackable> t2 = p.track(2);

				// ASSERT
				assert_not_null(t1);
				assert_equal(0u, t1->index());
				assert_not_null(t2);
				assert_equal(2u, t2->index());
			}


			test( ChangingOrderKeepsTrackablesAtProperIndex )
			{
				typedef vector< pair<string, string> > underlying_t;

				// INIT
				pair<string, string> values[]= {
					make_pair("a", "lorem"),
					make_pair("b", "dolor"),
					make_pair("f", "quot"),
					make_pair("C", "ipsum"),
					make_pair("fff", "amet"),
				};
				auto underlying = mkvector(values);

				// INIT / ACT
				trackables_provider<underlying_t> p(underlying);
				shared_ptr<const wpl::trackable> t[] = {
					p.track(0), p.track(1), p.track(2), p.track(3), p.track(4),
				};

				// ACT
				sort(underlying.begin(), underlying.end(), [] (const pair<string, string> &lhs, const pair<string, string> &rhs) {
					return lhs.second < rhs.second;
				});
				p.fetch();

				// ASSERT
				assert_equal(3u, t[0]->index());
				assert_equal(1u, t[1]->index());
				assert_equal(4u, t[2]->index());
				assert_equal(2u, t[3]->index());
				assert_equal(0u, t[4]->index());

				// ACT
				sort(underlying.begin(), underlying.end(), [] (const pair<string, string> &lhs, const pair<string, string> &rhs) {
					return lhs.first > rhs.first;
				});
				p.fetch();
				assert_equal(3u, t[0]->index());
				assert_equal(2u, t[1]->index());
				assert_equal(1u, t[2]->index());
				assert_equal(4u, t[3]->index());
				assert_equal(0u, t[4]->index());
			}


			test( AddingItemsKeepTrackablesAtProperIndex )
			{
				typedef vector< pair<string, string> > underlying_t;

				// INIT
				pair<string, string> values[]= {
					make_pair("a", "lorem"),
					make_pair("b", "dolor"),
					make_pair("f", "quot"),
					make_pair("C", "ipsum"),
					make_pair("fff", "amet"),
				};
				auto underlying = mkvector(values);

				// INIT / ACT
				trackables_provider<underlying_t> p(underlying);
				shared_ptr<const wpl::trackable> t[] = {
					p.track(0), p.track(1), p.track(2), p.track(3), p.track(4),
				};

				// ACT
				underlying.insert(underlying.begin() + 2u, make_pair("z", "viva"));
				p.fetch();

				// ASSERT
				assert_equal(0u, t[0]->index());
				assert_equal(1u, t[1]->index());
				assert_equal(3u, t[2]->index());
				assert_equal(4u, t[3]->index());
				assert_equal(5u, t[4]->index());
			}


			test( TrackablesOfRemovedItemsReturnNpos )
			{
				typedef vector< pair<string, string> > underlying_t;

				// INIT
				pair<string, string> values[]= {
					make_pair("a", "lorem"),
					make_pair("b", "dolor"),
					make_pair("f", "quot"),
					make_pair("C", "ipsum"),
					make_pair("fff", "amet"),
				};
				auto underlying = mkvector(values);

				// INIT / ACT
				trackables_provider<underlying_t> p(underlying);
				shared_ptr<const wpl::trackable> t[] = {
					p.track(0), p.track(1), p.track(2), p.track(3), p.track(4),
				};

				// ACT
				underlying.erase(underlying.begin() + 2u);
				p.fetch();

				// ASSERT
				assert_equal(0u, t[0]->index());
				assert_equal(1u, t[1]->index());
				assert_equal(wpl::index_traits::npos(), t[2]->index());
				assert_equal(2u, t[3]->index());
				assert_equal(3u, t[4]->index());

				// ACT
				underlying.erase(underlying.begin() + 3u);
				p.fetch();

				// ASSERT
				assert_equal(0u, t[0]->index());
				assert_equal(1u, t[1]->index());
				assert_equal(wpl::index_traits::npos(), t[2]->index());
				assert_equal(2u, t[3]->index());
				assert_equal(wpl::index_traits::npos(), t[4]->index());
			}


			test( TrackablesOfRestoredItemsReturnProperIndex )
			{
				typedef vector< pair<string, string> > underlying_t;

				// INIT
				pair<string, string> values[]= {
					make_pair("a", "lorem"),
					make_pair("b", "dolor"),
					make_pair("f", "quot"),
					make_pair("C", "ipsum"),
					make_pair("fff", "amet"),
				};
				auto underlying = mkvector(values);

				// INIT / ACT
				trackables_provider<underlying_t> p(underlying);
				auto t = p.track(2);

				underlying.erase(underlying.begin() + 2u);

				// ACT
				underlying.push_back(make_pair("f", "abcdef"));
				p.fetch();

				// ASSERT
				assert_equal(4u, t->index());
			}

			test( AllTrackablesReturnNposAfterProviderIsDestroyed )
			{
				typedef vector< pair<string, string> > underlying_t;

				// INIT
				pair<string, string> values[]= {
					make_pair("a", "lorem"),
					make_pair("b", "dolor"),
					make_pair("f", "quot"),
					make_pair("C", "ipsum"),
					make_pair("fff", "amet"),
				};
				auto underlying = mkvector(values);

				// INIT / ACT
				auto p = make_shared< trackables_provider<underlying_t> >(underlying);
				shared_ptr<const wpl::trackable> t[] = {
					p->track(0), p->track(1), p->track(2), p->track(3), p->track(4),
				};

				// ACT
				p.reset();

				// ASSERT
				assert_equal(wpl::index_traits::npos(), t[0]->index());
				assert_equal(wpl::index_traits::npos(), t[1]->index());
				assert_equal(wpl::index_traits::npos(), t[2]->index());
				assert_equal(wpl::index_traits::npos(), t[3]->index());
				assert_equal(wpl::index_traits::npos(), t[4]->index());
			}
		end_test_suite
	}
}
