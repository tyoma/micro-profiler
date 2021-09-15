//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <dlfcn.h>
#include <link.h>
#include <stdexcept>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	shared_ptr<void> load_library(const string &path)
	{
		return shared_ptr<void>(::dlopen(path.c_str(), RTLD_NOW), [] (void *h) {
			if (h)
				::dlclose(h);
		});
	}

	string get_current_executable()
	{
		char path[1000];
		int result = ::readlink("/proc/self/exe", path, sizeof(path) - 1);

		return path[result >= 0 ? result : 0] = 0, path;
	}

	mapped_module get_module_info(const void *address)
	{
		Dl_info di = { };

		::dladdr(address, &di);

		mapped_module info = {
			di.dli_fname && *di.dli_fname ? di.dli_fname : get_current_executable(),
			static_cast<byte *>(di.dli_fbase),
			std::vector<byte_range>()
		};

		return info;
	}

	void enumerate_process_modules(const module_callback_t &callback)
	{
		struct local
		{
			static int on_phdr(dl_phdr_info *phdr, size_t, void *cb)
			{
				int n = phdr->dlpi_phnum;
				const module_callback_t &callback = *static_cast<const module_callback_t *>(cb);
				mapped_module m = {
					phdr->dlpi_name && *phdr->dlpi_name ? phdr->dlpi_name : get_current_executable(),
					reinterpret_cast<byte *>(phdr->dlpi_addr),
					std::vector<byte_range>()
				};

				for (const ElfW(Phdr) *segment = phdr->dlpi_phdr; n; --n, ++segment)
					if (segment->p_type == PT_LOAD)
						m.addresses.push_back(byte_range(m.base + segment->p_vaddr, segment->p_memsz));
				if (!access(m.path.c_str(), 0))
					callback(m);
				return 0;
			}
		};

		::dl_iterate_phdr(&local::on_phdr, const_cast<module_callback_t *>(&callback));
	}
}
