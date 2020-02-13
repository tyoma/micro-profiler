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

#include <common/symbol_resolver.h>

#include <algorithm>
#include <common/module.h>
#include <common/protocol.h>
#include <map>

using namespace std;

namespace micro_profiler
{
	symbol_resolver::~symbol_resolver()
	{	}

	const string &symbol_resolver::symbol_name_by_va(long_address_t address) const
	{
		cached_names_map::const_iterator i = find_symbol_by_va(address);

		return i != _mapped_symbols.end() ? i->second.name : _empty;
	}

	bool symbol_resolver::symbol_fileline_by_va(long_address_t address, fileline_t &result) const
	{
		cached_names_map::const_iterator i = find_symbol_by_va(address);

		if (i == _mapped_symbols.end())
			return false;
		files_map::const_iterator j = _files.find(i->second.file_id);

		return j != _files.end() ? result.first = j->second, result.second = i->second.line, true : false;
	}

	void symbol_resolver::add_metadata(const mapped_module &basic, const module_info_metadata &metadata)
	{
		const unsigned int file_id_base = _files.empty() ? 0u : max_element(_files.begin(), _files.end())->first;

		for (vector<symbol_info>::const_iterator i = metadata.symbols.begin(); i != metadata.symbols.end(); ++i)
		{
			symbol_info s = *i;

			s.file_id += file_id_base;
			_mapped_symbols.insert(make_pair(basic.load_address + i->rva, s));
		}

		for (vector< pair<unsigned int, string> >::const_iterator i = metadata.source_files.begin();
			i != metadata.source_files.end(); ++i)
		{
			_files.insert(make_pair(file_id_base + i->first, i->second));
		}
	}

	symbol_resolver::cached_names_map::const_iterator symbol_resolver::find_symbol_by_va(
		long_address_t address) const
	{
		cached_names_map::const_iterator i = _mapped_symbols.upper_bound(address);

		if (i != _mapped_symbols.begin())
			--i;
		else if (i == _mapped_symbols.end())
			return _mapped_symbols.end();
		return (i->first <= address) & (address < i->second.size + i->first) ? i : _mapped_symbols.end();
	}
}
