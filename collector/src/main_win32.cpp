//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <crtdbg.h>

#include "detour.h"
#include "main.h"

#include <tchar.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		collector_app_instance *g_instance_singleton = nullptr;
		shared_ptr<void> *g_exit_process_detour = nullptr;

		void WINAPI ExitProcessHooked(UINT exit_code)
		{
			if (g_exit_process_detour)
				g_exit_process_detour->reset();
			g_instance_singleton->terminate();
			::ExitProcess(exit_code);
		}
	}

	void collector_app_instance::platform_specific_init()
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		static shared_ptr<void> exit_process_detour;

		g_instance_singleton = this;
		g_exit_process_detour = &exit_process_detour;

		HMODULE hmodule;

		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("kernel32"), &hmodule);

		const auto exit_process = ::GetProcAddress(hmodule, "ExitProcess");

		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
			reinterpret_cast<LPCTSTR>(&ExitProcessHooked), &hmodule);
		exit_process_detour = detour(exit_process, &ExitProcessHooked);
	}
}
