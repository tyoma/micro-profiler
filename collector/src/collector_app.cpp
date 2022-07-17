//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/collector_app.h>

#include <collector/analyzer.h>
#include <collector/module_tracker.h>
#include <collector/serialization.h>
#include <collector/thread_monitor.h>

#include <common/constants.h>
#include <common/protocol.h>
#include <common/time.h>
#include <ipc/server_session.h>
#include <logger/log.h>
#include <patcher/interface.h>

#define PREAMBLE "Collector app: "

using namespace std;

namespace micro_profiler
{
	collector_app::collector_app(const active_server_app::frontend_factory_t &factory, calls_collector_i &collector,
			const overhead &overhead_, thread_monitor &thread_monitor_, patch_manager &patch_manager_)
		: _collector(collector), _analyzer(new analyzer(overhead_)), _thread_monitor(thread_monitor_),
			_patch_manager(patch_manager_), _server(*this, factory)
	{	}

	collector_app::~collector_app()
	{	_collector.flush();	}

	void collector_app::initialize_session(ipc::server_session &session)
	{
		typedef ipc::server_session::response response;

		auto module_tracker_ = make_shared<module_tracker>();
		auto loaded = make_shared<loaded_modules>();
		auto unloaded = make_shared<unloaded_modules>();
		auto metadata = make_shared<module_info_metadata>();
		auto threads_buffer = make_shared< vector< pair<thread_monitor::thread_id, thread_info> > >();
		auto apply_results = make_shared<response_patched_data>();
		auto revert_results = make_shared<response_reverted_data>();
		shared_ptr<telemetry> telemetry_results(new telemetry);

		session.add_handler(request_update, [this, module_tracker_, loaded, unloaded] (response &resp) {
			module_tracker_->get_changes(*loaded, *unloaded);
			if (!loaded->empty())
				resp(response_modules_loaded, *loaded);
			resp(response_statistics_update, *_analyzer);
			if (!unloaded->empty())
				resp(response_modules_unloaded, *unloaded);
			_analyzer->clear();
		});

		session.add_handler(request_module_metadata,
			[module_tracker_, metadata] (response &resp, unsigned int persistent_id) {

			const auto l = module_tracker_->lock_mapping(persistent_id);
			const auto metadata_ = module_tracker_->get_metadata(persistent_id);
			auto &md = *metadata;

			metadata->path = l->second.path;
			metadata->hash = l->second.hash;
			metadata->symbols.clear();
			metadata->source_files.clear();
			metadata_->enumerate_functions([&] (const symbol_info &symbol) {
				md.symbols.push_back(symbol);
			});
			metadata_->enumerate_files([&] (const pair<unsigned, string> &file) {
				md.source_files.insert(file);
			});
			resp(response_module_metadata, *metadata);
		});

		session.add_handler(request_threads_info,
			[this, threads_buffer] (response &resp, const vector<thread_monitor::thread_id> &ids) {

			threads_buffer->resize(ids.size());
			_thread_monitor.get_info(threads_buffer->begin(), ids.begin(), ids.end());
			resp(response_threads_info, *threads_buffer);
		});

		session.add_handler(request_apply_patches,
			[this, module_tracker_, apply_results] (response &resp, const patch_request &payload) {

			const auto l = module_tracker_->lock_mapping(payload.image_persistent_id);

			apply_results->clear();
			_patch_manager.apply(*apply_results, payload.image_persistent_id,
				reinterpret_cast<void *>(static_cast<size_t>(l->second.base)), l,
				range<const unsigned int, size_t>(payload.functions_rva.data(), payload.functions_rva.size()));
			resp(response_patched, *apply_results);
		});

		session.add_handler(request_revert_patches,
			[this, revert_results] (response &resp, const patch_request &payload) {

			revert_results->clear();
			_patch_manager.revert(*revert_results, payload.image_persistent_id,
				range<const unsigned int, size_t>(payload.functions_rva.data(), payload.functions_rva.size()));
			resp(response_reverted, *revert_results);
		});

		session.add_handler(request_telemetry,
			[this, telemetry_results] (response &resp) {

			telemetry_results->reset();
			_analyzer->get_telemetry(*telemetry_results);
			telemetry_results->timestamp = read_tick_counter();
			resp(response_telemetry, *telemetry_results);
		});


		session.message(init, [] (ipc::serializer &ser) {
			initialization_data idata = {
				get_current_executable(),
				ticks_per_second(),
				!!getenv(constants::profiler_injected_ev),
			};

			ser(idata);
		});

		_server.schedule([this] {	collect_and_reschedule();	}, mt::milliseconds(10));
	}

	bool collector_app::finalize_session(ipc::server_session &session)
	{
		_collector.read_collected(*_analyzer);
		session.message(exiting, [] (ipc::serializer &) {	});
		return true;
	}

	void collector_app::collect_and_reschedule()
	{
		_collector.read_collected(*_analyzer);
		_server.schedule([this] {	collect_and_reschedule();	}, mt::milliseconds(10));
	}
}
