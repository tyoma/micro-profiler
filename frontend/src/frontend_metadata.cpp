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

#include <frontend/frontend.h>

#include <frontend/helpers.h>
#include <frontend/profiling_preferences_db.h>
#include <frontend/serialization.h>

#include <common/path.h>
#include <logger/log.h>
#include <sdb/integrated_index.h>
#include <sqlite++/database.h>

#define PREAMBLE "Frontend (metadata): "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class cache_lookup_request
		{
		public:
			typedef function<void (const tables::module &module)> on_found;
			typedef function<void ()> on_not_found;

		public:
			cache_lookup_request(on_found &&found, on_not_found &&not_found,
					scheduler::queue &worker_queue, scheduler::queue &apartment_queue)
				: _found(move(found)), _not_found(move(not_found)), _queue(worker_queue, apartment_queue)
			{	}

			void run(sql::connection_ptr connection, uint32_t hash, uint32_t assign_module_id)
			{
				_queue.schedule([this, connection, hash, assign_module_id] (scheduler::private_worker_queue::completion &c) {
					lookup(c, connection, hash, assign_module_id);
				});
			}

		private:
			void lookup(scheduler::private_worker_queue::completion &completion, sql::connection_ptr connection,
				uint32_t hash, uint32_t assign_module_id)
			{
				sql::transaction t(connection);
				auto r_modules = t.select<tables::module>("modules", sql::c(&tables::module::hash) == sql::p(hash));

				for (auto m = make_shared<tables::module>(); r_modules(*m); )
				{
					auto r_symbols = t.select<tables::symbol_info>("symbols",
						sql::c(&tables::symbol_info::module_id) == sql::p(m->id));
					auto r_sources = t.select<tables::source_file>("source_files",
						sql::c(&tables::source_file::module_id) == sql::p(m->id));

					for (tables::symbol_info item; r_symbols(item); )
						m->symbols.push_back(item);
					for (tables::source_file item; r_sources(item); )
						m->source_files[item.id] = item.path;
					m->id = assign_module_id;
					completion.deliver([this, m] {
						_found(*m);
					});
					return;
				}
				completion.deliver(move(_not_found));
			}

		private:
			const on_found _found;
			on_not_found _not_found;
			scheduler::private_worker_queue _queue;
		};
	}

	void frontend::request_metadata(shared_ptr<void> &request_, unsigned int persistent_id,
		const tables::modules::metadata_ready_cb &ready)
	{
		const auto init = mx_metadata_requests_t::create(request_, _mx_metadata_requests, persistent_id, ready);
		const auto m = sdb::unique_index(_db->modules, keyer::external_id()).find(persistent_id);

		if (m)
		{
			ready(*m);
		}
		else if (init.underlying)
		{
			auto &mx = init.multiplexed;

			request_metadata_nw_cached(*init.underlying, persistent_id, [&mx] (const module_info_metadata &metadata) {
				mx.invoke([&metadata] (const tables::modules::metadata_ready_cb &ready) {	ready(metadata);	});
			});
		}
	}

	void frontend::request_metadata_nw_cached(shared_ptr<void> &request_, unsigned int persistent_id,
		const tables::modules::metadata_ready_cb &ready)
	{
		const auto h = _module_hashes.find(persistent_id);

		if (h == _module_hashes.end())
			return request_metadata_nw(request_, persistent_id, ready);

		const auto req = make_shared< shared_ptr<void> >();
		auto &rreq = *req;
		const auto caching_ready = [this, persistent_id, ready] (const module_info_metadata &m) {
			auto self = this;
			auto m_ = make_shared<module_info_metadata>(m);

			_worker_queue.schedule([self, m_] {	self->store_metadata(*m_);	});
			ready(m);
		};
		const auto cache_lookup = make_shared<cache_lookup_request>([this, persistent_id, ready] (const tables::module &module) {
			auto rec = sdb::unique_index(_db->modules, keyer::external_id())[persistent_id];
			auto &m_ = *rec;

			m_ = module;
			rec.commit();
			ready(m_);
		}, [this, persistent_id, &rreq, caching_ready] {
			request_metadata_nw(rreq, persistent_id, caching_ready);
		}, _worker_queue_raw, _apartment_queue);

		rreq = cache_lookup;
		request_ = req;
		cache_lookup->run(_preferences_db_connection, h->second, h->first);
	}

	void frontend::request_metadata_nw(shared_ptr<void> &request_, unsigned int persistent_id,
		const tables::modules::metadata_ready_cb &ready)
	{
		LOG(PREAMBLE "requesting from remote...") % A(this) % A(persistent_id);
		request(request_, request_module_metadata, persistent_id, response_module_metadata,
			[this, persistent_id, ready] (ipc::deserializer &d) {

			auto rec = sdb::unique_index(_db->modules, keyer::external_id())[persistent_id];
			auto &m = *rec;

			d(static_cast<module_info_metadata &>(m));
			rec.commit();

			LOG(PREAMBLE "received...") % A(m.id) % A(m.symbols.size()) % A(m.source_files.size());
			ready(m);
		});
	}

	void frontend::store_metadata(const module_info_metadata &m)
	{
		sql::transaction t(_preferences_db_connection, sql::transaction::immediate);
		tables::module mm;
		tables::symbol_info s;
		tables::source_file sf;
		auto w_modules = t.insert<tables::module>("modules");
		auto w_symbols = t.insert<tables::symbol_info>("symbols");
		auto w_sources = t.insert<tables::source_file>("source_files");

		static_cast<module_info_metadata &>(mm) = m;
		w_modules(mm);
		sf.module_id = s.module_id = mm.id;
		for (auto i = m.symbols.begin(); i != m.symbols.end(); ++i)
			static_cast<symbol_info &>(s) = *i, w_symbols(s);
		for (auto i = m.source_files.begin(); i != m.source_files.end(); ++i)
			sf.id = i->first, sf.path = i->second, w_sources(sf);
		t.commit();
		LOG(PREAMBLE "metadata written to cache...") % A(this) % A(mm.id) % A(m.symbols.size());
	}
}
