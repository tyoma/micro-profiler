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

#include <frontend/frontend_ui.h>
#include <ipc/client_session.h>

using namespace std;

#define PREAMBLE "Frontend manager: "

namespace micro_profiler
{
	frontend_manager::instance_impl::instance_impl(ipc::client_session *frontend_)
		: frontend(frontend_)
	{	}


	frontend_manager::~frontend_manager()
	{
		for (auto i = _instances->begin(); i != _instances->end(); )
			reset_entry(*_instances, i++);
		LOG(PREAMBLE "destroyed.") % A(this);
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

	ipc::channel_ptr_t frontend_manager::create_session(ipc::channel &outbound)
	{	return _frontend_factory(outbound);	}

	pair<ipc::channel_ptr_t, frontend_manager::instance_container::iterator> frontend_manager::attach(ipc::client_session *pfrontend)
	{
		unique_ptr<ipc::client_session> uf(pfrontend);
		const auto instances = _instances;
		const auto active_instance = _active_instance;
		const auto i = _instances->insert(_instances->end(), instance_impl(uf.get()));

		return make_pair(shared_ptr<ipc::client_session>(uf.release(),  [instances, active_instance, i] (ipc::client_session *p) {
			if (*active_instance == &*i)
				*active_instance = nullptr;
			delete p;
			i->frontend = nullptr;
			if (!i->ui)
			{
				instances->erase(i);
				LOG(PREAMBLE "no UI controller - instance removed.") % A(instances->size());
			}
		}), i);
	}

	void frontend_manager::set_ui(instance_container::iterator i, shared_ptr<frontend_ui> ui)
	{
		i->ui_activated_connection = ui->activated += [this, i] {	on_ui_activated(i);	};
		i->ui_closed_connection = ui->closed += [this, i] {	on_ui_closed(i);	};
		i->ui = ui;
		*_active_instance = &*i;
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
