#include <test-helpers/helpers.h>

#include <common/smart_ptr.h>
#include <ut/assert.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		image::image(string path)
			: shared_ptr<void>(::LoadLibraryA(path.c_str()), &::FreeLibrary)
		{
			if (!get())
				throw runtime_error("Cannot load module specified: " + path + "!");

			char fullpath[MAX_PATH + 1] = { };

			::GetModuleFileNameA(static_cast<HMODULE>(get()), fullpath, MAX_PATH);
			_fullpath = fullpath;
		}

		byte *image::base_ptr() const
		{	return static_cast<byte *>(get());	}

		long_address_t image::base() const
		{	return reinterpret_cast<size_t>(get());	}

		const char *image::absolute_path() const
		{	return _fullpath.c_str();	}

		void *image::get_symbol_address(const char *name) const
		{
			if (void *symbol = ::GetProcAddress(static_cast<HMODULE>(get()), name))
				return symbol;
			throw runtime_error("Symbol specified was not found!");
		}

		unsigned image::get_symbol_rva(const char *name) const
		{	return static_cast<unsigned>(static_cast<byte *>(get_symbol_address(name)) - base_ptr());	}

		shared_ptr<void> occupy_memory(void *start, unsigned int length)
		{
			return shared_ptr<void>(::VirtualAlloc(start, length, MEM_RESERVE | MEM_COMMIT, PAGE_READONLY | PAGE_GUARD),
				[length] (void *p) {
				::VirtualFree(p, 0, MEM_RELEASE);
			});
		}

		shared_ptr<void> allocate_edge()
		{
			enum {	page_size = 4096	};

			shared_ptr<void> m(::VirtualAlloc(0, 2 * page_size, MEM_RESERVE, PAGE_EXECUTE_READ), 
				[] (void *p) {
				::VirtualFree(p, 0, MEM_RELEASE);
			});
			auto address = static_cast<byte *>(m.get()) + page_size;

			::VirtualAlloc(address, page_size, MEM_COMMIT, PAGE_EXECUTE);
			return make_shared_aspect(m, static_cast<byte *>(m.get()) + page_size);
		}
	}
}

extern "C" int setenv(const char *name, const char *value, int overwrite)
{
	if (overwrite || !GetEnvironmentVariableA(name, NULL, 0))
		_putenv((string(name) + "=" + value).c_str());
	return 0;
}

extern "C" int unsetenv(const char *name)
{	return _putenv((string(name) + "=").c_str()), 0;	}
