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
				wpl::ui::table_model &t = l;

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
				wstring text;

				// ACT
				l.update(enumerate_processes(p1));

				// ASSERT
				assert_equal(2u, l.get_count());
				assert_equal(L"foo", (l.get_text(0, 0, text), text));
				assert_equal(L"12", (l.get_text(0, 1, text), text));
				assert_equal(L"bar", (l.get_text(1, 0, text), text));
				assert_equal(L"12111", (l.get_text(1, 1, text), text));

				// ACT
				l.update(enumerate_processes(p2));

				// ASSERT
				assert_equal(3u, l.get_count());
				assert_equal(L"FOO", (l.get_text(0, 0, text), text));
				assert_equal(L"1", (l.get_text(0, 1, text), text));
				assert_equal(L"bar", (l.get_text(1, 0, text), text));
				assert_equal(L"11111", (l.get_text(1, 1, text), text));
				assert_equal(L"BAZ", (l.get_text(2, 0, text), text));
				assert_equal(L"16111", (l.get_text(2, 1, text), text));
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
				wpl::slot_connection c = l.invalidated += [&] (size_t n) {
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
		end_test_suite
	}
}
