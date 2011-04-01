#pragma once

#include <afx.h>
#include <atlstr.h>

namespace micro_profiler
{
	class frontend
	{
		HANDLE _commands_mailslot;
		HANDLE _dispatcher_thread_handle;

		frontend(const frontend &);
		frontend &operator =(const frontend &);

		static unsigned int __stdcall dispatch_proc(frontend *pthis);
		void dispatch();

	public:
		frontend();
		~frontend();

		void clear_statistics();
		void dump(const CString &filename);
	};
}
