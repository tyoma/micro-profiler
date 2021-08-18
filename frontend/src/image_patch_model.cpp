#include <frontend/image_patch_model.h>

#include <common/formatting.h>
#include <common/path.h>
#include <map>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class by_rva
		{
		public:
			template <typename T1, typename T2>
			bool operator ()(const T1 &lhs_key, const T2 &/*lhs_value*/, const T1 &rhs_key, const T2 &/*rhs_value*/) const
			{	return lhs_key.second < rhs_key.second;	}
		};

		class by_persistent_id
		{
		public:
			template <typename T1, typename T2>
			bool operator ()(const T1 &lhs_key, const T2 &/*lhs_value*/, const T1 &rhs_key, const T2 &/*rhs_value*/) const
			{	return lhs_key.first < rhs_key.first;	}
		};

		class by_name
		{
		public:
			template <typename T1, typename T2>
			bool operator ()(const T1 &/*lhs_key*/, const T2 &lhs_value, const T1 &/*rhs_key*/, const T2 &rhs_value) const
			{	return lhs_value.name < rhs_value.name;	}
		};

		class by_size
		{
		public:
			template <typename T1, typename T2>
			bool operator ()(const T1 &/*lhs_key*/, const T2 &lhs_value, const T1 &/*rhs_key*/, const T2 &rhs_value) const
			{	return lhs_value.size < rhs_value.size;	}
		};
	}

	image_patch_model::image_patch_model(shared_ptr<const tables::patches> patches,
			shared_ptr<const tables::modules> modules, shared_ptr<const tables::module_mappings> mappings)
		: _patches(patches), _modules(modules), _mappings(mappings), _flatten(*modules), _filter(_flatten),
			_ordered(_filter)
	{
		auto on_invalidate = [this] {
			_ordered.fetch();
			invalidate(npos());
		};
		auto update_mapping_index = [this] {
			_mappings_index.clear();
			for (auto i = _mappings->begin(); i != _mappings->end(); ++i)
				_mappings_index[i->second.persistent_id] = i, _modules->request_presence(i->second.persistent_id);
		};

		_connections.push_back(_patches->invalidated += on_invalidate);
		_connections.push_back(_modules->invalidated += on_invalidate);
		_connections.push_back(_mappings->invalidated += [on_invalidate, update_mapping_index] {
			update_mapping_index();
			on_invalidate();
		});
		update_mapping_index();
	}

	image_patch_model::index_type image_patch_model::get_count() const throw()
	{	return _ordered.get_count();	}

	shared_ptr<const wpl::trackable> image_patch_model::track(index_type row) const
	{	return _ordered.track(row);	}

	void image_patch_model::get_text(index_type row, index_type column, agge::richtext_t &value) const
	{
		const auto &record = _ordered[row];

		switch (column)
		{
		case 0: // rva
			itoa<16>(value, record.second.rva, 8);
			break;

		case 1: // name
			add_styles(value, record.first.first, record.first.second);
			value.append(record.second.name.begin(), record.second.name.end());
			break;

		case 2: // size
			itoa<10>(value, record.second.size);
			break;

		case 3: // module's persistent_id
			itoa<10>(value, record.first.first);
			break;

		case 4: // module's name
			get_module_name(value, record.first.first);
			break;
		}
	}

	void image_patch_model::sort_by(wpl::columns_model::index_type column, bool ascending)
	{
		switch (column)
		{
		case 0:	_ordered.set_order(by_rva(), ascending);	break;
		case 1:	_ordered.set_order(by_name(), ascending);	break;
		case 2:	_ordered.set_order(by_size(), ascending);	break;
		case 3:	_ordered.set_order(by_persistent_id(), ascending);	break;
		}
		invalidate(npos());
	}

	void image_patch_model::set_filter(const string &filter)
	{
		if (filter.empty())
		{
			_filter.set_filter();
		}
		else
		{
			_filter.set_filter([filter] (const flattener::value_type &e) {
				return string::npos != e.second.name.find(filter);
			});
		}
		_ordered.fetch();
		invalidate(npos());
	}

	void image_patch_model::patch(const vector<index_type> &items, bool apply)
	{
		map<unsigned int /*persistent_id*/, vector<unsigned int> /*rva*/> s;

		for (auto i = items.begin(); i != items.end(); ++i)
		{
			const auto &record = _ordered[*i];

			s[record.first.first].push_back(record.first.second);
		}
		for (auto i = s.begin(); i != s.end(); ++i)
		{
			if (apply)
				_patches->apply(i->first, range<unsigned int, size_t>(i->second.data(), i->second.size()));
			else
				_patches->revert(i->first, range<unsigned int, size_t>(i->second.data(), i->second.size()));
		}
	}

	void image_patch_model::get_module_name(agge::richtext_t &value, unsigned int persistent_id) const
	{
		const auto i = _mappings_index.find(persistent_id);

		if (_mappings_index.end() != i)
		{
			auto name = *i->second->second.path;

			value.append(name.begin(), name.end());
		}
	}

	void image_patch_model::add_styles(agge::richtext_t &value, unsigned int persistent_id, unsigned int rva) const
	{
		const auto i = _patches->find(persistent_id);

		if (i == _patches->end())
			return;

		const auto j = i->second.find(rva);

		if (j == i->second.end())
			return;

		if (j->second.state.active)
			value << agge::style::weight(agge::bold);
		if (j->second.state.requested)
			value << agge::style::italic(true);
		if (j->second.state.error)
			value << "(E) ";
	}
}
