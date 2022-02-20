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

#include "primitives.h"

#include <memory>
#include <vector>
#include <wpl/models.h>
#include <wpl/layout.h>

namespace wpl
{
	struct combobox;
	class factory;
	struct listview;
}

namespace micro_profiler
{
	class function_hint;
	class headers_model;
	struct hive;
	class piechart;
	struct profiling_session;
	class symbol_resolver;

	class tables_ui : public wpl::stack
	{
	public:
		tables_ui(const wpl::factory &factory_, const profiling_session &session, hive &configuration);

		void dump(std::string &content) const;
		void save(hive &configuration);

	public:
		wpl::signal<void(const std::string &file, unsigned line)> open_source;

	private:
		struct models;

	private:
		void init_layout(const wpl::factory &factory_);
		void attach(std::shared_ptr<symbol_resolver> resolver, std::shared_ptr<models> m);

	private:
		const std::shared_ptr<wpl::combobox> _filter_selector;
		const std::shared_ptr<piechart> _main_piechart, _callees_piechart;
		const std::shared_ptr<function_hint> _main_hint, _callees_hint;
		const std::shared_ptr<wpl::listview> _main_view, _callers_view, _callees_view;

		const std::shared_ptr<headers_model> _cm_main;
		const std::shared_ptr<headers_model> _cm_parents;
		const std::shared_ptr<headers_model> _cm_children;

		std::shared_ptr<models> _models;
		std::function<void (std::string &content)> _dump_main;

		wpl::slot_connection _filter_connection;
		std::vector<wpl::slot_connection> _connections;
	};
}
