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

#include "frontend_manager_impl.h"

#include "frontend.h"

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace
	{
		void terminate_factory(frontend_manager_impl *p, DWORD cookie)
		{
			p->close_all();
			::CoRevokeClassObject(cookie);
		}

		void terminate_regular(frontend_manager_impl *p)
		{
			p->close_all();
			p->Release();
		}
	}

	frontend_manager_impl::frontend_manager_impl()
		: _external_references(0), _external_lock_id(0)
	{	_ui_factory = &default_ui_factory;	}

	STDMETHODIMP frontend_manager_impl::CreateInstance(IUnknown * /*outer*/, REFIID riid, void **object)
	try
	{
		CComObject<Frontend> *p = 0;
		CComObject<Frontend>::CreateInstance(&p);
		CComPtr<ISequentialStream> sp(p);

		register_frontend(*p);
		return sp->QueryInterface(riid, object);
	}
	catch (const bad_alloc &)
	{
		return E_OUTOFMEMORY;
	}

	void frontend_manager_impl::lock() throw()
	{
		CComPtr<IRunningObjectTable> rot;
		CComPtr<IMoniker> m;

		AddRef();
		if (!_external_references++ && S_OK == ::GetRunningObjectTable(0, &rot) && S_OK == ::CreateItemMoniker(OLESTR("!"), OLESTR("olock"), &m))
			rot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, this, m, &_external_lock_id);
	}

	void frontend_manager_impl::unlock() throw()
	{
		CComPtr<IRunningObjectTable> rot;

		if (!--_external_references && S_OK == ::GetRunningObjectTable(0, &rot))
			rot->Revoke(_external_lock_id);
		Release();
	}



	shared_ptr<frontend_manager> frontend_manager::create(const guid_t &id, const frontend_ui_factory &ui_factory)
	{
		const CLSID &clsid = reinterpret_cast<const CLSID &>(id);
		CComObject<frontend_manager_impl> *p = 0;
		CComObject<frontend_manager_impl>::CreateInstance(&p);
		CComPtr<IClassFactory> lock(p);
		DWORD cookie;

		p->_ui_factory = ui_factory;
		if (S_OK == ::CoRegisterClassObject(clsid, p, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie))
			return shared_ptr<frontend_manager>(p, bind(&terminate_factory, _1, cookie));

		// Untested: if factory creation fails for some reason, let's retreat into non-COM frontend_manager - for loading
		//	purposes.
		p->AddRef();
		return shared_ptr<frontend_manager>(p, &terminate_regular);
	}
}
