#include "command-ids.h"
#include "helpers.h"
#include "ui_helpers.h"

#include <common/formatting.h>
#include <common/path.h>
#include <common/string.h>
#include <frontend/file.h>
#include <frontend/frontend_ui.h>
#include <frontend/image_patch_model.h>
#include <frontend/image_patch_ui.h>
#include <frontend/persistence.h>
#include <frontend/statistic_models.h>
#include <frontend/statistics_poll.h>
#include <frontend/symbol_resolver.h>
#include <frontend/tables_ui.h>
#include <frontend/view_dump.h>
#include <strmd/serializer.h>
#include <windows.h>
#include <wpl/controls.h>
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
		shared_ptr<wpl::control> create_telemetry_ui(const wpl::factory &factory, shared_ptr<profiling_session> session)
		{
			const auto lbl = factory.create_control<wpl::label>("label");
			const auto ticks_per_second = static_cast<double>(session->process_info.ticks_per_second);
			const auto tick_interval = 1.0 / ticks_per_second;
			const auto telemetry = shared_ptr<tables::telemetry_history>(session, &session->telemetry_history);
			const auto text = make_shared<agge::richtext_modifier_t>(agge::style_modifier::empty);
			const auto p = make_shared< pair<shared_ptr<wpl::control>, wpl::slot_connection> >(lbl,
				telemetry->invalidate += [lbl, ticks_per_second, tick_interval, telemetry, text] {

				if (telemetry->size() < 2)
					return;

				auto i = telemetry->rbegin();
				const auto &current = *i++;
				const auto &previous = *i++;
				const auto snapshots_interval = current.timestamp - previous.timestamp;

				text->clear();
				*text << "Analysis overhead: ";
				if (current.total_analyzed)
					format_interval(*text, 2 * tick_interval * current.total_analysis_time / current.total_analyzed);
				else
					*text << "--";
				*text << "\nCalls per second analysed: ";
				if (snapshots_interval)
					itoa<10>(*text, static_cast<unsigned int>(0.5 * ticks_per_second * current.total_analyzed / snapshots_interval));
				else
					*text << "--";
				lbl->set_text(*text);
			});

			return shared_ptr<wpl::control>(p, lbl.get());
		}

		void init_instance_menu(command_target &target, list< shared_ptr<void> > &running_objects,
			const wpl::vs::factory &factory, shared_ptr<profiling_session> session, tables_ui &ui,
			shared_ptr<scheduler::queue> queue)
		{
			const auto statistics = micro_profiler::statistics(session);
			const auto injected = !!session->process_info.injected;
			const auto executable = session->process_info.executable;
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

			target.add_command(cmdidViewHierarchy, [&ui] (unsigned) {
				ui.set_hierarchical(true);
			}, false, [&ui] (unsigned, unsigned &state) {
				return state = (ui.get_hierarchical() ? command_target::checked : 0) | command_target::enabled | command_target::supported | command_target::visible, true;
			});

			target.add_command(cmdidViewFlat, [&ui] (unsigned) {
				ui.set_hierarchical(false);
			}, false, [&ui] (unsigned, unsigned &state) {
				return state = (!ui.get_hierarchical() ? command_target::checked : 0) | command_target::enabled | command_target::supported | command_target::visible, true;
			});

			target.add_command(cmdidSaveStatistics, [session, executable] (unsigned) {
				auto s = create_file(NULL/*get_frame_hwnd(ctx.shell)*/, executable);

				if (s.get())
				{
					strmd::serializer<write_file_stream, packer> ser(*s);
					ser(*session);
				}
			}, false, [] (unsigned, unsigned &state) {
				return state = command_target::visible | command_target::supported | command_target::enabled, true;
			});

			target.add_command(cmdidClearStatistics, [statistics] (unsigned) {
				statistics->clear();
			}, false, [] (unsigned, unsigned &state) {
				return state = command_target::visible | command_target::supported | command_target::enabled, true;
			});

			target.add_command(cmdidCopyStatistics, [&ui] (unsigned) {
				string result_utf8;

				ui.dump(result_utf8);

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
				const auto patch_ui = make_shared<image_patch_ui>(factory, make_shared<image_patch_model>(patches(session),
					modules(session), mappings(session)), patches(session));

				ui_helpers::show_dialog(running_objects, factory, patch_ui, 800, 530,
					"Select Profiled Scope - " + (string)*session->process_info.executable,
					[] (vector<wpl::slot_connection> &, function<void()>) {	});
			}, false, [injected] (unsigned, unsigned &state) {
				return state = injected ? command_target::visible | command_target::supported | command_target::enabled : 0, true;
			});

			target.add_command(cmdidShowTelemetry, [&running_objects, &factory, session] (unsigned) {
				ui_helpers::show_dialog(running_objects, factory, create_telemetry_ui(factory, session), 330, 91,
					"MicroProfiler - Telemetry", [] (vector<wpl::slot_connection> &, function<void()>) {	});
			}, false, [] (unsigned, unsigned &state) {
				return state = command_target::visible | command_target::supported | command_target::enabled, true;
			});
		}
	}
}
