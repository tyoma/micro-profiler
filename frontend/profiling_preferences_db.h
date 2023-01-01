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

#pragma once

#include "database.h"

#include <sqlite++/types.h>

namespace micro_profiler
{
	template <typename VisitorT>
	inline void describe(VisitorT &&v, identity *)
	{	v(sql::pk(&identity::id), "id");	}

	template <typename VisitorT>
	inline void describe(VisitorT &&v, module_info_metadata *)
	{
		v(&module_info_metadata::path, "path");
		v(&module_info_metadata::hash, "hash");
	}

	namespace tables
	{
		template <typename VisitorT>
		inline void describe(VisitorT &&v, module *m)
		{
			v("modules");
			describe(v, static_cast<identity *>(m));
			describe(v, static_cast<module_info_metadata *>(m));
		}

		template <typename VisitorT>
		inline void describe(VisitorT &&v, symbol_info *)
		{
			v("symbols");
			v(&symbol_info::module_id, "module_id");
			v(&symbol_info::rva, "rva");
			v(&symbol_info::size, "size");
			v(&symbol_info::name, "name");
			v(&symbol_info::file_id, "source_file_id");
			v(&symbol_info::line, "line_number");
		}

		template <typename VisitorT>
		inline void describe(VisitorT &&v, source_file *)
		{
			v("source_files");
			v(&identity::id, "id");
			v(&source_file::module_id, "module_id");
			v(&source_file::path, "path");
		}

		template <typename VisitorT>
		inline void describe(VisitorT &&v, cached_patch *)
		{
			v("patches");
			v(&cached_patch::scope_id, "scope_id");
			v(&cached_patch::module_id, "module_id");
			v(&cached_patch::rva, "rva");
		}
	}
}
