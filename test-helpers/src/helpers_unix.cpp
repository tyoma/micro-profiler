#include <test-helpers/helpers.h>

#define __STDC_FORMAT_MACROS

#include <common/file_id.h>
#include <common/path.h>
#include <dlfcn.h>
#include <inttypes.h>
#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		image::image(string path)
		{
			reset(::dlopen(path.c_str(), RTLD_NOW), [] (void *h) {
				if (h)
					::dlclose(h);
			});
			if (!get())
				throw runtime_error(("Cannot load module '" + path + "' specified!").c_str());

			Dl_info di = { };
			auto addr = get_symbol_address("track_unload"); // A symbol known to be present.

			::dladdr(addr, &di);
			_base = static_cast<byte *>(di.dli_fbase);
			_fullpath = di.dli_fname;
		}

		byte *image::base_ptr() const
		{	return _base;	}

		long_address_t image::base() const
		{	return reinterpret_cast<size_t>(base_ptr());	}

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
				return static_cast<byte *>(symbol) - base_ptr();
			throw runtime_error("Symbol specified was not found!");
		}

		shared_ptr<void> occupy_memory(void *s, unsigned int l)
		{
			return shared_ptr<void>(::mmap(s, l, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
				[l] (void *p) {
				::munmap(p, l);
			});
		}

		shared_ptr<void> allocate_edge()
		{
			enum {	page_size = 4096	};

			return shared_ptr<void>(::mmap(0, page_size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
				[] (void *p) {
				::munmap(p, page_size);
			});
		}
	}
}
