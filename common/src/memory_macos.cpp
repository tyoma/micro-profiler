//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/memory.h>

#include <mach/vm_map.h>
#include <mach-o/dyld_images.h>

using namespace std;

namespace micro_profiler
{
	function<bool (pair<void *, size_t> &allocation)> virtual_memory::enumerate_allocations()
	{
		struct enumerator
		{
			bool operator ()(pair<void *, size_t> &allocation)
			{
				auto a = reinterpret_cast<vm_address_t>(ptr);
				const auto self = mach_task_self();
				size_t sz = 0;
				uint32_t depth = 0;
				vm_region_submap_info_64 info;
				mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

				if (KERN_SUCCESS == vm_region_recurse_64(self, &a, &sz, &depth, (vm_region_recurse_info_t)&info, &count))
				{
					allocation = make_pair(reinterpret_cast<void *>(a), sz);
					ptr = reinterpret_cast<void *>(a + sz);
					return true;
				}
				return false;
			}
			
			void *ptr = nullptr;
		};
		
		return enumerator();
	}
}
