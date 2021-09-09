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

#include <common/module.h>

#include <common/noncopyable.h>
#include <common/string.h>
#include <memory>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class module_lock : noncopyable
		{
		public:
			explicit module_lock(const void *address)
				: _handle(0)
			{	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCTSTR>(address), &_handle);	}

			~module_lock()
			{	::FreeLibrary(_handle);	}

			operator HMODULE() const
			{	return _handle;	}

		private:
			HMODULE _handle;
		};

		void get_module_path(string &path, HMODULE hmodule)
		{
			enum {	length = 32768,	};
			TCHAR buffer[length + 1];

			buffer[length] = 0;
			::GetModuleFileName(hmodule, buffer, length);
			unicode(path, buffer);
		}
	}

	shared_ptr<void> load_library(const string &path)
	{	return shared_ptr<void>(LoadLibraryW(unicode(path).c_str()), &::FreeLibrary);	}

	string get_current_executable()
	{
		string path;

		get_module_path(path, 0);
		return path;
	}

	mapped_module get_module_info(const void *address)
	{
		mapped_module info = {};
		module_lock h(address);

		if (h)
		{
			info.base = static_cast<byte *>(static_cast<void *>(h));
			get_module_path(info.path, h);
		}
		return info;
	}

	void enumerate_process_modules(const module_callback_t &callback)
	{
		mapped_module module;
		shared_ptr<void> snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0), &::CloseHandle);
		MODULEENTRY32 entry = { sizeof(MODULEENTRY32), };

		for (auto lister = &::Module32First;
			lister(snapshot.get(), &entry);
			lister = &::Module32Next, module.addresses.clear())
		{
			module_lock h(entry.modBaseAddr);

			if (entry.hModule != h)
				continue;
			unicode(module.path, entry.szExePath);
			module.base = entry.modBaseAddr;
			module.addresses.push_back(byte_range(entry.modBaseAddr, entry.modBaseSize));
			callback(module);
		}
	}
}
