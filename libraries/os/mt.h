#pragma once

#include <functional>

namespace std
{
	using tr1::function;
}

namespace os
{
	struct waitable
	{
		enum wait_status { satisfied, timeout, abandoned };

		static const unsigned int infinite = static_cast<unsigned int>(-1);

		virtual wait_status wait(unsigned int timeout) volatile = 0;
	};


	class event_monitor : public waitable
	{
		void *_handle;

	public:
		explicit event_monitor(bool auto_reset);
		~event_monitor();

		void set();
		void reset();
		virtual wait_status wait(unsigned int timeout) volatile;
	};


	class thread
	{
		unsigned int _id;
		void *_thread;

	public:
		thread(const std::function<void()> &job);
		virtual ~thread() throw();

		unsigned int id() const throw();
	};


	// thread - inline definitions
	inline unsigned int thread::id() const throw()
	{	return _id;	}
}
