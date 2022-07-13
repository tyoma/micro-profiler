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

#include <frontend/symbol_resolver.h>

#include <frontend/keyer.h>
#include <frontend/helpers.h>
#include <sdb/integrated_index.h>

using namespace std;

namespace micro_profiler
{
	symbol_resolver::symbol_resolver(shared_ptr<const tables::modules> modules,
			shared_ptr<const tables::module_mappings> mappings)
		: _modules(modules), _mappings(mappings)
	{	}

	const string &symbol_resolver::symbol_name_by_va(long_address_t address) const
	{
		unsigned int persistent_id;

		if (const auto symbol = find_symbol_by_va(address, persistent_id))
			return symbol->name;
		return _empty;
	}

	bool symbol_resolver::symbol_fileline_by_va(long_address_t address, fileline_t &result) const
	{
		unsigned int persistent_id;

		if (const auto symbol = find_symbol_by_va(address, persistent_id))
		{
			const auto i = _file_lines.find(persistent_id);

			if (_file_lines.end() != i)
			{
				const auto j = i->second->find(symbol->file_id);

				if (i->second->end() != j)
					return result.first = j->second, result.second = symbol->line, true;
			}
		}
		return false;
	}

	const symbol_info *symbol_resolver::find_symbol_by_va(long_address_t address, unsigned int &persistent_id) const
	{
		if (const auto mapping = find_range(sdb::ordered_index_(*_mappings, keyer::base()), address))
		{
			auto m = _symbols_ordered.find(persistent_id = mapping->persistent_id);

			if (_symbols_ordered.end() == m)
			{
				const auto i = _requests.insert(make_pair(mapping->persistent_id, shared_ptr<void>()));

				if (i.second)
				{
					_modules->request_presence(i.first->second, mapping->persistent_id,
						[this, i] (const module_info_metadata &metadata) {

						auto &cached_symbols = _symbols_ordered[i.first->first];

						for (auto j = metadata.symbols.begin(); j != metadata.symbols.end(); ++j)
							cached_symbols[j->rva] = &*j;
						_file_lines[i.first->first] = &metadata.source_files;
						invalidate();
						_requests.erase(i.first);
					});
					m = _symbols_ordered.find(mapping->persistent_id);
				}
			}

			if (_symbols_ordered.end() != m)
			{
				auto &cached_symbols = m->second;
				const auto rva = static_cast<unsigned int>(address - mapping->base);

				if (const auto candidate = find_range(cached_symbols, rva))
				{
					if (rva - candidate->second->rva < candidate->second->size)
						return candidate->second;
				}
			}
		}
		return nullptr;
	}
}
