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

#include <common/noncopyable.h>
#include <string>
#include <wpl/layout.h>
#include <wpl/queue.h>

namespace wpl
{
	struct control_context;
	class factory;
	struct listview;
}

namespace micro_profiler
{
	class process_list;

	class attach_ui : public wpl::stack, noncopyable
	{
	public:
		attach_ui(const wpl::factory &factory, const wpl::queue &queue, const std::string &frontend_id);
		~attach_ui();

		wpl::signal<void()> close;

	private:
		void update();

	private:
		const std::shared_ptr<wpl::listview> _processes_lv;
		const std::shared_ptr<process_list> _model;
		std::vector<wpl::slot_connection> _connections;
		const wpl::queue _queue;
		const std::shared_ptr<bool> _alive;
	};
}
