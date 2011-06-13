#pragma once

#include <functional>
#include <memory>

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

		virtual wait_status wait(unsigned int timeout = infinite) volatile = 0;
	};


	class event_flag : waitable
	{
		void *_handle;

	public:
		explicit event_flag(bool raised, bool auto_reset);
		~event_flag();

		void raise();
		void lower();

		virtual wait_status wait(unsigned int timeout = infinite) volatile;
	};


	class thread
	{
		unsigned int _id;
		void *_thread;

	public:
		typedef std::function<void()> action;

	public:
		explicit thread(const action &job);
		virtual ~thread() throw();

		static std::auto_ptr<thread> run(const action &initializer, const action &job);

		unsigned int id() const throw();
	};


	// thread - inline definitions
	inline unsigned int thread::id() const throw()
	{	return _id;	}
}
