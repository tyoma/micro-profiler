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

#include <frontend/async_file.h>
#include <frontend/helpers.h>
#include <frontend/serialization.h>

#include <common/path.h>
#include <logger/log.h>

#define PREAMBLE "Frontend (metadata): "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		typedef strmd::deserializer<read_file_stream, strmd::varint> file_deserializer;
		typedef strmd::serializer<write_file_stream, strmd::varint> file_serializer;
	}

	void frontend::request_metadata(shared_ptr<void> &request_, unsigned int persistent_id,
		const tables::modules::metadata_ready_cb &ready)
	{
		const auto init = mx_metadata_requests_t::create(request_, _mx_metadata_requests, persistent_id, ready);
		const auto m = _db->modules.find(persistent_id);

		if (m != _db->modules.end())
		{
			ready(m->second);
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
		const auto cache_path = _symbol_cache_paths.find(persistent_id);

		if (cache_path == _symbol_cache_paths.end())
		{
			request_metadata_nw(request_, persistent_id, ready);
			return;
		}

		const auto req = make_shared< shared_ptr<void> >();
		auto &rreq = *req;
		const auto cached_invoke = [this, ready, cache_path] (const module_info_metadata &m) {
			store_metadata(cache_path->second, m);
			ready(m);
		};

		request_ = req;
		async_file_read<module_info_metadata>(rreq, _worker_queue, _apartment_queue, cache_path->second,
			[] (module_info_metadata &m, read_file_stream &r) {
			file_deserializer d(r);
			d(m);
		}, [this, persistent_id, ready, &rreq, cached_invoke] (shared_ptr<module_info_metadata> m) {
			if (m)
				ready((_db->modules)[persistent_id] = *m);
			else
				request_metadata_nw(rreq, persistent_id, cached_invoke);
		});
	}

	void frontend::request_metadata_nw(shared_ptr<void> &request_, unsigned int persistent_id,
		const tables::modules::metadata_ready_cb &ready)
	{
		LOG(PREAMBLE "requesting from remote...") % A(this) % A(persistent_id);
		request(request_, request_module_metadata, persistent_id, response_module_metadata,
			[this, persistent_id, ready] (ipc::deserializer &d) {

			auto &m = (_db->modules)[persistent_id];

			d(m);

			LOG(PREAMBLE "received...") % A(persistent_id) % A(m.symbols.size()) % A(m.source_files.size());
			ready(m);
		});
	}

	void frontend::store_metadata(const string &cache_path, const module_info_metadata &m)
	{
		auto r = new_request_handle();

		async_file_write(*r, _worker_queue, _apartment_queue, cache_path, [&m] (write_file_stream &w) {
			file_serializer s(w);

			s(m);
		}, [this, cache_path, r] (bool success) {
			LOG(PREAMBLE "written to cache...") % A(this) % A(cache_path) % A(success);
			_requests.erase(r);
		});
	}

	string frontend::construct_cache_path(const mapped_module_ex &mapping) const
	{
		auto cache_path = _cache_directory & *mapping.path;

		cache_path += "-";
		itoa<16>(cache_path, mapping.hash, 8);
		cache_path += ".symcache";
		return cache_path;
	}
}
