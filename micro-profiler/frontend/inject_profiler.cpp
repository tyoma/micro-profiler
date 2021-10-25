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

#include <micro-profiler/frontend/inject_profiler.h>

#include <common/constants.h>
#include <common/module.h>
#include <common/path.h>
#include <common/pod_vector.h>
#include <common/range.h>
#include <common/stream.h>
#include <injector/process.h>
#include <strmd/deserializer.h>
#include <strmd/packer.h>
#include <strmd/serializer.h>

using namespace std;

extern "C" int setenv(const char *name, const char *value, int overwrite);

namespace micro_profiler
{
	namespace
	{
#ifdef _M_IX86
		const string c_profiler_module_name = "micro-profiler_Win32.dll";
#elif _M_X64
		const string c_profiler_module_name = "micro-profiler_x64.dll";
#endif

		struct parameters
		{
			string frontend_id;
		};

		template <typename ArchiveT>
		void serialize(ArchiveT &archive, parameters &data)
		{
			archive(data.frontend_id);
		}

		void inject_profiler_worker(const_byte_range payload)
		{
			buffer_reader r(payload);
			strmd::deserializer<buffer_reader, strmd::varint> d(r);
			parameters p;
			const auto dir = ~get_module_info(&inject_profiler).path;

			d(p);
			setenv(constants::frontend_id_ev, p.frontend_id.c_str(), 1);
			setenv(constants::profiler_injected_ev, "1", 1);
			load_library(dir & c_profiler_module_name); // No need to hold the handle, as profiler pins to the process.
		}
	}

	void inject_profiler(process &process_, const std::string &frontend_id)
	{
		pod_vector<byte> buffer;
		buffer_writer< pod_vector<byte> > w(buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, strmd::varint> s(w);
		parameters p = {
			frontend_id,
		};

		s(p);
		process_.remote_execute(&inject_profiler_worker, const_byte_range(buffer.data(), buffer.size()));
	}
}
