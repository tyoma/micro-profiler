//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <frontend/frontend_manager.h>

#include <frontend/frontend.h>

#include <logger/log.h>

using namespace std;
using namespace placeholders;

#define PREAMBLE "Frontend manager: "

namespace micro_profiler
{
	frontend_manager::instance_impl::instance_impl(micro_profiler::frontend *frontend_)
		: frontend(frontend_)
	{	}


	frontend_manager::frontend_manager(const frontend_ui_factory &ui_factory)
		: _references(1), _ui_factory(ui_factory), _active_instance(0)
	{	LOG(PREAMBLE "constructed.");	}

	frontend_manager::~frontend_manager()
	{	LOG(PREAMBLE "destroyed.");	}

	shared_ptr<frontend_manager> frontend_manager::create(const frontend_ui_factory &ui_factory)
	{	return shared_ptr<frontend_manager>(new frontend_manager(ui_factory), &destroy);	}

	void frontend_manager::close_all() throw()
	{
		for (instance_container::iterator i = _instances.begin(); i != _instances.end(); )
		{
			instance_container::iterator ii = i++;
			shared_ptr<frontend_ui> ui = ii->ui;

			ii->ui.reset();
			if (!ui)
				ii->frontend->disconnect_session();
		}
		LOG(PREAMBLE "all instances closed.");
	}

	size_t frontend_manager::instances_count() const throw()
	{	return _instances.size();	}

	const frontend_manager::instance *frontend_manager::get_instance(unsigned index) const throw()
	{
		instance_container::const_iterator i = _instances.begin();

		if (index >= _instances.size())
			return 0;
		return advance(i, index), &*i;
	}

	const frontend_manager::instance *frontend_manager::get_active() const throw()
	{	return _active_instance;	}

	void frontend_manager::load_session(const string &executable, const shared_ptr<functions_list> &model)
	{	on_ready_for_ui(_instances.insert(_instances.end(), instance_impl(0)), executable, model);	}

	shared_ptr<ipc::channel> frontend_manager::create_session(ipc::channel &outbound)
	{
		shared_ptr<frontend> f(new frontend(outbound));
		instance_container::iterator i = _instances.insert(_instances.end(), instance_impl(f.get()));

		try
		{
			// Untested: guarantee that any exception thrown after the instance is in the list would remove the entry.
			f->initialized = bind(&frontend_manager::on_ready_for_ui, this, i, _1, _2);
			f->released = bind(&frontend_manager::on_frontend_released, this, i); // Must go the last.
		}
		catch (...)
		{
			_instances.erase(i);
			throw;
		}
		addref();
		return f;
	}

	void frontend_manager::on_frontend_released(instance_container::iterator i) throw()
	{
		i->frontend = 0;
		if (!i->ui)
		{
			_instances.erase(i);
			LOG(PREAMBLE "no UI controller - instance removed.") % A(_instances.size());
		}
		release();
	}

	void frontend_manager::on_ready_for_ui(instance_container::iterator i, const string &executable,
		const shared_ptr<functions_list> &model)
	{
		i->executable = executable;
		i->model = model;
		if (shared_ptr<frontend_ui> ui = _ui_factory(model, executable))
		{
			i->ui_activated_connection = ui->activated += bind(&frontend_manager::on_ui_activated, this, i);
			i->ui_closed_connection = ui->closed += bind(&frontend_manager::on_ui_closed, this, i);
			i->ui = ui;
			_active_instance = &*i;
			addref();
		}
	}

	void frontend_manager::on_ui_activated(instance_container::iterator i)
	{	_active_instance = &*i;	}

	void frontend_manager::on_ui_closed(instance_container::iterator i) throw()
	{
		shared_ptr<frontend_ui> ui = i->ui;

		if (_active_instance == &*i)
			_active_instance = 0;
		i->ui_closed_connection.reset();
		i->ui.reset();
		if (i->frontend)
			i->frontend->disconnect_session();
		else
			_instances.erase(i);
		release();
	}

	void frontend_manager::addref() throw()
	{	++_references;	}

	void frontend_manager::release() throw()
	{
		if (!--_references)
			delete this;
	}

	void frontend_manager::destroy(frontend_manager *p)
	{
		p->close_all();
		p->release();
	}
}
