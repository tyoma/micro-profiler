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
		namespace
		{
			void *find_any_mapped_for(const string &name)
			{
				if (shared_ptr<FILE> f = shared_ptr<FILE>(fopen("/proc/self/maps", "r"), &fclose))
				{
					char line[1000] = { 0 };

					while (fgets(line, sizeof(line) - 1, f.get()))
					{
						uintptr_t from, to, offset;
						char rights[10], path[1000] = { 0 };
						unsigned dummy;
						int dummy_int;

						if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %s %" SCNxPTR " %x:%x %d %s\n",
							&from, &to, rights, &offset, &dummy, &dummy, &dummy_int, path) > 0 && path[0])
						{
							if (string(path).find(name) != string::npos)
								return reinterpret_cast<void *>(from);
						}
					}
				}
				return 0;
			}

			string get_current_dir()
			{
				char path[4096] = { 0 };
				char *p = getcwd(path, 4096);

				assert_not_null(p);
				return p;
			}
		}



		image::image(string path)
		{
			reset(::dlopen(path.c_str(), RTLD_NOW), [] (void *h) {
				if (h)
					::dlclose(h);
			});
			if (!get())
				throw runtime_error(("Cannot load module '" + path + "' specified!").c_str());

            Dl_info di = { };
#ifndef __APPLE__
			void *addr = find_any_mapped_for(*path);

			if (!addr)
				addr = get();
#else
            void *addr = nullptr;
            
            for (auto n = ::_dyld_image_count(); !addr && n--; )
            {
                if (file_id(::_dyld_get_image_name(n)) == file_id(path))
                    addr = reinterpret_cast<byte *>(::_dyld_get_image_vmaddr_slide(n));
            }
#endif
			::dladdr(addr, &di);
			_base = static_cast<byte *>(di.dli_fbase);
			_fullpath = path.front() != '/' ? get_current_dir() & path : path;
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

			return shared_ptr<void>(::mmap(0, page_size, PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
				[] (void *p) {
				::munmap(p, page_size);
			});
		}
	}
}
