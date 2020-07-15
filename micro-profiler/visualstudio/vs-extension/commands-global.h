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

#pragma once

#include "command-ids.h"

#include <visualstudio/command.h>

#include <atlbase.h>
#include <comdef.h>
#include <list>
#include <vsshell.h>
#include <wpl/signals.h>

#pragma warning(disable:4510; disable:4610)

namespace micro_profiler
{
	class AttachToProcessDialog;
	class frontend_manager;
	class ipc_manager;

	namespace integration
	{
		struct global_context
		{
			typedef std::list< std::shared_ptr<void> > running_objects_t;

			std::vector<IDispatchPtr> selected_items;
			std::shared_ptr<frontend_manager> frontend;
			CComPtr<IVsUIShell> shell;
			std::shared_ptr<ipc_manager> ipc_manager;
			running_objects_t &running_objects;

		private:
			const global_context &operator =(const global_context &rhs);
		};


		struct toggle_profiling : command_defined<global_context, cmdidToggleProfiling>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct remove_profiling_support : command_defined<global_context, cmdidRemoveProfilingSupport>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		struct open_statistics : command_defined<global_context, cmdidLoadStatistics>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		struct save_statistics : command_defined<global_context, cmdidSaveStatistics>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual bool get_name(const context_type &ctx, unsigned item, std::wstring &name) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		class profile_process : public command_defined<global_context, cmdidProfileProcess>
		{
		public:
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);

		private:
			std::shared_ptr<AttachToProcessDialog> _dialog;
			wpl::slot_connection _closed_connection;
		};


		struct enable_remote_connections : command_defined<global_context, cmdidIPCEnableRemote>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		struct port_display : command_defined<global_context, cmdidIPCSocketPort>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual bool get_name(const context_type &ctx, unsigned item, std::wstring &name) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		struct window_activate : command_defined<global_context, cmdidWindowActivateDynamic, true>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual bool get_name(const context_type &ctx, unsigned item, std::wstring &name) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct close_all : command_defined<global_context, cmdidCloseAll>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct support_developer : command_defined<global_context, cmdidSupportDeveloper>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};
	}
}
