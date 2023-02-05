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
#include <list>
#include <mach/vm_map.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
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
			{	return reinterpret_cast<size_t>(item.base);	}

			bool operator ()(const module::mapping &lhs, const module::mapping &rhs) const
			{	return lhs.base == rhs.base;	}
		};

		mt::mutex g_mapping_mutex;
		unordered_set<module::mapping, address_hash_eq, address_hash_eq> g_mappings;
		list< pair<module::mapping_callback_t, module::unmapping_callback_t> > g_callbacks;

		int generic_protection(int protection)
		{
			auto value = 0;

			value |= (protection & VM_PROT_EXECUTE) ? mapped_region::execute : 0;
			value |= (protection & VM_PROT_WRITE) ? mapped_region::write : 0;
			value |= (protection & VM_PROT_READ) ? mapped_region::read : 0;
			return value;
		}

		struct notifier_init
		{
			notifier_init()
			{
				_dyld_register_func_for_add_image(&on_add_image);
				_dyld_register_func_for_remove_image(&on_remove_image);
			}

			static shared_ptr<void> notify(module::mapping_callback_t mapped, module::unmapping_callback_t unmapped)
			{
				mt::lock_guard<mt::mutex> l(g_mapping_mutex);
				auto i = g_callbacks.insert(g_callbacks.end(), make_pair(mapped, unmapped));
				auto handle = shared_ptr<void>(&*i, [i] (void *) {
					mt::lock_guard<mt::mutex> l(g_mapping_mutex);
					g_callbacks.erase(i);
				});
				
				for (const auto &m : g_mappings)
					mapped(m);
				return handle;
			}
			
			static void on_add_image(const mach_header *, intptr_t base)
			{
				mt::lock_guard<mt::mutex> l(g_mapping_mutex);
				Dl_info dinfo = {};

				dladdr(reinterpret_cast<void *>(base), &dinfo);
				if (!dinfo.dli_fbase)
					return;
				module::mapping m = {	dinfo.dli_fname, reinterpret_cast<byte *>(base),	};
				read_regions(m.regions, dinfo.dli_fbase);
				g_mappings.insert(m);
				for (const auto &i : g_callbacks)
					i.first(m);
			}

			static void on_remove_image(const mach_header *, intptr_t base)
			{
				mt::lock_guard<mt::mutex> l(g_mapping_mutex);

				g_mappings.erase(module::mapping {	string(), reinterpret_cast<byte *>(base),	});
				for (const auto &i : g_callbacks)
					i.second(reinterpret_cast<byte *>(base));
			}
			
			static void read_regions(vector<mapped_region> &regions, void *address)
			{
				const auto self = mach_task_self();
				size_t sz = 0;
				uint32_t depth = 0;
				vm_region_submap_info_64 info;
				mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
				Dl_info dinfo = {};
				
				for (vm_address_t a = reinterpret_cast<mach_vm_address_t>(address);
					  dladdr(reinterpret_cast<void *>(a), &dinfo) && dinfo.dli_fbase == address
							&& KERN_SUCCESS == vm_region_recurse_64(self, &a, &sz, &depth, (vm_region_recurse_info_t)&info, &count);
					  info.is_submap ? depth++ : (a += sz))
				{
					regions.push_back(mapped_region {
						reinterpret_cast<byte *>(a),
						sz,
						generic_protection(info.protection)
					});
				}
			}
		} g_init;
	}

	class module::tracker
	{
	public:
		tracker(mapping_callback_t mapped, unmapping_callback_t unmapped);
		~tracker();

	};

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

	shared_ptr<void> module::notify(mapping_callback_t mapped, unmapping_callback_t unmapped)
	{	return notifier_init::notify(mapped, unmapped);	}
}
