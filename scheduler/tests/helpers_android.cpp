#include "helpers.h"

#include <android/looper.h>
#include <common/time.h>

using namespace std;

namespace scheduler
{
	namespace tests
	{
		class message_loop_android : public message_loop
		{
		public:
			message_loop_android()
				: _exit(false), _looper(ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS))
			{	ALooper_acquire(_looper);	}

			~message_loop_android()
			{	ALooper_release(_looper);	}

			virtual void run() override
			{
				int dummy[2];
				void *vdummy;

				while (!_exit && ALOOPER_POLL_ERROR != ALooper_pollOnce(-1, &dummy[0], &dummy[1], &vdummy))
				{	}
			}

			virtual void exit() override
			{	_exit = true, ALooper_wake(_looper);	}

		private:
			bool _exit;
			ALooper *_looper;
		};



		shared_ptr<message_loop> message_loop::create()
		{	return make_shared<message_loop_android>();	}


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
