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

#include <wpl/models.h>

namespace micro_profiler
{
	namespace tables
	{
		struct module_mappings;
		struct modules;
		struct patches;
	}

	class image_patch_model : public wpl::richtext_table_model
	{
	public:
//		typedef record_type;

	public:
		image_patch_model(std::shared_ptr<const tables::patches> patches, std::shared_ptr<const tables::modules> modules,
			std::shared_ptr<const tables::module_mappings> mappings);

		virtual index_type get_count() const throw() override;
		virtual void get_text(index_type row, index_type column, agge::richtext_t &value) const override;

	private:
		struct flattener
		{
			struct const_reference;
		};

		typedef flatten_view<tables::modules, flattener> flatten_view_t;
		typedef ordered_view<flatten_view_t> ordered_view_t;

	private:
//		flatten_view_t _flatten_view;
//		ordered_view_t _ordered_view;
	};
}
