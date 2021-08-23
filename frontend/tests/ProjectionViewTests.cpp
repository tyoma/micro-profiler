#include <frontend/projection_view.h>

#include "helpers.h"

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
			struct POD
			{
				int a;
				int b;
				double c;
			};

			POD mkpod(int a, int b, double c)
			{
				POD v = {	a,b ,c	};
				return v;
			}

			template <typename T>
			vector<T> get_values(const wpl::list_model<T> &model)
			{
				vector<T> result;
				const auto l = model.get_count();

				for (auto i = l - l; i != l; ++i)
					result.resize(result.size() + 1), model.get_value(i, result.back());
				return result;
			}
		}

		begin_test_suite( ProjectionViewTests )
			test( ProjectionViewIsAListModel )
			{
				// INIT
				vector< pair<string, POD> > u1;
				vector< pair<int, string> > u2;

				// INIT / ACT
				projection_view<vector< pair<string, POD> >, double> pv1(u1);
				projection_view<vector< pair<int, string> >, string> pv2(u2);

				// INIT / ACT
				wpl::list_model<double> &m1 = pv1;
				wpl::list_model<string> &m2 = pv2;

				// ASSERT
				assert_equal(0u, m1.get_count());
				assert_equal(0u, m2.get_count());
			}


			test( ValueIsProjectedAccordinglyToTheTransform )
			{
				typedef pair<int, POD> value_type;

				// INIT
				value_type values[] = {
					make_pair(101, mkpod(13, 109, 10.4)),
					make_pair(102, mkpod(17, 110, 11.4)),
					make_pair(103, mkpod(18, 1109, 10.3)),
					make_pair(104, mkpod(19, 11, 19.7)),
				};
				auto underlying = mkvector(values);
				projection_view<vector<value_type>, double> pv1(underlying);
				projection_view<vector<value_type>, int> pv2(underlying);

				// ACT
				pv1.project([] (value_type v) {	return v.second.c;	});
				auto r1 = get_values(pv1);

				// ASSERT
				double reference1[] = {	10.4, 11.4, 10.3, 19.7,	};

				assert_equal(reference1, r1);

				// ACT
				pv2.project([] (value_type v) {	return v.second.a;	});
				auto r2 = get_values(pv2);

				// ASSERT
				double reference2[] = {	13, 17, 18, 19,	};

				assert_equal(reference2, r2);

				// ACT
				pv2.project([] (value_type v) {	return v.second.b;	});
				r2 = get_values(pv2);

				// ASSERT
				double reference3[] = {	109, 110, 1109, 11,	};

				assert_equal(reference3, r2);

				// ACT
				underlying.push_back(make_pair(1000, mkpod(100, 100, 100.1)));
				r1 = get_values(pv1);
				r2 = get_values(pv2);

				// ASSERT
				double reference4[] = {	10.4, 11.4, 10.3, 19.7, 100.1,	};
				double reference5[] = {	109, 110, 1109, 11, 100,	};

				assert_equal(reference4, r1);
				assert_equal(reference5, r2);
			}


			test( ProjectionBecomesEmptyOnReset )
			{
				typedef pair<int, POD> value_type;

				// INIT
				value_type values[] = {
					make_pair(101, mkpod(13, 109, 10.4)),
					make_pair(102, mkpod(17, 110, 11.4)),
					make_pair(103, mkpod(18, 1109, 10.3)),
					make_pair(104, mkpod(19, 11, 19.7)),
				};
				auto underlying = mkvector(values);
				projection_view<vector<value_type>, double> pv(underlying);

				pv.project([] (value_type v) {	return v.second.c;	});

				// ACT
				pv.project();

				// ASSERT
				assert_equal(0u, pv.get_count());
			}


			test( ProjectionIsInvalidatedOnMutation )
			{
				typedef pair<int, POD> value_type;

				// INIT
				value_type values[] = {
					make_pair(101, mkpod(13, 109, 10.4)),
					make_pair(102, mkpod(17, 110, 11.4)),
					make_pair(103, mkpod(18, 1109, 10.3)),
					make_pair(104, mkpod(19, 11, 19.7)),
				};
				auto underlying = mkvector(values);
				projection_view<vector<value_type>, double> pv(underlying);
				auto invalidations = 0;
				vector<unsigned> log;
				auto conn = pv.invalidate += [&] (projection_view<vector<value_type>, double>::index_type item) {

				// ACT
					invalidations++;
					log.push_back(static_cast<unsigned>(pv.get_count()));

				// ASSERT
					assert_equal(wpl::index_traits::npos(), item);
				};

				// ACT
				pv.fetch();

				// ASSERT
				assert_equal(1, invalidations);
				assert_equal(plural + 0u, log);

				// ACT
				pv.project([] (value_type v) {	return v.second.c;	});
				pv.fetch();

				// ASSERT
				assert_equal(3, invalidations);
				assert_equal(plural + 0u + 4u + 4u, log);

				// ACT
				pv.project();

				// ASSERT
				assert_equal(4, invalidations);
				assert_equal(plural + 0u + 4u + 4u + 0u, log);
			}


			test( ProjectionViewProvidesTrackablesUpdatedOnFetch )
			{
				typedef pair<int, POD> value_type;

				// INIT
				value_type values[] = {
					make_pair(101, mkpod(13, 109, 10.4)),
					make_pair(102, mkpod(17, 110, 11.4)),
					make_pair(103, mkpod(18, 1109, 10.3)),
					make_pair(104, mkpod(19, 11, 19.7)),
				};
				auto underlying = mkvector(values);
				projection_view<vector<value_type>, double> pv(underlying);

				pv.project([] (value_type v) {	return v.second.c;	});

				// INIT / ACT
				auto t = pv.track(2);

				// ACT / ASSERT
				assert_equal(2u, t->index());

				// ACT
				underlying.insert(underlying.begin(), make_pair(10, mkpod(1, 1, 1)));
				pv.fetch();

				// ASSERT
				assert_equal(3u, t->index());
			}

		end_test_suite
	}
}
