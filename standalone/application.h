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

#include <memory>
#include <string>
#include <vector>

namespace scheduler
{
	struct queue;
	class ui_queue;
}

namespace wpl
{
	class factory;
}

namespace micro_profiler
{
	struct hive;

	class application
	{
	public:
		application();
		~application();

		wpl::factory &get_factory();
		scheduler::queue &get_ui_queue();
		std::shared_ptr<hive> get_configuration();

		void run();
		void stop();

		void clipboard_copy(const std::string &text);
		void open_link(const std::string &address);

	private:
		class impl;

	private:
		static const std::vector<std::string> c_configuration_path;

	private:
		std::shared_ptr<wpl::factory> _factory;
		std::shared_ptr<scheduler::ui_queue> _queue;
		std::unique_ptr<impl> _impl;
		std::shared_ptr<hive> _config;
	};



	inline wpl::factory &application::get_factory()
	{	return *_factory;	}

	inline std::shared_ptr<hive> application::get_configuration()
	{	return _config;	}


	void main(application &app);
}
