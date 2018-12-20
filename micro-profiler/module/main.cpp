//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/constants.h>
#include <frontend/frontend2.h>
#include <frontend/frontend_manager_impl.h>
#include <frontend/ProfilerMainDialog.h>
#include <setup/environment.h>

#include <vector>

using namespace std;
using namespace micro_profiler;

namespace micro_profiler
{
	HINSTANCE g_instance;

	namespace
	{
		class Module : public CAtlDllModuleT<Module> { } g_module;

		OBJECT_ENTRY_AUTO(reinterpret_cast<const GUID &>(c_standalone_frontend_id), Frontend);
	}

	shared_ptr<frontend_ui> frontend_manager_impl::default_ui_factory(const shared_ptr<functions_list> &model,
		const wstring &executable)
	{	return shared_ptr<frontend_ui>(new ProfilerMainDialog(model, executable));	}
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD reason, LPVOID reserved)
{
	if (DLL_PROCESS_ATTACH == reason)
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		g_instance = hinstance;
	}

	return g_module.DllMain(reason, reserved);
}

STDAPI DllCanUnloadNow()
{	return g_module.DllCanUnloadNow();	}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{	return g_module.DllGetClassObject(rclsid, riid, ppv);	}

STDAPI DllRegisterServer()
{	return register_path(false), g_module.DllRegisterServer(FALSE);	}

STDAPI DllUnregisterServer()
{	return unregister_path(false), g_module.DllUnregisterServer(FALSE);	}
