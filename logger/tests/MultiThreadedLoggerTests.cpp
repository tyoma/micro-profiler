#include <logger/multithreaded_logger.h>

#include <common/memory.h>
#include <mt/event.h>
#include <mt/thread.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( MultiThreadedLoggerTests )
			string buffer;
			datetime dt;
			function<void (const char *)> writer;
			function<datetime ()> time_provider;
			unsigned skip;

			void set_time(int year = 0, unsigned month = 0, unsigned day = 0, unsigned hour = 0, unsigned minute = 0,
				unsigned second = 0, unsigned millisecond = 0)
			{
				dt.year = year, dt.month = month, dt.day = day;
				dt.hour = hour, dt.minute = minute, dt.second = second, dt.millisecond = millisecond;
			}

			init( Init )
			{
				skip = 1;
				writer = [this] (const char *text) {
					if (!skip)
						buffer += text;
					else
						--skip;
				};
				time_provider = [this] {
					return dt;
				};
			}


			test( IncompleteLoggerCycleDoesNotWriteAnything )
			{
				// INIT
				skip = 3;
				log::multithreaded_logger l1(writer, time_provider);
				log::multithreaded_logger l2(writer, time_provider);
				log::multithreaded_logger l3(writer, time_provider);

				// ACT
				l1.begin("zubazuba", log::info);
				l1.add_attribute(A(123));

				l2.add_attribute(A("abc"));
				l2.commit();

				l3.commit();
				l3.begin("Cucu", log::severe);
				l3.add_attribute(A(12346));

				// ASSERT
				assert_is_empty(buffer);
			}


			test( CompleteCycleWithNoArgumentsWritesLoggedEventToBuffer )
			{
				// INIT
				log::multithreaded_logger l(writer, time_provider);

				// ACT
				set_time(120, 2, 19, 19, 11, 41, 157);
				l.begin("Lorem ipsum...", log::info);
				l.commit();

				// ASSERT
				assert_equal("20200219T191141.157Z I Lorem ipsum...\n", buffer);

				// INIT
				buffer.clear();

				// ACT
				set_time(70, 1, 9, 5, 7, 1, 5);
				l.begin("amet dolor!", log::severe);
				l.commit();

				// ASSERT
				assert_equal("19700109T050701.005Z S amet dolor!\n", buffer);
			}


			test( CompleteCycleWithArgumentsWritesLoggedEventToBuffer )
			{
				// INIT
				log::multithreaded_logger l(writer, time_provider);

				// ACT
				set_time(120, 2, 19, 19, 11, 41, 157);
				l.begin("message #1", log::info);
				l.add_attribute(A(15));
				l.add_attribute(A(1011211l));
				l.add_attribute(A("z"));
				l.commit();

				// ASSERT
				assert_equal("20200219T191141.157Z I message #1\n\t15: 15\n\t1011211l: 1011211\n\t\"z\": z\n", buffer);

				// INIT
				buffer.clear();
				string s = "Pride only hurts. It never helps.";

				// ACT
				set_time(10, 1, 9, 5, 7, 1, 5);
				l.begin("Pulp fiction", log::severe);
				l.add_attribute(A(s));
				l.commit();

				// ASSERT
				assert_equal("19100109T050701.005Z S Pulp fiction\n\ts: Pride only hurts. It never helps.\n", buffer);
			}


			test( ConcurrentThreadFillsItsOwnBuffer )
			{
				// INIT
				mt::event ready, go;
				log::multithreaded_logger l(writer, time_provider);

				set_time();

				// ACT
				mt::thread t([&] {
					l.begin("other thread message", log::info);
					l.add_attribute(A("123"));
					ready.set();
					go.wait();
					l.commit();
					ready.set();
				});
				l.begin("main thread message", log::severe);
				l.add_attribute(A(1));
				l.add_attribute(A(17));
				ready.wait();
				l.commit();

				// ASSERT
				assert_equal("19000000T000000.000Z S main thread message\n\t1: 1\n\t17: 17\n", buffer);

				// INIT
				buffer.clear();

				// ACT
				go.set();
				ready.wait();

				// ASSERT
				assert_equal("19000000T000000.000Z I other thread message\n\t\"123\": 123\n", buffer);

				t.join();
			}


			test( DateFormattingDoesNotOverflow )
			{
				// INIT
				log::multithreaded_logger l(writer, time_provider);

				mem_set(&dt, 0xFF, sizeof(dt));

				// ACT
				l.begin("", log::severe);
				l.commit();

				// ASSERT
				assert_equal("21551531T316363.023Z S \n", buffer);
			}

		end_test_suite
	}
}
