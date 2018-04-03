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

#include "frontend_manager.h"

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	frontend_manager::frontend_manager()
		: _active_instance(0)
	{	}

	frontend_manager::~frontend_manager()
	{	}

	void frontend_manager::close_all() throw()
	{
		for (instance_container::iterator i = _instances.begin(); i != _instances.end(); )
		{
			instance_container::iterator ii = i++;
			shared_ptr<frontend_ui> ui = ii->ui;

			ii->ui.reset();
			if (!ui)
				ii->frontend->disconnect();
		}
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

	void frontend_manager::load_instance(const instance &/*data*/)
	{	throw 0;	}

	void frontend_manager::register_frontend(frontend &new_frontend)
	{
		instance_container::iterator i = _instances.insert(_instances.end(), instance_impl());

		i->frontend = &new_frontend;
		try
		{
			// Untested: guarantee that any exception thrown after the instance is in the list would remove the entry.
			new_frontend.initialized = bind(&frontend_manager::on_ready_for_ui, this, i, _1, _2);
			new_frontend.released = bind(&frontend_manager::on_frontend_released, this, i); // Must go the last.
		}
		catch (...)
		{
			_instances.erase(i);
			throw;
		}
		lock();
	}

	void frontend_manager::on_frontend_released(instance_container::iterator i) throw()
	{
		i->frontend = 0;
		if (!i->ui)
			_instances.erase(i);
		unlock();
	}

	void frontend_manager::on_ready_for_ui(instance_container::iterator i, const wstring &executable,
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
			lock();
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
			i->frontend->disconnect();
		else
			_instances.erase(i);
		unlock();
	}
}
