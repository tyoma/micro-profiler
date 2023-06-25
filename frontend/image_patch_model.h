//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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
#include "table_model_impl.h"

#include <tuple>
#include <views/filter.h>
#include <views/flatten.h>
#include <views/ordered.h>
#include <wpl/models.h>

namespace micro_profiler
{
	template <typename KeyT>
	class selection;

	template <typename UnderlyingT>
	class trackables_provider;


	class image_patch_model : public wpl::richtext_table_model, noncopyable
	{
	public:
		struct record_type
		{
			symbol_key first;
			const symbol_info *symbol;
		};

	public:
		image_patch_model(std::shared_ptr<const tables::patches> patches, std::shared_ptr<const tables::modules> modules,
			std::shared_ptr<const tables::module_mappings> mappings);

		template <typename Predicate>
		void set_filter(const Predicate &predicate);
		void set_filter();
		void set_order(index_type column, bool ascending);
		std::shared_ptr< selection<selected_symbol> > create_selection() const;

		virtual index_type get_count() const throw() override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type row) const override;
		virtual void get_text(index_type row, index_type column, agge::richtext_t &value) const override;

	private:
		struct flattener
		{
			typedef record_type const_reference;
			typedef record_type value_type;
			typedef std::vector<symbol_info>::const_iterator const_iterator;

			static std::pair<const_iterator, const_iterator> equal_range(const tables::modules::value_type &from);
			static const_reference get(const tables::modules::value_type &l1, const symbol_info &l2);
		};

		typedef views::flatten<tables::modules, flattener> flatten_view_t;
		typedef views::filter<flatten_view_t> filter_view_t;
		typedef views::ordered<filter_view_t> ordered_view_t;

	private:
		void request_missing(const tables::module_mappings &mappings);

		void fetch();

		template <typename KeyT>
		void format_state(agge::richtext_t &value, const KeyT &key) const;

		void format_module_name(agge::richtext_t &value, unsigned int module_id) const;
		void format_module_path(agge::richtext_t &value, unsigned int module_id) const;

	private:
		const std::shared_ptr<const tables::patches> _patches;
		const std::shared_ptr<const tables::modules> _modules;
		containers::unordered_map<unsigned int /*module_id*/, std::string> _module_paths;
		flatten_view_t _flatten_view;
		filter_view_t _filter_view;
		ordered_view_t _ordered_view;
		const std::shared_ptr< trackables_provider<ordered_view_t> > _trackables;
		wpl::slot_connection _connections[2];
		containers::unordered_map< unsigned int, std::shared_ptr<void> > _requests;
	};



	template <typename Predicate>
	inline void image_patch_model::set_filter(const Predicate &predicate)
	{
		_filter_view.set_filter(predicate);
		_ordered_view.fetch();
		fetch();
	}

	inline void image_patch_model::set_filter()
	{
		_filter_view.set_filter();
		_ordered_view.fetch();
		fetch();
	}
}
