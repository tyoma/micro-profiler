//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <injector/process.h>

#include <common/file_id.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>
#include <common/win32/module.h>
#include <stdexcept>
#include <windows.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	class process::impl
	{
	public:
		impl(unsigned int pid)
			: _pid(pid), _hprocess(::OpenProcess(rights, FALSE, pid), &::CloseHandle)
		{
			if (!_hprocess)
				throw runtime_error("");
		}

		void remote_execute(injection_function_t *injection, const_byte_range payload)
		{
			const auto m = module::platform().locate(&foreign_worker);
			const auto wpath = unicode(m.path);
			const auto fpath = foreign_allocate(sizeof(wchar_t) * (wpath.size() + 1));
			const auto injection_offset = reinterpret_cast<byte *>(injection) - m.base;
			const auto payload_size = payload.length();
			const auto fpayload = foreign_allocate(sizeof(injection_offset) + sizeof(payload_size) + payload.length());
			auto write_position = static_cast<byte *>(fpayload.get());
			const auto kernel = module::platform().load("kernel32");

			::WriteProcessMemory(_hprocess.get(), fpath.get(), wpath.c_str(), sizeof(wchar_t) * (wpath.size() + 1), NULL);
			foreign_execute_sync(kernel / "LoadLibraryW", fpath.get());

			const auto fbase = find_loaded_module(m.path);

			if (!fbase)
				throw runtime_error("Loading library into a remote process failed!");

			::WriteProcessMemory(_hprocess.get(), write_position, &injection_offset, sizeof(injection_offset), NULL);
			write_position += sizeof(injection_offset);
			::WriteProcessMemory(_hprocess.get(), write_position, &payload_size, sizeof(payload_size), NULL);
			write_position += sizeof(payload_size);
			::WriteProcessMemory(_hprocess.get(), write_position, payload.begin(), payload.length(), NULL);
			foreign_execute_sync((PTHREAD_START_ROUTINE)(fbase + ((byte *)&foreign_worker - m.base)), fpayload.get());

			// TODO: untested
			foreign_execute_sync(kernel / "FreeLibrary", fbase);
		}

	private:
		enum {
			rights = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE
				| PROCESS_VM_READ,
		};

	private:
		void foreign_execute_sync(PTHREAD_START_ROUTINE thread_routine, void *data)
		{
			shared_ptr<void> hthread(::CreateRemoteThread(_hprocess.get(), NULL, 0, thread_routine, data, 0, NULL),
				&::CloseHandle);

			if (!hthread)
				throw runtime_error("Remote thread injection failed!"); // TODO: untested
			::WaitForSingleObject(hthread.get(), INFINITE);
		}

		shared_ptr<byte> foreign_allocate(size_t size)
		{
			return shared_ptr<byte>(static_cast<byte *>(::VirtualAllocEx(_hprocess.get(), 0, size, MEM_COMMIT,
				PAGE_EXECUTE_READWRITE)), bind(&::VirtualFreeEx, _hprocess.get(), _1, 0, MEM_RELEASE));
		}

		static void __stdcall foreign_worker(void *data_)
		{
			const auto m = module::platform().locate(&foreign_worker);
			const auto injection_offset = static_cast<const size_t *>(data_);
			const auto size = injection_offset + 1;
			const auto data = reinterpret_cast<const byte *>(size + 1);
			const auto injection = reinterpret_cast<injection_function_t *>(m.base + *injection_offset);

			(*injection)(const_byte_range(data, *size));
		};

		byte *find_loaded_module(const string &path)
		{
			file_id fid(path);
			byte *base = nullptr;

			modules_enumerate_mapped(_hprocess.get(), [&] (const module::mapping &mapping) {
				base = file_id(mapping.path) == fid ? mapping.base : base;
			});
			return base;
		}

	private:
		unsigned _pid;
		shared_ptr<void> _hprocess;
	};


	process::process(unsigned int pid)
		: _impl(new impl(pid))
	{	}

	process::~process()
	{	delete _impl;	}

	void process::remote_execute(injection_function_t *injection_function, const_byte_range payload)
	{	_impl->remote_execute(injection_function, payload);	}
}
