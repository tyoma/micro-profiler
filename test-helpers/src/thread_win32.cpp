#include <test-helpers/thread.h>

#include <test-helpers/helpers.h>

#include <cstdint>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			mt::milliseconds to_milliseconds(const FILETIME &ftime)
			{	return mt::milliseconds(((static_cast<uint64_t>(ftime.dwHighDateTime) << 32) + ftime.dwLowDateTime) / 10000);	}
		}

		unsigned int this_thread::get_native_id()
		{	return ::GetCurrentThreadId();	}

		mt::milliseconds this_thread::get_cpu_time()
		{
			FILETIME dummy, user = {}, kernel = {};

			::GetThreadTimes(::GetCurrentThread(), &dummy, &dummy, &kernel, &user);
			return to_milliseconds(user);
		}

		bool this_thread::set_description(const wchar_t *description)
		try
		{
			typedef HRESULT (WINAPI *SetThreadDescription_t)(HANDLE hthread, PCWSTR description);

			image kernel32("kernel32.dll");
			auto pSetThreadDescription
				= reinterpret_cast<SetThreadDescription_t>(kernel32.get_symbol_address("SetThreadDescription"));

			return S_OK == pSetThreadDescription(::GetCurrentThread(), description);
		}
		catch (...)
		{
			return false;
		}
	}
}
