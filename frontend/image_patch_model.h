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
#include "filter_view.h"
#include "ordered_view.h"
#include "symbol_resolver.h"

#include <wpl/models.h>

namespace micro_profiler
{
	class symbol_resolver;

	class image_patch_model : public wpl::richtext_table_model
	{
	public:
		typedef std::function<void (unsigned int persistent_id, const std::vector<unsigned int> &rva, bool apply)> request_patch_t;

	public:
		image_patch_model(std::shared_ptr<symbol_resolver> resolver, const request_patch_t &requestor);

		virtual index_type get_count() const throw() override;
		virtual std::shared_ptr<const wpl::trackable> track(index_type row) const override;
		virtual void get_text(index_type row, index_type column, agge::richtext_t &value) const override;

		void set_filter(const std::string &filter);
		void sort_by(wpl::columns_model::index_type column, bool ascending);

		void patch(const std::vector<index_type> &items, bool apply);

	private:
		struct flattener
		{
			typedef std::vector<symbol_info>::const_iterator nested_const_iterator;

			struct value_type
			{
				typedef std::pair<unsigned int /*persistent_id*/, unsigned int /*rva*/> first_type;
				typedef const symbol_info second_type;

				value_type(unsigned int persistent_id_, const symbol_info &second_)
					: first(persistent_id_, second_.rva), second(second_)
				{	}

				first_type first;
				const symbol_info &second;

			private:
				void operator =(const value_type &rhs);
			};

			template <typename T1>
			static value_type get(const T1 &v1, const symbol_info &v2)
			{	return value_type(v1.first, v2);	}

			template <typename Type>
			static nested_const_iterator begin(const Type &v)
			{	return v.second.symbols.begin();	}

			template <typename Type>
			static nested_const_iterator end(const Type &v)
			{	return v.second.symbols.end();	}
		};

	private:
		std::shared_ptr<symbol_resolver> _resolver;
		request_patch_t _requestor;
		flatten_view<symbol_resolver::modules_map_t, flattener> _flatten;
		filter_view< flatten_view<symbol_resolver::modules_map_t, flattener> > _filter;
		ordered_view< filter_view< flatten_view<symbol_resolver::modules_map_t, flattener> > > _ordered;

		wpl::slot_connection _uinvalidate;
	};
}
