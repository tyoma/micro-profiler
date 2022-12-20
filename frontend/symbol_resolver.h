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

#include <map>

namespace micro_profiler
{
	class symbol_resolver
	{
	public:
		typedef std::pair<std::string, unsigned> fileline_t;
		typedef std::function<void (unsigned module_id)> request_metadata_t;

	public:
		symbol_resolver(std::shared_ptr<const tables::modules> modules,
			std::shared_ptr<const tables::module_mappings> mappings);

		virtual const std::string &symbol_name_by_va(long_address_t address) const;
		virtual bool symbol_fileline_by_va(long_address_t address, fileline_t &result) const;

	public:
		wpl::signal<void ()> invalidate;

	public:
		typedef std::map<unsigned int /*rva*/, const symbol_info *> ordered_symbols_map_t;
		typedef containers::unordered_map<unsigned int /*file_id*/, std::string /*file*/> file_lines_map_t;

	private:
		const symbol_info *find_symbol_by_va(long_address_t address, unsigned int &module_id) const;

	private:
		std::string _empty;
		const std::shared_ptr<const tables::modules> _modules;
		const std::shared_ptr<const tables::module_mappings> _mappings;

		mutable containers::unordered_map<unsigned int /*module_id*/, ordered_symbols_map_t> _symbols_ordered;
		mutable containers::unordered_map<unsigned int /*module_id*/, const file_lines_map_t *> _file_lines;
		mutable containers::unordered_map< unsigned int /*module_id*/, std::shared_ptr<void> > _requests;
	};
}
