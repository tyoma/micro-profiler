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

#pragma once

#include "constructors.h"
#include "model_context.h"
#include "table_model_impl.h"

#include <explorer/process.h>
#include <wpl/models.h>

namespace micro_profiler
{
	template <>
	struct key_traits<process_info>
	{
		typedef id_t key_type;

		static key_type get_key(const process_info &item)
		{	return item.pid;	}
	};

	template <typename U, typename ColumnsT>
	inline std::shared_ptr< table_model_impl<wpl::richtext_table_model, U, process_model_context, process_info> > process_list(
		std::shared_ptr<U> underlying, const ColumnsT &columns)
	{
		typedef table_model_impl<wpl::richtext_table_model, U, process_model_context, process_info> model_type;
		typedef std::tuple<std::shared_ptr<model_type>, wpl::slot_connection> composite_t;

		const auto m = std::make_shared<model_type>(underlying, initialize<process_model_context>());
		const auto c = std::make_shared<composite_t>(m, underlying->invalidate += [m] {	m->fetch();	});

		m->add_columns(columns);
		return std::shared_ptr<model_type>(c, std::get<0>(*c).get());
	}
}
