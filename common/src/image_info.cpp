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

#include <common/image_info.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	symbol_info_mapped::symbol_info_mapped(const char *name_, byte_range body_)
		: name(name_), body(body_)
	{	}


	offset_image_info::offset_image_info(const std::shared_ptr< image_info<symbol_info> > &underlying, size_t base)
		: _underlying(underlying), _base((byte *)base)
	{	}

	void offset_image_info::enumerate_functions(const symbol_callback_t &callback) const
	{
		struct local
		{
			static void offset_symbol(const symbol_callback_t &callback, const symbol_info &si, byte *base)
			{
				symbol_info_mapped offset_si(si.name.c_str(), byte_range(base + si.rva, si.size));

				callback(offset_si);
			}
		};
		_underlying->enumerate_functions(bind(&local::offset_symbol, callback, _1, _base));
	}
}
