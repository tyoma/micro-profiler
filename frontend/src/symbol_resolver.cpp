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

#include <frontend/symbol_resolver.h>

#include <algorithm>
#include <common/module.h>
#include <map>

using namespace std;

namespace micro_profiler
{
	const symbol_info *symbol_resolver::module_info::find_symbol_by_va(unsigned address) const
	{
		if (symbol_index.empty())
			for (vector<symbol_info>::const_iterator i = symbols.begin(); i != symbols.end(); ++i)
				symbol_index.insert(make_pair(i->rva, &*i));

		module_info::addressed_symbols::const_iterator j = symbol_index.upper_bound(address);

		if (j != symbol_index.begin())
			--j;
		else if (j == symbol_index.end())
			return 0;
		return (j->first <= address) & (address < j->second->size + j->first) ? j->second : 0;
	}


	symbol_resolver::symbol_resolver(const request_metadata_t &requestor)
		: _requestor(requestor)
	{	}

	const string &symbol_resolver::symbol_name_by_va(long_address_t address) const
	{
		const module_info *m;
		const symbol_info *i = find_symbol_by_va(address, m);

		return i ? i->name : _empty;
	}

	bool symbol_resolver::symbol_fileline_by_va(long_address_t address, fileline_t &result) const
	{
		const module_info *m;
		const symbol_info *i = find_symbol_by_va(address, m);

		if (!i)
			return false;
		module_info::files_map::const_iterator j = m->files.find(i->file_id);

		return j != m->files.end() ? result.first = j->second, result.second = i->line, true : false;
	}

	void symbol_resolver::add_mapping(const mapped_module_identified &mapping)
	{	_mappings.insert(make_pair(mapping.base, mapping));	}

	void symbol_resolver::add_metadata(unsigned persistent_id, module_info_metadata &metadata)
	{
		module_info &m = _modules[persistent_id];

		for (vector< pair<unsigned int, string> >::const_iterator i = metadata.source_files.begin(); i != metadata.source_files.end(); ++i)
			m.files.insert(make_pair(i->first, i->second));
		swap(m.symbols, metadata.symbols);
		invalidate();
	}

	const symbol_info *symbol_resolver::find_symbol_by_va(long_address_t address, const module_info *&module) const
	{
		auto i = _mappings.upper_bound(address);

		if (i != _mappings.begin())
			--i;
		else if (i == _mappings.end())
			return 0;

		const auto m = _modules.find(i->second.persistent_id);

		if (m == _modules.end())
		{
			if (!i->second.requested)
				_requestor(i->second.persistent_id), i->second.requested = true;
			return 0;
		}
		module = &m->second;
		return module->find_symbol_by_va(static_cast<unsigned>(address - i->second.base));
	}
}
