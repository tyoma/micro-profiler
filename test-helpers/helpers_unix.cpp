#include <test-helpers/helpers.h>

#include <common/path.h>
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		wstring get_current_process_executable()
		{
			char path[1000] = {};

			::readlink("/proc/self/exe", path, sizeof(path) - 1);
			string apath(path);
			return wstring(apath.begin(), apath.end());
		}


		image::image(const wchar_t *path_)
		{
			wstring path(path_);

			if (path.find(L".so") == wstring::npos)
				path = path + L".so";
			if (path.find(L'/') == wstring::npos)
				path = L"./" & (L"lib" + path);

			string apath(path.begin(), path.end());
			
			reset(::dlopen(apath.c_str(), RTLD_NOW), &::dlclose);
			if (!get())
				throw runtime_error("Cannot load module specified!");

			link_map *lm = 0;
			
			::dlinfo(get(), RTLD_DI_LINKMAP, &lm);
			apath = lm->l_name;
			_fullpath.assign(apath.begin(), apath.end());
		}

		long_address_t image::load_address() const
		{
			link_map *lm = 0;
			
			::dlinfo(get(), RTLD_DI_LINKMAP, &lm);
			return reinterpret_cast<size_t>(lm->l_addr);
		}

		const wchar_t *image::absolute_path() const
		{	return _fullpath.c_str();	}

		void *image::get_symbol_address(const char *name) const
		{
			if (void *symbol = ::dlsym(get(), name))
				return symbol;
			throw runtime_error("Symbol specified was not found!");
		}
	}
}
