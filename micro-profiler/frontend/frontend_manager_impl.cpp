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

#include "Frontend.h"

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	namespace
	{
		void terminate_factory(frontend_manager_impl *p, DWORD cookie)
		{
			p->terminate();
			::CoRevokeClassObject(cookie);
		}

		void terminate_regular(frontend_manager_impl *p)
		{
			p->terminate();
			p->Release();
		}
	}

	frontend_manager_impl::frontend_manager_impl()
		: _ui_factory(&default_ui_factory), _external_references(0), _external_lock_id(0)
	{	}

	frontend_manager_impl::~frontend_manager_impl()
	{	}

	void frontend_manager_impl::set_ui_factory(const frontend_ui_factory &ui_factory)
	{	_ui_factory = ui_factory;	}

	void frontend_manager_impl::terminate()
	{
		for (instance_container::iterator i = _instances.begin(); i != _instances.end(); )
		{
			shared_ptr<frontend_ui> ui;
			instance_container::iterator ii = i++;

			swap(ui, ii->ui); // ->ui must be empty when closed is fired to avoid double shared_ptr destruction.
			if (!ui)
				ii->frontend->Disconnect();
		}
	}

	size_t frontend_manager_impl::instances_count() const
	{	return _instances.size();	}

	const frontend_manager::instance *frontend_manager_impl::get_instance(unsigned index) const
	{
		instance_container::const_iterator i = _instances.begin();

		if (index >= _instances.size())
			return 0;
		return advance(i, index), &*i;
	}

	void frontend_manager_impl::load_instance(const instance &/*data*/)
	{	throw 0;	}

	STDMETHODIMP frontend_manager_impl::CreateInstance(IUnknown * /*outer*/, REFIID riid, void **object)
	try
	{
		CComObject<Frontend> *p = 0;
		CComObject<Frontend>::CreateInstance(&p);
		CComPtr<ISequentialStream> sp(p);		
		instance_container::iterator i = _instances.insert(_instances.end(), instance_impl());

		i->frontend = p;
		try
		{
			// Untested: guarantee that any exception thrown after the instance is in the list would remove the entry.
			p->initialized = bind(&frontend_manager_impl::on_ready_for_ui, this, i, _1, _2);
			p->released = bind(&frontend_manager_impl::on_frontend_released, this, i); // Must go the last.
		}
		catch (...)
		{
			_instances.erase(i);
			throw;
		}
		AddRef();
		return sp->QueryInterface(riid, object);
	}
	catch (const bad_alloc &)
	{
		return E_OUTOFMEMORY;
	}

	void frontend_manager_impl::on_frontend_released(instance_container::iterator i)
	{
		i->frontend = 0;
		if (!i->ui)
			_instances.erase(i);
		Release();
	}

	void frontend_manager_impl::on_ready_for_ui(instance_container::iterator i, const wstring &executable,
		const shared_ptr<functions_list> &model)
	{
		i->executable = executable;
		i->model = model;
		if (shared_ptr<frontend_ui> ui = _ui_factory(model, executable))
		{
			i->ui_closed_connection = ui->closed += bind(&frontend_manager_impl::on_ui_closed, this, i);
			i->ui = ui;
			add_external_reference();
		}
	}

	void frontend_manager_impl::on_ui_closed(instance_container::iterator i)
	{
		i->ui.reset();
		if (i->frontend)
			i->frontend->Disconnect();
		else
			_instances.erase(i);
		release_external_reference();
	}

	void frontend_manager_impl::add_external_reference()
	{
		CComPtr<IRunningObjectTable> rot;
		CComPtr<IMoniker> m;

		if (_external_references++)
			return;
		AddRef();
		if (S_OK == ::GetRunningObjectTable(0, &rot) && S_OK == ::CreateItemMoniker(OLESTR("!"), OLESTR("olock"), &m))
			rot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, this, m, &_external_lock_id);
	}

	void frontend_manager_impl::release_external_reference()
	{
		CComPtr<IRunningObjectTable> rot;

		if (--_external_references)
			return;
		if (S_OK == ::GetRunningObjectTable(0, &rot))
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

		p->set_ui_factory(ui_factory);
		if (S_OK == ::CoRegisterClassObject(clsid, p, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie))
			return shared_ptr<frontend_manager>(p, bind(&terminate_factory, _1, cookie));

		// Untested: if factory creation fails for some reason, let's retreat into non-COM frontend_manager - for loading
		//	purposes.
		p->AddRef();
		return shared_ptr<frontend_manager>(p, &terminate_regular);
	}
}
