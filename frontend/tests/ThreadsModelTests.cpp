#include <frontend/threads_model.h>

#include <frontend/serialization.h>

#include <common/types.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>
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
			thread_info make_thread_info(unsigned native_id, string description, mt::milliseconds start_time,
				mt::milliseconds end_time, mt::milliseconds cpu_time, bool complete)
			{
				thread_info ti = { native_id, description, start_time, end_time, cpu_time, complete };
				return ti;
			}

			wstring get_text(const wpl::ui::list_model &m, unsigned index)
			{
				wstring text;

				m.get_text(index, text);
				return text;
			}
		}

		begin_test_suite( ThreadsModelTests )

			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;

			ThreadsModelTests()
				: ser(_buffer), dser(_buffer)
			{	}

			test( ThreadModelIsAListModel )
			{
				// INIT / ACT / ASSERT
				shared_ptr<wpl::ui::list_model> m(new threads_model);
			}


			test( ThreadModelCanBeDeserializedFromContainerData )
			{
				// INIT
				shared_ptr<threads_model> m1(new threads_model);
				shared_ptr<threads_model> m2(new threads_model);
				pair<unsigned int, thread_info> data1[] = {
					make_pair(11, make_thread_info(1717, "thread 1", mt::milliseconds(5001), mt::milliseconds(20707),
						mt::milliseconds(100), true)),
					make_pair(110, make_thread_info(11717, "thread 2", mt::milliseconds(10001), mt::milliseconds(0),
						mt::milliseconds(20003), false)),
				};
				pair<unsigned int, thread_info> data2[] = {
					make_pair(10, make_thread_info(1718, "thread #7", mt::milliseconds(1), mt::milliseconds(0),
						mt::milliseconds(180), false)),
					make_pair(110, make_thread_info(11717, "thread #2", mt::milliseconds(10001), mt::milliseconds(4),
						mt::milliseconds(10000), true)),
					make_pair(111, make_thread_info(22717, "thread #3", mt::milliseconds(5), mt::milliseconds(0),
						mt::milliseconds(1001), false)),
					make_pair(112, make_thread_info(32717, "thread #4", mt::milliseconds(20001), mt::milliseconds(0),
						mt::milliseconds(1002), false)),
				};

				ser(mkvector(data1));
				ser(mkvector(data2));

				// ACT
				dser(*m1);

				// ASSERT
				assert_equal(2u, m1->get_count());
				assert_equal(L"#11717 - thread 2 - CPU: 20s, started: +10s", get_text(*m1, 0));
				assert_equal(L"#1717 - thread 1 - CPU: 100ms, started: +5s, ended: +20.7s", get_text(*m1, 1));

				// ACT
				dser(*m2);

				// ASSERT
				assert_equal(4u, m2->get_count());
				assert_equal(L"#11717 - thread #2 - CPU: 10s, started: +10s, ended: +4ms", get_text(*m2, 0));
				assert_equal(L"#32717 - thread #4 - CPU: 1s, started: +20s", get_text(*m2, 1));
				assert_equal(L"#22717 - thread #3 - CPU: 1s, started: +5ms", get_text(*m2, 2));
				assert_equal(L"#1718 - thread #7 - CPU: 180ms, started: +1ms", get_text(*m2, 3));
			}


			test( DeserializationKeepsEntriesAndUpdatesExisting )
			{
				// INIT
				shared_ptr<threads_model> m(new threads_model);
				pair<unsigned int, thread_info> data1[] = {
					make_pair(11, make_thread_info(1717, "thread 1", mt::milliseconds(5001), mt::milliseconds(20707),
						mt::milliseconds(100), true)),
					make_pair(110, make_thread_info(11717, "thread 2", mt::milliseconds(10001), mt::milliseconds(0),
						mt::milliseconds(20003), false)),
				};
				pair<unsigned int, thread_info> data2[] = {
					make_pair(10, make_thread_info(1718, "thread #7", mt::milliseconds(1), mt::milliseconds(0),
						mt::milliseconds(180), false)),
					make_pair(110, make_thread_info(11717, "thread #2", mt::milliseconds(10001), mt::milliseconds(4),
						mt::milliseconds(10), true)),
				};

				ser(mkvector(data1));
				ser(mkvector(data2));
				dser(*m);

				// ACT
				dser(*m);

				// ASSERT
				assert_equal(3u, m->get_count());
				assert_equal(L"#1718 - thread #7 - CPU: 180ms, started: +1ms", get_text(*m, 0));
				assert_equal(L"#1717 - thread 1 - CPU: 100ms, started: +5s, ended: +20.7s", get_text(*m, 1));
				assert_equal(L"#11717 - thread #2 - CPU: 10ms, started: +10s, ended: +4ms", get_text(*m, 2));
			}


			test( ModelIsInvalidatedOnDeserialize )
			{
				// INIT
				shared_ptr<threads_model> m(new threads_model);
				pair<unsigned int, thread_info> data[] = {
					make_pair(11, make_thread_info(1717, "thread 1", mt::milliseconds(5001), mt::milliseconds(20707),
						mt::milliseconds(100), true)),
					make_pair(110, make_thread_info(11717, "thread 2", mt::milliseconds(10001), mt::milliseconds(0),
						mt::milliseconds(20003), false)),
				};
				bool invalidated = false;
				wpl::slot_connection c = m->invalidated += [&] {
					assert_equal(2u, m->get_count());
					invalidated = true;
				};

				ser(mkvector(data));

				// ACT
				dser(*m);

				// ASSERT
				assert_is_true(invalidated);
			}
		end_test_suite
	}
}
