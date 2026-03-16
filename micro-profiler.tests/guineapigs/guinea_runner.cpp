#include "guinea_runner.h"

#include <common/time.h>
#include <coipc/endpoint_spawn.h>
#include <coipc/server_session.h>
#include <map>
#include <test-helpers/helpers.h>

#ifdef _WIN32
	#include <process.h>

	#define getpid _getpid
#else
	#include <unistd.h>
#endif

using namespace coipc;
using namespace micro_profiler;
using namespace std;


channel_ptr_t spawn::create_session(const vector<string> &/*arguments*/, channel &outbound)
{
	using namespace tests;

	auto session = make_shared<server_session>(outbound);
	auto images = make_shared< map<string, image> >();

	session->add_handler(load_module, [images] (server_session::response &resp, const string &path) {
		images->insert(make_pair(path, image(path)));
		resp(1);
	});
	session->add_handler(unload_module, [images] (server_session::response &resp, const string &path) {
		images->erase(path);
		resp(1);
	});
	session->add_handler(get_process_id, [images] (server_session::response &resp, int) {
		resp(1, static_cast<unsigned>(getpid()));
	});
	session->add_handler(run_load, [] (server_session::response &resp, int cpu_time_ms) {
		auto t0 = micro_profiler::clock();

		while (micro_profiler::clock() - t0 < cpu_time_ms)
			for (volatile int n = 10000; n; n--)
			{	}
		resp(1);
	});
	return session;
}
