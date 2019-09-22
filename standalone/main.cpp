#include <crtdbg.h>

#include "ProfilerMainDialog.h"

#include <resources/resource.h>

#include <atlbase.h>
#include <common/constants.h>
#include <common/string.h>
#include <frontend/function_list.h>
#include <frontend/ipc_manager.h>
#include <ipc/com/endpoint.h>
#include <tchar.h>
#include <windows.h>
#include <wpl/ui/form.h>
#include <wpl/ui/win32/form.h>

using namespace std;

namespace
{
	struct com_initialize
	{
		com_initialize()
		{	::CoInitialize(NULL);	}

		~com_initialize()
		{	::CoUninitialize();	}
	};

	class Module : public CAtlExeModuleT<Module>
	{
	public:
		Module()
		{
			if (::AttachConsole(ATTACH_PARENT_PROCESS))
			{
				_stdout.reset(freopen("CONOUT$", "wt", stdout), [] (FILE *f) {
					fclose(f);
					::FreeConsole();
				});
			}
		}

		HRESULT RegisterServer(BOOL /*bRegTypeLib*/ = FALSE, const CLSID* pCLSID = NULL) throw()
		{	return CAtlExeModuleT<Module>::RegisterServer(FALSE, pCLSID);	}

		HRESULT UnregisterServer(BOOL /*bRegTypeLib*/ = FALSE, const CLSID* pCLSID = NULL) throw()
		{	return CAtlExeModuleT<Module>::UnregisterServer(FALSE, pCLSID);	}

	private:
		shared_ptr<FILE> _stdout;
	};

	class FauxFrontend : public IUnknown, public CComObjectRoot, public CComCoClass<FauxFrontend>
	{
	public:
		DECLARE_REGISTRY_RESOURCEID(IDR_PROFILER_FRONTEND)
		BEGIN_COM_MAP(FauxFrontend)
			COM_INTERFACE_ENTRY(IUnknown)
		END_COM_MAP()
	};

	OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(reinterpret_cast<const GUID &>(micro_profiler::c_standalone_frontend_id),
		FauxFrontend);
}

namespace micro_profiler
{
	HINSTANCE g_instance;

	shared_ptr<ipc::server> ipc::com::server::create_default_session_factory()
	{	return shared_ptr<ipc::server>();	}
}

int WINAPI _tWinMain(HINSTANCE instance, HINSTANCE /*previous_instance*/, LPTSTR command_line, int show_command)
try
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	micro_profiler::g_instance = instance;

	HRESULT hresult;
	Module module;

	if (!module.ParseCommandLine(command_line, &hresult))
	{
		printf("(Un)Registration result: 0x%08X\n", hresult);
		return 0;
	}

	auto ui_factory = [] (const shared_ptr<micro_profiler::functions_list> &model, const string &executable)	{
		return make_shared<micro_profiler::ProfilerMainDialog>(model, executable);
	};
	auto main_form = wpl::ui::create_form();
	auto cancellation = main_form->close += [] { ::PostQuitMessage(0); };
	auto frontend_manager = micro_profiler::frontend_manager::create(ui_factory);
	micro_profiler::ipc::ipc_manager ipc_manager(frontend_manager, make_pair(6100u, 10u));
	com_initialize ci;
	auto com_server = micro_profiler::ipc::run_server(
		("com|" + micro_profiler::to_string(micro_profiler::c_standalone_frontend_id)).c_str(), frontend_manager);

	//setenv(micro_profiler::c_frontend_id_ev, micro_profiler::ipc::ipc_manager::format_endpoint("127.0.0.1",
	//	ipc_manager->get_sockets_port()).c_str(), 1);

	main_form->set_visible(true);
	module.Run(show_command);
	return 0;
}
catch (const exception &e)
{
	printf("Caught exception: %s...\nExiting!\n", e.what());
	return -1;
}
catch (...)
{
	printf("Caught an unknown exception...\nExiting!\n");
	return -1;
}
