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

#include <common/file_id.h>
#include <common/noncopyable.h>
#include <common/string.h>
#include <memory>
#include <mt/mutex.h>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct UNICODE_STRING {
			USHORT Length;
			USHORT MaximumLength;
			PWCH Buffer;
		};

		struct LDR_DLL_LOADED_NOTIFICATION_DATA {
			ULONG Flags;
			const UNICODE_STRING *FullDllName;
			const UNICODE_STRING *BaseDllName;
			void *DllBase;
			ULONG SizeOfImage;
		};

		struct LDR_DLL_UNLOADED_NOTIFICATION_DATA {
			ULONG Flags;
			const UNICODE_STRING *FullDllName;
			const UNICODE_STRING *BaseDllName;
			void *DllBase;
			ULONG SizeOfImage;
		};

		union LDR_DLL_NOTIFICATION_DATA {
			LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
			LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
		};


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

	class module::tracker
	{
	public:
		tracker(module::mapping_callback_t mapped, module::unmapping_callback_t unmapped);
		~tracker();

	private:
		static void CALLBACK on_module(ULONG reason, const LDR_DLL_NOTIFICATION_DATA *data, void *context);

	private:
		shared_ptr<module::dynamic> _ntdll;
		NTSTATUS (NTAPI *_unregister)(void *cookie);
		module::mapping_callback_t _mapped;
		module::unmapping_callback_t _unmapped;
		void *_cookie;
	};



	void *module::dynamic::find_function(const char *name) const
	{	return GetProcAddress(static_cast<HMODULE>(_handle.get()), name);	}


	module::lock::lock(const void *base, const string &path)
	{
		HMODULE hmodule = nullptr;

		_handle = nullptr;
		if (::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCTSTR>(base), &hmodule) && hmodule)
		{
			string loaded_path;

			get_module_path(loaded_path, hmodule);
			if (file_id(path) == file_id(loaded_path))
			{
				_handle = hmodule;
				return;
			}
			::FreeLibrary(hmodule);
		}
	}

	module::lock::~lock()
	{
		if (_handle)
			::FreeLibrary(static_cast<HMODULE>(_handle));
	}


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

	shared_ptr<void> module::notify(mapping_callback_t mapped, module::unmapping_callback_t unmapped)
	{	return make_shared<tracker>(mapped, unmapped);	}


	module::tracker::tracker(module::mapping_callback_t mapped, module::unmapping_callback_t unmapped)
		: _ntdll(module::load("ntdll")), _unregister(_ntdll / "LdrUnregisterDllNotification"),
			_mapped(mapped), _unmapped(unmapped)
	{
		NTSTATUS (NTAPI *register_)(ULONG flags, decltype(&tracker::on_module) callback, void *context,
			void **cookie) = _ntdll / "LdrRegisterDllNotification";

		register_(0, on_module, this, &_cookie);
		modules_enumerate_mapped(::GetCurrentProcess(), mapped);
	}

	module::tracker::~tracker()
	{	_unregister(_cookie);	}

	void CALLBACK module::tracker::on_module(ULONG reason, const LDR_DLL_NOTIFICATION_DATA *data, void *context)
	{
		auto self = static_cast<tracker *>(context);

		switch (reason)
		{
		case 2: // LDR_DLL_NOTIFICATION_REASON_UNLOADED
			self->_unmapped(data->Unloaded.DllBase);
			break;

		case 1: // LDR_DLL_NOTIFICATION_REASON_LOADED
			module::mapping m = {
				unicode(data->Loaded.FullDllName->Buffer), static_cast<byte *>(data->Loaded.DllBase),
			};

			enumerate_regions(m.regions, m.base);
			self->_mapped(m);
			break;
		}
	}


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
