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

#include "commands-global.h"

#include "helpers.h"
#include "vcmodel.h"

#include <common/constants.h>
#include <common/formatting.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>
#include <frontend/about_ui.h>
#include <frontend/AttachToProcessDialog.h>
#include <frontend/file.h>
#include <frontend/frontend_manager.h>
#include <frontend/function_list.h>
#include <frontend/ipc_manager.h>
#include <frontend/persistence.h>
#include <logger/log.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>
#include <wpl/ui/win32/form.h>

#include <io.h>
#include <memory>

#define PREAMBLE "Command processors: "

#pragma warning(disable:4996)

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		namespace
		{
			const wstring c_profilerdir_macro = L"$(" + unicode(constants::profilerdir_ev) + L")";
			const wstring c_initializer_cpp_filename = L"micro-profiler.initializer.cpp";
			const wstring c_profiler_library_filename = L"micro-profiler_$(PlatformName).lib";
			const wstring c_profiler_library = c_profilerdir_macro & c_profiler_library_filename;
			const wstring c_profiler_library_quoted = L"\"" + c_profiler_library + L"\"";
			const wstring c_GH_option = L"/GH";
			const wstring c_Gh_option = L"/Gh";
			const wstring c_separator = L" ";
			const wstring c_inherit = L"%(AdditionalOptions)";

			string extension(const string &filename)
			{
				size_t pos = filename.find_last_of('.');

				return pos != string::npos ? filename.substr(pos) : string();
			}

			void trim_space(wstring &text)
			{
				size_t first = text.find_first_not_of(L' ');
				size_t last = text.find_last_not_of(L' ');
				text = first != wstring::npos ? text.substr(first, wstring::npos != last ? last - first + 1 : wstring::npos)
					: wstring();
			}

			vcmodel::file_ptr find_item_by_filename(const vcmodel::project &project, const wstring &filename)
			{
				vcmodel::file_ptr file;

				project.enum_files([&] (vcmodel::file_ptr f) {
					if (!file && wcsicmp((*f->unexpanded_relative_path()).c_str(), filename.c_str()) == 0)
						file = f;
				});
				return file;
			}

			bool has_instrumentation(vcmodel::compiler_tool &compiler)
			{
				wstring options = compiler.additional_options();

				return wstring::npos != options.find(c_GH_option) && wstring::npos != options.find(c_Gh_option);
			}

			template <typename LinkerT>
			bool has_library(LinkerT &linker)
			{
				wstring deps = linker.additional_dependencies();

				return wstring::npos != deps.find(c_profiler_library_filename);
			}

			struct instrument : vcmodel::tool::visitor
			{
				instrument(bool enable)
					: _enable(enable)
				{	}

				void visit(vcmodel::compiler_tool &compiler) const
				{
					bool changed = false;
					const wstring options_were = compiler.additional_options();
					wstring options = options_were;

					if (_enable)
					{
						if (wstring::npos == options.find(c_GH_option))
							options += L" " + c_GH_option, changed = true;
						if (wstring::npos == options.find(c_Gh_option))
							options += L" " + c_Gh_option, changed = true;
						if (wstring::npos == options.find(c_inherit))
							options += L" " + c_inherit, changed = true;
					}
					else
					{
						changed = replace(options, L" " + c_GH_option, wstring()) || changed;
						changed = replace(options, c_GH_option + L" ", wstring()) || changed;
						changed = replace(options, c_GH_option, wstring()) || changed;
						changed = replace(options, L" " + c_Gh_option, wstring()) || changed;
						changed = replace(options, c_Gh_option + L" ", wstring()) || changed;
						changed = replace(options, c_Gh_option, wstring()) || changed;
					}
					if (changed)
					{
						trim_space(options);
						compiler.additional_options(options);
					}
					LOG(PREAMBLE "instrumentation change attempt...")
						% A(_enable) % A(changed) % A(unicode(options_were)) % A(unicode(options));
				}

			private:
				bool _enable;
			};

			struct link_library : vcmodel::tool::visitor
			{
				link_library(bool enable)
					: _enable(enable)
				{	}

				virtual void visit(vcmodel::linker_tool &linker) const
				{	apply(linker, false);	}

				virtual void visit(vcmodel::librarian_tool &librarian) const
				{	apply(librarian, true);	}

				template <typename LinkerT>
				void apply(LinkerT &linker, bool is_static) const
				{
					bool changed = false;
					const wstring deps_were = linker.additional_dependencies();
					wstring deps = deps_were;

					if (_enable)
					{
						if (wstring::npos == deps.find(c_profiler_library_filename))
						{
							if (!deps.empty())
								deps += c_separator;
							deps += c_profiler_library_quoted;
							changed = true;
						}
					}
					else
					{
						changed = replace(deps, c_separator + c_profiler_library_quoted, wstring()) || changed;
						changed = replace(deps, c_profiler_library_quoted + c_separator, wstring()) || changed;
						changed = replace(deps, c_profiler_library_quoted, wstring()) || changed;
						changed = replace(deps, c_separator + c_profiler_library, wstring()) || changed;
						changed = replace(deps, c_profiler_library + c_separator, wstring()) || changed;
						changed = replace(deps, c_profiler_library, wstring()) || changed;
					}

					if (changed)
					{
						trim_space(deps);
						linker.additional_dependencies(deps);
					}
					LOG(PREAMBLE "dependencies change attempt...")
						% A(is_static) % A(_enable) % A(changed) % A(unicode(deps_were)) % A(unicode(deps));
				}

			private:
				bool _enable;
			};

			struct check_presence : vcmodel::tool::visitor
			{
				check_presence(pair<bool, bool>  &enabled) : _enabled(&enabled) {	}
				void visit(vcmodel::compiler_tool &t) const {	_enabled->second = has_instrumentation(t);	}
				void visit(vcmodel::linker_tool &t) const {	_enabled->first = has_library(t);	}
				void visit(vcmodel::librarian_tool &t) const {	_enabled->first = has_library(t);	}
				pair<bool /*library*/, bool /*instrumentation*/> *_enabled;
			};


			pair<bool /*library*/, bool /*instrumentation*/> is_enabled_detailed(vcmodel::project &project)
			{
				pair<bool, bool> enabled = make_pair(false, false);

				project.get_active_configuration()->enum_tools([&] (vcmodel::tool_ptr t) {
					t->visit(check_presence(enabled));
				});
				return enabled;
			}

			bool is_enabled(IDispatch *dte_project)
			{
				if (vcmodel::project_ptr project = vcmodel::create(dte_project))
				{
					pair<bool, bool> e = is_enabled_detailed(*project);
					return e.first && e.second;
				}
				return false;
			}
		}


		bool toggle_profiling::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = supported | enabled | visible;
			if (all_of(ctx.selected_items.begin(), ctx.selected_items.end(), &is_enabled))
				state |= checked;
			return true;
		}

		void toggle_profiling::exec(context_type &ctx, unsigned /*item*/)
		{
			bool add = !all_of(ctx.selected_items.begin(), ctx.selected_items.end(), &is_enabled);

			for_each(ctx.selected_items.begin(), ctx.selected_items.end(), [&] (IDispatchPtr dte_project) {
				if (vcmodel::project_ptr project = vcmodel::create(dte_project))
				{
					project->get_active_configuration()->enum_tools([&] (vcmodel::tool_ptr t) {
						t->visit(instrument(add));
						t->visit(link_library(add));
					});
				}
			});
		}


		bool remove_profiling_support::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			bool has_it = false;

			state = supported;
			for_each(ctx.selected_items.begin(), ctx.selected_items.end(), [&has_it, this] (IDispatchPtr dte_project) {
				if (vcmodel::project_ptr p = vcmodel::create(dte_project))
				{
					if (find_item_by_filename(*p, c_profiler_library_filename) || find_item_by_filename(*p, c_initializer_cpp_filename))
					{
						has_it = true;
					}
					else
					{
						pair<bool, bool> e = make_pair(false, false);

						p->enum_configurations([&] (vcmodel::configuration_ptr cfg) {
							cfg->enum_tools([&] (vcmodel::tool_ptr t) {
								t->visit(check_presence(e));
							});
						});
						has_it = e.first || e.second;
					}
				}
			});
			state |= has_it ? enabled | visible : 0;
			return true;
		}

		void remove_profiling_support::exec(context_type &ctx, unsigned /*item*/)
		{
			for_each(ctx.selected_items.begin(), ctx.selected_items.end(), [] (IDispatchPtr dte_project) {
				if (vcmodel::project_ptr project = vcmodel::create(dte_project))
				{
					// For compatibility - remove legacy settings.
					if (vcmodel::file_ptr f = find_item_by_filename(*project, c_profiler_library_filename))
						f->remove();
					if (vcmodel::file_ptr f = find_item_by_filename(*project, c_initializer_cpp_filename))
						f->remove();

					project->enum_configurations([] (vcmodel::configuration_ptr cfg) {
						cfg->enum_tools([] (vcmodel::tool_ptr t) {
							t->visit(instrument(false));
							t->visit(link_library(false));
						});
					});
				}
			});
		}


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
				const string ext = extension(*path);
				strmd::deserializer<read_stream, packer> dser(*s);
				shared_ptr<functions_list> model = !stricmp(ext.c_str(), ".mpstat3")
					? snapshot_load<scontext::file_v3>(dser) : snapshot_load<scontext::file_v4>(dser);

				ctx.frontend->load_session(*path, model);
			}
		}


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
					snapshot_save<scontext::file_v4>(ser, *model);
				}
			}
		}


		bool profile_process::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{
			state = 0;//visible | supported | (_dialog ? 0 : enabled);
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


		bool enable_remote_connections::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = visible | supported | enabled | (ctx.ipc_manager->remote_sockets_enabled() ? checked : 0);
			return true;
		}

		void enable_remote_connections::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.ipc_manager->enable_remote_sockets(!ctx.ipc_manager->remote_sockets_enabled());	}


		bool port_display::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = visible | supported, true;	}

		bool port_display::get_name(const context_type &ctx, unsigned /*item*/, wstring &name) const
		{
			name.clear();
			name += L"  TCP Port (autoconfigured): #";
			itoa<10>(name, ctx.ipc_manager->get_sockets_port());
			return true;
		}

		void port_display::exec(context_type &/*ctx*/, unsigned /*item*/)
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


		bool close_all::query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const
		{
			state = (ctx.frontend->instances_count() ? enabled : 0 ) | visible | supported;
			return true;
		}

		void close_all::exec(context_type &ctx, unsigned /*item*/)
		{	ctx.frontend->close_all();	}


		bool support_developer::query_state(const context_type &/*ctx*/, unsigned /*item*/, unsigned &state) const
		{	return state = enabled | visible | supported, true;	}

		void support_developer::exec(context_type &ctx, unsigned /*item*/)
		{
			shared_ptr< pair<shared_ptr<about_ui>, wpl::slot_connection> > o(
				new pair<shared_ptr<about_ui>, wpl::slot_connection>);
			HWND hshell = get_frame_hwnd(ctx.shell);
			shared_ptr<wpl::ui::form> form = wpl::ui::create_form(hshell);
			o->first.reset(new about_ui(form));
			global_context::running_objects_t &running_objects = ctx.running_objects;
			global_context::running_objects_t::iterator i = running_objects.insert(ctx.running_objects.end(), o);

			o->second = form->close += [i, &running_objects, hshell] {
				::EnableWindow(hshell, TRUE);
				running_objects.erase(i);
			};

			::EnableWindow(hshell, FALSE);
			form->set_visible(true);
		}
	}
}
