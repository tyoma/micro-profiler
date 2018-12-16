#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		wstring get_current_process_executable()
		{
			wchar_t fullpath[MAX_PATH + 1] = { };

			::GetModuleFileNameW(NULL, fullpath, MAX_PATH);
			return fullpath;
		}


		image::image(const wchar_t *path)
			: shared_ptr<void>(::LoadLibraryW(path), &::FreeLibrary)
		{
			if (!get())
				throw runtime_error("Cannot load module specified!");

			wchar_t fullpath[MAX_PATH + 1] = { };

			::GetModuleFileNameW(static_cast<HMODULE>(get()), fullpath, MAX_PATH);
			_fullpath = fullpath;
		}

		long_address_t image::load_address() const
		{	return reinterpret_cast<size_t>(get());	}

		const wchar_t *image::absolute_path() const
		{	return _fullpath.c_str();	}

		void *image::get_symbol_address(const char *name) const
		{
			if (void *symbol = ::GetProcAddress(static_cast<HMODULE>(get()), name))
				return symbol;
			throw runtime_error("Symbol specified was not found!");
		}
	}
}

extern "C" int setenv(const char *name, const char *value, int overwrite)
{
	if (overwrite || !GetEnvironmentVariableA(name, NULL, 0))
		::SetEnvironmentVariableA(name, value);
	return 0;
}
