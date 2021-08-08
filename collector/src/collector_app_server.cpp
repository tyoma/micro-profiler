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

	shared_ptr<channel> collector_app::init_server(channel &outbound, shared_ptr<analyzer> analyzer_)
	{
		auto session = make_shared<server_session>(outbound);

		session->add_handler<int>(request_update, [this, analyzer_] (server_session::request &req, int) {
			loaded_modules loaded;
			unloaded_modules unloaded;
			auto &a = *analyzer_;
		
			_module_tracker->get_changes(loaded, unloaded);
			if (!loaded.empty())
				req.respond(response_modules_loaded, [&] (server_session::serializer &ser) {	ser(loaded);	});
			req.respond(response_statistics_update, [&] (server_session::serializer &ser) {	ser(a);	});
			if (!unloaded.empty())
				req.respond(response_modules_unloaded, [&] (server_session::serializer &ser) {	ser(unloaded);	});
			analyzer_->clear();
		});

		session->add_handler<unsigned int>(request_module_metadata,
			[this] (server_session::request &req, unsigned int persistent_id) {

			const auto metadata = _module_tracker->get_metadata(persistent_id);
			auto md = make_pair(persistent_id, module_info_metadata());

			metadata->enumerate_functions([&] (const symbol_info &symbol) {
				md.second.symbols.push_back(symbol);
			});
			metadata->enumerate_files([&] (const pair<unsigned, string> &file) {
				md.second.source_files.push_back(file);
			});
			req.respond(response_module_metadata, [&] (server_session::serializer &ser) {	ser(md);	});
		});

		session->add_handler< vector<thread_monitor::thread_id> >(request_threads_info,
			[this] (server_session::request &req, const vector<thread_monitor::thread_id> &ids) {

			vector< pair<thread_monitor::thread_id, thread_info> > threads_buffer(ids.size());

			_thread_monitor->get_info(threads_buffer.begin(), ids.begin(), ids.end());
			req.respond(response_threads_info, [&] (server_session::serializer &ser) {	ser(threads_buffer);	});
		});

		session->add_handler<patch_request>(request_apply_patches,
			[this] (server_session::request &req, const patch_request &payload) {

			vector<unsigned int> failures;
			const auto l = _module_tracker->lock_mapping(payload.image_persistent_id);

			_patch_manager.apply(failures, payload.image_persistent_id, reinterpret_cast<void *>(static_cast<size_t>(l->base)), l,
				range<const unsigned int, size_t>(payload.functions_rva.data(), payload.functions_rva.size()));
			req.respond(response_patched, [&] (server_session::serializer &ser) {	ser(failures);	});
		});

		session->add_handler<patch_request>(request_revert_patches,
			[this] (server_session::request &req, const patch_request &payload) {

			vector<unsigned int> failures;

			_patch_manager.revert(failures, payload.image_persistent_id,
				range<const unsigned int, size_t>(payload.functions_rva.data(), payload.functions_rva.size()));
			req.respond(response_reverted, [&] (server_session::serializer &ser) {	ser(failures);	});
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
