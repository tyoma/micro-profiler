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

#pragma once

#include <common/argument_traits.h>
#include <common/noncopyable.h>
#include <common/types.h>
#include <ipc/endpoint.h>
#include <list>
#include <logger/log.h>
#include <type_traits>
#include <wpl/signal.h>

#define PREAMBLE "Frontend manager: "

namespace micro_profiler
{
	struct frontend_ui;

	namespace ipc
	{
		class client_session;
	}

	class frontend_manager : public ipc::server, noncopyable
	{
	public:
		struct instance
		{
			std::string title;
			std::shared_ptr<frontend_ui> ui;
		};

	public:
		template <typename FrontendFactoryT, typename FrontendUIFactoryT>
		frontend_manager(FrontendFactoryT frontend_factory, FrontendUIFactoryT ui_factory);
		~frontend_manager();

		void close_all() throw();

		size_t instances_count() const throw();
		const instance *get_instance(unsigned index) const throw();
		const instance *get_active() const throw();

		template <typename ContextT>
		void load_session(const ContextT &ui_context);

		// ipc::server methods
		virtual ipc::channel_ptr_t create_session(ipc::channel &outbound) override;

	private:
		struct instance_impl : instance
		{
			instance_impl(ipc::client_session *frontend_);

			ipc::client_session *frontend;
			wpl::slot_connection ui_activated_connection;
			wpl::slot_connection ui_closed_connection;
		};

		typedef std::list<instance_impl> instance_container;

	private:
		std::pair<ipc::channel_ptr_t, instance_container::iterator> attach(ipc::client_session *session);
		void on_frontend_released(instance_container::iterator i) throw();
		void set_ui(instance_container::iterator i, std::shared_ptr<frontend_ui> ui);

		void on_ui_activated(instance_container::iterator i);
		void on_ui_closed(instance_container::iterator i) throw();

		static void reset_entry(instance_container &instances, instance_container::iterator i);

	private:
		const std::shared_ptr<instance_container> _instances;
		const std::shared_ptr<const instance_impl *> _active_instance;
		std::function<ipc::channel_ptr_t (ipc::channel &outbound)> _frontend_factory;
		std::function<void (const void *)> _load_session;
	};



	template <typename FrontendFactoryT, typename FrontendUIFactoryT>
	inline frontend_manager::frontend_manager(FrontendFactoryT frontend_factory, FrontendUIFactoryT ui_factory)
		: _instances(new instance_container), _active_instance(new const instance_impl *())
	{
		typedef typename argument_traits<FrontendFactoryT>::types prototype_types;
		typedef typename std::remove_pointer<typename prototype_types::return_type>::type frontend_type;
		typedef typename frontend_type::session_type session_type;

		const auto prepare_ui = [this, ui_factory] (instance_container::iterator i, const session_type &context) {
			i->title = context.get_title();
			if (auto ui = ui_factory(context))
				set_ui(i, ui);
		};

		_frontend_factory = [this, frontend_factory, prepare_ui] (ipc::channel &o) -> ipc::channel_ptr_t {
			const auto prepare_ui_ = prepare_ui;
			const auto frontend = frontend_factory(o);
			const auto channel_instance = attach(frontend);
			const auto i = channel_instance.second;

			frontend->initialized = [prepare_ui_, i] (const session_type &context) {	prepare_ui_(i, context);	};
			return channel_instance.first;
		};

		_load_session = [this, prepare_ui] (const void *p) {
			prepare_ui(_instances->insert(_instances->end(), frontend_manager::instance_impl(nullptr)),
				*static_cast<const session_type *>(p)); // TODO: rather implement load directly from here.
		};

		LOG(PREAMBLE "constructed.") % A(this);
	}

	template <typename ContextT>
	inline void frontend_manager::load_session(const ContextT &ui_context)
	{	_load_session(&ui_context);	}
}

#undef PREAMBLE
