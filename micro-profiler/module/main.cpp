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

#include <crtdbg.h>

#include <atlbase.h>
#include <common/constants.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>
#include <direct.h>
#include <ipc/com/endpoint.h>
#include <logger/log.h>
#include <logger/multithreaded_logger.h>
#include <logger/writer.h>

using namespace std;

namespace micro_profiler
{
	class Module : public CAtlDllModuleT<Module> { } g_module;

	const string c_logname = "micro-profiler_vspackage.log";
	HINSTANCE g_instance;

	shared_ptr<ipc::server> ipc::com::server::create_default_session_factory()
	{	return shared_ptr<ipc::server>();	}


	struct file_version
	{
		unsigned short major, minor, build, patch;
	};

	file_version get_file_version(const string &path_)
	{
		const wstring path = unicode(path_);
		file_version v = {};

		if (const DWORD version_size = ::GetFileVersionInfoSizeW(path.c_str(), 0))
		{
			unique_ptr<byte[]> buffer(new byte[version_size]);

			if (::GetFileVersionInfoW(path.c_str(), 0, version_size, buffer.get()))
			{
				void *fragment = 0;
				UINT fragment_size;

				if (::VerQueryValueW(buffer.get(), L"\\", &fragment, &fragment_size) && fragment_size)
				{
					const VS_FIXEDFILEINFO *version = static_cast<const VS_FIXEDFILEINFO *>(fragment);

					if (version->dwSignature == 0xfeef04bd)
					{
						v.major = static_cast<unsigned short>(version->dwFileVersionMS >> 16);
						v.minor = static_cast<unsigned short>(version->dwFileVersionMS >> 0);
						v.build = static_cast<unsigned short>(version->dwFileVersionLS >> 16);
						v.patch = static_cast<unsigned short>(version->dwFileVersionLS >> 0);
					}
				}
			}
		}
		return v;
	}
}

using namespace micro_profiler;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID reserved)
{
	if (DLL_PROCESS_ATTACH == reason)
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		g_instance = hinstance;
		mkdir(constants::data_directory().c_str());
		log::g_logger.reset(new log::multithreaded_logger(log::create_writer(constants::data_directory() & c_logname),
			&get_datetime));

		const string self = get_module_info(&c_logname).path;
		const string exe = get_module_info(0).path;
		const file_version vs = get_file_version(exe);

		LOG("MicroProfiler vspackage module loaded...")
			% A(getpid()) % A(self) % A(exe)
			% A(vs.major) % A(vs.minor) % A(vs.build) % A(vs.patch);
	}

	return g_module.DllMain(reason, reserved);
}

STDAPI DllCanUnloadNow()
{	return g_module.DllCanUnloadNow();	}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{	return g_module.DllGetClassObject(rclsid, riid, ppv);	}
