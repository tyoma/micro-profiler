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

#include <common/module.h>

#include <common/string.h>
#include <memory>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		void get_module_path(string &path, HMODULE hmodule)
		{
			wchar_t buffer[MAX_PATH + 1];

			buffer[MAX_PATH] = L'\0';
			::GetModuleFileNameW(hmodule, buffer, sizeof(buffer));
			path = unicode(buffer);
		}
	}

	string get_current_executable()
	{	return get_module_info(0).path;	}

	mapped_module get_module_info(const void *address)
	{
		HMODULE base = 0;
		::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCWSTR>(address), &base);
		shared_ptr<void> h(base, &::FreeLibrary);
		mapped_module info = { string(), static_cast<byte *>(static_cast<void *>(base)), };

		get_module_path(info.path, base);
		return info;
	}

	void enumerate_process_modules(const module_callback_t &callback)
	{
		mapped_module module;
		shared_ptr<void> snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0), &::CloseHandle);
		MODULEENTRY32W entry = { sizeof(MODULEENTRY32W), };

		for (auto lister = &::Module32FirstW;
			lister(snapshot.get(), &entry);
			lister = &::Module32NextW, module.addresses.clear())
		{
			get_module_path(module.path, entry.hModule);
			module.base = entry.modBaseAddr;
			module.addresses.push_back(byte_range(entry.modBaseAddr, entry.modBaseSize));
			callback(module);
		}
	}
}
