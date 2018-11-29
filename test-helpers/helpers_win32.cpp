#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		com_event::com_event()
			: _handle(::CreateEvent(NULL, FALSE, FALSE, NULL), &CloseHandle)
		{	}

		void com_event::signal()
		{	::SetEvent(_handle.get());	}

		void com_event::wait()
		{
			for (HANDLE h = _handle.get();
				WAIT_OBJECT_0 + 1 == ::MsgWaitForMultipleObjects(1, &h, FALSE, INFINITE, QS_ALLINPUT); )
			{
				MSG msg;

				while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
					::DispatchMessage(&msg);
			}
		}

		wstring get_current_process_executable()
		{
			wchar_t fullpath[MAX_PATH + 1] = { };

			::GetModuleFileNameW(NULL, fullpath, MAX_PATH);
			return fullpath;
		}

		namespace this_thread
		{
			void sleep_for(unsigned int duration)
			{	::Sleep(duration);	}

			shared_ptr<running_thread> open()
			{
				class this_running_thread : shared_ptr<void>, public running_thread
				{
					mt::thread::id _id;

				public:
					this_running_thread()
						: shared_ptr<void>(::OpenThread(THREAD_QUERY_INFORMATION | SYNCHRONIZE, FALSE,
							::GetCurrentThreadId()), &::CloseHandle), _id(mt::this_thread::get_id())
					{	}

					virtual ~this_running_thread() throw()
					{	}

					virtual void join()
					{	::WaitForSingleObject(get(), INFINITE);	}

					virtual mt::thread::id get_id() const
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


		image::image(const wchar_t *path)
			: shared_ptr<void>(::LoadLibraryW(path), &::FreeLibrary)
		{
			if (!get())
				throw runtime_error("Cannot load module specified!");

			wchar_t fullpath[MAX_PATH + 1] = { };

			::GetModuleFileNameW(static_cast<HMODULE>(get()), fullpath, MAX_PATH);
			_fullpath = fullpath;
		}

		long_address_t image::load_address() const
		{	return reinterpret_cast<size_t>(get());	}

		const wchar_t *image::absolute_path() const
		{	return _fullpath.c_str();	}

		void *image::get_symbol_address(const char *name) const
		{
			if (void *symbol = ::GetProcAddress(static_cast<HMODULE>(get()), name))
				return symbol;
			throw runtime_error("Symbol specified was not found!");
		}
	}
}
