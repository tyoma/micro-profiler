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

#include <collector/collector_app.h>

#include <collector/analyzer.h>
#include <collector/module_tracker.h>
#include <collector/serialization.h>
#include <collector/thread_monitor.h>

#include <common/time.h>
#include <ipc/server_session.h>
#include <patcher/interface.h>

using namespace std;

namespace micro_profiler
{
	using namespace ipc;

	shared_ptr<server_session> collector_app::init_server(channel &outbound)
	{
		auto session = make_shared<server_session>(outbound);

		auto module_tracker_ = make_shared<module_tracker>();
		auto loaded = make_shared<loaded_modules>();
		auto unloaded = make_shared<unloaded_modules>();
		auto metadata = make_shared<module_info_metadata>();
		auto threads_buffer = make_shared< vector< pair<thread_monitor::thread_id, thread_info> > >();
		auto apply_results = make_shared<response_patched_data>();
		auto revert_results = make_shared<response_reverted_data>();

		session->add_handler(request_update, [this, module_tracker_, loaded, unloaded] (server_session::response &resp) {
			module_tracker_->get_changes(*loaded, *unloaded);
			if (!loaded->empty())
				resp(response_modules_loaded, *loaded);
			resp(response_statistics_update, *_analyzer);
			if (!unloaded->empty())
				resp(response_modules_unloaded, *unloaded);
			_analyzer->clear();
		});

		session->add_handler(request_module_metadata,
			[this, module_tracker_, metadata] (server_session::response &resp, unsigned int persistent_id) {

			const auto metadata_ = module_tracker_->get_metadata(persistent_id);
			auto &md = *metadata;

			metadata->path = metadata_->get_path();
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

		session->add_handler(request_threads_info,
			[this, threads_buffer] (server_session::response &resp, const vector<thread_monitor::thread_id> &ids) {

			threads_buffer->resize(ids.size());
			_thread_monitor.get_info(threads_buffer->begin(), ids.begin(), ids.end());
			resp(response_threads_info, *threads_buffer);
		});

		session->add_handler(request_apply_patches,
			[this, module_tracker_, apply_results] (server_session::response &resp, const patch_request &payload) {

			const auto l = module_tracker_->lock_mapping(payload.image_persistent_id);

			apply_results->clear();
			_patch_manager.apply(*apply_results, payload.image_persistent_id,
				reinterpret_cast<void *>(static_cast<size_t>(l->second.base)), l,
				range<const unsigned int, size_t>(payload.functions_rva.data(), payload.functions_rva.size()));
			resp(response_patched, *apply_results);
		});

		session->add_handler(request_revert_patches,
			[this, revert_results] (server_session::response &resp, const patch_request &payload) {

			revert_results->clear();
			_patch_manager.revert(*revert_results, payload.image_persistent_id,
				range<const unsigned int, size_t>(payload.functions_rva.data(), payload.functions_rva.size()));
			resp(response_reverted, *revert_results);
		});


		session->message(init, [] (server_session::serializer &ser) {
			initialization_data idata = {
				get_current_executable(),
				ticks_per_second()
			};

			ser(idata);
		});

		return session;
	}
}
