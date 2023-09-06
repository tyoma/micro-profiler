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

#include "database_views.h"
#include "model_context.h"
#include "table_model_impl.h"

#include <tuple>
#include <views/filter.h>
#include <wpl/models.h>

namespace micro_profiler
{
	struct image_patch_model_context;

	template <typename KeyT>
	class selection;

	template <>
	struct key_traits<tables::patched_symbols::value_type>
	{
		typedef symbol_key key_type;

		static key_type get_key(const tables::patched_symbol_adaptor &item)
		{	return std::make_tuple(item.symbol().module_id, item.symbol().rva);	}
	};


	class image_patch_model : public table_model_impl<wpl::richtext_table_model, views::filter<tables::patched_symbols>,
		image_patch_model_context>
	{
	public:
		typedef views::filter<tables::patched_symbols> filter_view_t;

	private:
		image_patch_model(std::shared_ptr<filter_view_t> underlying, std::shared_ptr<const tables::modules> modules);

	public:
		static std::shared_ptr<image_patch_model> create(std::shared_ptr<const tables::patches> patches,
			std::shared_ptr<const tables::modules> modules, std::shared_ptr<const tables::module_mappings> mappings,
			std::shared_ptr<const tables::symbols> symbols, std::shared_ptr<const tables::source_files> source_files);

		template <typename Predicate>
		void set_filter(const Predicate &predicate);
		void set_filter();

		static wpl::slot_connection maintain_legacy_symbols(tables::modules &modules,
			std::shared_ptr<tables::symbols> symbols, std::shared_ptr<tables::source_files> source_files);

	private:
		void request_missing(const tables::module_mappings &mappings);

	private:
		const std::shared_ptr<filter_view_t> _underlying;
		const std::shared_ptr<const tables::modules> _modules;
		wpl::slot_connection _connections[3];
		containers::unordered_map< id_t, std::shared_ptr<void> > _requests;
	};



	template <typename Predicate>
	inline void image_patch_model::set_filter(const Predicate &predicate)
	{
		_underlying->set_filter(predicate);
		fetch();
	}

	inline void image_patch_model::set_filter()
	{
		_underlying->set_filter();
		fetch();
	}
}
