#include <scheduler/ui_queue.h>

#include "helpers.h"

#include <mt/event.h>
#include <mt/thread.h>
#include <stdexcept>
#include <ut/assert.h>
#include <ut/test.h>

#pragma warning(disable: 4355)

using namespace std;

namespace scheduler
{
	namespace tests
	{
		namespace
		{
			class threaded_message_loop
			{
			public:
				threaded_message_loop(const function<void ()> &loop_init, const function<void ()> &loop_term = [] {})
					: _thread([this, loop_init, loop_term] {
						_loop = _loop->create();
						loop_init();
						_ready.set();
						_loop->run();
						loop_term();
						_loop.reset();
					})
				{	_ready.wait();	}

				mt::thread::id get_id() const
				{	return _thread.get_id();	}

				~threaded_message_loop()
				{
					_loop->exit();
					_thread.join();
				}

			private:
				mt::event _ready;
				shared_ptr<message_loop> _loop;
				mt::thread _thread;
			};
		}

		begin_test_suite( AUIQueueTests )
			test( ScheduledUndeferredTasksAreExecutedInHostingThreads )
			{
				// INIT
				mt::thread::id id[2];
				mt::milliseconds time;
				shared_ptr<queue> queue_[2];
				mt::event done[2];
				threaded_message_loop tl1([&] {
					queue_[0].reset(new ui_queue([&] {	return time;	}));
				}, [&] {	queue_[0].reset();	});
				threaded_message_loop tl2([&] {
					queue_[1].reset(new ui_queue([&] {	return time;	}));
				}, [&] {	queue_[1].reset();	});

				// ACT
				queue_[0]->schedule([&] {	id[0] = mt::this_thread::get_id(), done[0].set();	});
				queue_[1]->schedule([&] {	id[1] = mt::this_thread::get_id(), done[1].set();	});

				done[0].wait();
				done[1].wait();

				// ASSERT
				assert_equal(tl1.get_id(), id[0]);
				assert_equal(tl2.get_id(), id[1]);
			}


			test( TasksAreExecutedInOrderOfScheduling )
			{
				// INIT
				vector<int> data;
				mt::milliseconds time;
				shared_ptr<queue> queue_;
				mt::event done;
				threaded_message_loop tl([&] {
					queue_.reset(new ui_queue([&] {	return time;	}));
				}, [&] {	queue_.reset();	});

				// ACT
				queue_->schedule([&] {	data.push_back(3);	});
				queue_->schedule([&] {	data.push_back(1);	});
				queue_->schedule([&] {	data.push_back(4);	});
				queue_->schedule([&] {	data.push_back(1);	});
				queue_->schedule([&] {	done.set();	});
				done.wait();

				// ASSERT
				int reference1[] = {	3, 1, 4, 1,	};

				assert_equal(reference1, data);

				// ACT
				queue_->schedule([&] {	data.push_back(5);	});
				queue_->schedule([&] {	data.push_back(9);	});
				queue_->schedule([&] {	data.push_back(2);	});
				queue_->schedule([&] {	done.set();	});
				done.wait();

				// ASSERT
				int reference2[] = {	3, 1, 4, 1, 5, 9, 2,	};

				assert_equal(reference2, data);
			}


			test( DeferredTasksAreExecutedAccordinglyToTheirTimeouts )
			{
				// INIT
				shared_ptr<queue> queue_;
				mt::event done;
				threaded_message_loop tl([&] {
					queue_.reset(new ui_queue(get_clock()));
				}, [&] {	queue_.reset();	});
				mt::milliseconds delay[3];

				// ACT
				const auto sw1 = create_stopwatch();
				const auto sw2 = create_stopwatch();
				const auto sw3 = create_stopwatch();

				queue_->schedule([&] {
					delay[0] = sw1();
				}, mt::milliseconds(150));
				queue_->schedule([&] {
					delay[1] = sw2();
				}, mt::milliseconds(500));
				queue_->schedule([&] {
					delay[2] = sw3(), done.set();
				}, mt::milliseconds(700));
				done.wait();

				// ASSERT
				assert_approx_equal(150.0, 1.0 * delay[0].count(), 0.15);
				assert_approx_equal(500.0, 1.0 * delay[1].count(), 0.15);
				assert_approx_equal(700.0, 1.0 * delay[2].count(), 0.15);
			}


			test( ExceptionThrownByATaskIsConsumed )
			{
				// INIT
				shared_ptr<queue> queue_;
				mt::event done;
				threaded_message_loop tl([&] {
					queue_.reset(new ui_queue(get_clock()));
				}, [&] {	queue_.reset();	});

				// ACT / ASSERT (must exit)
				queue_->schedule([&] {	throw 0;	});
				queue_->schedule([&] {	throw runtime_error("dummy");	});
				queue_->schedule([&] {	throw 0;	}, mt::milliseconds(10));
				queue_->schedule([&] {	throw runtime_error("dummy");	}, mt::milliseconds(10));
				queue_->schedule([&] {	done.set();	}, mt::milliseconds(100));
				done.wait();
			}
		end_test_suite
	}
}
