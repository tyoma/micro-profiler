#pragma once

#include "basics.h"

#include <comdef.h>
#include <functional>
#include <memory>

class event_bridge_base : public IDispatch
{
	const IID _iid;
	DISPID _dispid;
	unsigned _refcount;

public:
	event_bridge_base(const IID &iid, DISPID dispid)
		: _iid(iid), _dispid(dispid), _refcount(0)
	{	}

	virtual ~event_bridge_base()
	{	}

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

private:
	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
	{	return *pctinfo = 0, S_OK;	}

	STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
	{	return S_OK;	}

	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
	{	return S_OK;	}

	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
	{
		if (_dispid == dispIdMember)
			try
			{
				invoke(pDispParams);
				return S_OK;
			}
			catch (...)
			{
				return E_FAIL;
			}
		return S_OK;
	}

	virtual void invoke(DISPPARAMS *pDispParams) = 0;
};

class event_bridge : public event_bridge_base
{
	std::function<void()> _receiver;

	virtual void invoke(DISPPARAMS *pDispParams)
	{	_receiver();	}

public:
	event_bridge(const IID &iid, DISPID dispid, const std::function<void()> &receiver)
		: event_bridge_base(iid, dispid), _receiver(receiver)
	{	}
};

template <typename Arg1>
class event_bridge1 : public event_bridge_base
{
	std::function<void(Arg1)> _receiver;

	virtual void invoke(DISPPARAMS *pDispParams)
	{	_receiver(_variant_t(pDispParams->rgvarg[0]));	}

public:
	event_bridge1(const IID &iid, DISPID dispid, const std::function<void(Arg1)> &receiver)
		: event_bridge_base(iid, dispid), _receiver(receiver)
	{	}
};

template <typename Arg1, typename Arg2>
class event_bridge2 : public event_bridge_base
{
	std::function<void(Arg1, Arg2)> _receiver;

	virtual void invoke(DISPPARAMS *pDispParams)
	{	_receiver(_variant_t(pDispParams->rgvarg[1]), _variant_t(pDispParams->rgvarg[0]));	}

public:
	event_bridge2(const IID &iid, DISPID dispid, const std::function<void(Arg1, Arg2)> &receiver)
		: event_bridge_base(iid, dispid), _receiver(receiver)
	{	}
};


class connection : public destructible
{
	IConnectionPointPtr _cp;
	DWORD _cookie;

	connection(IConnectionPointPtr cp, DWORD cookie)
		: _cp(cp), _cookie(cookie)
	{	}

	static std::shared_ptr<destructible> make_connection(IConnectionPointContainerPtr cpc, REFIID iid, IUnknownPtr sink)
	{
		DWORD cookie;
		IConnectionPointPtr cp;

		cpc->FindConnectionPoint(iid, &cp);
		cp->Advise(sink, &cookie);
		return std::shared_ptr<destructible>(new connection(cp, cookie));
	}

public:
	~connection()
	{	_cp->Unadvise(_cookie);	}

	static std::shared_ptr<destructible> make(IConnectionPointContainerPtr cpc, REFIID iid, DISPID dispid, const std::function<void()> &receiver)
	{	return make_connection(cpc, iid, IUnknownPtr(new event_bridge(iid, dispid, receiver)));	}

	template <typename Arg1>
	static std::shared_ptr<destructible> make(IConnectionPointContainerPtr cpc, REFIID iid, DISPID dispid, const std::function<void(Arg1)> &receiver)
	{	return make_connection(cpc, iid, IUnknownPtr(new event_bridge1<Arg1>(iid, dispid, receiver)));	}

	template <typename Arg1, typename Arg2>
	static std::shared_ptr<destructible> make(IConnectionPointContainerPtr cpc, REFIID iid, DISPID dispid, const std::function<void(Arg1, Arg2)> &receiver)
	{	return make_connection(cpc, iid, IUnknownPtr(new event_bridge2<Arg1, Arg2>(iid, dispid, receiver)));	}
};
