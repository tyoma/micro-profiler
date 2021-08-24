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

#include <frontend/image_patch_model.h>

#include <common/formatting.h>
#include <frontend/tables.h>

using namespace std;

namespace micro_profiler
{
	image_patch_model::flattener::nested_const_iterator image_patch_model::flattener::begin(const tables::modules::value_type &from)
	{	return from.second.symbols.begin();	}

	image_patch_model::flattener::nested_const_iterator image_patch_model::flattener::end(const tables::modules::value_type &from)
	{	return from.second.symbols.end();	}

	image_patch_model::flattener::const_reference image_patch_model::flattener::get(const tables::modules::value_type &l1, const symbol_info &l2)
	{
		record_type r = {	{	l1.first, l2.rva	}, &l2	};
		return r;
	}


	image_patch_model::image_patch_model(shared_ptr<const tables::patches> patches,
		shared_ptr<const tables::modules> modules, shared_ptr<const tables::module_mappings> mappings)
		: _patches(patches), _modules(modules), _flatten_view(*modules), _ordered_view(_flatten_view)
	{
		auto invalidate_me = [this] {
			_ordered_view.fetch();
			invalidate(npos());
		};

		_connections[0] = patches->invalidated += invalidate_me;
		_connections[1] = modules->invalidated += invalidate_me;
		_connections[2] = mappings->invalidated += invalidate_me;
	}

	image_patch_model::index_type image_patch_model::get_count() const throw()
	{	return _ordered_view.size();	}

	void image_patch_model::get_text(index_type row, index_type column, agge::richtext_t &value) const
	{
		const auto &record = _ordered_view[row];

		switch (column)
		{
		case 0:	itoa<16>(value, record.symbol->rva, 8);	break;
		case 1:	value.append(record.symbol->name.begin(), record.symbol->name.end());	break;
		case 2:	format_state(value, record.first);	break;
		case 3:	itoa<10>(value, record.symbol->size);	break;
		}
	}

	template <typename KeyT>
	const tables::patch *image_patch_model::find_patch(const KeyT &key) const
	{
		const auto l1 = _patches->find(key.persistent_id);

		if (_patches->end() != l1)
		{
			const auto l2 = l1->second.find(key.rva);

			if (l1->second.end() != l2)
				return &l2->second;
		}
		return nullptr;
	}

	template <typename KeyT>
	void image_patch_model::format_state(agge::richtext_t &value, const KeyT &key) const
	{
		if (const auto patch = find_patch(key))
		{
			auto &state = patch->state;

			if (state.error)
				value << "error";
			else if (state.requested && state.active)
				value << "removing";
			else if (state.requested && !state.active)
				value << "applying";
			else if (state.active)
				value << "active";
			else
				value << "inactive";
		}
	}
}
