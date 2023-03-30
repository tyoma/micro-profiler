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

			value |= (segment_access & PF_X) ? protection::execute : 0;
			value |= (segment_access & PF_W) ? protection::write : 0;
			value |= (segment_access & PF_R) ? protection::read : 0;
			return value;
		}

		template <typename T>
		void enumerate_headers(T callback)
		{
			struct local
			{
				static int on_module(dl_phdr_info *header, size_t, void *callback_)
				{	return (*(T *)callback_)(*header), 0;	}
			};

			::dl_iterate_phdr(&local::on_module, &callback);
		}

		template <typename ExeNameT>
		void update_mapping(module::mapping &mapping, const dl_phdr_info &header, const ExeNameT &get_executable_name)
		{
			auto n = header.dlpi_phnum;

			mapping.path = header.dlpi_name && *header.dlpi_name ? header.dlpi_name : get_executable_name();
			mapping.base = reinterpret_cast<byte *>(header.dlpi_addr);
			mapping.regions.clear();
			for (const ElfW(Phdr) *segment = header.dlpi_phdr; n; --n, ++segment)
				if (segment->p_type == PT_LOAD)
					mapping.regions.push_back(mapped_region {
						mapping.base + segment->p_vaddr, segment->p_memsz, generic_protection(segment->p_flags)
					});
		}
	}

	class module_platform : public module
	{
	private:
		class tracker;

	private:
		virtual shared_ptr<dynamic> load(const string &path) override;
		virtual string executable() override;
		virtual mapping locate(const void *address) override;
		virtual shared_ptr<mapping> lock_at(void *address) override;
		virtual shared_ptr<void> notify(events &consumer) override;
	};

	class module_platform::tracker
	{
	public:
		tracker(module &owner, events &consumer);
		~tracker();

	private:
		int on_module(const dl_phdr_info &header);
		void on_module(const module::mapping &m);

	private:
		mt::event _exit;
		string _executable;
		events &_consumer;
		unordered_map< file_id, pair<module::mapping, bool /*deleted*/> > _mappings;
		module::mapping _tmp_mapping;
		unique_ptr<mt::thread> _monitor;
	};



	void *module::dynamic::find_function(const char *name) const
	{	return ::dlsym(_handle.get(), name);	}


	shared_ptr<module::dynamic> module_platform::load(const string &path)
	{
		shared_ptr<void> handle(::dlopen(path.c_str(), RTLD_NOW), [] (void *h) {
			if (h)
				::dlclose(h);
		});

		return handle ? shared_ptr<module::dynamic>(new module::dynamic(handle)) : nullptr;
	}

	string module_platform::executable()
	{
		char path[1000];
		int result = ::readlink("/proc/self/exe", path, sizeof(path) - 1);

		return path[result >= 0 ? result : 0] = 0, path;
	}

	module::mapping module_platform::locate(const void *address)
	{
		Dl_info di = { };

		::dladdr(address, &di);
		return mapping {
			di.dli_fname && *di.dli_fname ? di.dli_fname : executable(),
			static_cast<byte *>(di.dli_fbase),
		};
	}

	shared_ptr<module::mapping> module_platform::lock_at(void *address_)
	{
		typedef pair<shared_ptr<void>, mapping> composite_t;

		const auto address = static_cast<byte *>(address_);
		shared_ptr<composite_t> m;

		enumerate_headers([&] (const dl_phdr_info &header) {
			auto n = header.dlpi_phnum;
			auto base = reinterpret_cast<byte *>(header.dlpi_addr);

			for (const auto *segment = header.dlpi_phdr; n; --n, ++segment)
				if (base + segment->p_vaddr <= address && address < base + segment->p_vaddr + segment->p_memsz)
				{
					m = make_shared<composite_t>();
					update_mapping(m->second, header, [] {	return string();	});
					m->first.reset(::dlopen(m->second.path.c_str(), RTLD_LAZY | RTLD_NOLOAD), [] (void *h) {
						if (h)
							::dlclose(h);
					});
				}
		});
		return m && m->first ? shared_ptr<mapping>(m, &m->second) : nullptr;
	}

	shared_ptr<void> module_platform::notify(events &consumer)
	{	return make_shared<tracker>(*this, consumer);	}


	module_platform::tracker::tracker(module &owner, events &consumer)
		: _executable(owner.executable()), _consumer(consumer)
	{
		enumerate_headers([this] (const dl_phdr_info &header) {	on_module(header);	});
		_monitor.reset(new mt::thread([this] {
			while (!_exit.wait(c_mapping_monitor_interval))
			{
				for (auto &i : _mappings)
					i.second.second = true;
				enumerate_headers([this] (const dl_phdr_info &header) {	on_module(header);	});
				for (auto i = _mappings.begin(); i != _mappings.end(); )
					if (i->second.second)
						_consumer.unmapped(i->second.first.base), i = _mappings.erase(i);
					else
						i++;
			}
		}));
	}

	module_platform::tracker::~tracker()
	{
		_exit.set();
		_monitor->join();
	}

	int module_platform::tracker::on_module(const dl_phdr_info &header)
	{
		update_mapping(_tmp_mapping, header, [this] {	return _executable;	});
		if (!access(_tmp_mapping.path.c_str(), 0))
			on_module(_tmp_mapping);
		return 0;
	}

	void module_platform::tracker::on_module(const module::mapping &m)
	{
		file_id id(m.path);
		const auto i = _mappings.find(id);

		if (i == _mappings.end())
		{
			_consumer.mapped(m);
			_mappings.emplace(id, make_pair(m, false));
			return;
		}
		else if (m.base != i->second.first.base)
		{
			_consumer.unmapped(i->second.first.base);
			_consumer.mapped(i->second.first = m);
		}
		i->second.second = false;
	}


	module &module::platform()
	{
		static module_platform m;
		return m;
	}
}
