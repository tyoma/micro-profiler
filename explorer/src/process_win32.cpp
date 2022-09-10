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
#include <common/win32/time.h>
#include <sdb/integrated_index.h>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct processes_snapshot
		{
			processes_snapshot()
				: hsnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))
			{	}

			~processes_snapshot()
			{	::CloseHandle(hsnapshot);	}

			const HANDLE hsnapshot;
		};
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
		static SYSTEM_INFO sysinfo = {};
		static const auto os_is_64 = (::GetNativeSystemInfo(&sysinfo), sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64);

		auto &idx = sdb::unique_index(*this, keyer::pid());
		processes_snapshot snapshot;
		PROCESSENTRY32W entry = {	sizeof(PROCESSENTRY32W),	};
		const auto now = _clock();
		const auto time_diff = (now - _last_update).count();
		const auto itime_diff = time_diff ? 1.0f / time_diff : 0.0f;

		_cycle++;
		_last_update = now;
		for (auto lister = &::Process32FirstW; lister(snapshot.hsnapshot, &entry); lister = &::Process32NextW)
		{
			auto rec = idx[entry.th32ProcessID];
			auto &p = *rec;

			if (rec.is_new())
			{
				p.architecture = process_info::x86;
				p.cpu_time = mt::milliseconds(0);
				p.parent_pid = entry.th32ParentProcessID;
				unicode(p.path, entry.szExeFile);
				if (auto handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID))
				{
					BOOL wow64 = FALSE;

					p.architecture = os_is_64 && ::IsWow64Process(handle, &wow64) && !wow64
						? process_info::x64 : process_info::x86;
					p.handle.reset(handle, &::CloseHandle);
				}
			}
			if (p.handle)
			{
				FILETIME creation_, exit_, kernel_, user_;

				::GetProcessTimes(p.handle.get(), &creation_, &exit_, &kernel_, &user_);

				const auto user = to_milliseconds(user_);

				p.cpu_usage = (user - p.cpu_time).count() * itime_diff;
				p.cpu_time = user;
			}
			p.cycle = _cycle;
			rec.commit();
		}
		for (auto i = begin(); i != end(); )
		{
			auto j = i;

			++i;
			if (j->cycle != _cycle)
				modify(j).remove();
		}
		invalidate();
		_apartment.schedule([this] {	update();	}, _update_interval);
	}
}
