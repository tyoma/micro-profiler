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

#include <patcher/image_patch_manager.h>

using namespace std;

namespace micro_profiler
{
	image_patch_manager::image_patch::image_patch()
		: patches_applied(0)
	{	}

	void image_patch_manager::apply(vector<unsigned int /*rva*/> &failures, unsigned int persistent_id,
		shared_ptr<void> lock, range<const unsigned int /*rva*/, size_t> functions)
	{
		auto &image = _patched_images[persistent_id];

		for (auto i = functions.begin(); i != functions.end(); ++i)
		{
			auto &patch = image.patched[*i];

			if (!patch)
			{
				patch = true;
				image.patches_applied++;
				continue;
			}
			failures.push_back(*i);
		}
		if (image.patches_applied)
			image.lock = lock;
	}

	void image_patch_manager::revert(vector<unsigned int /*rva*/> &failures, unsigned int persistent_id,
		range<const unsigned int /*rva*/, size_t> functions)
	{
		auto &image = _patched_images[persistent_id];

		for (auto i = functions.begin(); i != functions.end(); ++i)
		{
			auto &patch = image.patched[*i];

			if (patch)
			{
				patch = false;
				image.patches_applied--;
				continue;
			}
			failures.push_back(*i);
		}
		if (!image.patches_applied)
			image.lock.reset();
	}
}
