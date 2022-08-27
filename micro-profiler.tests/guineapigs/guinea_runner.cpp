#include "guinea_runner.h"

#include <common/time.h>
#include <ipc/endpoint_spawn.h>
#include <ipc/server_session.h>
#include <map>
#include <test-helpers/helpers.h>

#ifdef _WIN32
	#include <process.h>

	#define getpid _getpid
#else
	#include <unistd.h>
#endif

using namespace std;

namespace micro_profiler
{
	using namespace tests;

	ipc::channel_ptr_t ipc::spawn::create_session(const vector<string> &/*arguments*/, ipc::channel &outbound)
	{
		auto session = make_shared<ipc::server_session>(outbound);
		auto images = make_shared< map<string, image> >();

		session->add_handler(load_module, [images] (ipc::server_session::response &resp, const string &path) {
			images->insert(make_pair(path, image(path)));
			resp(1);
		});
		session->add_handler(unload_module, [images] (ipc::server_session::response &resp, const string &path) {
			images->erase(path);
			resp(1);
		});
		session->add_handler(get_process_id, [images] (ipc::server_session::response &resp, int) {
			resp(1, static_cast<unsigned>(getpid()));
		});
		session->add_handler(run_load, [] (ipc::server_session::response &resp, int cpu_time_ms) {
			auto t0 = clock();

			while (clock() - t0 < cpu_time_ms)
				for (volatile int n = 10000; n; n--)
				{	}
			resp(1);
		});
		return session;
	}
}
