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

#pragma once

#include "database.h"
#include "serialization_context.h"

#include <common/noncopyable.h>
#include <common/protocol.h>
#include <functional>
#include <ipc/client_session.h>
#include <list>
#include <reqm/multiplexing_request.h>
#include <sqlite++/misc.h>

namespace scheduler
{
	struct queue;
}

namespace micro_profiler
{
	class frontend : public ipc::client_session, noncopyable
	{
	public:
		typedef std::shared_ptr<profiling_session> session_type;

	public:
		frontend(ipc::channel &outbound, const std::string &preferences_db,
			scheduler::queue &worker, scheduler::queue &apartment);
		~frontend();

		static void create_database(const std::string &preferences_db);

	public:
		std::function<void (const session_type &ui_context)> initialized;

	private:
		typedef std::shared_ptr<module_info_metadata> module_ptr;
		typedef containers::unordered_map<unsigned int /*persistent_id*/, std::uint32_t> module_hashes_t;
		typedef reqm::multiplexing_request<unsigned int, tables::modules::metadata_ready_cb> mx_metadata_requests_t;
		typedef std::list< std::shared_ptr<void> > requests_t;

	private:
		// ipc::channel methods
		virtual void disconnect() throw() override;

		void init_patcher();
		void apply(unsigned int persistent_id, range<const unsigned int, size_t> rva);
		void revert(unsigned int persistent_id, range<const unsigned int, size_t> rva);

		template <typename OnUpdate>
		void request_full_update(std::shared_ptr<void> &request_, const OnUpdate &on_update);
		void update_threads(std::vector<unsigned int> &thread_ids);
		void finalize();

		void request_metadata(std::shared_ptr<void> &request_, unsigned int persistent_id,
			const tables::modules::metadata_ready_cb &ready);

		template <typename F>
		void request_metadata_nw_cached(std::shared_ptr<void> &request_, unsigned int persistent_id, unsigned int hash,
			const F &ready);

		template <typename F>
		void request_metadata_nw(std::shared_ptr<void> &request_, unsigned int persistent_id, const F &ready);

		static module_ptr load_metadata(sql::connection_ptr preferences_db, unsigned int hash);
		static void store_metadata(sql::connection_ptr preferences_db, const module_info_metadata &metadata);

		requests_t::iterator new_request_handle();

	private:
		scheduler::queue &_worker_queue, &_apartment_queue;
		const std::shared_ptr<profiling_session> _db;
		const sql::connection_ptr _preferences_db_connection;
		module_hashes_t _module_hashes;
		scontext::additive _serialization_context;
		bool _initialized;

		mx_metadata_requests_t::map_type_ptr _mx_metadata_requests;
		requests_t _requests;
		std::shared_ptr<void> _update_request;

		// request_apply_patches buffers
		patch_request _patch_request_payload;
		response_patched_data _patched_buffer;

		// request_revert_patches buffers
		response_reverted_data _reverted_buffer;
	};
}
