//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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


	frontend_manager::frontend_manager(const frontend_ui_factory &ui_factory, shared_ptr<scheduler::queue> queue)
		: _ui_factory(ui_factory), _queue(queue), _instances(new instance_container),
			_active_instance(new const instance_impl *())
	{	LOG(PREAMBLE "constructed.");	}

	frontend_manager::~frontend_manager()
	{
		for (auto i = _instances->begin(); i != _instances->end(); )
			reset_entry(*_instances, i++);
		LOG(PREAMBLE "destroyed.");
	}

	void frontend_manager::close_all() throw()
	{
		for (auto i = _instances->begin(); i != _instances->end(); )
			reset_entry(*_instances, i++);
		*_active_instance = nullptr;
		LOG(PREAMBLE "all instances closed.");
	}

	size_t frontend_manager::instances_count() const throw()
	{	return _instances->size();	}

	const frontend_manager::instance *frontend_manager::get_instance(unsigned index) const throw()
	{
		auto i = _instances->begin();

		if (index >= _instances->size())
			return nullptr;
		return advance(i, index), &*i;
	}

	const frontend_manager::instance *frontend_manager::get_active() const throw()
	{	return *_active_instance;	}

	void frontend_manager::load_session(const string &executable, const shared_ptr<functions_list> &model)
	{	on_ready_for_ui(_instances->insert(_instances->end(), instance_impl(nullptr)), executable, model);	}

	shared_ptr<ipc::channel> frontend_manager::create_session(ipc::channel &outbound)
	{
		unique_ptr<frontend> uf(new frontend(outbound, _queue));
		const auto instances = _instances;
		const auto active_instance = _active_instance;
		const auto i = _instances->insert(_instances->end(), instance_impl(uf.get()));
		shared_ptr<frontend> f(uf.release(), [instances, active_instance, i] (frontend *p) {
			if (*active_instance == &*i)
				*active_instance = nullptr;
			delete p;
			i->frontend = nullptr;
			if (!i->ui)
			{
				instances->erase(i);
				LOG(PREAMBLE "no UI controller - instance removed.") % A(instances->size());
			}
		});

		f->initialized = bind(&frontend_manager::on_ready_for_ui, this, i, _1, _2);
		return f;
	}

	void frontend_manager::on_ready_for_ui(instance_container::iterator i, const string &executable,
		const shared_ptr<functions_list> &model)
	{
		i->executable = executable;
		i->model = model;
		if (const auto ui = _ui_factory(model, executable))
		{
			i->ui_activated_connection = ui->activated += bind(&frontend_manager::on_ui_activated, this, i);
			i->ui_closed_connection = ui->closed += bind(&frontend_manager::on_ui_closed, this, i);
			i->ui = ui;
			*_active_instance = &*i;
		}
	}

	void frontend_manager::on_ui_activated(instance_container::iterator i)
	{	*_active_instance = &*i;	}

	void frontend_manager::on_ui_closed(instance_container::iterator i) throw()
	{
		if (*_active_instance == &*i)
			*_active_instance = nullptr;
		reset_entry(*_instances, i);
	}

	void frontend_manager::reset_entry(instance_container &instances, instance_container::iterator i)
	{
		i->ui_activated_connection.reset();
		i->ui_closed_connection.reset();
		i->ui.reset();
		if (i->frontend)
			i->frontend->disconnect_session();
		else
			instances.erase(i);
	}
}
