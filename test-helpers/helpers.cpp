#include <test-helpers/helpers.h>

#include <atlbase.h>
#include <ut/assert.h>

using wpl::mt::thread;
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


		vector_adapter::vector_adapter()
			: ptr(0)
		{	}

		void vector_adapter::write(const void *buffer_, size_t size)
		{
			const unsigned char *b = reinterpret_cast<const unsigned char *>(buffer_);

			buffer.insert(buffer.end(), b, b + size);
		}

		void vector_adapter::read(void *buffer_, size_t size)
		{
			assert_is_true(size <= buffer.size() - ptr);
			memcpy(buffer_, &buffer[ptr], size);
			ptr += size;
		}

		void vector_adapter::rewind(size_t pos)
		{	ptr = pos;	}

		size_t vector_adapter::end_position() const
		{	return buffer.size();	}
	}

	bool operator <(const function_statistics &lhs, const function_statistics &rhs)
	{
		return lhs.times_called < rhs.times_called ? true : lhs.times_called > rhs.times_called ? false :
			lhs.max_reentrance < rhs.max_reentrance ? true : lhs.max_reentrance > rhs.max_reentrance ? false :
			lhs.inclusive_time < rhs.inclusive_time ? true : lhs.inclusive_time > rhs.inclusive_time ? false :
			lhs.exclusive_time < rhs.exclusive_time ? true : lhs.exclusive_time > rhs.exclusive_time ? false :
			lhs.max_call_time < rhs.max_call_time;
	}

	bool operator ==(const function_statistics &lhs, const function_statistics &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}
}
