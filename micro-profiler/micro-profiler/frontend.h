#pragma once

#include <afx.h>
#include <atlstr.h>

namespace micro_profiler
{
	class frontend
	{
		HANDLE _exit_event;
		HANDLE _dispatcher_thread_handle;

		frontend(const frontend &);
		frontend &operator =(const frontend &);

		static unsigned int __stdcall dispatch(frontend *pthis);

		void clear_statistics();
		void dump(const CString &filename);

	public:
		frontend();
		~frontend();
	};
}
