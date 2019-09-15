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

#include "commands-global.h"

#include "command-ids.h"
#include "helpers.h"

#include <common/constants.h>
#include <common/module.h>
#include <common/path.h>
#include <common/serialization.h>
#include <common/string.h>
#include <frontend/about_ui.h>
#include <frontend/AttachToProcessDialog.h>
#include <frontend/file.h>
#include <frontend/frontend_manager.h>
#include <frontend/function_list.h>
#include <frontend/ipc_manager.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>
#include <visualstudio/dispatch.h>
#include <wpl/ui/win32/form.h>

#include <io.h>
#include <memory>

#pragma warning(disable:4996)

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			const string c_profilerdir_macro = "$(" + string(c_profilerdir_ev) + ")";
			const string c_initializer_cpp = c_profilerdir_macro + "\\micro-profiler.initializer.cpp";
			const string c_profiler_library = c_profilerdir_macro + "\\micro-profiler_$(PlatformName).lib";
			const wstring c_GH_option = L"/GH";
			const wstring c_Gh_option = L"/Gh";

			bool replace(wstring &text, const wstring &what, const wstring &replacement)
			{
				size_t pos = text.find(what);

				if (pos != wstring::npos)
				{
					text.replace(pos, what.size(), replacement);
					return true;
				}
				return false;
			}

			dispatch find_item_by_relpath(dispatch project, const wstring &relative_unexpanded_path)
			{
				dispatch files = project.get(L"Object").get(L"Files");

				for (long i = 1, count = files.get(L"Count"); i <= count; ++i)
				{
					dispatch file = files[i];
					const wstring path = file.get(L"UnexpandedRelativePath");

					if (wcsicmp(path.c_str(), relative_unexpanded_path.c_str()) == 0)
						return file.get(L"Object");
				}
				return dispatch(IDispatchPtr());
			}

			template <typename FuncT>
			void for_each_configuration_tool(const dispatch &project, const wchar_t *tool_name, FuncT action)
			{
				dispatch configurations(project.get(L"Object").get(L"Configurations"));

				for (long i = 1, count = configurations.get(L"Count"); i <= count; ++i)
					action(configurations[i].get(L"Tools")[tool_name]);
			}

			dispatch get_tool(const dispatch &project, const wchar_t *tool_name)
			{
				dispatch vcproject(project.get(L"Object"));
				wstring activeConfigurationName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"ConfigurationName"));
				wstring activePlatformName(project.get(L"ConfigurationManager").get(L"ActiveConfiguration").get(L"PlatformName"));
				dispatch configuration(vcproject.get(L"Configurations")[(activeConfigurationName + L"|" + activePlatformName).c_str()]);

				return configuration.get(L"Tools")[tool_name];
			}

			void disable_pch(const dispatch &item)
			{
				dispatch configurations(item.get(L"Object").get(L"FileConfigurations"));

				for (long i = 1, count = configurations.get(L"Count"); i <= count; ++i)
					configurations[i].get(L"Tool").put(L"UsePrecompiledHeader", 0 /*pchNone*/);
			}

			bool has_instrumentation(const dispatch &compiler)
			{
				wstring additionalOptions = compiler.get(L"AdditionalOptions");

				return wstring::npos != additionalOptions.find(c_GH_option) && wstring::npos != additionalOptions.find(c_Gh_option);
			}

			void enable_instrumentation(const dispatch &compiler)
			{
				bool changed = false;
				wstring additionalOptions = compiler.get(L"AdditionalOptions");

				if (-1 == additionalOptions.find(c_GH_option))
					additionalOptions += L" " + c_GH_option, changed = true;
				if (-1 == additionalOptions.find(c_Gh_option))
					additionalOptions += L" " + c_Gh_option, changed = true;
				if (changed)
				{
//					additionalOptions.Trim();
					compiler.put(L"AdditionalOptions", additionalOptions.c_str());
				}
			}

			void disable_instrumentation(dispatch compiler)
			{
				bool changed = false;
				wstring additionalOptions = compiler.get(L"AdditionalOptions");

				changed = replace(additionalOptions, L" " + c_GH_option, L"") || changed;
				changed = replace(additionalOptions, c_GH_option + L" ", L"") || changed;
				changed = replace(additionalOptions, c_GH_option, L"") || changed;
				changed = replace(additionalOptions, L" " + c_Gh_option, L"") || changed;
				changed = replace(additionalOptions, c_Gh_option + L" ", L"") || changed;
				changed = replace(additionalOptions, c_Gh_option, L"") || changed;
				if (changed)
				{
//					additionalOptions.Trim();
					compiler.put(L"AdditionalOptions", !additionalOptions.empty() ? additionalOptions.c_str() : NULL);
				}
			}
		}


		toggle_profiling::toggle_profiling()
			: global_command(cmdidToggleProfiling)
		{	}

		bool toggle_profiling::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = 0;
			if (IDispatchPtr tool = get_tool(ctx.project, L"VCCLCompilerTool"))
			{
				state = supported | enabled | visible | (tool && has_instrumentation(dispatch(tool))
					&& IDispatchPtr(find_item_by_relpath(ctx.project, unicode(c_initializer_cpp)))
					&& IDispatchPtr(find_item_by_relpath(ctx.project, unicode(c_profiler_library)))
					? checked : 0);
			}
			return true;
		}

		void toggle_profiling::exec(context_type &ctx, unsigned /*item*/)
		{
			dispatch compiler(get_tool(ctx.project, L"VCCLCompilerTool"));
			IDispatchPtr initializer_item = find_item_by_relpath(ctx.project, unicode(c_initializer_cpp));
			IDispatchPtr library_item = find_item_by_relpath(ctx.project, unicode(c_profiler_library));
			const bool has_profiling = has_instrumentation(compiler) && initializer_item && library_item;

			if (!has_profiling)
			{
				if (!initializer_item)
					disable_pch(ctx.project.get(L"ProjectItems")(L"AddFromFile", c_initializer_cpp.c_str()));
				if (!library_item)
					ctx.project.get(L"ProjectItems")(L"AddFromFile", c_profiler_library.c_str());
				enable_instrumentation(compiler);
			}
			else
				disable_instrumentation(compiler);
		}


		remove_profiling_support::remove_profiling_support()
			: global_command(cmdidRemoveProfilingSupport)
		{	}

		bool remove_profiling_support::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = 0;
			if (IDispatchPtr tool = get_tool(ctx.project, L"VCCLCompilerTool"))
				state = supported | (IDispatchPtr(find_item_by_relpath(ctx.project, unicode(c_initializer_cpp))) ? enabled | visible : 0);
			return true;
		}

		void remove_profiling_support::exec(context_type &ctx, unsigned /*item*/)
		{
			dispatch initializer = find_item_by_relpath(ctx.project, unicode(c_initializer_cpp));
			dispatch library = find_item_by_relpath(ctx.project, unicode(c_profiler_library));

			for_each_configuration_tool(ctx.project, L"VCCLCompilerTool", &disable_instrumentation);
			if (IDispatchPtr(initializer))
				initializer(L"Remove");
			if (IDispatchPtr(library))
				library(L"Remove");
		}


		open_statistics::open_statistics()
			: global_command(cmdidLoadStatistics)
		{	}

		bool open_statistics::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{
			state = enabled | visible | supported;
			return true;
		}

		void open_statistics::exec(context_type &ctx, unsigned /*item*/)
		{
			string path;
			auto_ptr<read_stream> s = open_file(get_frame_hwnd(ctx.shell), path);

			if (s.get())
			{
				strmd::deserializer<read_stream, packer> dser(*s);
				shared_ptr<functions_list> model = functions_list::load(dser);

				ctx.frontend->load_session(*path, model);
			}
		}

		save_statistics::save_statistics()
			: global_command(cmdidSaveStatistics)
		{	}

		bool save_statistics::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = visible | supported | (ctx.frontend->get_active() ? enabled : 0);
			return true;
		}

		bool save_statistics::get_name(const context_type &ctx, unsigned /*item*/, wstring &name) const
		{
			if (const frontend_manager::instance *i = ctx.frontend->get_active())
				return name = L"Save " + unicode(*i->executable) + L" Statistics As...", true;
			return false;
		}

		void save_statistics::exec(context_type &ctx, unsigned /*item*/)
		{
			if (const frontend_manager::instance *i = ctx.frontend->get_active())
			{
				shared_ptr<functions_list> model = i->model;
				auto_ptr<write_stream> s = create_file(get_frame_hwnd(ctx.shell), i->executable);

				if (s.get())
				{
					strmd::serializer<write_stream, packer> ser(*s);
					model->save(ser);
				}
			}
		}


		profile_process::profile_process()
			: global_command(cmdidProfileProcess)
		{	}

		bool profile_process::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{
			state = visible | supported | (_dialog ? 0 : enabled);
			return true;
		}

		void profile_process::exec(context_type &ctx, unsigned /*item*/)
		{
			shared_ptr<wpl::ui::form> form(wpl::ui::create_form(get_frame_hwnd(ctx.shell)));
			_dialog.reset(new AttachToProcessDialog(form));
			_closed_connection = _dialog->closed += [this] {
				_dialog.reset();
			};
		}


		enable_remote_connections::enable_remote_connections()
			: global_command(cmdidIPCEnableRemote)
		{	}

		bool enable_remote_connections::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = visible | supported | enabled | (ctx.ipc_manager->remote_sockets_enabled() ? checked : 0);
			return true;
		}

		void enable_remote_connections::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.ipc_manager->enable_remote_sockets(!ctx.ipc_manager->remote_sockets_enabled());	}


		port_display::port_display()
			: global_command(cmdidIPCSocketPort)
		{	}

		bool port_display::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = visible | supported, true;	}

		bool port_display::get_name(const context_type &ctx, unsigned /*item*/, wstring &name) const
		{
			wchar_t buffer[100] = { 0 };

			swprintf(buffer, sizeof buffer - 1, L"  TCP Port (autoconfigured): #%d", ctx.ipc_manager->get_sockets_port());
			name = buffer;
			return true;
		}

		void port_display::exec(context_type &/*ctx*/, unsigned /*item*/)
		{	}


		window_activate::window_activate()
			: global_command(cmdidWindowActivateDynamic, true)
		{	}

		bool window_activate::query_state(const context_type &ctx, unsigned item, unsigned &state) const
		{
			if (item >= ctx.frontend->instances_count())
				return false;
			if (const frontend_manager::instance *i = ctx.frontend->get_instance(item))
				state = i->ui ? enabled | visible | supported | (ctx.frontend->get_active() == i ? checked : 0): 0;
			return true;
		}

		bool window_activate::get_name(const context_type &ctx, unsigned item, wstring &name) const
		{
			const frontend_manager::instance *i = ctx.frontend->get_instance(item);
			return i ? name = unicode(*i->executable), true : false;
		}

		void window_activate::exec(context_type &ctx, unsigned item)
		{
			if (const frontend_manager::instance *i = ctx.frontend->get_instance(item))
				i->ui->activate();
		}


		close_all::close_all()
			: global_command(cmdidCloseAll)
		{	}

		bool close_all::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = (ctx.frontend->instances_count() ? enabled : 0 ) | visible | supported;
			return true;
		}

		void close_all::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.frontend->close_all();	}


		support_developer::support_developer()
			: global_command(cmdidSupportDeveloper)
		{	}

		bool support_developer::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = enabled | visible | supported, true;	}

		void support_developer::exec(context_type &ctx, unsigned /*item*/)
		{
			SupportDevDialog dlg;

			dlg.DoModal(get_frame_hwnd(ctx.shell));
		}
	}
}
