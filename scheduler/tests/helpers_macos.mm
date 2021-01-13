#include "helpers.h"

#include <Cocoa/Cocoa.h>
#include <common/time.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		class message_loop_macos : public message_loop
		{
		public:
			message_loop_macos()
				: _run_loop([NSRunLoop currentRunLoop]), _thread([NSThread currentThread]), _exit(false)
			{
				_keep_loop = [[NSTimer timerWithTimeInterval:FLT_MAX repeats:false block: ^(NSTimer *timer) {
				}] retain];
				[_run_loop addTimer:_keep_loop forMode:NSDefaultRunLoopMode];
			}
			
			~message_loop_macos()
			{	[_keep_loop release];	}

			virtual void run() override
			{
				while (!_exit && [_run_loop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]])
				{	}				
			}

			virtual void exit() override
			{
				_exit = true;
				[_keep_loop performSelector:@selector(invalidate) onThread:_thread withObject:nil waitUntilDone:false];
			}

		private:
			NSRunLoop *_run_loop;
			NSTimer *_keep_loop;
			NSThread *_thread;
			bool _exit;
		};



		shared_ptr<message_loop> message_loop::create()
		{	return make_shared<message_loop_macos>();	}


		function<mt::milliseconds ()> get_clock()
		{	return [] {	return mt::milliseconds(micro_profiler::clock());	};	}

		function<mt::milliseconds ()> create_stopwatch()
		{
			const auto myclock = get_clock();
			const auto previous = make_shared<mt::milliseconds>(myclock());

			return [myclock, previous] () -> mt::milliseconds {
				const auto now = myclock();
				const auto d = now - *previous;

				return *previous = now, d;
			};
		}
	}
}
