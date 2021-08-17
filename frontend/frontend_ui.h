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

#include <common/types.h>
#include <string>
#include <wpl/signal.h>

namespace micro_profiler
{
	class functions_list;
	
	namespace tables
	{
		struct module_mappings;
		struct modules;
		struct patches;
	}

	struct frontend_ui_context
	{
		initialization_data process_info;
		std::shared_ptr<functions_list> model;
		std::shared_ptr<const tables::module_mappings> module_mappings;
		std::shared_ptr<const tables::modules> modules;
		std::shared_ptr<const tables::patches> patches;
	};

	struct frontend_ui
	{
		typedef std::shared_ptr<frontend_ui> ptr;

		virtual void activate() = 0;

		wpl::signal<void ()> activated;
		wpl::signal<void ()> closed;
	};

	typedef std::function<frontend_ui::ptr (const frontend_ui_context &context)> frontend_ui_factory;
}
