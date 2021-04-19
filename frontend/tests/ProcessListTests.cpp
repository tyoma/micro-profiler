#include <frontend/process_list.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class process : public micro_profiler::process
			{
			public:
				process(unsigned pid, string name)
					: _pid(pid), _name(name)
				{	}

				virtual unsigned get_pid() const
				{	return _pid;	}

				virtual std::string name() const
				{	return _name;	}

				virtual void remote_execute(injection_function_t * /*injection_function*/, const_byte_range /*payload*/)
				{	}

			private:
				unsigned _pid;
				string _name;
			};
		}

		begin_test_suite( ProcessListTests )

			template <size_t n>
			static process_list::process_enumerator_t enumerate_processes(shared_ptr<process> (&processes)[n])
			{
				return [&] (const process::enumerate_callback_t &callback) {
					for (size_t i = 0; i != n; ++i)
						callback(processes[i]);
				};
			}

			test( ProcessListIsEmptyInitially )
			{
				// INIT / ACT
				process_list l;
				wpl::string_table_model &t = l;

				// ACT / ASSERT
				assert_equal(0u, t.get_count());
			}


			test( ProcessListEmptyAfterUpdateWithNoProcesses )
			{
				// INIT
				process_list l;

				// ACT
				l.update([] (const process::enumerate_callback_t &/*callback*/) {});

				// ASSERT
				assert_equal(0u, l.get_count());
			}


			test( ProcessListIsPopulatedFromEnumerator )
			{
				// INIT
				process_list l;
				shared_ptr<process> p1[] = {
					shared_ptr<process>(new mocks::process(12, "foo")),
					shared_ptr<process>(new mocks::process(12111, "bar")),
				};
				shared_ptr<process> p2[] = {
					shared_ptr<process>(new mocks::process(1, "FOO")),
					shared_ptr<process>(new mocks::process(11111, "bar")),
					shared_ptr<process>(new mocks::process(16111, "BAZ")),
				};
				string text;

				// ACT
				l.update(enumerate_processes(p1));

				// ASSERT
				assert_equal(2u, l.get_count());
				assert_equal("foo", (l.get_text(0, 0, text), text));
				assert_equal("12", (l.get_text(0, 1, text), text));
				assert_equal("bar", (l.get_text(1, 0, text), text));
				assert_equal("12111", (l.get_text(1, 1, text), text));

				// ACT
				l.update(enumerate_processes(p2));

				// ASSERT
				assert_equal(3u, l.get_count());
				assert_equal("FOO", (l.get_text(0, 0, text), text));
				assert_equal("1", (l.get_text(0, 1, text), text));
				assert_equal("bar", (l.get_text(1, 0, text), text));
				assert_equal("11111", (l.get_text(1, 1, text), text));
				assert_equal("BAZ", (l.get_text(2, 0, text), text));
				assert_equal("16111", (l.get_text(2, 1, text), text));
			}


			test( InvalidationIsSignalledOnUpdate )
			{
				// INIT
				size_t n_arg = 0, n_count = 0;
				process_list l;
				shared_ptr<process> p[] = {
					shared_ptr<process>(new mocks::process(12, "foo")),
					shared_ptr<process>(new mocks::process(12111, "bar")),
				};
				wpl::slot_connection c = l.invalidate += [&] (size_t n) {
					n_arg = n;
					n_count = l.get_count();
				};

				// ACT
				l.update(enumerate_processes(p));

				// ASSERT
				assert_equal(2u, n_arg);
				assert_equal(2u, n_count);

				// ACT
				l.update([&] (const process::enumerate_callback_t &/*callback*/) { });

				// ASSERT
				assert_equal(0u, n_arg);
				assert_equal(0u, n_count);
			}


			test( ProcessIsReturnedByIndex )
			{
				// INIT
				process_list l;
				shared_ptr<process> p[] = {
					shared_ptr<process>(new mocks::process(12, "foo")),
					shared_ptr<process>(new mocks::process(12111, "bar")),
					shared_ptr<process>(new mocks::process(12113, "BAZ")),
				};

				l.update(enumerate_processes(p));

				// ACT / ASSERT
				assert_equal(p[0], l.get_process(0));
				assert_equal(p[1], l.get_process(1));
				assert_equal(p[2], l.get_process(2));
			}


			test( ProcessListIsSortable )
			{
				// INIT
				process_list l;
				shared_ptr<process> p[] = {
					shared_ptr<process>(new mocks::process(12, "Lorem")),
					shared_ptr<process>(new mocks::process(12111, "Amet")),
					shared_ptr<process>(new mocks::process(1211, "Quand")),
				};
				string text;

				l.update(enumerate_processes(p));

				// ACT
				l.set_order(1, true);

				// ASSERT
				assert_equal("Lorem", (l.get_text(0, 0, text), text));
				assert_equal(p[0], l.get_process(0));
				assert_equal("Quand", (l.get_text(1, 0, text), text));
				assert_equal(p[2], l.get_process(1));
				assert_equal("Amet", (l.get_text(2, 0, text), text));
				assert_equal(p[1], l.get_process(2));

				// ACT
				l.set_order(1, false);

				// ASSERT
				assert_equal("Amet", (l.get_text(0, 0, text), text));
				assert_equal("Quand", (l.get_text(1, 0, text), text));
				assert_equal("Lorem", (l.get_text(2, 0, text), text));

				// ACT
				l.set_order(0, true);

				// ASSERT
				assert_equal("12111", (l.get_text(0, 1, text), text));
				assert_equal("12", (l.get_text(1, 1, text), text));
				assert_equal("1211", (l.get_text(2, 1, text), text));

				// ACT
				l.set_order(0, false);

				// ASSERT
				assert_equal("1211", (l.get_text(0, 1, text), text));
				assert_equal(p[2], l.get_process(0));
				assert_equal("12", (l.get_text(1, 1, text), text));
				assert_equal(p[0], l.get_process(1));
				assert_equal("12111", (l.get_text(2, 1, text), text));
				assert_equal(p[1], l.get_process(2));
			}


			test( SortOrderIsAppliedOnUpdate )
			{
				// INIT
				process_list l;
				shared_ptr<process> p1[] = {
					shared_ptr<process>(new mocks::process(12, "Lorem")),
					shared_ptr<process>(new mocks::process(12111, "Amet")),
					shared_ptr<process>(new mocks::process(1211, "Quand")),
				};
				shared_ptr<process> p2[] = {
					shared_ptr<process>(new mocks::process(12, "Lorem")),
					shared_ptr<process>(new mocks::process(12111, "Amet")),
					shared_ptr<process>(new mocks::process(1311, "Dolor")),
					shared_ptr<process>(new mocks::process(1211, "Quand")),
				};
				string text;

				l.set_order(0, true);

				// ACT
				l.update(enumerate_processes(p1));

				// ASSERT
				assert_equal("12111", (l.get_text(0, 1, text), text));
				assert_equal("12", (l.get_text(1, 1, text), text));
				assert_equal("1211", (l.get_text(2, 1, text), text));

				// ACT
				l.update(enumerate_processes(p2));

				// ASSERT
				assert_equal("12111", (l.get_text(0, 1, text), text));
				assert_equal("1311", (l.get_text(1, 1, text), text));
				assert_equal("12", (l.get_text(2, 1, text), text));
				assert_equal("1211", (l.get_text(3, 1, text), text));
			}
		end_test_suite
	}
}
