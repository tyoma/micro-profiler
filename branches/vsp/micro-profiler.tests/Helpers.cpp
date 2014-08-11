#include <crtdbg.h>

#include "Helpers.h"

#include <common/primitives.h>
#include <_generated/frontend.h>

#include <windows.h>

using wpl::mt::thread;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class Module
			{
			public:
				Module()
				{	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);	}
			} g_module;
		}

		int get_current_process_id()
		{	return ::GetCurrentProcessId();	}

		namespace this_thread
		{
			void sleep_for(unsigned int duration)
			{	::Sleep(duration);	}

			thread::id get_id()
			{	return ::GetCurrentThreadId();	}

			shared_ptr<running_thread> open()
			{
				class this_running_thread : shared_ptr<void>, public running_thread
				{
					thread::id _id;

				public:
					this_running_thread()
						: shared_ptr<void>(::OpenThread(THREAD_QUERY_INFORMATION | SYNCHRONIZE, FALSE,
							::GetCurrentThreadId()), &::CloseHandle), _id(::GetCurrentThreadId())
					{	}

					virtual ~this_running_thread() throw()
					{	}

					virtual void join()
					{	::WaitForSingleObject(get(), INFINITE);	}

					virtual wpl::mt::thread::id get_id() const
					{	return _id;	}

					virtual bool is_running() const
					{
						DWORD exit_code = STILL_ACTIVE;

						return ::GetExitCodeThread(get(), &exit_code) && exit_code == STILL_ACTIVE;
					}

					virtual void suspend()
					{	::SuspendThread(get());	}

					virtual void resume()
					{	::ResumeThread(get());	}
				};

				return shared_ptr<running_thread>(new this_running_thread());
			}
		}


		image::image(const TCHAR *path)
			: shared_ptr<void>(::LoadLibrary(path), &::FreeLibrary)
		{
			if (!get())
				throw runtime_error("Cannot load module specified!");

			wchar_t fullpath[MAX_PATH + 1] = { 0 };

			::GetModuleFileNameW(static_cast<HMODULE>(get()), fullpath, MAX_PATH);
			_fullpath = fullpath;
		}

		const void *image::load_address() const
		{	return get();	}

		const wchar_t *image::absolute_path() const
		{	return _fullpath.c_str();	}

		const void *image::get_symbol_address(const char *name) const
		{
			if (void *symbol = ::GetProcAddress(static_cast<HMODULE>(get()), name))
				return symbol;
			throw runtime_error("Symbol specified was not found!");
		}


		bool less_fs(const FunctionStatistics &lhs, const FunctionStatistics &rhs)
		{	return lhs.FunctionAddress < rhs.FunctionAddress; }

		bool less_fsd(const FunctionStatisticsDetailed &lhs, const FunctionStatisticsDetailed &rhs)
		{	return lhs.Statistics.FunctionAddress < rhs.Statistics.FunctionAddress; }

		bool operator ==(const function_statistics &lhs, const function_statistics &rhs)
		{
			return lhs.times_called == rhs.times_called && lhs.max_reentrance == rhs.max_reentrance
				&& lhs.inclusive_time == rhs.inclusive_time && lhs.exclusive_time == rhs.exclusive_time
				&& lhs.max_call_time == rhs.max_call_time;
		}

		function_statistics_detailed function_statistics_ex(unsigned __int64 times_called, unsigned __int64 max_reentrance, __int64 inclusive_time, __int64 exclusive_time, __int64 max_call_time)
		{
			function_statistics_detailed r;

			(function_statistics &)r = function_statistics(times_called, max_reentrance, inclusive_time, exclusive_time, max_call_time);
			return r;
		}
	}
}
