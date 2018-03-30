#pragma once

#include <resources/resource.h>

#include <atlbase.h>
#include <atlcom.h>
#include <functional>
#include <memory>
#include <wpl/base/concepts.h>

namespace micro_profiler
{
	class frontend_manager_impl;
	class functions_list;
	struct symbol_resolver;

	class ATL_NO_VTABLE Frontend : public ISequentialStream, public CComObjectRootEx<CComSingleThreadModel>,
		public CComCoClass<Frontend>, wpl::noncopyable
	{
	public:
		DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)
		DECLARE_CLASSFACTORY_EX(frontend_manager_impl)

		BEGIN_COM_MAP(Frontend)
			COM_INTERFACE_ENTRY(ISequentialStream)
		END_COM_MAP()

	public:
		Frontend();

		void Disconnect();

		void FinalRelease();

		STDMETHODIMP Read(void *, ULONG, ULONG *);
		STDMETHODIMP Write(const void *message, ULONG size, ULONG *written);

	public:
		std::function<void(const std::wstring &process_name, const std::shared_ptr<functions_list> &model)> initialized;
		std::function<void()> released;

	private:
		const std::shared_ptr<symbol_resolver> _resolver;
		std::shared_ptr<functions_list> _model;
	};
}