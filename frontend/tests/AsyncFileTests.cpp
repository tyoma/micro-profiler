#include <frontend/async_file.h>

#include <list>
#include <test-helpers/file_helpers.h>
#include <test-helpers/helpers.h>
#include <test-helpers/mock_queue.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			template <typename T>
			void write(const string &path, const T &object)
			{
				write_file_stream w(path);
				strmd::serializer<write_file_stream, strmd::varint> s(w);

				s(object);
			}

			int dummy;
		}

		begin_test_suite( AsyncFileTests )
			temporary_directory dir;
			mocks::queue worker, apartment;


			test( RequestingFilePostsTasksToWorkerQueue )
			{
				// INIT / ACT
				shared_ptr<void> req[3];

				// ASSERT
				assert_is_empty(worker.tasks);

				// ACT
				async_file_read<int>(req[0], worker, apartment, dir.track_file("x.x"), [] (int &, file_deserializer &) {}, [] (shared_ptr<int>) {
					assert_is_false(true);
				});

				// ASSERT
				assert_equal(1u, worker.tasks.size());
				assert_not_null(req[0]);

				// ACT
				async_file_read<int>(req[1], worker, apartment, dir.track_file("y.x"), [] (int &, file_deserializer &) {}, [] (shared_ptr<int>) {});
				async_file_read<int>(req[2], worker, apartment, dir.track_file("z.x"), [] (int &, file_deserializer &) {}, [] (shared_ptr<int>) {});

				// ASSERT
				assert_is_empty(apartment.tasks);
				assert_equal(3u, worker.tasks.size());
				assert_not_null(req[1]);
				assert_not_null(req[2]);
				assert_not_equal(req[0], req[1]);
				assert_not_equal(req[1], req[2]);
				assert_not_equal(req[0], req[2]);
			}


			test( UnderlyingMethodIsCalledFromApartmentQueueIfCacheIsMissing )
			{
				// INIT
				vector<int> log;
				shared_ptr<void> req[5];

				async_file_read<int>(req[0], worker, apartment, dir.track_file("a.x"), [] (int &, file_deserializer &) {},
					[&] (shared_ptr<int> p) {	assert_null(p); log.push_back(1);	});
				async_file_read<int>(req[1], worker, apartment, dir.track_file("b.x"), [] (int &, file_deserializer &) {},
					[&] (shared_ptr<int> p) {	assert_null(p); log.push_back(2);	});
				async_file_read<int>(req[2], worker, apartment, dir.track_file("c.x"), [] (int &, file_deserializer &) {},
					[&] (shared_ptr<int> p) {	assert_null(p); log.push_back(3);	});
				async_file_read<int>(req[3], worker, apartment, dir.track_file("d.x"), [] (int &, file_deserializer &) {},
					[&] (shared_ptr<int> p) {	assert_null(p); log.push_back(4);	});

				// ACT
				worker.run_one();

				// ASSERT
				assert_is_empty(log);
				assert_equal(1u, apartment.tasks.size());

				// ACT (make an underlying request)
				apartment.run_one();

				// ASSERT
				int reference1[] = {	1,	};

				assert_equal(reference1, log);
				assert_is_empty(apartment.tasks);

				// ACT
				worker.run_one();

				// ASSERT
				assert_equal(reference1, log);
				assert_equal(1u, apartment.tasks.size());

				// ACT (make an underlying request)
				apartment.run_one();

				// ASSERT
				int reference2[] = {	1, 2,	};

				assert_equal(reference2, log);

				// ACT (task for the reset request is ignored in worker queue)
				req[2].reset();
				worker.run_one();

				// ASSERT
				assert_is_empty(apartment.tasks);

				// ACT (task for the reset request is ignored in apartment queue)
				worker.run_one();
				req[3].reset();
				apartment.run_one();

				// ASSERT
				assert_is_empty(apartment.tasks);
			}


			test( FileIsDeserializedInTheWorkerQueueAndDeliveredInTheApartmentOne )
			{
				// INIT
				auto called_wrk = 0;
				auto called_apt = 0;
				shared_ptr<void> req[2];
				string data1[] = {	"Lorem", "Ipsum", "Amet", "Dolor",	};
				int data2[] = {	3, 1, 4, 1, 5, 9,	2, 6,	};
				const auto f1 = dir.track_file("lorem.123123456");
				const auto f2 = dir.track_file("pi.A23123456");
				
				// INIT / ACT
				async_file_read< vector<string> >(req[0], worker, apartment, f1,
					[&] (vector<string> &value, file_deserializer &d) {

					assert_is_empty(value);

					d(value);
					called_wrk++;
				}, [&] (shared_ptr< vector<string> > p) {
					assert_equal(data1, *p); called_apt++;
				});
				async_file_read< vector<int> >(req[1], worker, apartment, f2,
					[&] (vector<int> &value, file_deserializer &d) {

					assert_is_empty(value);

					d(value);
					called_wrk += 10;
				}, [&] (shared_ptr< vector<int> > p) {
					assert_equal(data2, *p); called_apt += 10;
				});

				// ACT
				write(f1, mkvector(data1));
				worker.run_one();

				// ASSERT
				assert_equal(1, called_wrk);

				// ACT
				write(f2, mkvector(data2));
				worker.run_one();

				// ASSERT
				assert_equal(11, called_wrk);
				assert_equal(0, called_apt);

				// ACT
				apartment.run_one();

				// ASSERT
				assert_equal(11, called_wrk);
				assert_equal(1, called_apt);

				// ACT
				apartment.run_one();

				// ASSERT
				assert_equal(11, called_wrk);
				assert_equal(11, called_apt);
			}
		end_test_suite
	}
}
