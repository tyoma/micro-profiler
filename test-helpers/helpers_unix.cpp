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
		string get_current_process_executable()
		{
			int n;
			char path[1000] = {};

			if (n = ::readlink("/proc/self/exe", path, sizeof(path) - 1), n == -1)
				return string();
			path[n] = 0;
			return path;
		}


		image::image(const char *path_)
		{
			string path(path_);

			if (path.find(".so") == string::npos)
				path = path + ".so";
			if (path.find('/') == string::npos)
				path = "./" & ("lib" + path);
			
			reset(::dlopen(path.c_str(), RTLD_NOW), &::dlclose);
			if (!get())
				throw runtime_error("Cannot load module specified!");

			link_map *lm = 0;
			
			::dlinfo(get(), RTLD_DI_LINKMAP, &lm);
			path = lm->l_name;
			_fullpath.assign(path.begin(), path.end());
		}

		byte *image::load_address_ptr() const
		{
			link_map *lm = 0;
			
			::dlinfo(get(), RTLD_DI_LINKMAP, &lm);
			return reinterpret_cast<byte *>(lm->l_addr);
		}

		long_address_t image::load_address() const
		{	return reinterpret_cast<size_t>(load_address_ptr());	}

		const char *image::absolute_path() const
		{	return _fullpath.c_str();	}

		void *image::get_symbol_address(const char *name) const
		{
			if (void *symbol = ::dlsym(get(), name))
				return symbol;
			throw runtime_error("Symbol specified was not found!");
		}

		unsigned image::get_symbol_rva(const char *name) const
		{
			if (void *symbol = ::dlsym(get(), name))
				return static_cast<byte *>(symbol) - load_address_ptr();
			throw runtime_error("Symbol specified was not found!");
		}
	}
}
