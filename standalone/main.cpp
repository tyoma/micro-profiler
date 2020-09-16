#include <crtdbg.h>

#include <wpl/win32/font_loader.h>

#include "ProfilerMainDialog.h"

#include "resources/resource.h"

#include <atlbase.h>
#include <common/constants.h>
#include <common/string.h>
#include <frontend/about_ui.h>
#include <frontend/system_stylesheet.h>
#include <frontend/factory.h>
#include <frontend/function_list.h>
#include <frontend/ipc_manager.h>
#include <ipc/com/endpoint.h>
#include <tchar.h>
#include <windows.h>
#include <wpl/factory.h>
#include <wpl/form.h>

using namespace micro_profiler;
using namespace std;
using namespace wpl;

namespace
{
	struct text_engine_composite : wpl::noncopyable
	{
		text_engine_composite()
			: text_engine(loader, 4)
		{	}

		win32::font_loader loader;
		gcontext::text_engine_type text_engine;
	};

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

	OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(static_cast<const GUID &>(constants::standalone_frontend_id),
		FauxFrontend);
}

namespace micro_profiler
{
	HINSTANCE g_instance;

	shared_ptr<ipc::server> ipc::com::server::create_default_session_factory()
	{	return shared_ptr<ipc::server>();	}

	struct ui_composite
	{
		shared_ptr<standalone_ui> ui;
		slot_connection connections[4];
		shared_ptr<form> about_form;
	};
}

int WINAPI _tWinMain(HINSTANCE instance, HINSTANCE /*previous_instance*/, LPTSTR command_line, int /*show_command*/)
try
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	g_instance = instance;

	HRESULT hresult;
	Module module;

	if (!module.ParseCommandLine(command_line, &hresult))
	{
		printf("(Un)Registration result: 0x%08X\n", hresult);
		return 0;
	}

	com_initialize ci;

	shared_ptr<text_engine_composite> tec(new text_engine_composite);
	shared_ptr<gcontext::text_engine_type> te(tec, &tec->text_engine);
	auto factory = make_shared<wpl::factory>(make_shared<gcontext::surface_type>(1, 1, 16),
		make_shared<gcontext::renderer_type>(2), te, make_shared<system_stylesheet>(te));

	wpl::factory::setup_default(*factory);
	setup_factory(*factory);

	auto ui_factory = [factory] (const shared_ptr<functions_list> &model, const string &executable) -> shared_ptr<frontend_ui>	{
		auto composite_ptr = make_shared<ui_composite>();
		auto &composite = *composite_ptr;
		auto factory2 = factory;

		composite.ui = make_shared<standalone_ui>(*factory, model, executable);
		composite.connections[0] = composite.ui->copy_to_buffer += [] (const string &text_utf8) {
			const auto text = unicode(text_utf8);
			if (::OpenClipboard(NULL))
			{
				if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t)))
				{
					wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

					copy(text.c_str(), text.c_str() + text.size() + 1, gtext_memory);
					::GlobalUnlock(gtext_memory);
					::EmptyClipboard();
					::SetClipboardData(CF_UNICODETEXT, gtext);
				}
				::CloseClipboard();
			}
		};
		composite.connections[1] = composite.ui->show_about += [factory2, &composite] (agge::point<int> center, const shared_ptr<form> &new_form) {
			if (!composite.about_form)
			{
				view_location l = { center.x - 200, center.y - 150, 400, 300 };
				auto &composite2 = composite;
				auto on_close = [&composite2] {
					composite2.about_form.reset();
				};
				auto about = make_shared<about_ui>(*factory2);

				new_form->set_view(about);
				new_form->set_location(l);
				new_form->set_visible(true);
				composite.connections[2] = new_form->close += on_close;
				composite.connections[3] = about->close += on_close;
				composite.about_form = new_form;
			}
		};
		return shared_ptr<frontend_ui>(composite_ptr, composite.ui.get());
	};
	auto main_form = factory->create_form();
	auto cancellation = main_form->close += [] { ::PostQuitMessage(0); };
	auto frontend_manager_ = make_shared<frontend_manager>(ui_factory);
	ipc_manager ipc_manager(frontend_manager_,
		make_pair(static_cast<unsigned short>(6100u), static_cast<unsigned short>(10u)),
		&constants::standalone_frontend_id);

	main_form->set_visible(true);

	MSG msg;

	while (::GetMessage(&msg, NULL, 0, 0))
	{
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}
//	module.Run(show_command);
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
