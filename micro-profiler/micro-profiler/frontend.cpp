#include "frontend.h"
#include "profiler.h"
#include "symbol_resolver.h"

#include <tchar.h>
#include <process.h>

using namespace std;

namespace micro_profiler
{
	const LPCTSTR c_mailslot_name = _T("\\\\.\\mailslot\\CQG\\ProfilerMailslot");

	unsigned int __stdcall frontend::dispatch(frontend *pthis)
	{
		::CoInitialize(NULL);

		HANDLE mailslot = ::CreateMailslot(c_mailslot_name, 0, MAILSLOT_WAIT_FOREVER, NULL);
		HANDLE wait_handles[2] = { pthis->_exit_event, mailslot };

		while (WAIT_OBJECT_0 + 1 == ::WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE))
		{
			char text[1024] = { 0 };
			DWORD read = 0;

			::ReadFile(mailslot, text, sizeof(text), &read, NULL);

			CStringA command(text);

			if (command == "CLEAR")
				pthis->clear_statistics();
			else if (command.Find("DUMP ") == 0)
				pthis->dump(CString(command.Mid(5)));
		}
		::CloseHandle(mailslot);
		::CoUninitialize();
		return 0;
	}

	frontend::frontend()
	{
		unsigned dummy;

		_exit_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		_dispatcher_thread_handle = (HANDLE)_beginthreadex(0, 0, reinterpret_cast<unsigned int (__stdcall *)(void *)>(&frontend::dispatch), this, 0, &dummy);
	}

	frontend::~frontend()
	{
		::SetEvent(_exit_event);
		WaitForSingleObject(_dispatcher_thread_handle, INFINITE);
		::CloseHandle(_dispatcher_thread_handle);
		::CloseHandle(_exit_event);
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

			CFile output(thread_filename, CFile::modeCreate | CFile::modeWrite);
			CStringA tmp("Function,Calls #,Exclusive Time,Inclusive Time\r\n");

			output.Write((LPCSTR)tmp, tmp.GetLength());

			for (vector< pair<void * /*function*/, function_statistics> >::const_iterator j = i->statistics.begin();  j != i->statistics.end(); ++j)
			{
				CStringA name(sr.symbol_name_by_va(j->first));
				tmp.Format("\"%s\",%d,%f,%f\r\n", (LPCSTR)name, j->second.calls, j->second.exclusive_time / precision, j->second.inclusive_time / precision);
				output.Write((LPCSTR)tmp, tmp.GetLength());
			}
		}
	}

	namespace
	{
		frontend g_frontend;
	}
}
