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

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdexcept>

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
		char dummy;
		uint32_t l = 0;
		
		::_NSGetExecutablePath(&dummy, &l);
			
		vector<char> buffer(l);

		::_NSGetExecutablePath(&buffer[0], &l);
		buffer.push_back('\0');
		return &buffer[0];
	}

	mapped_module get_module_info(const void *address)
	{
		Dl_info di = { };

		::dladdr(address, &di);

		mapped_module info = {
			di.dli_fname && *di.dli_fname ? di.dli_fname : get_current_executable(),
			static_cast<byte *>(di.dli_fbase),
		};

		return info;
	}

	void enumerate_process_modules(const module_callback_t &callback)
	{
		for (uint32_t n = ::_dyld_image_count(); n--; )
		{
			mapped_module m = {
				::_dyld_get_image_name(n),
				reinterpret_cast<byte *>(::_dyld_get_image_vmaddr_slide(n)),
			};
			callback(m);
		}
	}
}
