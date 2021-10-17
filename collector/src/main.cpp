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

#include <collector/calibration.h>
#include <collector/thread_monitor.h>
#include <common/constants.h>
#include <common/module.h>
#include <common/path.h>
#include <common/time.h>
#include <ipc/endpoint.h>
#include <ipc/misc.h>
#include <logger/writer.h>
#include <mt/thread_callbacks.h>

#ifdef _WIN32
	#include <process.h>
	
	#define getpid _getpid
	#define mkdir _mkdir

	extern "C" int _mkdir(const char *pathname, int mode);

#else
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <unistd.h>

#endif

#define PREAMBLE "Profiler Instance: "

using namespace std;

#ifdef _MSC_VER
	extern "C"
#endif
	micro_profiler::calls_collector *g_collector_ptr = nullptr;

namespace micro_profiler
{
	namespace
	{
		const size_t c_trace_limit = 5000000;


		struct null_channel : ipc::channel
		{
			null_channel(ipc::channel &inbound)
				: _inbound(inbound)
			{	}

			virtual void disconnect() throw()
			{	}

			virtual void message(const_byte_range /*payload*/)
			{	_inbound.disconnect();	}

			ipc::channel &_inbound;
		};



		ipc::channel_ptr_t probe_create_channel(ipc::channel &inbound)
		{
			const string c_candidate_endpoints[] = {
				ipc::sockets_endpoint_id(ipc::localhost, 6100),
				ipc::com_endpoint_id(constants::integrated_frontend_id),
				ipc::com_endpoint_id(constants::standalone_frontend_id),
			};

			vector<string> candidate_endpoints(c_candidate_endpoints, c_candidate_endpoints
				+ sizeof(c_candidate_endpoints) / sizeof(c_candidate_endpoints[0]));

			if (const char *env_id = getenv(constants::frontend_id_ev))
				candidate_endpoints.insert(candidate_endpoints.begin(), env_id);

			for (vector<string>::const_iterator i = candidate_endpoints.begin(); i != candidate_endpoints.end(); ++i)
			{
				try
				{
					LOG(PREAMBLE "connecting...") % A(*i);
					const auto channel = ipc::connect_client(i->c_str(), inbound);
					LOG(PREAMBLE "connected...") % A(channel.get());
					return channel;
				}
				catch (const exception &e)
				{
					LOG(PREAMBLE "failed.") % A(e.what());
				}
			}
			LOG(PREAMBLE "using null channel.");
			return ipc::channel_ptr_t(new null_channel(inbound));
		}

		log::writer_t create_writer()
		{
			const auto logname = (string)"micro-profiler." + *get_current_executable() + ".log";

			mkdir(constants::data_directory().c_str(), 0777);
			return log::create_writer(constants::data_directory() & logname);
		}


		collector_app_instance g_instance(&probe_create_channel, mt::get_thread_callbacks(), c_trace_limit,
			g_collector_ptr);
	}

	collector_app_instance::collector_app_instance(const active_server_app::frontend_factory_t &frontend_factory,
			mt::thread_callbacks &thread_callbacks, size_t trace_limit, calls_collector *&collector_ptr)
		: _logger(create_writer(), (log::g_logger = &_logger, &get_datetime)),
			_thread_monitor(make_shared<thread_monitor>(thread_callbacks)),
			_collector(_allocator, trace_limit, *_thread_monitor, thread_callbacks),
			_patch_manager(_collector, _eallocator)
	{
		collector_ptr = &_collector;

		const auto oh = calibrate_overhead(_collector, trace_limit);
		const auto period = 1e9 / ticks_per_second();
		const auto inner_ns = static_cast<int>(oh.inner * period);
		const auto total_ns = static_cast<int>((oh.inner + oh.outer) * period);

		LOG(PREAMBLE "overhead calibrated...") % A(inner_ns) % A(total_ns);
		_app.reset(new collector_app(frontend_factory, _collector, oh, *_thread_monitor, _patch_manager));
		platform_specific_init();
		LOG(PREAMBLE "initialized...")
			% A(get_current_executable()) % A(getpid()) % A(get_module_info(&g_collector_ptr).path);
	}

	collector_app_instance::~collector_app_instance()
	{
		terminate();
		log::g_logger = nullptr;
	}

	void collector_app_instance::terminate() throw()
	{	_app.reset();	}
}

#if defined(__clang__) || defined(__GNUC__)
	#define PUBLIC __attribute__ ((visibility ("default")))
#else
	#define PUBLIC
#endif

extern "C" PUBLIC void __cyg_profile_func_enter(void *callee, void * /*call_site*/)
{
	if (g_collector_ptr)
		g_collector_ptr->track(micro_profiler::read_tick_counter(), callee);
}

extern "C" PUBLIC void __cyg_profile_func_exit(void * /*callee*/, void * /*call_site*/)
{
	if (g_collector_ptr)
		g_collector_ptr->track(micro_profiler::read_tick_counter(), 0);
}
