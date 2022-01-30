#include "command-ids.h"
#include "helpers.h"

#include <common/path.h>
#include <common/string.h>
#include <frontend/file.h>
#include <frontend/frontend_ui.h>
#include <frontend/function_list.h>
#include <frontend/image_patch_model.h>
#include <frontend/image_patch_ui.h>
#include <frontend/persistence.h>
#include <frontend/statistics_poll.h>
#include <frontend/symbol_resolver.h>
#include <strmd/serializer.h>
#include <windows.h>
#include <wpl/form.h>
#include <wpl/layout.h>
#include <wpl/vs/command-target.h>
#include <wpl/vs/factory.h>

using namespace std;
using namespace wpl::vs;

namespace micro_profiler
{
	namespace integration
	{
		void init_instance_menu(list< shared_ptr<void> > &running_objects, const wpl::vs::factory &factory,
			command_target &target, const profiling_session &session, shared_ptr<scheduler::queue> queue)
		{
			const auto statistics = session.statistics;
			const auto model = make_shared<functions_list>(statistics, 1.0 / session.process_info.ticks_per_second,
				make_shared<symbol_resolver>(session.modules, session.module_mappings), session.threads);
			const auto injected = !!session.process_info.injected;
			const auto executable = session.process_info.executable;
			const auto poller = make_shared<statistics_poll>(statistics, *queue);

			poller->enable(true);

			target.add_command(cmdidPauseUpdates, [poller] (unsigned) {
				poller->enable(false);
			}, false, [poller] (unsigned, unsigned &state) {
				return state = (poller->enabled() ? command_target::enabled : 0) | command_target::supported | command_target::visible, true;
			});

			target.add_command(cmdidResumeUpdates, [poller] (unsigned) {
				poller->enable(true);
			}, false, [poller] (unsigned, unsigned &state) {
				return state = (poller->enabled() ? 0 : command_target::enabled) | command_target::supported | command_target::visible, true;
			});

			target.add_command(cmdidSaveStatistics, [session, executable] (unsigned) {
				auto s = create_file(NULL/*get_frame_hwnd(ctx.shell)*/, executable);

				if (s.get())
				{
					strmd::serializer<write_file_stream, packer> ser(*s);
					ser(session);
				}
			}, false, [] (unsigned, unsigned &state) {
				return state = command_target::visible | command_target::supported | command_target::enabled, true;
			});

			target.add_command(cmdidClearStatistics, [statistics] (unsigned) {
				statistics->clear();
			}, false, [] (unsigned, unsigned &state) {
				return state = command_target::visible | command_target::supported | command_target::enabled, true;
			});

			target.add_command(cmdidCopyStatistics, [model] (unsigned) {
				string result_utf8;

				model->print(result_utf8);

				wstring result = unicode(result_utf8);

				if (::OpenClipboard(NULL))
				{
					if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (result.size() + 1) * sizeof(wchar_t)))
					{
						wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

						std::copy(result.c_str(), result.c_str() + result.size() + 1, gtext_memory);
						::GlobalUnlock(gtext_memory);
						::EmptyClipboard();
						::SetClipboardData(CF_UNICODETEXT, gtext);
					}
					::CloseClipboard();
				}
			}, false, [] (unsigned, unsigned &state) {
				return state = command_target::visible | command_target::supported | command_target::enabled, true;
			});

			target.add_command(cmdidProfileScope, [&running_objects, &factory, session] (unsigned) {
				wpl::rect_i l = { 0, 0, 800, 530 };
				const auto o = make_shared< pair< shared_ptr<wpl::form>, vector<wpl::slot_connection> > >();
				auto &running_objects_ = running_objects;
				const auto i = running_objects.insert(running_objects.end(), o);
				const auto onclose = [i, &running_objects_/*, hshell*/] {
					running_objects_.erase(i);
				};
				const auto root = make_shared<wpl::overlay>();
					root->add(factory.create_control<wpl::control>("background"));
					const auto patch_ui = make_shared<image_patch_ui>(factory,
						make_shared<image_patch_model>(session.patches, session.modules, session.module_mappings),
							session.patches);
					root->add(wpl::pad_control(patch_ui, 5, 5));

				o->first = factory.create_modal();
				o->second.push_back(o->first->close += onclose);

				o->first->set_caption("Select Profiled Scope - " + (string)*session.process_info.executable);
				o->first->set_root(root);
				o->first->set_location(l);
				o->first->center_parent();
				o->first->set_visible(true);
			}, false, [injected] (unsigned, unsigned &state) {
				return state = injected ? command_target::visible | command_target::supported | command_target::enabled : 0, true;
			});
		}
	}
}
