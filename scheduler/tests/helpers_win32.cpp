#include "helpers.h"

#include <common/time.h>
#include <windows.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		class message_loop_win32 : public message_loop
		{
		public:
			message_loop_win32()
				: _thread_id(::GetCurrentThreadId())
			{	}

			virtual void run() override
			{
				MSG msg = {};

				while (::GetMessage(&msg, NULL, 0, 0) && WM_QUIT != msg.message)
					::DispatchMessage(&msg);
			}

			virtual void exit() override
			{	::PostThreadMessage(_thread_id, WM_QUIT, 0, 0);	}

		private:
			unsigned _thread_id;
		};



		shared_ptr<message_loop> message_loop::create()
		{	return make_shared<message_loop_win32>();	}


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
