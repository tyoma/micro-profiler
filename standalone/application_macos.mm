//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <scheduler/scheduler.h>
#include <scheduler/task_queue.h>
#include <stdexcept>
#include <wpl/factory.h>
#include <wpl/freetype2/font_loader.h>
#include <wpl/macos/cursor_manager.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	class macos_ui_queue : public scheduler::queue
	{
	public:
		macos_ui_queue(const wpl::clock &clock_)
			: _tasks([clock_] () -> mt::milliseconds {	return mt::milliseconds(clock_());	})
		{	}
		
		virtual void schedule(std::function<void ()> &&task, mt::milliseconds defer_by = mt::milliseconds(0)) override
		{	schedule_wakeup(_tasks.schedule(move(task), defer_by));		}
		
	private:
		void schedule_wakeup(const scheduler::task_queue::wake_up &wakeup)
		{
			if (wakeup.second)
			{
				const auto on_fire = ^(NSTimer *) {
					this->schedule_wakeup(this->_tasks.execute_ready(mt::milliseconds(50)));
				};
				
				[NSTimer scheduledTimerWithTimeInterval:0.001 * wakeup.first.count() repeats:false block:on_fire];
			}
		}
	
	private:
		scheduler::task_queue _tasks;
	};
	
	
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

	shared_ptr<hive> application::get_configuration()
	{	return file_hive::open_ini("~/.MicroProfiler/settings.ini");	}

	void application::impl::run()
	{	[_application run];	}

	void application::impl::stop()
	{	[_application stop:nil];	}


	application::application()
		: _impl(new impl)
	{
		const auto clock_ = &micro_profiler::clock;
		const auto queue = make_shared<macos_ui_queue>(clock_);
		const auto text_engine = create_text_engine();
		const factory_context context = {
			make_shared<gcontext::surface_type>(1, 1, 16),
			make_shared<gcontext::renderer_type>(2),
			text_engine,
			nullptr /*make_shared<system_stylesheet>(text_engine)*/,
			make_shared<macos::cursor_manager>(),
			clock_,
			[queue] (wpl::queue_task t, wpl::timespan defer_by) {
				return queue->schedule(move(t), mt::milliseconds(defer_by)), true;
			},
		};

		_factory = wpl::factory::create_default(context);
		_queue = queue;
		setup_factory(*_factory);
	}

	application::~application()
	{	}

	void application::run()
	{	_impl->run();	}

	void application::stop()
	{	_impl->stop();	}

	void application::clipboard_copy(const string &/*text_utf8*/)
	{	}

	void application::open_link(const wstring &/*address*/)
	{	}
}

int main()
try
{
	micro_profiler::application app;

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
