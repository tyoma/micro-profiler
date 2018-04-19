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

#include <visualstudio/command.h>
#include <visualstudio/dispatch.h>

#include <atlbase.h>
#include <vsshell.h>

namespace micro_profiler
{
	class frontend_manager;

	namespace integration
	{
		struct global_context
		{
			global_context(const dispatch &project, const std::shared_ptr<frontend_manager> &frontend,
				const CComPtr<IVsUIShell> &shell);

			dispatch project;
			
			std::shared_ptr<frontend_manager> frontend;
			CComPtr<IVsUIShell> shell;
		};

		typedef command<global_context> global_command;

		struct toggle_profiling : global_command
		{
			toggle_profiling();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct remove_profiling_support : global_command
		{
			remove_profiling_support();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		struct open_statistics : global_command
		{
			open_statistics();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};


		struct save_statistics : global_command
		{
			save_statistics();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual bool get_name(const context_type &ctx, unsigned item, std::wstring &name) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct window_activate : global_command
		{
			window_activate();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual bool get_name(const context_type &ctx, unsigned item, std::wstring &name) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct close_all : global_command
		{
			close_all();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct support_developer : global_command
		{
			support_developer();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};



		inline global_context::global_context(const dispatch &project_,
				const std::shared_ptr<frontend_manager> &frontend_, const CComPtr<IVsUIShell> &shell_)
			: project(project_), frontend(frontend_), shell(shell_)
		{	}
	}
}
