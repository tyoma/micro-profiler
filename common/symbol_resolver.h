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

#include "image_info.h"

#include <map>
#include <unordered_map>

namespace micro_profiler
{
	struct module_info_basic;
	struct module_info_metadata;

	class symbol_resolver
	{
	public:
		typedef std::pair<std::string, unsigned> fileline_t;

	public:
		virtual ~symbol_resolver();
		virtual const std::string &symbol_name_by_va(long_address_t address) const;
		virtual bool symbol_fileline_by_va(long_address_t address, fileline_t &result) const;
		void add_metadata(const module_info_basic &basic, const module_info_metadata &metadata);

	private:
		typedef std::unordered_map<unsigned int, std::string> files_map;
		typedef std::map<long_address_t, symbol_info> cached_names_map;

	private:
		cached_names_map::const_iterator find_symbol_by_va(long_address_t address) const;

	private:
		std::string _empty;
		files_map _files;
		cached_names_map _mapped_symbols;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, symbol_resolver &data);
	};
}
