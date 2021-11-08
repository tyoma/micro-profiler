#include <frontend/multiplexing_request.h>

#include <functional>
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
			typedef multiplexing_request< int, function<void ()> > request_a;
			typedef multiplexing_request< string, function<void ()> > request_b;

			struct executor_0
			{
				template <typename T>
				void operator ()(const T &cb) const
				{	cb();	}
			};
		}

		begin_test_suite( MultiplexingRequestTests )
			test( FirstRequestAtAKeyCreatesAnEntryAndProvidesAnUnderlyingToSet )
			{
				// INIT / ACT
				auto m1 = make_shared<request_a::map_type>();
				auto m2 = make_shared<request_b::map_type>();
				shared_ptr<void> req[3];

				// ACT
				auto init1 = request_a::create(req[0], m1, 11, [] {});
				auto init2 = request_a::create(req[1], m1, 13, [] {});
				auto init3 = request_b::create(req[2], m2, "1311", [] {});

				// ASSERT
				assert_equal(2u, m1->size());
				assert_equal(1u, m2->size());

				assert_not_null(req[0]);
				assert_equal(1u, m1->count(11));
				assert_equal(&m1->find(11)->second, &init1.multiplexed);
				assert_not_null(init1.underlying);

				assert_not_null(req[1]);
				assert_equal(1u, m1->count(13));
				assert_equal(&m1->find(13)->second, &init2.multiplexed);
				assert_not_null(init2.underlying);

				assert_not_null(req[2]);
				assert_equal(1u, m2->count("1311"));
				assert_equal(&m2->find("1311")->second, &init3.multiplexed);
				assert_not_null(init3.underlying);
			}


			test( RepeatedRequestsDoNotProvideAnUnderlyingToSet )
			{
				// INIT / ACT
				auto m = make_shared<request_a::map_type>();
				shared_ptr<void> req[3];

				// ACT
				auto init1 = request_a::create(req[0], m, 11, [] {});
				auto init2 = request_a::create(req[1], m, 11, [] {});
				auto init3 = request_a::create(req[2], m, 11, [] {});

				// ASSERT
				assert_not_null(req[1]);
				assert_not_equal(req[0], req[1]);
				assert_equal(&init1.multiplexed, &init2.multiplexed);
				assert_null(init2.underlying);

				assert_not_null(req[2]);
				assert_not_equal(req[1], req[2]);
				assert_equal(&init1.multiplexed, &init3.multiplexed);
				assert_null(init3.underlying);
			}


			test( ChildRequestsHoldTheMap )
			{
				// INIT
				auto m1 = make_shared<request_a::map_type>();
				weak_ptr<request_a::map_type> wm1 = m1;
				auto m2 = make_shared<request_a::map_type>();
				weak_ptr<request_a::map_type> wm2 = m2;
				shared_ptr<void> req[5];

				// INIT / ACT
				request_a::create(req[0], m1, 11, [] {});
				request_a::create(req[1], m1, 12, [] {});
				request_a::create(req[2], m2, 13, [] {});
				request_a::create(req[3], m2, 121, [] {});
				request_a::create(req[4], m2, 13, [] {});

				// ACT
				m1.reset();
				m2.reset();

				// ASSERT
				assert_is_false(wm1.expired());
				assert_is_false(wm2.expired());

				// ACT
				req[1].reset();
				req[4].reset();

				// ASSERT
				assert_is_false(wm1.expired());
				assert_is_false(wm2.expired());

				// ACT
				req[0].reset();
				req[3].reset();

				// ASSERT
				assert_is_true(wm1.expired());
				assert_is_false(wm2.expired());

				// ACT
				req[2].reset();

				// ASSERT
				assert_is_true(wm2.expired());
			}


			test( InvocationCallsAllCallbacksAlive )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				int called[5] = {	0	};
				shared_ptr<void> req[5];

				auto init1 = request_a::create(req[0], m, 1, [&] {	called[0]++;	});
				request_a::create(req[1], m, 1, [&] {	called[1]++;	});
				request_a::create(req[2], m, 1, [&] {	called[2]++;	});
				auto init4 = request_a::create(req[3], m, 2, [&] {	called[3]++;	});
				request_a::create(req[4], m, 2, [&] {	called[4]++;	});

				// ACT
				init1.multiplexed.invoke(executor_0());

				// ASSERT
				int reference1[] = {	1, 1, 1, 0, 0,	};

				assert_equal(reference1, mkvector(called));

				// ACT
				init4.multiplexed.invoke(executor_0());

				// ASSERT
				int reference2[] = {	1, 1, 1, 1, 1,	};

				assert_equal(reference2, mkvector(called));

				// ACT
				init4.multiplexed.invoke(executor_0());

				// ASSERT
				int reference3[] = {	1, 1, 1, 2, 2,	};

				assert_equal(reference3, mkvector(called));
			}


			test( OnlyCallbacksAliveAreExecuted )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				int called[5] = {	0	};
				shared_ptr<void> req[5];

				auto init1 = request_a::create(req[0], m, 1, [&] {	called[0]++;	});
				request_a::create(req[1], m, 1, [&] {	called[1]++;	}), req[1].reset();
				request_a::create(req[2], m, 1, [&] {	called[2]++;	});
				request_a::create(req[3], m, 2, [&] {	called[3]++;	}), req[3].reset();
				auto init5 = request_a::create(req[4], m, 2, [&] {	called[4]++;	});

				// ACT
				init1.multiplexed.invoke(executor_0());

				// ASSERT
				int reference1[] = {	1, 0, 1, 0, 0,	};

				assert_equal(reference1, mkvector(called));

				// ACT
				init5.multiplexed.invoke(executor_0());

				// ASSERT
				int reference2[] = {	1, 0, 1, 0, 1,	};

				assert_equal(reference2, mkvector(called));

				// ACT
				req[2].reset();
				init1.multiplexed.invoke(executor_0());

				// ASSERT
				int reference3[] = {	2, 0, 1, 0, 1,	};

				assert_equal(reference3, mkvector(called));
			}


			test( InvocationsContinueOnSelfDestroy )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				int called[4] = {	0	};
				shared_ptr<void> req[4];

				auto init1 = request_a::create(req[0], m, 1, [&] {	called[0]++;	});
				request_a::create(req[1], m, 1, [&] {	called[1]++, req[1].reset();	});
				request_a::create(req[2], m, 1, [&] {	called[2]++;	});
				request_a::create(req[3], m, 1, [&] {	called[3]++, req[3].reset();	});

				// ACT
				init1.multiplexed.invoke(executor_0());

				// ASSERT
				int reference[] = {	1, 1, 1, 1,	};

				assert_equal(reference, mkvector(called));
			}


			test( UnderlyingRequestIsReleasedWithTheLastRemovalOfAChild )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				const auto u = make_shared<bool>();
				shared_ptr<void> req[3];

				auto init = request_a::create(req[0], m, 1, [&] {	});
				request_a::create(req[1], m, 1, [&] {	});
				request_a::create(req[2], m, 2, [&] {	});

				*init.underlying = u;

				// ACT
				req[0].reset();

				// ASSERT
				assert_not_equal(1, u.use_count());
				assert_equal(2u, m->size());

				// ACT
				req[1].reset();

				// ASSERT
				assert_equal(1, u.use_count());
				assert_equal(1u, m->size());
				assert_equal(1u, m->count(2));
				
			}


			test( CallbacksForRequestsRemovedDuringInvocationAreNotInvoked )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				int called[4] = {	0	};
				shared_ptr<void> req[4];

				auto init = request_a::create(req[0], m, 1, [&] {	called[0]++; req[3].reset();	});
				request_a::create(req[1], m, 1, [&] {	called[1]++; req[2].reset();	});
				request_a::create(req[2], m, 1, [&] {	called[2]++;	});
				request_a::create(req[3], m, 1, [&] {	called[3]++;	});

				// ACT
				init.multiplexed.invoke(executor_0());

				// ASSERT
				int reference[] = {	1, 1, 0, 0,	};

				assert_equal(reference, mkvector(called));
			}


			test( RequestRemovalAfterInvocationIsWorking )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				const auto u = make_shared<bool>();
				shared_ptr<void> req[2];
				auto init = request_a::create(req[0], m, 1, [&] {	});
				request_a::create(req[1], m, 1, [&] {	});

				*init.underlying = u;

				// ACT
				init.multiplexed.invoke(executor_0());
				req[0].reset();
				req[1].reset();

				// ASSERT
				assert_equal(1, u.use_count());
			}


			test( RequestRemovalAfterExceptionIsWorking )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				const auto u = make_shared<bool>();
				shared_ptr<void> req;
				auto init = request_a::create(req, m, 1, [&] {	throw 0;	});

				*init.underlying = u;

				// ACT
				assert_throws(init.multiplexed.invoke(executor_0()), int);
				req.reset();

				// ASSERT
				assert_equal(1, u.use_count());
			}


			test( UnderlyingRequestIsReleasedIfAllChildRequestsAreReleasedDuringInvocation )
			{
				// INIT
				auto m = make_shared<request_a::map_type>();
				auto u = make_shared<bool>();
				int called[3] = {	0	};
				shared_ptr<void> req[3];
				auto init = request_a::create(req[0], m, 1, [&] {	called[0]++, req[1].reset();	});
				request_a::create(req[1], m, 1, [&] {	called[1]++;	});
				request_a::create(req[2], m, 1, [&, u, m] {
					auto uu = u;
					auto mm = m;

					called[2]++; req[0].reset(), req[2].reset();
					assert_not_equal(1, uu.use_count());
					assert_not_equal(1, mm.use_count());
				});

				*init.underlying = u;
				u.reset();
				m.reset();

				// ACT
				init.multiplexed.invoke(executor_0());

				// ASSERT
				int reference[] = {	1, 0, 1,	};

				assert_equal(reference, mkvector(called));
			}


			test( SelfDestructionOfALastRequestWorksFine )
			{
				// INIT
				auto m = make_shared<request_a::map_type>();
				shared_ptr<void> req;
				auto init = request_a::create(req, m, 1, [&] {	req.reset();	});

				m.reset();

				// ACT / ASSERT
				init.multiplexed.invoke(executor_0());
			}


			test( SelfDestructionOfAllChildrenOfTheLastRequestWorksFine )
			{
				// INIT
				auto m = make_shared<request_a::map_type>();
				shared_ptr<void> req[3];
				auto init = request_a::create(req[0], m, 1, [&] {	req[1].reset(), req[2].reset(), req[0].reset();	});
				request_a::create(req[1], m, 1, [&] {	});
				request_a::create(req[2], m, 1, [&] {	});

				m.reset();

				// ACT / ASSERT
				init.multiplexed.invoke(executor_0());
			}


			test( UnderlyingRequestIsNotReleasedIfAtLeastOneChildHoldsItAfterInInvocationReleases )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				auto u = make_shared<bool>();
				shared_ptr<void> req[3];
				auto init = request_a::create(req[0], m, 1, [&] {	req[1].reset();	});
				request_a::create(req[1], m, 1, [&] {	});
				request_a::create(req[2], m, 1, [&] {	req[2].reset();	});

				*init.underlying = u;

				// ACT
				init.multiplexed.invoke(executor_0());

				// ASSERT
				assert_not_equal(1, u.use_count());
			}


			test( UnderlyingRequestIsNeededForRepeatedInitializaionOfAMultiplexedRecord )
			{
				// INIT
				const auto m = make_shared<request_a::map_type>();
				shared_ptr<void> req[3];
				auto init = request_a::create(req[0], m, 1, [&] {	});
				request_a::create(req[1], m, 1, [&] {	req[0].reset(), req[1].reset();	});

				// ACT
				init.multiplexed.invoke(executor_0());
				auto init2 = request_a::create(req[2], m, 1, [&] {	});

				// ASSERT
				assert_not_null(init2.underlying);
			}
		end_test_suite
	}
}
