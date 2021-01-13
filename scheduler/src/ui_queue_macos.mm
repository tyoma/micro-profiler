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

#include "../ui_queue.h"

#include <Cocoa/Cocoa.h>
#include <logger/log.h>

#define PREAMBLE "Scheduler UI Queue: "

using namespace std;

@implementation Queue : NSObject
	{
		@public scheduler::task_queue *tasks;
		NSThread *_thread;
	}
	
	-(id) init
	{
		self = [super init];
		_thread = [NSThread currentThread];
		return self;
	}
	
	-(void) dealloc
	{	[super dealloc];	}
	
	-(void) scheduleWakeup: (scheduler::task_queue::wake_up)wakeup;
	{
		if (wakeup.second)
		{
			const auto m = wakeup.first.count() ? @selector(executeReadyDefer:) : @selector(executeReady:);
			const auto defer_by = [NSNumber numberWithLongLong:0.001 * wakeup.first.count()];

			[self performSelector:m onThread:_thread withObject:defer_by waitUntilDone:false];
		}
	}
	
	-(void) executeReady: (NSObject *)nothing;
	{
		scheduler::task_queue::wake_up wakeup(mt::milliseconds(0), true);

		if (!tasks)
			return;
		try
		{	wakeup = tasks->execute_ready(mt::milliseconds(50));	}
		catch (exception &e)
		{	LOGE(PREAMBLE "exception during scheduled task processing!") % A(_thread) % A(e.what());	}
		catch (...)
		{	LOGE(PREAMBLE "unknown exception during scheduled task processing!") % A(_thread);	}
		[self scheduleWakeup:wakeup];
	}

	-(void) executeReadyDefer: (NSNumber *)defer_by;
	{	[self performSelector:@selector(executeReady:) withObject:nil afterDelay:[defer_by doubleValue]];	}
@end

namespace scheduler
{
	struct ui_queue::impl
	{
		impl(task_queue &queue_)
			: _queue([[Queue alloc] init])
		{	_queue->tasks = &queue_;	}
		
		~impl()
		{
			_queue->tasks = nullptr;
			[_queue release];
		}
		
		void schedule_wakeup(const task_queue::wake_up &wakeup)
		{	[_queue scheduleWakeup:wakeup];	}

	private:
		Queue *_queue;
	};
	


	ui_queue::ui_queue(const clock &clock_)
		: _tasks(clock_), _impl(make_shared<impl>(_tasks))
	{	}
		
	ui_queue::~ui_queue()
	{	}
		
	void ui_queue::schedule(std::function<void ()> &&task, mt::milliseconds defer_by)
	{	_impl->schedule_wakeup(_tasks.schedule(move(task), defer_by));	}
}
