#include <test-helpers/helpers.h>

#define __STDC_FORMAT_MACROS

#include <common/path.h>
#include <dlfcn.h>
#include <link.h>
#include <linux/limits.h>
#include <inttypes.h>
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
				shared_ptr<FILE> f(fopen("/proc/self/maps", "r"), &fclose);
				char line[1000] = { 0 };

				while (fgets(line, sizeof(line) - 1, f.get()))
				{
					void *from, *to, *offset;
					char rights[10], path[1000] = { 0 };
					unsigned dummy;

					if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %s %" SCNxPTR " %x:%x %d %s\n",
						&from, &to, rights, &offset, &dummy, &dummy, &dummy, path) > 0 && path[0])
					{
						if (string(path).find(name) != string::npos)
							return from;
					}
				}
				return 0;
			}

			string get_current_dir()
			{
				char path[PATH_MAX] = { 0 };
				char *p = getcwd(path, PATH_MAX);

				assert_not_null(p);
				return p;
			}

			void release_module(void *h)
			{
				if (h)
					::dlclose(h);
			}

			string make_dlpath(const string &from)
			{
				if (from.find('/') == string::npos)
					return "./" & from;
				return from;
			}
		}

		string get_current_process_executable()
		{
			int n;
			char path[1000] = {};

			if (n = ::readlink("/proc/self/exe", path, sizeof(path) - 1), n == -1)
				return string();
			path[n] = 0;
			return path;
		}


		image::image(string path)
		{
			if (path.find(".so") == string::npos)
				path = path + ".so";
			reset(::dlopen(make_dlpath(path).c_str(), RTLD_NOW), &release_module);
			if (!get())
			{
				path = "lib" + path;
				reset(::dlopen(make_dlpath(path).c_str(), RTLD_NOW), &release_module);
				if (!get())
					throw runtime_error(("Cannot load module '" + path + "' specified!").c_str());
			}

			void *addr = find_any_mapped_for(*path);
			Dl_info di = { };

			::dladdr(addr, &di);
			_base = static_cast<byte *>(di.dli_fbase);
			_fullpath = get_current_dir() & path;
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
	}
}
