#include "frontend.h"
#include "profiler.h"
#include "symbol_resolver.h"

#include <tchar.h>
#include <process.h>

using namespace std;

namespace micro_profiler
{
	const LPCTSTR c_mailslot_name = _T("\\\\.\\mailslot\\CQG\\ProfilerMailslot");

	unsigned int __stdcall frontend::dispatch_proc(frontend *pthis)
	{
		::CoInitialize(NULL);
		try
		{
			pthis->dispatch();
		}
		catch(...)
		{
		}
		::CoUninitialize();
		return 0;
	}

	void frontend::dispatch()
	{
		DWORD read;
		char command_text_buffer[1024] = { 0 };

		while (::ReadFile(_commands_mailslot, command_text_buffer, sizeof(command_text_buffer), &read, NULL))
		{
			CString command(command_text_buffer);

			if (command == _T("CLEAR"))
				clear_statistics();
			else if (command.Find(_T("DUMP ")) == 0)
				dump(command.Mid(5));
		}
	}

	frontend::frontend()
		: _commands_mailslot(::CreateMailslot(c_mailslot_name, 0, MAILSLOT_WAIT_FOREVER, NULL)),
			_dispatcher_thread_handle((HANDLE)_beginthreadex(0, 0, reinterpret_cast<unsigned int (__stdcall *)(void *)>(&frontend::dispatch_proc), this, 0, NULL))
	{	}

	frontend::~frontend()
	{
		// TODO: actually, this code does not terminate the thread, since CRT terminates it just before destroying global objects...
		::CloseHandle(_commands_mailslot);
		WaitForSingleObject(_dispatcher_thread_handle, INFINITE);
		::CloseHandle(_dispatcher_thread_handle);
	}

	void frontend::clear_statistics()
	{
		profiler::instance().clear();
	}

	void frontend::dump(const CString &filename)
	{
		symbol_resolver sr;
		vector<thread_calls_statistics> statistics;
		double precision(static_cast<double>(timestamp_precision()));

		profiler::instance().retrieve(statistics);

		for (vector<thread_calls_statistics>::const_iterator i = statistics.begin(); i != statistics.end(); ++i)
		{
			CString thread_filename;

			thread_filename.Format(_T("%s-%d.csv"), filename, i->thread_id);

//			CFile output(thread_filename, CFile::modeCreate | CFile::modeWrite);
			CStringA tmp("Function,Calls #,Exclusive Time,Inclusive Time\r\n");

//			output.Write((LPCSTR)tmp, tmp.GetLength());

			for (vector< pair<void * /*function*/, function_statistics> >::const_iterator j = i->statistics.begin();  j != i->statistics.end(); ++j)
			{
				CStringA name(sr.symbol_name_by_va(j->first));
				tmp.Format("\"%s\",%d,%f,%f\r\n", (LPCSTR)name, j->second.calls, j->second.exclusive_time / precision, j->second.inclusive_time / precision);
//				output.Write((LPCSTR)tmp, tmp.GetLength());
			}
		}
	}
}

micro_profiler::frontend g_pfrontend;
