#pragma once

#include "basics.h"

#include <comdef.h>
#include <functional>
#include <memory>

class event_bridge_base : public IDispatch
{
	const IID _iid;
	unsigned _refcount;

public:
	event_bridge_base(const IID &iid)
		: _iid(iid), _refcount(0)
	{	}

	virtual ~event_bridge_base()
	{	}

protected:
   STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
	{
		if (riid == _iid || riid == IID_IUnknown || riid == IID_IDispatch)
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

   STDMETHODIMP_(ULONG) AddRef()
	{	return ++_refcount;	}

   STDMETHODIMP_(ULONG) Release()
	{
		if (--_refcount)
			return _refcount;
		delete this;
		return 0;
	}

	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
	{
		*pctinfo = 0;
		return S_OK;
	}

	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
	{
		return S_OK;
	}

	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
	{
		return S_OK;
	}
};

class event_bridge : public event_bridge_base
{
	std::function<void()> _receiver;

	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
	{
		_receiver();
		return S_OK;
	}

public:
	event_bridge(const IID &iid, const std::function<void()> &receiver)
		: event_bridge_base(iid), _receiver(receiver)
	{	}
};



class connection : public destructible
{
	IConnectionPointPtr _cp;
	DWORD _cookie;

	connection(IConnectionPointPtr cp, DWORD cookie)
		: _cp(cp), _cookie(cookie)
	{	}

public:
	~connection()
	{	_cp->Unadvise(_cookie);	}

	static std::shared_ptr<destructible> connect(IConnectionPointContainerPtr cpc, REFIID iid, DISPID dispid, const std::function<void()> &receiver)
	{
		DWORD cookie;
		IConnectionPointPtr cp;
		IUnknownPtr sink((IUnknown *)new event_bridge(iid, receiver));

		cpc->FindConnectionPoint(iid, &cp);
		cp->Advise(sink, &cookie);
		return std::shared_ptr<destructible>(new connection(cp, cookie));
	}
};
