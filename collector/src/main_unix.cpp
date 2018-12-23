//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/frontend_controller.h>
#include <collector/entry.h>
#include <common/constants.h>
#include <common/time.h>
#include <ipc/endpoint.h>

using namespace micro_profiler;
using namespace std;

calls_collector *g_collector_ptr = nullptr;
frontend_controller *g_frontend_controller_ptr = nullptr;

extern "C" micro_profiler::handle *micro_profiler_initialize(void *image_address)
{
	vector<string> endpoints = c_candidate_endpoints;

	if (const char *env_id = getenv(c_frontend_id_env))
		endpoints.insert(endpoints.begin(), env_id);

	auto factory = [endpoints] (ipc::channel &inbound) {	
			for (vector<string>::const_iterator i = endpoints.begin(); i != endpoints.end(); ++i)
			{
				try
				{
					return ipc::connect_client(i->c_str(), inbound);
				}
				catch (const exception &)
				{
				}
			}

			struct dummy : ipc::channel
			{
				virtual void disconnect() throw()
				{	}

				virtual void message(const_byte_range /*payload*/)
				{	}
			};

			return channel_t(new dummy);
	};

	const size_t c_trace_limit = 5000000;
	static calls_collector g_collector(c_trace_limit);
	static auto_ptr<frontend_controller> g_frontend_controller(new frontend_controller(factory, g_collector,
		calibrate_overhead(g_collector, c_trace_limit / 10)));

	g_collector_ptr = &g_collector;
	g_frontend_controller_ptr = g_frontend_controller.get();

	return g_frontend_controller->profile(image_address);
}

extern "C" void __attribute__((destructor)) finish_micro_profiler()
{
	g_frontend_controller_ptr->force_stop();
}

extern "C" void __cyg_profile_func_enter(void *callee, void * /*call_site*/)
{
	const void *stck = 0;
	calls_collector::on_enter(g_collector_ptr, &stck, read_tick_counter(), callee);
}

extern "C" void __cyg_profile_func_exit(void * /*callee*/, void * /*call_site*/)
{
	const void *stck = 0;
	calls_collector::on_exit(g_collector_ptr, &stck, read_tick_counter());
}
