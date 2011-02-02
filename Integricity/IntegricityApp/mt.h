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
		virtual void close() volatile = 0;
	};

	class thread : noncopyable
	{
		void *_thread;

	public:
		thread(const std::function<void()> &job);
		virtual ~thread() throw();
	};
}
