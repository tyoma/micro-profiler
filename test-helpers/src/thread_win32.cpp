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
			mt::milliseconds milliseconds(FILETIME t)
			{
				long long v = t.dwHighDateTime;

				v <<= 32;
				v += t.dwLowDateTime;
				v /= 10000; // FILETIME is expressed in 100-ns intervals
				return mt::milliseconds(v);
			}
		}

		unsigned int this_thread::get_native_id()
		{	return ::GetCurrentThreadId();	}

		mt::milliseconds this_thread::get_cpu_time()
		{
			FILETIME dummy, user = {}, kernel = {};

			::GetThreadTimes(::GetCurrentThread(), &dummy, &dummy, &kernel, &user);
			return milliseconds(user);
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
