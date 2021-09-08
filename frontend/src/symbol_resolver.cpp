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
#include <frontend/tables.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct by_address
		{
			typedef pair<unsigned, mapped_module_identified> value_type;

			bool operator ()(const value_type &lhs, const value_type &rhs) const
			{	return lhs.second.base < rhs.second.base;	}

			bool operator ()(const value_type &lhs, long_address_t rhs) const
			{	return lhs.second.base < rhs;	}

			bool operator ()(long_address_t lhs, const value_type &rhs) const
			{	return lhs < rhs.second.base;	}
		};

		template <typename T, typename V>
		const typename T::value_type *find_range(const T &container, const V &value)
		{
			auto i = container.upper_bound(value);

			return i != container.begin() ? &*--i : nullptr;
		}

		template <typename T, typename V, typename PredicateT>
		const typename T::value_type *find_range(const T &container, const V &value, const PredicateT &predicate)
		{
			auto i = upper_bound(container.begin(), container.end(), value, predicate);

			return i != container.begin() ? &*--i : nullptr;
		}
	}


	symbol_resolver::symbol_resolver(shared_ptr<const tables::modules> modules,
			shared_ptr<const tables::module_mappings> mappings)
		: _modules(modules), _mappings(mappings)
	{
		auto on_invalidate_mappings = [this] {
			_mappings_ordered.clear();
			for (auto i = _mappings->begin(); i != _mappings->end(); ++i)
			{
				_mappings_ordered.push_back(*i);
				_symbols_ordered[i->first].clear();
			}
			sort(_mappings_ordered.begin(), _mappings_ordered.end(), by_address());
		};

		_modules_invalidation = _modules->invalidate += [this] {	invalidate();	};
		_mappings_invalidation = _mappings->invalidate += on_invalidate_mappings;
		on_invalidate_mappings();
	}

	const string &symbol_resolver::symbol_name_by_va(long_address_t address) const
	{
		const module_info_metadata *m;
		const symbol_info *i = find_symbol_by_va(address, m);

		return i ? i->name : _empty;
	}

	bool symbol_resolver::symbol_fileline_by_va(long_address_t address, fileline_t &result) const
	{
		const module_info_metadata *m;
		
		if (const auto symbol = find_symbol_by_va(address, m))
		{
			const auto file = m->source_files.find(symbol->file_id);

			if (file != m->source_files.end())
				return result.first = file->second, result.second = symbol->line, true;
		}
		return false;
	}

	const symbol_info *symbol_resolver::find_symbol_by_va(long_address_t address,
		const module_info_metadata *&module) const
	{
		if (const auto m = find_range(_mappings_ordered, address, by_address()))
		{
			if (const auto symbol = find_symbol_by_rva(m->second.persistent_id, m->first, static_cast<unsigned int>(address - m->second.base), module))
				return symbol;
			_modules->request_presence(m->second.persistent_id);
		}
		return nullptr;
	}

	const symbol_info *symbol_resolver::find_symbol_by_rva(unsigned int persistent_id, unsigned int instance_id,
		unsigned int rva, const module_info_metadata *&module) const
	{
		const auto i = _modules->find(persistent_id);

		if (i != _modules->end())
		{
			auto &cached_symbols = _symbols_ordered[instance_id];

			if (cached_symbols.empty())
			{
				for (auto j = i->second.symbols.begin(); j != i->second.symbols.end(); ++j)
					cached_symbols[j->rva] = &*j;
			}

			if (const auto r = find_range(cached_symbols, rva))
			{
				if (rva - r->second->rva < r->second->size)
					return module = &i->second, r->second;
			}
		}
		return nullptr;
	}
}
