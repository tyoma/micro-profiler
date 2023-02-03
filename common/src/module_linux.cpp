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

#include <common/file_id.h>
#include <dlfcn.h>
#include <link.h>
#include <mt/event.h>
#include <mt/thread.h>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const mt::milliseconds c_mapping_monitor_interval(100);

		int generic_protection(uint64_t segment_access)
		{
			int value = 0;

			value |= (segment_access & PF_X) ? mapped_region::execute : 0;
			value |= (segment_access & PF_W) ? mapped_region::write : 0;
			value |= (segment_access & PF_R) ? mapped_region::read : 0;
			return value;
		}
	}

	class module::tracker
	{
	public:
		tracker(module::mapping_callback_t mapped, module::unmapping_callback_t unmapped);
		~tracker();

	private:
		static int on_module(dl_phdr_info *phdr, size_t, void *cb);
		void on_module(const module::mapping &m);

	private:
		mt::event _exit;
		const module::mapping_callback_t _mapped;
		const module::unmapping_callback_t _unmapped;
		unordered_map< file_id, pair<module::mapping, bool /*deleted*/> > _mappings;
		module::mapping _tmp_mapping;
		unique_ptr<mt::thread> _monitor;
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
		char path[1000];
		int result = ::readlink("/proc/self/exe", path, sizeof(path) - 1);

		return path[result >= 0 ? result : 0] = 0, path;
	}

	module::mapping module::locate(const void *address)
	{
		Dl_info di = { };

		::dladdr(address, &di);
		return mapping {
			di.dli_fname && *di.dli_fname ? di.dli_fname : executable(),
			static_cast<byte *>(di.dli_fbase),
		};
	}

	shared_ptr<void> module::notify(mapping_callback_t mapped, module::unmapping_callback_t unmapped)
	{	return make_shared<tracker>(mapped, unmapped);	}


	module::tracker::tracker(module::mapping_callback_t mapped, module::unmapping_callback_t unmapped)
		: _mapped(mapped), _unmapped(unmapped)
	{
		::dl_iterate_phdr(&tracker::on_module, this);
		_monitor.reset(new mt::thread([this] {
			while (!_exit.wait(c_mapping_monitor_interval))
			{
				for (auto &i : _mappings)
					i.second.second = true;
				::dl_iterate_phdr(&tracker::on_module, this);
				for (auto i = _mappings.begin(); i != _mappings.end(); )
					if (i->second.second)
						_unmapped(i->second.first.base), i = _mappings.erase(i);
					else
						i++;
			}
		}));
	}

	module::tracker::~tracker()
	{
		_exit.set();
		_monitor->join();
	}

	int module::tracker::on_module(dl_phdr_info *phdr, size_t, void *cb)
	{
		auto n = phdr->dlpi_phnum;
		auto &self = *static_cast<tracker *>(cb);
		auto &m = self._tmp_mapping;

		m.path = phdr->dlpi_name && *phdr->dlpi_name ? phdr->dlpi_name : module::executable();
		m.base = reinterpret_cast<byte *>(phdr->dlpi_addr);
		m.regions.clear();
		for (const ElfW(Phdr) *segment = phdr->dlpi_phdr; n; --n, ++segment)
			if (segment->p_type == PT_LOAD)
				m.regions.push_back(mapped_region {
					m.base + segment->p_vaddr, segment->p_memsz, generic_protection(segment->p_flags)
				});
		if (!access(m.path.c_str(), 0))
			self.on_module(m);
		return 0;
	}

	void module::tracker::on_module(const module::mapping &m)
	{
		file_id id(m.path);
		const auto i = _mappings.find(id);

		if (i == _mappings.end())
		{
			_mapped(m);
			_mappings.emplace(id, make_pair(m, false));
			return;
		}
		else if (m.base != i->second.first.base)
		{
			_unmapped(i->second.first.base);
			_mapped(i->second.first = m);
		}
		i->second.second = false;
	}
}
