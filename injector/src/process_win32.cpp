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

#include <injector/process.h>

#include <common/file_id.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>
#include <stdexcept>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace win32
	{
		int g_dummy;

		class process : public micro_profiler::process
		{
		public:
			process(unsigned int pid, const string &name_)
				: _pid(pid), _name(name_), _hprocess(::OpenProcess(rights, FALSE, pid), &::CloseHandle)
			{
				if (!_hprocess)
					throw runtime_error("");
			}

			virtual unsigned get_pid() const
			{	return _pid;	}

			virtual string name() const
			{	return _name;	}

			virtual void remote_execute(injection_function_t *injection, const_byte_range payload)
			{
				const mapped_module m = get_module_info(&g_dummy);
				const shared_ptr<void> fpath = foreign_allocate(m.path.size() + 1);
				const size_t injection_offset = (byte *)injection - m.base;
				const size_t payload_size = payload.length();
				const shared_ptr<byte> fpayload = foreign_allocate(sizeof(injection_offset) + sizeof(payload_size)
					+ payload.length());
				const auto hkernel = load_library("kernel32");

				::WriteProcessMemory(_hprocess.get(), fpath.get(), m.path.c_str(), m.path.size() + 1, NULL);
				foreign_execute((PTHREAD_START_ROUTINE)::GetProcAddress(static_cast<HMODULE>(hkernel.get()), "LoadLibraryA"),
					fpath.get());

				auto fbase = static_cast<byte *>(find_loaded_module(m.path));

				::WriteProcessMemory(_hprocess.get(), fpayload.get(), &injection_offset, sizeof(injection_offset),
					NULL);
				::WriteProcessMemory(_hprocess.get(), fpayload.get() + sizeof(injection_offset),
					&payload_size, sizeof(payload_size), NULL);
				::WriteProcessMemory(_hprocess.get(), fpayload.get() + sizeof(injection_offset) + sizeof(payload_size),
					payload.begin(), payload.length(), NULL);
				foreign_execute((PTHREAD_START_ROUTINE)(fbase + ((byte *)&foreign_worker - m.base)), fpayload.get());
			}

		private:
			enum {
				rights = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE
					| PROCESS_VM_READ,
			};

		private:
			void *foreign_execute(PTHREAD_START_ROUTINE thread_routine, void *data)
			{
				shared_ptr<void> hthread(::CreateRemoteThread(_hprocess.get(), NULL, 0, thread_routine, data, 0, NULL),
					&::CloseHandle);
				DWORD exit_code;

				::WaitForSingleObject(hthread.get(), INFINITE);
				return ::GetExitCodeThread(hthread.get(), &exit_code),
					reinterpret_cast<void *>(static_cast<size_t>(exit_code));
			}

			shared_ptr<byte> foreign_allocate(size_t size)
			{
				return shared_ptr<byte>(static_cast<byte *>(::VirtualAllocEx(_hprocess.get(), 0, size, MEM_COMMIT,
					PAGE_EXECUTE_READWRITE)), bind(&::VirtualFreeEx, _hprocess.get(), _1, 0, MEM_RELEASE));
			}

			static void __stdcall foreign_worker(void *data_)
			{
				const mapped_module m = get_module_info(&g_dummy);
				size_t *injection_offset = (size_t *)data_;
				size_t *size = (size_t *)(injection_offset + 1);
				const byte *data = (const byte *)(size + 1);
				injection_function_t *injection = (injection_function_t *)(m.base + *injection_offset);

				(*injection)(const_byte_range(data, *size));
			};

			void *find_loaded_module(const string &path)
			{
				file_id fid(path);
				DWORD needed;
				vector<HMODULE> buffer;

				if (!::EnumProcessModules(_hprocess.get(), 0, 0, &needed))
					throw runtime_error("Cannot enumerate modules in the target process!");
				buffer.resize(needed / sizeof(HMODULE));
				::EnumProcessModules(_hprocess.get(), buffer.data(), needed, &needed);
				for (auto i = buffer.begin(); i != buffer.end(); ++i)
				{
					wchar_t path_buffer[MAX_PATH + 1] = { 0 };

					::GetModuleFileNameExW(_hprocess.get(), *i, path_buffer, sizeof(path_buffer) / sizeof(wchar_t));
					if (file_id(unicode(path_buffer)) == fid)
						return *i;
				}
				return 0;
			}

		private:
			unsigned _pid;
			string _name;
			shared_ptr<void> _hprocess;
		};
	}

	shared_ptr<process> process::open(unsigned int pid)
	{	return shared_ptr<process>(new win32::process(pid, string()));	}

	void process::enumerate(const enumerate_callback_t &callback)
	{
		shared_ptr<void> snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0), &::CloseHandle);
		PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32), };

		for (auto lister = &::Process32First; lister(snapshot.get(), &entry); lister = &::Process32Next)
			try
			{
				callback(shared_ptr<process>(new win32::process(entry.th32ProcessID, unicode(entry.szExeFile))));
			}
			catch (...)
			{
			}
	}
}
