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

#include <frontend/frontend.h>

#include <frontend/tables.h>

using namespace std;

namespace micro_profiler
{
	void frontend::init_patcher()
	{
		_patches->apply = [this] (unsigned int persistent_id, range<const unsigned int, size_t> rva) {
			apply(persistent_id, rva);
		};
		_patches->revert = [this] (unsigned int persistent_id, range<const unsigned int, size_t> rva) {
			revert(persistent_id, rva);
		};
	}

	void frontend::apply(unsigned int persistent_id, range<const unsigned int, size_t> rva)
	{
		auto req = new_request_handle();
		auto &image_patches = (*_patches)[persistent_id];
		auto &targets = _patch_request_payload.functions_rva;

		_patch_request_payload.image_persistent_id = persistent_id;
		targets.clear();
		targets.reserve(rva.length());
		for_each(rva.begin(), rva.end(), [&] (unsigned int rva) {
			auto &state = image_patches[rva].state;

			if (!state.active && !state.error && !state.requested)
				targets.push_back(rva), state.requested = true;
		});
		if (targets.empty())
			return;
		_patches->invalidated();
		request(*req, request_apply_patches, _patch_request_payload, response_patched,
			[this, persistent_id, req] (deserializer &d) {

			auto &image_patches = (*_patches)[persistent_id];

			d(_patched_buffer);
			for (auto i = _patched_buffer.begin(); i != _patched_buffer.end(); ++i)
			{
				auto &patch = image_patches[i->first /*rva*/];

				patch.id = i->second.id;
				patch.state.requested = false;
				if (i->second.result != patch_result::ok)
					patch.state.error = true;
				else
					patch.state.active = true;
			}
			_patches->invalidated();
			_requests.erase(req);
		});
	}

	void frontend::revert(unsigned int persistent_id, range<const unsigned int, size_t> rva)
	{
		auto req = new_request_handle();

		_patch_request_payload.image_persistent_id = persistent_id;
		_patch_request_payload.functions_rva.assign(rva.begin(), rva.end());
		request(*req, request_revert_patches, _patch_request_payload, response_reverted,
			[this, req] (deserializer &) {

//			_requests.erase(req);
		});
	}
}
