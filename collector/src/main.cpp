//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <patcher/image_patch_manager.h>

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

const size_t c_trace_limit = 5000000;
const mt::milliseconds c_auto_connect_delay(50);
#ifdef _MSC_VER
	extern "C"
#endif
	micro_profiler::calls_collector *g_collector_ptr = nullptr;
micro_profiler::collector_app_instance g_instance(&micro_profiler::collector_app_instance::probe_create_channel,
	mt::get_thread_callbacks(), micro_profiler::module::platform(), c_trace_limit, g_collector_ptr);

namespace micro_profiler
{
	class collector_app_instance::default_memory_manager : public memory_manager
	{
		virtual shared_ptr<executable_memory_allocator> create_executable_allocator(const_byte_range /*reference_region*/,
			ptrdiff_t /*max_distance*/) override
		{	return make_shared<executable_memory_allocator>();	}

		virtual shared_ptr<void> scoped_protect(byte_range region, int /*scoped_protection*/, int /*released_protection*/)
		{
			try
			{
				return make_shared<scoped_unprotect>(region); // For a while forgive protection change exceptions.
			} 
			catch (exception &e)
			{
				LOGE("Failed to scope-protect memory region!") % A(e.what()) % A(region.begin()) % A(region.length());
				return nullptr;
			}
		}
	};


	namespace
	{
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
	}


	ipc::channel_ptr_t collector_app_instance::probe_create_channel(ipc::channel &inbound)
	{
		const string c_candidate_endpoints[] = {
			ipc::sockets_endpoint_id(ipc::localhost, 6100),
			ipc::com_endpoint_id(constants::integrated_frontend_id),
			ipc::com_endpoint_id(constants::standalone_frontend_id),
		};

		vector<string> candidate_endpoints(c_candidate_endpoints, c_candidate_endpoints
			+ sizeof(c_candidate_endpoints) / sizeof(c_candidate_endpoints[0]));

		if (auto env_id = getenv(constants::frontend_id_ev))
			candidate_endpoints.insert(candidate_endpoints.begin(), env_id);

		for (auto i = candidate_endpoints.begin(); i != candidate_endpoints.end(); ++i)
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
		return make_shared<null_channel>(inbound);
	}

	log::writer_t collector_app_instance::create_writer(module &module_helper)
	{
		const auto logname = (string)"micro-profiler." + *module_helper.executable() + ".log";

		mkdir(constants::data_directory().c_str(), 0777);
		return log::create_writer(constants::data_directory() & logname);
	}

	collector_app_instance::collector_app_instance(const active_server_app::client_factory_t &auto_frontend_factory,
			mt::thread_callbacks &thread_callbacks, module &module_helper, size_t trace_limit,
			calls_collector *&collector_ptr)
		: _logger(create_writer(module_helper), (log::g_logger = &_logger, &get_datetime)),
			_thread_monitor(make_shared<thread_monitor>(thread_callbacks)), _memory_manager(new default_memory_manager),
			_collector(_allocator, trace_limit, *_thread_monitor, thread_callbacks), _auto_connect(true)
	{
		collector_ptr = &_collector;

		const auto oh = calibrate_overhead(_collector, trace_limit);
		const auto period = 1e9 / ticks_per_second();
		const auto inner_ns = static_cast<int>(oh.inner * period);
		const auto total_ns = static_cast<int>((oh.inner + oh.outer) * period);

		LOG(PREAMBLE "overhead calibrated...") % A(inner_ns) % A(total_ns);
		_app.reset(new collector_app(_collector, oh, *_thread_monitor, module_helper, [this] (mapping_access &ma) -> shared_ptr<patch_manager> {
			auto &c = _collector;

			return make_shared<image_patch_manager>([&c] (void *target, id_t /*id*/, executable_memory_allocator &allocator) {
				return unique_ptr<patch>(new function_patch(target, &c, allocator));
			}, ma, *_memory_manager);
		}));
		_app->get_queue().schedule([this, auto_frontend_factory] {
			if (_auto_connect)
				_app->connect(auto_frontend_factory, false);
		}, c_auto_connect_delay);
		platform_specific_init();
		LOG(PREAMBLE "initialized...")
			% A(module_helper.executable()) % A(getpid()) % A(module_helper.locate(&g_collector_ptr).path);
	}

	collector_app_instance::~collector_app_instance()
	{
		terminate();
		log::g_logger = nullptr;
	}

	void collector_app_instance::terminate() throw()
	{	_app.reset();	}

	void collector_app_instance::block_auto_connect()
	{	_app->get_queue().schedule([this] {	_auto_connect = false;	});	}

	void collector_app_instance::connect(const active_server_app::client_factory_t &frontend_factory)
	{
		_app->get_queue().schedule([this, frontend_factory] {
			_auto_connect = false;
			_app->connect(frontend_factory, true);
		});
	}
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
