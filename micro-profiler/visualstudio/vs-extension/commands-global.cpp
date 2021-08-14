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

#include "vs-package.h"

#include "command-ids.h"
#include "helpers.h"
#include "vcmodel.h"

#include <common/constants.h>
#include <common/formatting.h>
#include <common/module.h>
#include <common/path.h>
#include <common/string.h>
#include <frontend/about_ui.h>
#include <frontend/attach_ui.h>
#include <frontend/file.h>
#include <frontend/frontend_manager.h>
#include <frontend/function_list.h>
#include <frontend/ipc_manager.h>
#include <frontend/persistence.h>
#include <logger/log.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>
#include <wpl/form.h>
#include <wpl/vs/factory.h>

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

		void profiler_package::init_menu()
		{
			add_command(cmdidToggleProfiling, [this] (unsigned) {
				const auto selected_items = get_selected_items();
				bool add = !all_of(selected_items.begin(), selected_items.end(), &is_enabled);

				for_each(selected_items.begin(), selected_items.end(), [&] (IDispatchPtr dte_project) {
					if (vcmodel::project_ptr project = vcmodel::create(dte_project))
					{
						project->get_active_configuration()->enum_tools([&] (vcmodel::tool_ptr t) {
							t->visit(instrument(add));
							t->visit(link_library(add));
						});
					}
				});
			}, false, [this] (unsigned, unsigned &state) -> bool {
				const auto selected_items = get_selected_items();
				state = supported | enabled | visible;
				if (all_of(selected_items.begin(), selected_items.end(), &is_enabled))
					state |= checked;
				return true;
			});


			add_command(cmdidRemoveProfilingSupport, [this] (unsigned) {
				const auto selected_items = get_selected_items();

				for_each(selected_items.begin(), selected_items.end(), [] (IDispatchPtr dte_project) {
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
			}, false, [this] (unsigned, unsigned &state) -> bool {
				const auto selected_items = get_selected_items();
				bool has_it = false;

				state = supported;
				for_each(selected_items.begin(), selected_items.end(), [&has_it] (IDispatchPtr dte_project) {
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
			});


			add_command(cmdidLoadStatistics, [this] (unsigned) {
				string path;
				auto_ptr<read_stream> s = open_file(get_frame_hwnd(get_shell()), path);

				if (s.get())
				{
					const string ext = extension(*path);
					strmd::deserializer<read_stream, packer> dser(*s);
					frontend_ui_context ui_context = {
						{	*path, 1u	},
						!stricmp(ext.c_str(), ".mpstat3")
							? snapshot_load<scontext::file_v3>(dser) : snapshot_load<scontext::file_v4>(dser)
					};

					_frontend_manager->load_session(ui_context);
				}
			}, false, [this] (unsigned, unsigned &state) -> bool {
				return state = enabled | visible | supported, true;
			});


			add_command(cmdidSaveStatistics, [this] (unsigned) {
				if (const frontend_manager::instance *i = _frontend_manager->get_active())
				{
					shared_ptr<functions_list> model = i->model;
					auto_ptr<write_stream> s = create_file(get_frame_hwnd(get_shell()), i->process_info.executable);

					if (s.get())
					{
						strmd::serializer<write_stream, packer> ser(*s);
						snapshot_save<scontext::file_v4>(ser, *model);
					}
				}
			}, false, [this] (unsigned, unsigned &state) -> bool {
				return state = visible | supported | (_frontend_manager->get_active() ? enabled : 0), true;
			}, [this] (unsigned /**/, string &caption) -> bool {
				if (_frontend_manager->instances_count())
					if (const frontend_manager::instance *i = _frontend_manager->get_active())
						return caption = "Save " + i->process_info.executable + " Statistics As...", true;
				return false;
			});


			add_command(cmdidProfileProcess, [this] (unsigned) {
				wpl::rect_i l = { 0, 0, 400, 300 }; // TODO: Center attach form.
				const auto o = make_shared< pair< shared_ptr<wpl::form>, vector<wpl::slot_connection> > >();
				auto &running_objects = _running_objects;
				const auto i = running_objects.insert(running_objects.end(), o);
				const auto onclose = [i, &running_objects] {
					running_objects.erase(i);
				};
				const auto &f = get_factory();
				const auto root = make_shared<wpl::overlay>();
					root->add(f.create_control<wpl::control>("background"));
					const auto attach = make_shared<attach_ui>(f, f.context.queue_);
					root->add(wpl::pad_control(attach, 5, 5));

				o->first = f.create_modal();
				o->second.push_back(o->first->close += onclose);
				o->second.push_back(attach->close += onclose);

				o->first->set_root(root);
				o->first->set_location(l);
				o->first->set_visible(true);
			}, false, [this] (unsigned, unsigned &state) -> bool {
				return state = visible | supported | enabled, true;
			});


			add_command(cmdidIPCEnableRemote, [this] (unsigned) {
				_ipc_manager->enable_remote_sockets(!_ipc_manager->remote_sockets_enabled());
			}, false, [this] (unsigned, unsigned &state) {
				return state = visible | supported | enabled | (_ipc_manager->remote_sockets_enabled() ? checked : 0), true;
			});


			add_command(cmdidIPCSocketPort, [] (unsigned) { }, false, [this] (unsigned, unsigned &state) {
				return state = visible | supported, true;
			}, [this] (unsigned, string &caption) ->bool {
				caption.clear();
				caption += "  TCP Port (autoconfigured): #";
				itoa<10>(caption, _ipc_manager->get_sockets_port());
				return true;
			});


			add_command(cmdidWindowActivateDynamic, [this] (unsigned item) {
				if (const frontend_manager::instance *i = _frontend_manager->get_instance(item))
					i->ui->activate();
			}, true, [this] (unsigned item, unsigned &state) ->bool {
				if (item >= _frontend_manager->instances_count())
					return false;
				if (const frontend_manager::instance *i = _frontend_manager->get_instance(item))
					state = i->ui ? enabled | visible | supported | (_frontend_manager->get_active() == i ? checked : 0) : visible | supported;
				return true;
			}, [this] (unsigned item, string &caption) -> bool {
				const frontend_manager::instance *i = _frontend_manager->get_instance(item);
				return i ? caption = *i->process_info.executable, true : false;
			});


			add_command(cmdidCloseAll, [this] (unsigned) {
				_frontend_manager->close_all();
			}, false, [this] (unsigned, unsigned &state) -> bool {
				return state = (_frontend_manager->instances_count() ? enabled : 0 ) | visible | supported, true;
			});


			add_command(cmdidSupportDeveloper, [this] (unsigned) {
				wpl::rect_i l = { 0, 0, 400, 300 }; // TODO: Center about form.
				const auto o = make_shared< pair< shared_ptr<wpl::form>, vector<wpl::slot_connection> > >();
				auto &running_objects = _running_objects;
				const auto i = running_objects.insert(running_objects.end(), o);
				const auto onclose = [i, &running_objects/*, hshell*/] {
					running_objects.erase(i);
				};
				const auto &f = get_factory();
				const auto root = make_shared<wpl::overlay>();
					root->add(f.create_control<wpl::control>("background"));
					const auto about = make_shared<about_ui>(f);
					root->add(wpl::pad_control(about, 5, 5));

				o->first = get_factory().create_modal();
				o->second.push_back(o->first->close += onclose);
				o->second.push_back(about->link += [] (const string &address) {
					::ShellExecuteW(NULL, L"open", unicode(address).c_str(), NULL, NULL, SW_SHOWNORMAL);
				});
				o->second.push_back(about->close += onclose);

				o->first->set_root(root);
				o->first->set_location(l);
				o->first->set_visible(true);
			}, false, [this] (unsigned, unsigned &state) -> bool {
				return state = enabled | visible | supported, true;
			});
		}
	}
}
