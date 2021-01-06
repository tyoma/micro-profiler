		mt::milliseconds delay;

		test( ScheduledTaskIsNotExecutedImmediately )
		{
			// INIT
			queue_ptr queue(create_queue());

			// ACT / ASSERT
			queue->schedule([] {	assert_is_true(false);	}, delay);
		}


		test( ScheduledTaskIsExecutedWhileProcessingOtherTasks )
		{
			// INIT
			bool called = false;
			queue_ptr queue(create_queue());

			queue->schedule([&called] {	called = true;	}, delay);
			queue->schedule([&] {	stop(queue);	}, 2 * delay);

			// ACT
			run(queue);

			// ASSERT
			assert_is_true(called);
		}


		test( ScheduledTasksAreExecutedInOrder )
		{
			// INIT
			vector<int> v;
			queue_ptr queue(create_queue());

			queue->schedule([&v] {	v.push_back(1);	}, delay);
			queue->schedule([&v] {	v.push_back(2);	}, delay.count() ? 2 * delay : mt::milliseconds(0));
			queue->schedule([&v] {	v.push_back(3);	}, delay.count() ? 5 * delay : mt::milliseconds(0));
			queue->schedule([&] {	stop(queue);	}, 10 * delay);

			// ACT
			run(queue);

			// ASSERT
			int reference[] = {	1, 2, 3	};
			assert_content_equal(reference, v);
		}


		test( ScheduledTasksAreHeldIfQueueIsAlive )
		{
			// INIT
			shared_ptr<bool> check1 = make_shared<bool>();
			shared_ptr<bool> check2 = make_shared<bool>();
			queue_ptr queue(create_queue());

			// ACT
			queue->schedule([check1] {	}, delay);
			queue->schedule([check2] {	}, delay);

			// ASSERT
			assert_is_false(unique(check1));
			assert_is_false(unique(check2));
		}


		test( ScheduledTasksAreDestroyedOnQueueDestruction )
		{
			// INIT
			shared_ptr<bool> check1 = make_shared<bool>();
			shared_ptr<bool> check2 = make_shared<bool>();
			queue_ptr queue(create_queue());

			// ACT
			queue->schedule([check1] {	}, delay);
			queue->schedule([check2] {	}, delay);
			queue.reset();

			// ASSERT
			assert_is_true(unique(check1));
			assert_is_true(unique(check2));
		}


		test( InterthreadPostingIsPossible )
		{
			// INIT
			auto properThread = false;
			queue_ptr queueM(create_queue());
			const auto threadidM = mt::this_thread::get_id();
			mt::thread::id threadidW;
			queue_ptr queueW;

			auto schedule_stop = [&] {	queueM->schedule([&] {	stop(queueM);	}, delay);	};
			auto schedule_1 = [&] {
				properThread = properThread && threadidW == mt::this_thread::get_id();
				stop(queueW);
				schedule_stop();
			};
			auto schedule_2 = [&] {
				properThread = threadidM == mt::this_thread::get_id();
				queueW->schedule(move(schedule_1), delay);
			};

			// ACT
			mt::thread t([&] {
				threadidW = mt::this_thread::get_id();
				queueW.reset(create_queue());
				queueM->schedule([&] {	schedule_2();	}, delay);
				run(queueW);
			});

			run(queueM);

			// ASSERT
			t.join(); // Must unblock eventually.
			assert_is_true(properThread);
		}


		test( InterthreadMessageBouncing )
		{
			// INIT
			int count = 100;
			queue_ptr queueM(create_queue());
			queue_ptr queueW;

			function<void(int)> f = [&] (int id) {
				switch (id)
				{
				case 0:
					if (!count)
					{
						stop(queueM);
						queueW->schedule([&] { stop(queueW); }, delay);
					}
					else
					{
						--count;
						queueW->schedule([&] { f(1); }, delay);
					}
					break;

				case 1:
				default:
					queueM->schedule([&] { f(0); }, delay);
				}
			};

			// ACT
			mt::thread t([&] {
				queueW.reset(create_queue());

				queueM->schedule([&] { f(0); }, delay);

				run(queueW);
			});

			run(queueM);

			// ASSERT
			t.join(); // Must unblock eventually.

			assert_equal(0, count);
		}


		test( TasksAfterScheduledSelfDestructionAreNotExecuted )
		{
			// INIT
			queue_ptr queue(create_queue());
			queue_ptr stopper(create_queue());
			auto mustNotBeSet = false;

			// ACT
			queue->schedule([&] {	queue.reset();	}, delay);
			queue->schedule([&] {	mustNotBeSet = true;	}, delay);
			stopper->schedule([&] {	stop(stopper);	}, delay);
			run(stopper);

			// ASSERT
			assert_is_false(mustNotBeSet);
		}
