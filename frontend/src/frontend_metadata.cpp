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
#include <scheduler/task.h>
#include <sdb/integrated_index.h>
#include <sqlite++/database.h>

#define PREAMBLE "Frontend (metadata): "

using namespace scheduler;
using namespace std;

namespace micro_profiler
{
	frontend::module_ptr frontend::load_metadata(sql::connection_ptr preferences_db, unsigned int hash)
	{
		sql::transaction t(preferences_db);
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
			return m;
		}
		return nullptr;
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
			const auto h = _module_hashes.find(persistent_id);
			const auto ready2 = [this, persistent_id, &mx] (const module_ptr &metadata) {
				auto rec = sdb::unique_index(_db->modules, keyer::external_id())[persistent_id];
				auto &m = static_cast<module_info_metadata &>(*rec) = *metadata;

				rec.commit();
				mx.invoke([&m] (const tables::modules::metadata_ready_cb &ready) {
					ready(m);
				});
			};

			if (h == _module_hashes.end())
				request_metadata_nw(*init.underlying, persistent_id, ready2);
			else
				request_metadata_nw_cached(*init.underlying, persistent_id, h->second, ready2);
		}
	}

	template <typename F>
	void frontend::request_metadata_nw_cached(shared_ptr<void> &request_, unsigned int persistent_id, unsigned int hash,
		const F &ready)
	{
		const auto req = make_shared< shared_ptr<void> >();
		const weak_ptr< shared_ptr<void> > wreq = req;
		const auto db = _preferences_db_connection;
		const auto ready2 = [ready, wreq] (const module_ptr &m) {
			if (!wreq.expired())
				ready(m);
		};
		const auto cache_completion = make_shared< task_node<module_ptr> >();
		const auto caching_ready = [ready2, cache_completion] (const module_ptr &m) {
			cache_completion->set(frontend::module_ptr(m));
			ready2(m);
		};

		request_ = req;
		task<module_ptr>::run([db, hash] {
			return load_metadata(db, hash);
		}, _worker_queue).continue_with([this, persistent_id, wreq, ready2, caching_ready] (const async_result<module_ptr> &m) {
			if (*m)
				ready2(*m);
			else if (const auto next_request = wreq.lock())
				request_metadata_nw(*next_request, persistent_id, caching_ready);
		}, _apartment_queue);
		task<module_ptr>(shared_ptr< task_node<module_ptr> >(cache_completion)).continue_with([db] (const async_result<module_ptr> &m) {
			frontend::store_metadata(db, **m);
		}, _worker_queue);
	}

	template <typename F>
	void frontend::request_metadata_nw(shared_ptr<void> &request_, unsigned int persistent_id, const F &ready)
	{
		LOG(PREAMBLE "requesting from remote...") % A(this) % A(persistent_id);
		request(request_, request_module_metadata, persistent_id, response_module_metadata,
			[this, persistent_id, ready] (ipc::deserializer &d) {

			auto m = make_shared<module_info_metadata>();

			d(*m);
			LOG(PREAMBLE "received...") % A(persistent_id) % A(m->symbols.size()) % A(m->source_files.size());
			ready(m);
		});
	}

	void frontend::store_metadata(sql::connection_ptr preferences_db, const module_info_metadata &m)
	{
		sql::transaction t(preferences_db, sql::transaction::immediate);
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
		LOG(PREAMBLE "metadata written to cache...") % A(mm.id) % A(m.symbols.size());
	}
}
