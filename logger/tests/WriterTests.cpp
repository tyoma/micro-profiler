#include <logger/writer.h>

#include <common/module.h>
#include <common/path.h>
#include <memory>
#include <mt/event.h>
#include <mt/thread.h>
#include <stdio.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace log
	{
		namespace tests
		{
			namespace
			{
				static int dummy;

				shared_ptr<FILE> open(const string &path)
				{	return shared_ptr<FILE>(fopen(path.c_str(), "r"), [] (FILE *p) { if (p) fclose(p); });	}

				pair<string, bool> read_line(FILE &f)
				{
					char buffer[1000];

					bool ok = !!fgets(buffer, sizeof(buffer), &f);
					return make_pair(buffer, ok);
				}
			}

			begin_test_suite( WriterTests )
				string base_path;
				vector<string> files;

				init( SetBasePath )
				{
					base_path = ~get_module_info(&dummy).path;
				}

				teardown( RemoveTemporaries )
				{
					for_each(files.begin(), files.end(), [] (string path) { remove(path.c_str()); });
				}


				test( FileCreatedWhenWriterIsCreated )
				{
					// INIT
					files.push_back(base_path & "test.lg");
					files.push_back(base_path & "test2.lg");
				
					// ACT
					writer_t w1 = create_writer(files[0]);
				
					// ASSERT
					assert_not_null(open(files[0]));
					assert_null(open(files[1]));
				
					// ACT
					writer_t w2 = create_writer(files[1]);
				
					// ASSERT
					assert_not_null(open(files[0]));
					assert_not_null(open(files[1]));
				}


				test( FileNameIsUniqualizedIfTheSameFileIsOpeneded )
				{
					// INIT
					files.push_back(base_path & "test.log");
					files.push_back(base_path & "test-2.log");
					files.push_back(base_path & "test-3.log");
					files.push_back(base_path & "test-4.log");
					files.push_back(base_path & "test-5.log");

					writer_t w = create_writer(files[0]);

					// ACT
					writer_t w2 = create_writer(files[0]); // gets 'test-2'

					// ASSERT
					assert_not_null(open(files[0]));
					assert_not_null(open(files[1]));
					assert_null(open(files[2]));

					// ACT
					writer_t w3 = create_writer(files[0]); // gets 'test-3'

					// ASSERT
					assert_not_null(open(files[0]));
					assert_not_null(open(files[1]));
					assert_not_null(open(files[2]));
					assert_null(open(files[3]));

					// INIT
					w2 = writer_t();
					remove(files[1].c_str());

					// ACT
					writer_t w2_ = create_writer(files[0]); // gets 'test-2' again

					// ASSERT
					assert_not_null(open(files[0]));
					assert_not_null(open(files[1]));
					assert_not_null(open(files[2]));
					assert_null(open(files[3]));

					// ACT
					writer_t w4 = create_writer(files[0]); // gets 'test-4'

					// ASSERT
					assert_not_null(open(files[0]));
					assert_not_null(open(files[1]));
					assert_not_null(open(files[2]));
					assert_not_null(open(files[3]));
					assert_null(open(files[4]));
				}


				test( TextIsWrittenToAFileWhenStreaming )
				{
					// INIT
					files.push_back(base_path & "test1.log");
					files.push_back(base_path & "test2.log");

					writer_t w1 = create_writer(files[0]);
					writer_t w2 = create_writer(files[1]);

					shared_ptr<FILE> f1 = open(files[0]);
					shared_ptr<FILE> f2 = open(files[1]);

					// ACT
					w1("Some loggers are better than the others\n");
					w1("Some are simpler\n");
					w1 = writer_t();
					w2("But sometimes logger is just a logger\n");
					w2 = writer_t();

					// ASSERT
					assert_equal("Some loggers are better than the others\n", read_line(*f1).first);
					assert_equal("Some are simpler\n", read_line(*f1).first);
					assert_equal("But sometimes logger is just a logger\n", read_line(*f2).first);
				}


				test( ConcurrentWritesAreSerialized )
				{
					// INIT
					const int n = 10000;
					mt::event ready, go;
					const string reference = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n";
					
					files.push_back(base_path & "test.log");

					writer_t w = create_writer(files[0]);

					mt::thread t([&] {

					// ACT
						ready.set();
						go.wait();
						for (int i = 0; i != n; ++i)
							w(reference.c_str());
					});

					ready.wait();
					go.set();
					for (int i = 0; i != n; ++i)
						w(reference.c_str());
					t.join();
					w = writer_t();

					// ASSERT
					shared_ptr<FILE> f = open(files[0]);

					for (pair<string, bool> r; r = read_line(*f), r.second; )
						assert_equal(reference, r.first);
				}


				test( NewWriterAppendsExistedFile )
				{
					// INIT
					files.push_back(base_path & "test.log");

					// ACT
					create_writer(files[0])("session #1\n");
					create_writer(files[0])("session #2\n");

					// ASSERT
					shared_ptr<FILE> f = open(files[0]);
					assert_equal("session #1\n", read_line(*f).first);
					assert_equal("session #2\n", read_line(*f).first);
				}


				test( AttemptToCreateWriterOverInvalidPathReturnsNullWriter )
				{
					// ACT
					auto w = create_writer("wer/\\wer?r");

					// ASSERT
					assert_is_true(!!w);

					// ACT / ASSERT (nothing happens)
					w("");
				}

			end_test_suite
		}
	}
}
