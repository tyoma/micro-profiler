#pragma once

#include "basics.h"

#include <functional>

namespace mt
{
	struct waitable : destructible
	{
		enum wait_status { satisfied, timeout, abandoned };

		static const unsigned int infinite = -1;

		virtual wait_status wait(unsigned int timeout) volatile = 0;
	};

	class event_monitor : public waitable, noncopyable
	{
		void *_handle;

	public:
		explicit event_monitor(bool auto_reset);
		~event_monitor();

		void set();
		void reset();
		virtual wait_status wait(unsigned int timeout) volatile;
	};

	class thread : noncopyable
	{
		void *_thread;

	public:
		thread(const std::function<void()> &job);
		virtual ~thread() throw();
	};
}
