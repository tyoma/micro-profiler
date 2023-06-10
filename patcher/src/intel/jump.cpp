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

#include "jump.h"

#include <cstdint>
#include <numeric>
#include <patcher/jump.h>

using namespace std;

namespace micro_profiler
{
	const size_t c_jump_size = sizeof(assembler::jump);

	namespace assembler
	{
		void jump::init(const void *address)
		{
			auto d = static_cast<const byte *>(address) - reinterpret_cast<byte *>(this + 1);

			if (d < numeric_limits<int32_t>::min() || numeric_limits<int32_t>::max() < d)
				throw out_of_range("address");
			opcode = 0xE9;
			displacement = static_cast<dword>(d);
		}

		void short_jump::init(const void *address)
		{
			short_jump j = {
				0xEB,
				static_cast<byte>(reinterpret_cast<size_t>(address) - reinterpret_cast<size_t>(this + 1))
			};

			*this = j;
		}
	}

	void jump_initialize(void *at, const void *target)
	{	static_cast<assembler::jump *>(at)->init(target);	}
}
