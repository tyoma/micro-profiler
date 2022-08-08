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

#include <injector/injector.h>

#include <common/constants.h>
#include <common/module.h>
#include <common/stream.h>
#include <injector/process.h>
#include <ipc/endpoint_spawn.h>
#include <ipc/server_session.h>
#include <sandbox/sandbox.h>
#include <strmd/deserializer.h>
#include <strmd/packer.h>

using namespace micro_profiler;
using namespace micro_profiler::ipc;
using namespace std;

extern "C" int setenv(const char *name, const char *value, int overwrite);

namespace
{
	void inject_profiler_worker(const_byte_range payload)
	{
		buffer_reader r(payload);
		strmd::deserializer<buffer_reader, strmd::varint> d(r);
		injection_info injection;

		d(injection);
		setenv(constants::frontend_id_ev, injection.frontend_endpoint_id.c_str(), 1);
		setenv(constants::profiler_injected_ev, "1", 1);
		load_library(injection.collector_path); // No need to hold the handle, as profiler pins to the process.
	}
}

extern "C" MP_EXPORT_PREFIX void ipc_spawn_server(micro_profiler::ipc::channel_ptr_t &session_,
	const vector<string> &/*arguments*/, micro_profiler::ipc::channel &outbound)
{
	const auto session = make_shared<server_session>(outbound);

	session->add_handler(request_injection, [] (server_session::response &response, const injection_info &injection) {
		auto p = make_shared<process>(injection.pid);
		pod_vector<byte> buffer;
		buffer_writer< pod_vector<byte> > w(buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, strmd::varint> s(w);

		s(injection);
		p->remote_execute(&inject_profiler_worker, const_byte_range(buffer.data(), buffer.size()));
		response(response_injected);
	});
	session_ = session;
}
