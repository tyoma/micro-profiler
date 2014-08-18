#include "../commands.h"
#include "../dispatch.h"
#include "../../resources/resource.h"
#include "command-ids.h"

#include <atlbase.h>
#include <atlcom.h>
#include <comdef.h>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vsshell.h>

#include <InitGuid.h>
#include "guids.h"

namespace std
{
	using tr1::shared_ptr;
	using tr1::unordered_map;
}

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		typedef shared_ptr<project_command> command_ptr;

		pair<int /*command_id*/, command_ptr> g_commands[] = {
			make_pair(cmdidToggleProfiling, command_ptr(new toggle_profiling)),
			make_pair(cmdidRemoveProfilingSupport, command_ptr(new remove_profiling_support)),
		};


		class profiler_package : public CComObjectRootEx<CComSingleThreadModel>,
			public CComCoClass<profiler_package, &CLSID_MicroProfilerPackage>,
			public IVsPackage,
			public IOleCommandTarget
		{
		public:
			profiler_package()
				: _commands(g_commands, g_commands + _countof(g_commands))
			{

			}

		public:
			DECLARE_REGISTRY_RESOURCEID(IDR_PROFILEREXT)
			DECLARE_NOT_AGGREGATABLE(profiler_package)

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IVsPackage)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
			END_COM_MAP()

		private:
			typedef unordered_map<int, command_ptr> commands;

		private:
			STDMETHODIMP SetSite(IServiceProvider *sp)
			{
				_service_provider = sp;
				return S_OK;
			}

			STDMETHODIMP QueryClose(BOOL * /*can_close*/)
			{	return S_OK;	}

			STDMETHODIMP Close()
			{	return S_OK;	}

			STDMETHODIMP GetAutomationObject(LPCOLESTR /*pszPropName*/, IDispatch ** /*ppDisp*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP CreateTool(REFGUID /*rguidPersistenceSlot*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP ResetDefaults(VSPKGRESETFLAGS /*grfFlags*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP GetPropertyPage(REFGUID /*rguidPage*/, VSPROPSHEETPAGE * /*ppage*/)
			{	return E_NOTIMPL;	}


			STDMETHODIMP QueryStatus(const GUID *group, ULONG count, OLECMD commands[], OLECMDTEXT * /*pCmdText*/)
			{
				if (CLSID_MicroProfilerCmdSet == *group)
				{
					while (count--)
					{
						const commands::const_iterator c = _commands.find(commands[count].cmdID);
						const int state = c != _commands.end() ? c->second->query_state(get_project(*_service_provider)) : 0;

						commands[count].cmdf = ((state & project_command::supported) ? OLECMDF_SUPPORTED : 0)
							| ((state & project_command::enabled) ? OLECMDF_ENABLED : 0)
							| ((state & project_command::checked) ? OLECMDF_LATCHED : 0)
							| ((state & project_command::visible) ? 0 : OLECMDF_DEFHIDEONCTXTMENU);
					}
					return S_OK;
				}
				return E_INVALIDARG;
			}

			STDMETHODIMP Exec(const GUID *group, DWORD command, DWORD /*nCmdexecopt*/, VARIANT * /*pvaIn*/, VARIANT * /*pvaOut*/)
			{
				if (CLSID_MicroProfilerCmdSet == *group)
				{
					const commands::const_iterator c = _commands.find(command);

					if (c != _commands.end())
						c->second->exec(get_project(*_service_provider));
					return S_OK;
				}
				return E_INVALIDARG;
			}

		private:
			static dispatch get_project(IServiceProvider &service_provider)
			{
				CComPtr<IVsSolution> solution;
				CComPtr<IVsUIShell> ui_shell;
				CComPtr<IVsWindowFrame> hierarchy_window_frame;
				CComQIPtr<IVsUIHierarchyWindow> hierarchy_window;
				CComVariant hierarchy_window_v;

				service_provider.QueryService(SID_SVsSolution, &solution);
				service_provider.QueryService(SID_SVsUIShell, &ui_shell);
				ui_shell->FindToolWindow(FTW_fFrameOnly, GUID_SolutionExplorer, &hierarchy_window_frame);
				hierarchy_window_frame->GetProperty(VSFPROPID_DocView, &hierarchy_window_v);
				hierarchy_window = hierarchy_window_v.pdispVal;

				CComPtr<IVsMultiItemSelect> multi_select;
				CComPtr<IVsHierarchy> hierarchy;
				VSITEMID item_id;

				hierarchy_window->GetCurrentSelection(&hierarchy, &item_id, &multi_select);

				_variant_t vdte_project;

				hierarchy->GetProperty(VSITEMID_ROOT, VSHPROPID_ExtObject, &vdte_project);

				return dispatch((IDispatchPtr)vdte_project);
			}

		private:
			CComPtr<IServiceProvider> _service_provider;
			commands _commands;
		};

		OBJECT_ENTRY_AUTO(CLSID_MicroProfilerPackage, profiler_package);
	}
}
