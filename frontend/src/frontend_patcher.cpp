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
#include <frontend/tables.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		bool set_requested(tables::patch &p, bool for_active)
		{
			return !!p.state.active == for_active && !p.state.error && !p.state.requested
				? p.state.requested = true, true : false;
		}

		void set_complete(tables::patch &p, patch_result::errors error, bool active)
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
		_db->patches.apply = [this] (unsigned int persistent_id, range<const unsigned int, size_t> rva) {
			apply(persistent_id, rva);
		};
		_db->patches.revert = [this] (unsigned int persistent_id, range<const unsigned int, size_t> rva) {
			revert(persistent_id, rva);
		};
	}

	void frontend::apply(unsigned int persistent_id, range<const unsigned int, size_t> rva)
	{
		auto req = new_request_handle();
		auto &image_patches = (_db->patches)[persistent_id];
		auto &targets = _patch_request_payload.functions_rva;

		_patch_request_payload.image_persistent_id = persistent_id;
		targets.clear();
		targets.reserve(rva.length());
		for_each(rva.begin(), rva.end(), [&] (unsigned int rva) {
			if (set_requested(image_patches[rva], false))
				targets.push_back(rva);
		});
		if (targets.empty())
			return;
		_db->patches.invalidate();
		request(*req, request_apply_patches, _patch_request_payload, response_patched,
			[this, persistent_id, req] (ipc::deserializer &d) {

			auto &image_patches = (_db->patches)[persistent_id];

			d(_patched_buffer);
			for (auto i = _patched_buffer.begin(); i != _patched_buffer.end(); ++i)
			{
				auto &patch = image_patches[i->first /*rva*/];

				patch.id = i->second.id;
				set_complete(patch, i->second.result, true);
			}
			_db->patches.invalidate();
			_requests.erase(req);
		});
	}

	void frontend::revert(unsigned int persistent_id, range<const unsigned int, size_t> rva)
	{
		auto req = new_request_handle();
		auto &image_patches = (_db->patches)[persistent_id];
		auto &targets = _patch_request_payload.functions_rva;

		_patch_request_payload.image_persistent_id = persistent_id;
		targets.clear();
		targets.reserve(rva.length());
		for_each(rva.begin(), rva.end(), [&] (unsigned int rva) {
			const auto i = image_patches.find(rva);

			if (i != image_patches.end() && set_requested(i->second, true))
				targets.push_back(rva);
		});
		if (targets.empty())
			return;
		_db->patches.invalidate();
		request(*req, request_revert_patches, _patch_request_payload, response_reverted,
			[this, persistent_id, req] (ipc::deserializer &d) {

			auto &image_patches = (_db->patches)[persistent_id];

			d(_reverted_buffer);
			for (auto i = _reverted_buffer.begin(); i != _reverted_buffer.end(); ++i)
				set_complete(image_patches[i->first /*rva*/], i->second, false);
			_db->patches.invalidate();
			_requests.erase(req);
		});
	}
}
