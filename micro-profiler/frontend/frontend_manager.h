#pragma once

#include <common/types.h>

#include <memory>
#include <string>
#include <wpl/base/signals.h>

namespace micro_profiler
{
	class functions_list;

	struct frontend_ui
	{
		typedef std::shared_ptr<frontend_ui> ptr;

		virtual ~frontend_ui() { }

		virtual void activate() = 0;
		wpl::signal<void()> closed;
	};

	struct frontend_manager
	{
	public:
		struct instance
		{
		};

		typedef std::function<frontend_ui::ptr(const std::shared_ptr<functions_list> &model,
			const std::wstring &process_name)> frontend_ui_factory;
		typedef std::shared_ptr<frontend_manager> ptr;

	public:
		static std::shared_ptr<frontend_manager> create(const guid_t &id, const frontend_ui_factory &ui_factory);

		virtual size_t instances_count() const = 0;
		virtual std::shared_ptr<const instance> get_instance(unsigned index) const = 0;
		virtual void load_instance(const instance &data) = 0; 

	protected:
		frontend_manager() { }
		~frontend_manager() { }
	};

}
