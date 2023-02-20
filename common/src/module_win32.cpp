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

		template <typename T>
		void enumerate_process_modules(HANDLE hprocess, const T &callback)
		{
			shared_ptr<void> snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ::GetProcessId(hprocess)), &::CloseHandle);
			MODULEENTRY32 entry = {	sizeof(MODULEENTRY32),	};

			for (auto lister = &::Module32First; lister(snapshot.get(), &entry); lister = &::Module32Next)
				callback(entry);
		}

		class module_lock : public module::mapping, noncopyable
		{
		public:
			explicit module_lock(const void *address)
				: _handle(0)
			{
				if (::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCTSTR>(address), &_handle)
					&& _handle)
				{
					base = static_cast<byte *>(static_cast<void *>(_handle));
					get_module_path(path, _handle);
					enumerate_regions(regions, address);
				}
			}

			~module_lock()
			{	::FreeLibrary(_handle);	}

			operator HMODULE() const
			{	return _handle;	}

		private:
			HMODULE _handle;
		};
	}

	class module_platform : public module
	{
	private:
		class tracker;

	private:
		virtual shared_ptr<dynamic> load(const string &path) override;
		virtual string executable() override;
		virtual mapping locate(const void *address) override;
		virtual shared_ptr<mapping> lock_at(void *address) override;
		virtual shared_ptr<void> notify(events &consumer) override;
	};

	class module_platform::tracker
	{
	public:
		tracker(module &owner, events &consumer);
		~tracker();

	private:
		static void CALLBACK on_module(ULONG reason, const LDR_DLL_NOTIFICATION_DATA *data, void *context);

	private:
		shared_ptr<module::dynamic> _ntdll;
		NTSTATUS (NTAPI *_unregister)(void *cookie);
		events &_consumer;
		void *_cookie;
	};



	void *module::dynamic::find_function(const char *name) const
	{	return GetProcAddress(static_cast<HMODULE>(_handle.get()), name);	}


	shared_ptr<module::dynamic> module_platform::load(const string &path)
	{
		shared_ptr<void> handle(LoadLibraryW(unicode(path).c_str()), &::FreeLibrary);
		return handle ? shared_ptr<module::dynamic>(new module::dynamic(handle)) : nullptr;
	}

	string module_platform::executable()
	{
		string path;

		get_module_path(path, 0);
		return path;
	}

	module::mapping module_platform::locate(const void *address)
	{	return module_lock(address);	}

	shared_ptr<module::mapping> module_platform::lock_at(void *address)
	{
		unique_ptr<module_lock> m(new module_lock(address));
		return *m ? shared_ptr<module_lock>(m.release()) : nullptr;
	}

	shared_ptr<void> module_platform::notify(events &consumer)
	{	return make_shared<tracker>(*this, consumer);	}


	module_platform::tracker::tracker(module &owner, events &consumer)
		: _ntdll(owner.load("ntdll")), _unregister(_ntdll / "LdrUnregisterDllNotification"), _consumer(consumer)
	{
		NTSTATUS (NTAPI *register_)(ULONG flags, decltype(&tracker::on_module) callback, void *context,
			void **cookie) = _ntdll / "LdrRegisterDllNotification";

		register_(0, on_module, this, &_cookie);
		enumerate_process_modules(::GetCurrentProcess(), [this] (const MODULEENTRY32 &entry) {
			module_lock m(entry.modBaseAddr);

			if (!!m)
				_consumer.mapped(m);
		});
	}

	module_platform::tracker::~tracker()
	{	_unregister(_cookie);	}

	void CALLBACK module_platform::tracker::on_module(ULONG reason, const LDR_DLL_NOTIFICATION_DATA *data, void *context)
	{
		auto self = static_cast<tracker *>(context);

		switch (reason)
		{
		case 2: // LDR_DLL_NOTIFICATION_REASON_UNLOADED
			self->_consumer.unmapped(data->Unloaded.DllBase);
			break;

		case 1: // LDR_DLL_NOTIFICATION_REASON_LOADED
			module::mapping m = {
				unicode(data->Loaded.FullDllName->Buffer), static_cast<byte *>(data->Loaded.DllBase),
			};

			enumerate_regions(m.regions, m.base);
			self->_consumer.mapped(m);
			break;
		}
	}


	void modules_enumerate_mapped(HANDLE hprocess, const function<void (const module::mapping &mapping)> &callback)
	{
		module::mapping m;

		enumerate_process_modules(hprocess, [&callback, &m] (const MODULEENTRY32 &entry) {
			m.base = entry.modBaseAddr;
			unicode(m.path, entry.szModule);
			callback(m);
		});
	}


	module &module::platform()
	{
		static module_platform m;
		return m;
	}
}
