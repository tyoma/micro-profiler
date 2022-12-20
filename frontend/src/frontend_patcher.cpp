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

#include <common/serialization.h>
#include <frontend/keyer.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		bool set_requested(patch &p, bool for_active)
		{
			return !!p.state.active == for_active && !p.state.error && !p.state.requested
				? p.state.requested = true, true : false;
		}

		void set_complete(patch &p, patch_result::errors error, bool active)
		{
			p.state.requested = false;
			if (error != patch_result::ok)
				p.state.error = true;
			else
				p.state.active = active;
		}
	}

	void frontend::init_patcher()
	{
		_db->patches.apply = [this] (unsigned int module_id, range<const unsigned int, size_t> rva) {
			apply(module_id, rva);
		};
		_db->patches.revert = [this] (unsigned int module_id, range<const unsigned int, size_t> rva) {
			revert(module_id, rva);
		};
	}

	void frontend::apply(unsigned int module_id, range<const unsigned int, size_t> rva)
	{
		auto req = new_request_handle();
		auto &idx = sdb::unique_index<keyer::symbol_id>(_db->patches);
		auto &targets = _patch_request_payload.functions_rva;

		_patch_request_payload.image_persistent_id = module_id;
		targets.clear();
		targets.reserve(rva.length());
		for_each(rva.begin(), rva.end(), [&] (unsigned int rva) {
			auto rec = idx[make_tuple(module_id, rva)];

			if (set_requested(*rec, false))
				targets.push_back(rva);
			rec.commit();
		});
		if (targets.empty())
			return;
		_db->patches.invalidate();
		request(*req, request_apply_patches, _patch_request_payload, response_patched,
			[this, module_id, req, &idx] (ipc::deserializer &d) {

			d(_patched_buffer);
			for (auto i = _patched_buffer.begin(); i != _patched_buffer.end(); ++i)
			{
				auto rec = idx[make_tuple(module_id, i->first)];

				(*rec).id = i->second.id;
				set_complete(*rec, i->second.result, true);
				rec.commit();
			}
			_db->patches.invalidate();
			_requests.erase(req);
		});
	}

	void frontend::revert(unsigned int module_id, range<const unsigned int, size_t> rva)
	{
		auto req = new_request_handle();
		auto &idx = sdb::unique_index<keyer::symbol_id>(_db->patches);
		auto &targets = _patch_request_payload.functions_rva;

		_patch_request_payload.image_persistent_id = module_id;
		targets.clear();
		targets.reserve(rva.length());
		for_each(rva.begin(), rva.end(), [&] (unsigned int rva) {
			auto symbol_id = make_tuple(module_id, rva);

			if (idx.find(symbol_id))
			{
				auto rec = idx[symbol_id];

				if (set_requested(*rec, true))
					targets.push_back(rva);
				rec.commit();
			}
		});
		if (targets.empty())
			return;
		_db->patches.invalidate();
		request(*req, request_revert_patches, _patch_request_payload, response_reverted,
			[this, module_id, req, &idx] (ipc::deserializer &d) {

			d(_reverted_buffer);
			for (auto i = _reverted_buffer.begin(); i != _reverted_buffer.end(); ++i)
			{
				auto rec = idx[make_tuple(module_id, i->first)];

				set_complete(*rec, i->second, false);
				rec.commit();
			}
			_db->patches.invalidate();
			_requests.erase(req);
		});
	}
}
