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
#include <mt/mutex.h>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace micro_profiler
{
    namespace
    {
        struct address_hash_eq
        {
            size_t operator ()(const module::mapping &item) const
            {   return reinterpret_cast<size_t>(item.base); }
            
            bool operator ()(const module::mapping &lhs, const module::mapping &rhs) const
            {   return lhs.base == rhs.base;    }
        };
    
        mt::mutex g_mapping_mutex;
        std::unordered_set<module::mapping, address_hash_eq, address_hash_eq> g_mappings;
        
        struct notifier_init
        {
            notifier_init()
            {
                _dyld_register_func_for_add_image(&on_add_image);
                _dyld_register_func_for_remove_image(&on_remove_image);
            }
            
            static void on_add_image(const mach_header *, intptr_t base)
            {
                mt::lock_guard<mt::mutex> l(g_mapping_mutex);
                Dl_info dinfo = { };
                dladdr(reinterpret_cast<void *>(base), &dinfo);
                if (!dinfo.dli_fbase)
                    return;
                module::mapping m = {   dinfo.dli_fname, reinterpret_cast<byte *>(base), };
                
                g_mappings.insert(move(m));
            }
            
            static void on_remove_image(const mach_header *, intptr_t base)
            {
                module::mapping m = {   string(), reinterpret_cast<byte *>(base), };
                mt::lock_guard<mt::mutex> l(g_mapping_mutex);

                g_mappings.erase(m);
            }
        } g_init;
    }

	void *module::dynamic::find_function(const char *name) const
	{	return ::dlsym(_handle.get(), name);	}


	shared_ptr<module::dynamic> module::load(const string &path)
	{
		shared_ptr<void> handle(::dlopen(path.c_str(), RTLD_NOW), [] (void *h) {
			if (h)
				::dlclose(h);
		});

		return handle ? shared_ptr<module::dynamic>(new module::dynamic(handle)) : nullptr;
	}

	string module::executable()
	{
		char dummy;
		uint32_t l = 0;
		
		::_NSGetExecutablePath(&dummy, &l);
			
		vector<char> buffer(l);

		::_NSGetExecutablePath(&buffer[0], &l);
		buffer.push_back('\0');
		return &buffer[0];
	}

	module::mapping module::locate(const void *address)
	{
		Dl_info di = { };

		::dladdr(address, &di);

		mapping info = {
			di.dli_fname && *di.dli_fname ? di.dli_fname : executable(),
			static_cast<byte *>(di.dli_fbase),
		};

		return info;
	}

	void module::enumerate_mapped(const mapping_callback_t &callback)
	{
        mt::lock_guard<mt::mutex> l(g_mapping_mutex);
        
        for (auto &i : g_mappings)
            callback(i);
	}
}
