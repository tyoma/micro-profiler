#pragma once

#include "frontend_manager.h"

#include <atlbase.h>
#include <atlcom.h>
#include <unordered_map>
#include <wpl/base/signals.h>

namespace micro_profiler
{
	class Frontend;
	class functions_list;

	class frontend_manager_impl : public frontend_manager, public CComClassFactory
	{
	public:
		frontend_manager_impl();
		~frontend_manager_impl();

		void set_ui_factory(const frontend_ui_factory &ui_factory);

		void terminate();

		virtual size_t instances_count() const;
		virtual std::shared_ptr<const instance> get_instance(unsigned index) const;
		virtual void load_instance(const instance &data);

		STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **object);

	private:
		struct instance_data
		{
			Frontend *frontend;
			std::shared_ptr<frontend_ui> ui;
			wpl::slot_connection ui_closed_connection;
		};

		typedef std::list<instance_data> instance_container;

	private:
		void on_frontend_released(instance_container::iterator i);
		void on_ready_for_ui(instance_container::iterator i, const std::wstring &process_name, const std::shared_ptr<functions_list> &model);
		void on_ui_closed(instance_container::iterator i);

		void add_external_reference();
		void release_external_reference();

		static std::shared_ptr<frontend_ui> default_ui_factory(const std::shared_ptr<functions_list> &model,
			const std::wstring &process_name);

	private:
		instance_container _instances;
		frontend_ui_factory _ui_factory;
		unsigned _external_references;
		DWORD _external_lock_id;
	};
}
