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

#include <explorer/process.h>

#include <common/string.h>
#include <sdb/integrated_index.h>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		mt::milliseconds from_filetime(FILETIME value)
		{
			return mt::milliseconds(
				((static_cast<uint64_t>(value.dwHighDateTime) << 32) + value.dwLowDateTime) / 10000
			);
		}
	}

	namespace keyer
	{
		struct pid
		{
			id_t operator ()(const process_info &record) const {	return record.pid;	}

			template <typename IndexT>
			void operator ()(const IndexT &, process_info &record, id_t key) const
			{	record.pid = key;	}
		};
	}


	process_explorer::process_explorer(mt::milliseconds update_interval, scheduler::queue &apartment_queue,
			const function<mt::milliseconds ()> &clock)
		: _apartment(apartment_queue), _clock(clock), _update_interval(update_interval), _cycle(0)
	{	update();	}

	void process_explorer::update()
	{
		auto &idx = sdb::unique_index(*this, keyer::pid());
		shared_ptr<void> snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0), &::CloseHandle);
		PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W), };
		const auto now = _clock();
		auto time_diff = static_cast<float>((now - _last_update).count());

		_cycle++;
		_last_update = now;
		for (auto lister = &::Process32FirstW;
			lister(snapshot.get(), &entry);
			lister = &::Process32NextW, entry.szExeFile[0] = 0)
		{
			auto rec = idx[entry.th32ProcessID];
			auto &p = *rec;

			p.parent_pid = entry.th32ParentProcessID;
			p.path = unicode(entry.szExeFile);
			if (!p.handle)
			{
				if (auto handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID))
				{
					BOOL wow64 = FALSE;

					// TODO: untested
					if (::IsWow64Process(handle, &wow64))
						p.architecture = wow64 ? process_info::x86 : process_info::x64;
					p.handle.reset(handle, &::CloseHandle);
				}
				else
				{
					rec.remove();
					continue;
				}
			}
			if (p.handle)
			{
				FILETIME creation_, exit_, kernel_, user_;

				::GetProcessTimes(p.handle.get(), &creation_, &exit_, &kernel_, &user_);

				const auto user = from_filetime(user_);

				if (time_diff > 0.0f)
					p.cpu_usage = static_cast<float>((user - p.cpu_time).count()) / time_diff;
				p.cpu_time = user;
			}
			p.cycle = _cycle;
			rec.commit();
		}
		for (auto i = begin(); i != end(); ++i)
		{
			if (i->cycle != _cycle)
			{
				auto rec = modify(i);
				auto &p = *rec;

				p.handle.reset();
				p.cpu_usage = 0.0f;
				rec.commit();
			}
		}
		invalidate();
		_apartment.schedule([this] {	update();	}, _update_interval);
	}
}
