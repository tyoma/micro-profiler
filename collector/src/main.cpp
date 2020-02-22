//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/calibration.h>
#include <collector/calls_collector.h>
#include <collector/collector_app.h>
#include <common/time.h>
#include <common/constants.h>
#include <ipc/endpoint.h>

using namespace micro_profiler;
using namespace std;
using namespace std::placeholders;

namespace
{
	struct null_channel : ipc::channel
	{
		virtual void disconnect() throw()
		{	}

		virtual void message(const_byte_range /*payload*/)
		{	}
	};

	collector_app::channel_t probe_create_channel(ipc::channel &inbound)
	{
		vector<string> candidate_endpoints = c_candidate_endpoints;

		if (const char *env_id = getenv(c_frontend_id_ev))
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
		return collector_app::channel_t(new null_channel);
	}
}

const size_t c_trace_limit = 5000000;
const shared_ptr<calls_collector> g_collector(new calls_collector(c_trace_limit));
extern "C" calls_collector *g_collector_ptr = g_collector.get();
const overhead c_overhead = calibrate_overhead(*g_collector_ptr, c_trace_limit / 10);
collector_app g_profiler_app(&probe_create_channel, g_collector, c_overhead);
const platform_initializer g_intializer(g_profiler_app);

#if defined(__clang__) || defined(__GNUC__)
	#define PUBLIC __attribute__ ((visibility ("default")))
#else
	#define PUBLIC
#endif

extern "C" PUBLIC void __cyg_profile_func_enter(void *callee, void * /*call_site*/)
{	g_collector_ptr->on_enter_nostack(read_tick_counter(), callee);	}

extern "C" PUBLIC void __cyg_profile_func_exit(void * /*callee*/, void * /*call_site*/)
{	g_collector_ptr->on_exit_nostack(read_tick_counter());	}
