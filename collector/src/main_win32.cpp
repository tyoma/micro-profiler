//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/collector_app.h>
#include <windows.h>

using namespace std;

extern micro_profiler::collector_app g_profiler_app;

namespace micro_profiler
{
	namespace
	{
		shared_ptr<void> *g_exit_process_detour = 0;

		shared_ptr<void> get_exit_process()
		{
			HMODULE hkernel;

			::GetModuleHandleExA(0, "kernel32", &hkernel);
			return shared_ptr<void>(::GetProcAddress(hkernel, "ExitProcess"), bind(&::FreeLibrary, hkernel));
		}

		void WINAPI ExitProcessHooked(UINT exit_code)
		{
			if (g_exit_process_detour)
				g_exit_process_detour->reset();
			g_profiler_app.stop();
			::ExitProcess(exit_code);
		}
	}

	platform_initializer::platform_initializer(collector_app &app)
		: _app(app)
	{
		HMODULE dummy;

		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
			reinterpret_cast<LPCTSTR>(&ExitProcessHooked), &dummy);

		static shared_ptr<void> exit_process_detour = detour(get_exit_process().get(), &ExitProcessHooked);

		g_exit_process_detour = &exit_process_detour;
	}

	platform_initializer::~platform_initializer()
	{	}
}
