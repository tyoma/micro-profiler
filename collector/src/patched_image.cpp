//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/patched_image.h>

#include <collector/binary_image.h>
#include <collector/calls_collector.h>
#include <collector/image_patch.h>

#include <common/allocator.h>
#include <common/module.h>
#include <iostream>
#include <set>

using namespace std;

extern "C" micro_profiler::calls_collector *g_collector_ptr;

namespace micro_profiler
{
	void patched_image::patch_image(void *in_image_address)
	{
		set<const void *> patched;

		std::shared_ptr<binary_image> image = load_image_at((void *)get_module_info(in_image_address).load_address);
		executable_memory_allocator em;
		int n = 0;

		image->enumerate_functions([&] (const function_body &fn) {
			try
			{
				if (!patched.insert(fn.body().begin()).second)
				{
					cout << fn.name() << " - the function has already been patched!" << endl;
				}
				else if (fn.name() == "_VEC_memcpy")
				{
				}
				else if (fn.body().length() >= 5)
				{
					shared_ptr<function_patch> patch(new function_patch(em, fn.effective_address(), fn.body(),
						g_collector_ptr));

					_patches.push_back(patch);
					++n;
				}
				else
				{
					cout << fn.name() << " - the function is too short!" << endl;
				}
			}
			catch (const exception &e)
			{
				cout << fn.name() << " - " << e.what() << endl;
			}
		});
	}
}
