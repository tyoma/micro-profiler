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

namespace
{
	class dispatch
	{
		_variant_t _underlying;

		IDispatchPtr this_object() const
		{
			if (IDispatchPtr o = _underlying)
				return o;
			throw runtime_error("The result is not an object!");
		}

		DISPID name_id(const wchar_t *name) const
		{
			DISPID id;
			LPOLESTR names[] = { const_cast<LPOLESTR>(name), };

			if (S_OK == this_object()->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &id))
				return id;
			throw runtime_error(_bstr_t(L"The name '") + name + L"' was not found for the object!");
		}

		HRESULT invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters, _variant_t &result) const
		{
			UINT invalidArg = 0;

			return this_object()->Invoke(name ? name_id(name) : 0, IID_NULL, LOCALE_USER_DEFAULT, wFlags,
					&parameters, &result, NULL, &invalidArg);
		}

		_variant_t invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters) const
		{
			_variant_t result;

			if (S_OK == invoke(name, wFlags, parameters, result))
				return result;
			throw runtime_error("Failed invoking method!");
		}

		explicit dispatch(const _variant_t &result)
			: _underlying(result)
		{	}

	public:
		explicit dispatch(const IDispatchPtr &object)
			: _underlying(object, true)
		{	}

		dispatch get(const wchar_t *name, const _variant_t &index = vtMissing) const
		{
			DISPPARAMS dispparams = { const_cast<_variant_t *>(&index), NULL, index != vtMissing ? 1 : 0, 0 };

			return dispatch(invoke(name, DISPATCH_PROPERTYGET, dispparams));
		}

		void put(const wchar_t *name, const _variant_t &value) const
		{
			DISPID propputnamed = DISPID_PROPERTYPUT;
			DISPPARAMS dispparams = { const_cast<_variant_t *>(&value), &propputnamed, 1, 1 };

			invoke(name, DISPATCH_PROPERTYPUT, dispparams);
		}

		template <typename IndexT>
		dispatch operator[](const IndexT &index) const
		{
			_variant_t vindex(index), result;
			DISPPARAMS dispparams = { &vindex, NULL, 1, 0 };

			if (S_OK == invoke(NULL, DISPATCH_PROPERTYGET, dispparams, result)
					|| S_OK == invoke(NULL, DISPATCH_METHOD, dispparams, result))
				return dispatch(result);
			throw runtime_error("Accessing an indexed property failed!");
		}

		dispatch operator()(const wchar_t *name)
		{
			DISPPARAMS dispparams = { 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename Arg1>
		dispatch operator()(const wchar_t *name, const Arg1 &arg1)
		{
			_variant_t args[] = { arg1, };
			DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename Arg1, typename Arg2>
		dispatch operator()(const wchar_t *name, const Arg1 &arg1, const Arg2 &arg2)
		{
			_variant_t args[] = { arg1, arg2, };
			DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename R>
		operator R() const
		{	return _underlying;	}
	};






	class command
	{
	public:
		typedef shared_ptr<command> ptr;

		int query_status(IServiceProvider &service_provider) const
		{
			CComPtr<IVsHierarchy> hierarchy;

			get_hierarchy(service_provider, &hierarchy);
			return query_status(*hierarchy);
		}

		void exec(IServiceProvider &service_provider) const
		{
			CComPtr<IVsHierarchy> hierarchy;

			get_hierarchy(service_provider, &hierarchy);
			exec(*hierarchy);
		}
		
	protected:
		virtual ~command()	{	}

		virtual int query_status(IVsHierarchy &hierarchy) const = 0;
		virtual void exec(IVsHierarchy &hierarchy) const = 0;

	private:
		static void get_hierarchy(IServiceProvider &service_provider, IVsHierarchy **hierarchy)
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
			VSITEMID item_id;

			hierarchy_window->GetCurrentSelection(hierarchy, &item_id, &multi_select);
		}
	};

	class enable_profiling : public command
	{
		virtual int query_status(IVsHierarchy &hierarchy) const;
		virtual void exec(IVsHierarchy &hierarchy) const;
	};

	class disable_profiling : public command
	{
		virtual int query_status(IVsHierarchy &hierarchy) const;
		virtual void exec(IVsHierarchy &hierarchy) const;
	};

	class remove_profiling_support : public command
	{
		virtual int query_status(IVsHierarchy &hierarchy) const;
		virtual void exec(IVsHierarchy &hierarchy) const;
	};

	pair<int /*command_id*/, command::ptr> g_commands[] = {
		make_pair(cmdidEnableProfiling, command::ptr(new enable_profiling)),
		make_pair(cmdidDisableProfiling, command::ptr(new disable_profiling)),
		make_pair(cmdidRemoveProfilingSupport, command::ptr(new remove_profiling_support)),
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
		typedef unordered_map<int, command::ptr> commands;

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

					commands[count].cmdf = c != _commands.end() ? c->second->query_status(*_service_provider) : 0;
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
					c->second->exec(*_service_provider);
				return S_OK;
			}
			return E_INVALIDARG;
		}

	private:
		CComPtr<IServiceProvider> _service_provider;
		commands _commands;
	};

	OBJECT_ENTRY_AUTO(CLSID_MicroProfilerPackage, profiler_package);


	int enable_profiling::query_status(IVsHierarchy &hierarchy) const
	{
		_variant_t vdte_project;

		hierarchy.GetProperty(VSITEMID_ROOT, VSHPROPID_ExtObject, &vdte_project);

		dispatch dte_project((IDispatchPtr)vdte_project);

		dispatch vcproject(dte_project.get(L"Object"));

		return OLECMDF_SUPPORTED | OLECMDF_ENABLED;
	}

	void enable_profiling::exec(IVsHierarchy &/*hierarchy*/) const
	{
	}


	int disable_profiling::query_status(IVsHierarchy &/*hierarchy*/) const
	{
		return OLECMDF_SUPPORTED;
	}

	void disable_profiling::exec(IVsHierarchy &/*hierarchy*/) const
	{
	}
	

	int remove_profiling_support::query_status(IVsHierarchy &/*hierarchy*/) const
	{
		return OLECMDF_SUPPORTED;
	}

	void remove_profiling_support::exec(IVsHierarchy &/*hierarchy*/) const
	{
	}
}
