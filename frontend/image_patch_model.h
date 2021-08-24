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

#pragma once

#include "flatten_view.h"
#include "ordered_view.h"
#include "tables.h"

#include <wpl/models.h>

namespace micro_profiler
{
	struct symbol_info;

	class image_patch_model : public wpl::richtext_table_model
	{
	public:
		struct record_type
		{
			struct
			{
				unsigned int persistent_id;
				unsigned int rva;
			} first;
			const symbol_info *symbol;
		};

	public:
		image_patch_model(std::shared_ptr<const tables::patches> patches, std::shared_ptr<const tables::modules> modules,
			std::shared_ptr<const tables::module_mappings> mappings);

		virtual index_type get_count() const throw() override;
		virtual void get_text(index_type row, index_type column, agge::richtext_t &value) const override;

	private:
		struct flattener
		{
			typedef record_type const_reference;
			typedef record_type value_type;
			typedef std::vector<symbol_info>::const_iterator nested_const_iterator;

			static nested_const_iterator begin(const tables::modules::value_type &from);
			static nested_const_iterator end(const tables::modules::value_type &from);
			static const_reference get(const tables::modules::value_type &l1, const symbol_info &l2);
		};

		typedef flatten_view<tables::modules, flattener> flatten_view_t;
		typedef ordered_view<flatten_view_t> ordered_view_t;

	private:
		template <typename KeyT>
		const tables::patch *find_patch(const KeyT &key) const;

		template <typename KeyT>
		void format_state(agge::richtext_t &value, const KeyT &key) const;

	private:
		std::shared_ptr<const tables::patches> _patches;
		std::shared_ptr<const tables::modules> _modules;
		flatten_view_t _flatten_view;
		ordered_view_t _ordered_view;
		wpl::slot_connection _connections[3];
	};
}
