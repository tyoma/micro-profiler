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

#pragma once

#include "database.h"
#include "keyer.h"

#include <sdb/transform_types.h>

namespace micro_profiler
{
	namespace tables
	{
		typedef sdb::left_joined<
			sdb::left_joined<
				sdb::left_joined<
					sdb::joined<
						symbols,
						modules
					>::table_type,
					source_files
				>::table_type,
				module_mappings
			>::table_type,
			patches
		>::table_type patched_symbols;

		class patched_symbol_adaptor
		{
		public:
			patched_symbol_adaptor(const patched_symbols::value_type &underlying)
				: _underlying(underlying)
			{	}

			const tables::symbol_info &symbol() const
			{	return _underlying.left().left().left().left();	}

			const tables::module &module() const
			{	return _underlying.left().left().left().right();	}

			nullable<const tables::source_file &> source_file() const;
			nullable<const tables::module_mapping &> mapping() const;
			nullable<const patch_state_ex&> patch() const
			{	return _underlying.right();	}

		private:
			const patched_symbols::value_type &_underlying;
		};
	}

	std::shared_ptr<const tables::patched_symbols> patched_symbols(std::shared_ptr<const tables::symbols> symbols,
		std::shared_ptr<const tables::modules> modules,
		std::shared_ptr<const tables::source_files> source_files,
		std::shared_ptr<const tables::module_mappings> mappings,
		std::shared_ptr<const tables::patches> patches);
}
