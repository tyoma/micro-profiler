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

#include <frontend/image_patch_model.h>

#include <common/formatting.h>
#include <common/path.h>
#include <frontend/selection_model.h>
#include <frontend/trackables_provider.h>

using namespace std;

namespace micro_profiler
{
	template <>
	struct key_traits<image_patch_model::record_type>
	{
		typedef symbol_key key_type;

		template <typename T>
		static key_type get_key(const T &item)
		{	return item.first;	}
	};

	namespace
	{
		struct nocase_compare
		{
			static char toupper(char c)
			{	return ((97 <= c) & (c <= 122)) ? c - 32 : c;	}

			bool operator ()(char lhs, char rhs) const
			{	return toupper(lhs) < toupper(rhs);	}
		};

		unsigned int encode_state(const tables::patch &p)
		{	return (p.state.error << 2) + (p.state.active << 1) + (p.state.requested << 0);	}

		const char *c_patch_states[8] = {
			"inactive", "applying", "active", "removing", "error", "error", "error", "error",
		};
	}


	pair<image_patch_model::flattener::const_iterator, image_patch_model::flattener::const_iterator>
		image_patch_model::flattener::equal_range(const tables::modules::value_type &from)
	{	return make_pair(from.second.symbols.begin(), from.second.symbols.end());	}

	image_patch_model::flattener::const_reference image_patch_model::flattener::get(const tables::modules::value_type &l1, const symbol_info &l2)
	{
		record_type r = {	{	l1.first, l2.rva	}, &l2	};
		return r;
	}


	image_patch_model::image_patch_model(shared_ptr<const tables::patches> patches,
			shared_ptr<const tables::modules> modules, shared_ptr<const tables::module_mappings> mappings)
		: _patches(patches), _modules(modules), _flatten_view(*modules), _filter_view(_flatten_view),
			_ordered_view(_filter_view), _trackables(new trackables_provider<ordered_view_t>(_ordered_view))
	{
		auto invalidate_me = [this] {
			_ordered_view.fetch();
			fetch();
		};
		auto refresh = [this, mappings, invalidate_me] {
			for (auto i = mappings->begin(); i != mappings->end(); ++i)
				_module_paths[i->second.persistent_id] = i->second.path;
			request_missing(*mappings);
			invalidate_me();
		};

		_connections[0] = patches->invalidate += invalidate_me;
		_connections[1] = mappings->invalidate += refresh;

		refresh();
	}

	void image_patch_model::set_order(index_type column, bool ascending)
	{
		switch (column)
		{
		case 0:
			_ordered_view.set_order([] (const record_type &lhs, const record_type &rhs) {
				return lhs.first.rva < rhs.first.rva;
			}, ascending);
			break;

		case 1:
			_ordered_view.set_order([] (const record_type &lhs_, const record_type &rhs_) -> bool {
				auto &lhs = lhs_.symbol->name;
				auto &rhs = rhs_.symbol->name;

				return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), nocase_compare());
			}, ascending);
			break;

		case 2:
			_ordered_view.set_order([this] (const record_type &lhs_, const record_type &rhs_) -> bool {
				const auto lhs = find_patch(lhs_.first);
				const auto rhs = find_patch(rhs_.first);

				return (!lhs & !rhs) ? false : !lhs ? true : !rhs ? false : encode_state(*lhs) < encode_state(*rhs);
			}, ascending);
			break;

		case 3:
			_ordered_view.set_order([] (const record_type &lhs, const record_type &rhs) {
				return lhs.symbol->size < rhs.symbol->size;
			}, ascending);
			break;

		case 4:
			_ordered_view.set_order([this] (const record_type &lhs_, const record_type &rhs_) -> bool {
				const auto e = _module_paths.end();
				const auto lhs = _module_paths.find(lhs_.first.persistent_id);
				const auto rhs = _module_paths.find(rhs_.first.persistent_id);

				if ((lhs == e) & (rhs == e))
					return false;
				if (lhs == e)
					return true;
				if (rhs == e)
					return false;

				auto &path_lhs = lhs->second;
				const auto module_lhs = *path_lhs;
				auto &path_rhs = rhs->second;
				const auto module_rhs = *path_rhs;

				return lexicographical_compare(module_lhs, path_lhs.c_str() + path_lhs.size(),
					module_rhs, path_rhs.c_str() + path_rhs.size(), nocase_compare());
			}, ascending);
			break;

		case 5:
			_ordered_view.set_order([this] (const record_type &lhs_, const record_type &rhs_) -> bool {
				const auto e = _module_paths.end();
				const auto lhs = _module_paths.find(lhs_.first.persistent_id);
				const auto rhs = _module_paths.find(rhs_.first.persistent_id);

				if ((lhs == e) & (rhs == e))
					return false;
				if (lhs == e)
					return true;
				if (rhs == e)
					return false;

				auto &path_lhs = lhs->second;
				auto &path_rhs = rhs->second;

				return lexicographical_compare(path_lhs.begin(), path_lhs.end(), path_rhs.begin(), path_rhs.end(),
					nocase_compare());
			}, ascending);
			break;
		}
		fetch();
	}

	shared_ptr< selection<symbol_key> > image_patch_model::create_selection() const
	{
		// TODO: possibly requires ownership of _ordered_view (and inners)
		return make_shared< selection_model<ordered_view_t> >(_ordered_view);
	}

	image_patch_model::index_type image_patch_model::get_count() const throw()
	{	return _ordered_view.size();	}

	shared_ptr<const wpl::trackable> image_patch_model::track(index_type row) const
	{	return _trackables->track(row);	}

	void image_patch_model::get_text(index_type row, index_type column, agge::richtext_t &value) const
	{
		const auto &record = _ordered_view[row];

		switch (column)
		{
		case 0:	itoa<16>(value, record.symbol->rva, 8);	break;
		case 1:	value.append(record.symbol->name.begin(), record.symbol->name.end());	break;
		case 2:	format_state(value, record.first);	break;
		case 3:	itoa<10>(value, record.symbol->size);	break;
		case 4:	format_module_name(value, record.first.persistent_id);	break;
		case 5:	format_module_path(value, record.first.persistent_id);	break;
		}
	}

	void image_patch_model::request_missing(const tables::module_mappings &mappings)
	{
		for (auto i = mappings.begin(); i != mappings.end(); ++i)
		{
			auto req = _requests.insert(make_pair(i->second.persistent_id, shared_ptr<void>()));

			if (!req.second)
				continue;
			_modules->request_presence(req.first->second, i->second.persistent_id,
				[this] (const module_info_metadata &/*metadata*/) {

				_ordered_view.fetch();
				fetch();
			});
		}
	}

	void image_patch_model::fetch()
	{
		_trackables->fetch();
		invalidate(npos());
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
			value << c_patch_states[encode_state(*patch)];
	}

	void image_patch_model::format_module_name(agge::richtext_t &value, unsigned int persistent_id) const
	{
		const auto m = _module_paths.find(persistent_id);

		if (m != _module_paths.end())
			value << *m->second;
	}

	void image_patch_model::format_module_path(agge::richtext_t &value, unsigned int persistent_id) const
	{
		const auto m = _module_paths.find(persistent_id);

		if (m != _module_paths.end())
			value << m->second.c_str();
	}
}
