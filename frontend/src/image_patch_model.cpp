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

#include <frontend/image_patch_model.h>

using namespace std;

namespace micro_profiler
{
	struct image_patch_model::flattener::const_reference
	{
	};

	//image_patch_model::image_patch_model(shared_ptr<const tables::patches> /*patches*/,
	//	shared_ptr<const tables::modules> modules, shared_ptr<const tables::module_mappings> /*mappings*/)
	//	: _flatten_view(*modules), _ordered_view(_flatten_view)
	//{
	//}

	//image_patch_model::index_type image_patch_model::get_count() const throw()
	//{
	//	return 0;
	//}

	//void image_patch_model::get_text(index_type /*row*/, index_type /*column*/, agge::richtext_t &/*value*/) const
	//{
	//}
}
