//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include "main.h"

#include <collector/allocator.h>
#include <collector/calibration.h>
#include <collector/calls_collector.h>
#include <collector/collector_app.h>
#include <collector/thread_monitor.h>
#include <common/constants.h>
#include <common/memory.h>
#include <common/time.h>
#include <ipc/endpoint.h>
#include <ipc/misc.h>
#include <mt/thread_callbacks.h>
#include <patcher/image_patch_manager.h>

using namespace micro_profiler;
using namespace std;
using namespace std::placeholders;

namespace
{
	const string c_candidate_endpoints[] = {
		ipc::sockets_endpoint_id(ipc::localhost, 6100),
		ipc::com_endpoint_id(constants::integrated_frontend_id),
		ipc::com_endpoint_id(constants::standalone_frontend_id),
	};

	struct null_channel : ipc::channel
	{
		virtual void disconnect() throw()
		{	}

		virtual void message(const_byte_range /*payload*/)
		{	}
	};

	collector_app::channel_ptr_t probe_create_channel(ipc::channel &inbound)
	{
		vector<string> candidate_endpoints(c_candidate_endpoints, c_candidate_endpoints
			+ sizeof(c_candidate_endpoints) / sizeof(c_candidate_endpoints[0]));

		if (const char *env_id = getenv(constants::frontend_id_ev))
			candidate_endpoints.insert(candidate_endpoints.begin(), env_id);

		for (vector<string>::const_iterator i = candidate_endpoints.begin(); i != candidate_endpoints.end(); ++i)
		{
			try
			{
				return ipc::connect_client(i->c_str(), inbound);
			}
			catch (const exception &)
			{
			}
		}
		return collector_app::channel_ptr_t(new null_channel);
	}
}

const size_t c_trace_limit = 5000000;
const auto g_thread_monitor = make_shared<thread_monitor>(mt::get_thread_callbacks());
micro_profiler::allocator g_allocator;
calls_collector g_collector(g_allocator, c_trace_limit, *g_thread_monitor, mt::get_thread_callbacks());
extern "C" calls_collector *g_collector_ptr = &g_collector;
const auto c_overhead = calibrate_overhead(*g_collector_ptr, c_trace_limit);
executable_memory_allocator g_eallocator;
image_patch_manager g_patch_manager(g_collector, g_eallocator);
collector_app g_profiler_app(&probe_create_channel, g_collector, c_overhead, *g_thread_monitor, g_patch_manager);
const platform_initializer g_intializer(g_profiler_app);

#if defined(__clang__) || defined(__GNUC__)
	#define PUBLIC __attribute__ ((visibility ("default")))
#else
	#define PUBLIC
#endif

extern "C" PUBLIC void __cyg_profile_func_enter(void *callee, void * /*call_site*/)
{
	if (g_collector_ptr)
		g_collector_ptr->track(read_tick_counter(), callee);
}

extern "C" PUBLIC void __cyg_profile_func_exit(void * /*callee*/, void * /*call_site*/)
{
	if (g_collector_ptr)
		g_collector_ptr->track(read_tick_counter(), 0);
}
