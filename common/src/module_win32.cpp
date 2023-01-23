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

#include <common/module.h>
#include <common/win32/module.h>

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
		int generic_protection(DWORD win32_protection)
		{
			switch (win32_protection)
			{
			case PAGE_READONLY: return mapped_region::read;
			case PAGE_READWRITE: return mapped_region::read | mapped_region::write;
			case PAGE_EXECUTE: return mapped_region::execute;
			case PAGE_EXECUTE_READ: return mapped_region::execute | mapped_region::read;
			case PAGE_EXECUTE_READWRITE: return mapped_region::execute | mapped_region::read | mapped_region::write;
			default: return 0;
			}
		}

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

		void enumerate_regions(vector<mapped_region> &regions, const void *inside_address)
		{
			MEMORY_BASIC_INFORMATION mi;

			::VirtualQuery(const_cast<void *>(inside_address), &mi, sizeof mi);
			for (byte *ptr = static_cast<byte *>(mi.AllocationBase), *base = ptr;
				::VirtualQuery(ptr, &mi, sizeof mi) && mi.AllocationBase == base;
				ptr += mi.RegionSize)
			{
				mapped_region region = {
					static_cast<const byte *>(mi.BaseAddress), mi.RegionSize, generic_protection(mi.Protect)
				};

				regions.push_back(region);
			}
		}
	}



	void *module::dynamic::find_function(const char *name) const
	{	return GetProcAddress(static_cast<HMODULE>(_handle.get()), name);	}


	shared_ptr<module::dynamic> module::load(const string &path)
	{
		shared_ptr<void> handle(LoadLibraryW(unicode(path).c_str()), &::FreeLibrary);
		return handle ? shared_ptr<dynamic>(new dynamic(handle)) : nullptr;
	}

	string module::executable()
	{
		string path;

		get_module_path(path, 0);
		return path;
	}

	module::mapping module::locate(const void *address)
	{
		mapping info = {};
		module_lock h(address);

		if (h)
		{
			info.base = static_cast<byte *>(static_cast<void *>(h));
			get_module_path(info.path, h);
		}
		return info;
	}

	void module::enumerate_mapped(const mapping_callback_t &callback)
	{	modules_enumerate_mapped(::GetCurrentProcess(), callback);	}

	void modules_enumerate_mapped(HANDLE hprocess, const module::mapping_callback_t &callback)
	{
		module::mapping module;
		shared_ptr<void> snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ::GetProcessId(hprocess)), &::CloseHandle);
		MODULEENTRY32 entry = {	sizeof(MODULEENTRY32),	};

		for (auto lister = &::Module32First; lister(snapshot.get(), &entry); lister = &::Module32Next)
		{
			module_lock h(entry.modBaseAddr);

			if (entry.hModule != h)
				continue;
			unicode(module.path, entry.szExePath);
			module.base = entry.modBaseAddr;
			enumerate_regions(module.regions, entry.modBaseAddr);
			callback(module);
			module.regions.clear();
		}
	}
}
