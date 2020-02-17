//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/image_info.h>
#include <common/protocol.h>

#include <functional>
#include <map>
#include <unordered_map>

namespace micro_profiler
{
	class symbol_resolver
	{
	public:
		typedef std::pair<std::string, unsigned> fileline_t;
		typedef std::function<void (unsigned persistent_id)> request_metadata_t;

	public:
		symbol_resolver(const request_metadata_t &requestor);
		virtual const std::string &symbol_name_by_va(long_address_t address) const;
		virtual bool symbol_fileline_by_va(long_address_t address, fileline_t &result) const;
		void add_mapping(const mapped_module_identified &mapping);
		void add_metadata(unsigned persistent_id, module_info_metadata &metadata);

	private:
		struct mapped_module_ex : mapped_module_identified
		{
			mapped_module_ex(const mapped_module_identified &mm = mapped_module_identified());

			bool requested;
		};

		struct module_info
		{
			typedef std::map<unsigned /*rva*/, const symbol_info * /*symbol*/> addressed_symbols;
			typedef std::unordered_map<unsigned int, std::string> files_map;

			const symbol_info *find_symbol_by_va(unsigned address) const;

			std::vector<symbol_info> symbols;
			files_map files;
			mutable addressed_symbols symbol_index;
		};

		typedef std::map<long_address_t /*base*/, mapped_module_ex> mappings_map;
		typedef std::unordered_map<unsigned int /*persistent_id*/, module_info> modules_map;

	private:
		const symbol_info *find_symbol_by_va(long_address_t address, const module_info *&module) const;

	private:
		request_metadata_t _requestor;
		std::string _empty;
		mutable mappings_map _mappings;
		modules_map _modules;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, symbol_resolver &data);

		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, module_info &data);
	};



	inline symbol_resolver::mapped_module_ex::mapped_module_ex(const mapped_module_identified &mmi)
		: mapped_module_identified(mmi), requested(false)
	{ }
}
