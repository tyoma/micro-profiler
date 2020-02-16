//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include "range.h"

#include <functional>
#include <string>
#include <vector>

namespace micro_profiler
{
	struct mapped_module
	{
		typedef unsigned int instance_id_t;

		instance_id_t instance_id; // Zero-based instance ID of this mapping to identify it among loaded.
		instance_id_t persistent_id; // Persistent one-based ID of the image this mapping is for.
		std::string path;
		union
		{
			long_address_t load_address;
			byte *base;
		};
		std::vector<byte_range> addresses;
	};

	typedef std::function<void (const mapped_module &module)> module_callback_t;

	std::string get_current_executable();
	mapped_module get_module_info(const void *address);
	void enumerate_process_modules(const module_callback_t &callback);
}
