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

		void set_ui_factory(const frontend_ui_factory &ui_factory);

		void close_all() throw();

		virtual size_t instances_count() const throw();
		virtual const instance *get_instance(unsigned index) const throw();
		virtual void load_instance(const instance &data);

		STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **object);

	private:
		struct instance_impl : instance
		{
			Frontend *frontend;
			wpl::slot_connection ui_closed_connection;
		};

		typedef std::list<instance_impl> instance_container;

	private:
		void on_frontend_released(instance_container::iterator i);
		void on_ready_for_ui(instance_container::iterator i, const std::wstring &executable, const std::shared_ptr<functions_list> &model);
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
