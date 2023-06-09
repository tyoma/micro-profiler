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

#include <inttypes.h>
#include <stdio.h>

using namespace std;

namespace micro_profiler
{
	function<bool (pair<void *, size_t> &allocation)> virtual_memory::enumerate_allocations()
	{
		class enumerator
		{
		public:
			enumerator()
				: _maps(fopen("/proc/self/maps", "r"), [] (FILE *file) {	if (file) fclose(file);	}), _line {}, _path {}
			{	}

			bool operator ()(pair<void *, size_t> &allocation)
			{
				void *from, *to;
				char rights[16];
				int dummy;
				unsigned long long int inode, offset;

				if (fgets(_line, sizeof _line - 1, _maps.get()))
					if (sscanf(_line, "%" SCNxPTR "-%" SCNxPTR " %s %" SCNxPTR " %x:%x %d %s\n",
							&from, &to, rights, &offset, &dummy, &dummy, &inode, _path) > 0 && _path[0])
						return allocation = make_pair(from, static_cast<byte *>(to) - static_cast<byte *>(from)), true;
				return false;
			}

		private:
			shared_ptr<FILE> _maps;
			char _line[1000], _path[1000];
		};

		return enumerator();
	}
}
