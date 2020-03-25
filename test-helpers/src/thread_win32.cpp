#include <test-helpers/thread.h>

#include <test-helpers/helpers.h>

#include <common/string.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			long long microseconds(FILETIME t)
			{
				long long v = t.dwHighDateTime;

				v <<= 32;
				v += t.dwLowDateTime;
				v /= 10; // FILETIME is expressed in 100-ns intervals
				return v;
			}
		}

		shared_ptr<running_thread> this_thread::open()
		{
			class this_running_thread : shared_ptr<void>, public running_thread
			{
			public:
				this_running_thread()
				{
					HANDLE handle = NULL;

					::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &handle, 0, FALSE,
						DUPLICATE_SAME_ACCESS);
					reset(handle, &CloseHandle);
				}

				virtual void join()
				{	::WaitForSingleObject(get(), INFINITE);	}

				virtual bool join(mt::milliseconds timeout)
				{	return WAIT_TIMEOUT != ::WaitForSingleObject(get(), static_cast<DWORD>(timeout.count()));	}
			};

			return shared_ptr<running_thread>(new this_running_thread());
		}

		unsigned int this_thread::get_native_id()
		{	return ::GetCurrentThreadId();	}

		mt::milliseconds this_thread::get_cpu_time()
		{
			FILETIME dummy, user = {}, kernel = {};

			::GetThreadTimes(::GetCurrentThread(), &dummy, &dummy, &kernel, &user);
			return mt::milliseconds(microseconds(user) / 1000);
		}

		bool this_thread::set_description(const wchar_t *description)
		{
			typedef HRESULT (WINAPI *SetThreadDescription_t)(HANDLE hthread, PCWSTR description);

			image kernel32("kernel32.dll");
			SetThreadDescription_t pSetThreadDescription
				= reinterpret_cast<SetThreadDescription_t>(kernel32.get_symbol_address("SetThreadDescription"));

			return pSetThreadDescription ? pSetThreadDescription(::GetCurrentThread(), description), true : false;
		}
	}
}
