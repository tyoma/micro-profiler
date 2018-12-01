#include <test-helpers/helpers.h>

#include <dlfcn.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		image::image(const wchar_t *path_)
		{
			wstring path(path_);
			string apath(path.begin(), path.end());

			reset(::dlopen(apath.c_str(), RTLD_NOW), &::dlclose);
			if (!get())
				throw runtime_error("Cannot load module specified!");
		}

		long_address_t image::load_address() const
		{	return reinterpret_cast<size_t>(get());	}

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
