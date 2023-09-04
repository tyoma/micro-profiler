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

#include <frontend/database_views.h>

#include <sdb/transforms.h>

namespace micro_profiler
{
	std::shared_ptr<const tables::patched_symbols> patched_symbols(std::shared_ptr<const tables::symbols> symbols,
		std::shared_ptr<const tables::modules> modules,
		std::shared_ptr<const tables::source_files> source_files,
		std::shared_ptr<const tables::module_mappings> mappings,
		std::shared_ptr<const tables::patches> patches)
	{
		auto t1 = sdb::join(*symbols, *modules, keyer::module_id(), keyer::id());
		auto t2 = sdb::left_join(*t1, *source_files,
			keyer::combine2<keyer::module_id, keyer::file_id>(), keyer::combine2<keyer::module_id, keyer::id>());
		auto t3 = sdb::left_join(*t2, *mappings, keyer::module_id(), keyer::module_id());
		auto t4 = sdb::left_join(*t3, *patches, keyer::symbol_id(), keyer::symbol_id());

		return make_shared_aspect(make_shared_copy(std::make_tuple(symbols, modules, source_files, mappings, patches,
			t1, t2, t3, t4)), t4.get());
	}
}
