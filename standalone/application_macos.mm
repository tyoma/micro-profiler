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

#include "application.h"

#include <Cocoa/Cocoa.h>
#include <common/configuration_file.h>
#include <common/constants.h>
#include <common/string.h>
#include <common/time.h>
#include <frontend/factory.h>
#include <logger/log.h>
#include <scheduler/thread_queue.h>
#include <scheduler/ui_queue.h>
#include <stdexcept>
#include <wpl/factory.h>
#include <wpl/freetype2/font_loader.h>
#include <wpl/macos/cursor_manager.h>

#define PREAMBLE "UI Queue: "

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	shared_ptr<stylesheet> create_static_stylesheet(gcontext::text_engine_type &text_engine);
	
	
	class application::impl
	{
	public:
		impl();
		~impl();

		void run();
		void stop();

		bool schedule(const queue_task &t, timespan defer_by);

	public:
	private:
		NSAutoreleasePool *_pool;
		NSApplication *_application;
	};


	application::impl::impl()
	{
		_pool = [[NSAutoreleasePool alloc] init];
		_application = [NSApplication sharedApplication];

		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp activateIgnoringOtherApps:YES];
	}

	application::impl::~impl()
	{	[_pool drain];	}

	void application::impl::run()
	{	[_application run];	}

	void application::impl::stop()
	{	[_application stop:nil];	}


	application::application(const char *argv[], size_t argc)
		: _arguments(argv, argv + argc), _impl(new impl)
	{
		const auto clock_raw = &clock;
		const auto clock_mt = [clock_raw] {	return mt::milliseconds(clock_raw());	};
		const auto queue = make_shared<scheduler::ui_queue>(clock_mt);
		const auto text_engine = create_text_engine();
		const factory_context context = {
			make_shared<gcontext::surface_type>(1, 1, 16),
			make_shared<gcontext::renderer_type>(2),
			text_engine,
			create_static_stylesheet(*text_engine),
			make_shared<macos::cursor_manager>(),
			clock_raw,
			[queue] (wpl::queue_task t, wpl::timespan defer_by) {
				return queue->schedule(move(t), mt::milliseconds(defer_by)), true;
			},
		};

		_factory = wpl::factory::create_default(context);
		_queue = queue;
		_worker_queue.reset(new scheduler::thread_queue(clock_mt));
		_config = file_hive::open_ini("~/.MicroProfiler/settings.ini");
		setup_factory(*_factory);
	}

	application::~application()
	{	_queue->stop();	}

	scheduler::queue &application::get_ui_queue()
	{	return *_queue;	}

	scheduler::queue &application::get_worker_queue()
	{	return *_worker_queue;	}

	void application::run()
	{	_impl->run();	}

	void application::stop()
	{	_impl->stop();	}

	void application::clipboard_copy(const string &/*text_utf8*/)
	{	}

	void application::open_link(const string &/*address*/)
	{	}
}

int main(int argc, const char *argv[])
try
{
	micro_profiler::application app(argv, argc);

	micro_profiler::main(app);
	return 0;
}
catch (const exception &e)
{
	printf("Caught exception: %s...\nExiting!\n", e.what());
	return -1;
}
catch (...)
{
	printf("Caught an unknown exception...\nExiting!\n");
	return -1;
}
