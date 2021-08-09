#include <frontend/image_patch_model.h>

#include <common/formatting.h>

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

	image_patch_model::image_patch_model(shared_ptr<symbol_resolver> resolver, const request_patch_t &requestor)
		: _resolver(resolver), _requestor(requestor), _flatten(resolver->get_metadata_map()), _filter(_flatten),
			_ordered(_filter)
	{
		_uinvalidate = _resolver->invalidate += [this] {
			_ordered.fetch();
			invalidate(npos());
		};
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
		case 0:
			itoa<16>(value, record.second.rva, 8);
			break;

		case 1:
			value.append(record.second.name.begin(), record.second.name.end());
			break;

		case 2:
			itoa<10>(value, record.second.size);
			break;

		case 3:
			itoa<10>(value, record.first.first);
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
			_ordered.fetch();
			invalidate(npos());
		}
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
			_requestor(i->first, i->second, apply);
	}
}
