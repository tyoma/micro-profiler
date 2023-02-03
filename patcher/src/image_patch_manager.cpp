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

#include <patcher/image_patch_manager.h>

#include <patcher/exceptions.h>

using namespace std;

namespace micro_profiler
{
	image_patch_manager::image_patch::image_patch()
		: patches_applied(0)
	{	}

	void image_patch_manager::unmap_all()
	{
		for (auto i = _patched_images.begin(); i != _patched_images.end(); ++i)
		{
			for (auto j = i->second.patched.begin(); j != i->second.patched.end(); ++j)
				j->second->detach();
		}
	}

	void image_patch_manager::map_module(id_t /*mapping_id*/, id_t /*module_id*/, const module::mapping &/*mapping*/)
	{
	}

	void image_patch_manager::unmap_module(id_t /*mapping_id*/)
	{
	}

	void image_patch_manager::query(patch_state &/*states*/, unsigned int /*module_id*/)
	{	}

	void image_patch_manager::apply(apply_results &results, unsigned int module_id, void *base,
		shared_ptr<void> lock, request_range targets)
	{
		auto &image = _patched_images[module_id];

		for (auto i = targets.begin(); i != targets.end(); ++i)
		{
			auto e = image.patched.find(*i);
			patch_apply result = {	patch_result::unchanged, 0u	};

			try
			{
				if (image.patched.end() == e)
					e = image.patched.insert(make_pair(*i, _create_patch(static_cast<byte *>(base) + *i))).first;

				auto &patch = *e->second;

				if (patch.activate(false))
				{
					image.patches_applied++;
					result.result = patch_result::ok;
				}
			}
			catch (patch_exception &)
			{
				result.result = patch_result::error;
			}
			results.push_back(make_pair(*i, result));
		}
		if (image.patches_applied)
			image.lock = lock;
	}

	void image_patch_manager::revert(revert_results &results, unsigned int module_id, request_range targets)
	{
		auto &image = _patched_images[module_id];

		for (auto i = targets.begin(); i != targets.end(); ++i)
		{
			const auto e = image.patched.find(*i);
			patch_result::errors result = patch_result::unchanged;

			if (image.patched.end() != e)
			{
				auto &patch = *e->second;

				if (patch.revert())
				{
					image.patches_applied--;
					result = patch_result::ok;
				}
			}
			results.push_back(make_pair(*i, result));
		}
		if (!image.patches_applied)
			image.lock.reset();
	}
}
