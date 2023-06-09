//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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
	collector_app::collector_app(calls_collector_i &collector, const overhead &overhead_, thread_monitor &threads,
			module_tracker &module_tracker_, patch_manager &patch_manager_)
		: _collector(collector), _analyzer(new analyzer(overhead_)), _thread_monitor(threads),
			_module_tracker(module_tracker_), _patch_manager(patch_manager_), _server(*this)
	{	}

	collector_app::~collector_app()
	{	_collector.flush();	}

	void collector_app::connect(const active_server_app::client_factory_t &factory, bool injected)
	{
		_injected = injected;
		_server.connect(factory);
	}

	scheduler::queue &collector_app::get_queue()
	{	return _server;	}

	void collector_app::initialize_session(ipc::server_session &session)
	{
		typedef ipc::server_session::response response;

		// Keep buffer objects to avoid excessive allocations.
		auto history_key = make_shared<module_tracker::mapping_history_key>();
		auto mapped_ = make_shared<loaded_modules>();
		auto unmapped_ = make_shared<unloaded_modules>();
		auto metadata = make_shared<module_info_metadata>();
		auto module_info = make_shared<module_tracker::module_info>();
		auto threads_buffer = make_shared< vector< pair<thread_monitor::thread_id, thread_info> > >();
		auto patch_results = make_shared<response_patched_data>();

		session.add_handler(request_update, [this, history_key, mapped_, unmapped_] (response &resp) {
			_module_tracker.get_changes(*history_key, *mapped_, *unmapped_);
			resp(response_modules_loaded, *mapped_);
			resp(response_statistics_update, *_analyzer);
			resp(response_modules_unloaded, *unmapped_);
			_analyzer->clear();
		});

		session.add_handler(request_module_metadata,
			[this, metadata, module_info] (response &resp, unsigned int module_id) {

			auto &md = *metadata;
			auto metadata_ = _module_tracker.get_metadata(module_id);

			_module_tracker.get_module(*module_info, module_id); // TODO: check the result.
			md.path = module_info->path;
			md.hash = module_info->hash;
			md.symbols.clear();
			md.source_files.clear();
			metadata_->enumerate_functions([&] (const symbol_info &symbol) {
				md.symbols.push_back(symbol);
			});
			metadata_->enumerate_files([&] (const pair<unsigned, string> &file) {
				md.source_files.insert(file);
			});
			resp(response_module_metadata, md);
		});

		session.add_handler(request_threads_info,
			[this, threads_buffer] (response &resp, const vector<thread_monitor::thread_id> &ids) {

			threads_buffer->resize(ids.size());
			_thread_monitor.get_info(threads_buffer->begin(), ids.begin(), ids.end());
			resp(response_threads_info, *threads_buffer);
		});

		session.add_handler(request_apply_patches, [this, patch_results] (response &resp, const patch_request &payload) {
			_patch_manager.apply(*patch_results, payload.image_persistent_id, make_range(payload.functions_rva));
			resp(response_patched, *patch_results);
		});

		session.add_handler(request_revert_patches, [this, patch_results] (response &resp, const patch_request &payload) {
			_patch_manager.revert(*patch_results, payload.image_persistent_id, make_range(payload.functions_rva));
			resp(response_reverted, *patch_results);
		});


		session.message(init, [this] (ipc::serializer &ser) {
			initialization_data idata = {
				_module_tracker.helper().executable(),
				ticks_per_second(),
				_injected,
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
