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

#include <atlbase.h>
#include <comdef.h>
#include <dte.h>
#include <list>
#include <scheduler/scheduler.h>
#include <wpl/vs/ole-command-target.h>
#include <wpl/vs/package.h>

namespace micro_profiler
{
	class frontend_manager;
	struct hive;
	class ipc_manager;

	namespace integration
	{
		extern const GUID c_guidMicroProfilerPkg;

		class profiler_package : public wpl::vs::package,
			public CComCoClass<profiler_package, &c_guidMicroProfilerPkg>,
			public wpl::vs::ole_command_target
		{
		public:
			profiler_package();
			~profiler_package();

		public:
			DECLARE_NO_REGISTRY()
			DECLARE_PROTECT_FINAL_CONSTRUCT()

			BEGIN_COM_MAP(profiler_package)
				COM_INTERFACE_ENTRY(IOleCommandTarget)
				COM_INTERFACE_ENTRY_CHAIN(wpl::vs::package)
			END_COM_MAP()

		private:
			typedef std::list< std::shared_ptr<void> > running_objects_t;

		private:
			virtual wpl::clock get_clock() const override;
			virtual wpl::queue initialize_queue() override;
			virtual std::shared_ptr<wpl::stylesheet> create_stylesheet(wpl::signal<void ()> &update,
				wpl::gcontext::text_engine_type &text_engine, IVsUIShell &shell,
				IVsFontAndColorStorage &font_and_color) const override;
			virtual void initialize(wpl::vs::factory &factory) override;
			virtual void terminate() throw() override;

			void init_menu();
			std::vector<IDispatchPtr> get_selected_items() const;
			void on_open_source(const std::string &file, unsigned line);

		private:
			std::function<mt::milliseconds ()> _clock;
			std::shared_ptr<scheduler::queue> _ui_queue;
			CComPtr<_DTE> _dte;
			std::shared_ptr<hive> _configuration;
			std::shared_ptr<frontend_manager> _frontend_manager;
			std::unique_ptr<ipc_manager> _ipc_manager;
			running_objects_t _running_objects;
		};
	}
}
