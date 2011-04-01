#pragma once

namespace micro_profiler
{
	unsigned __int64 timestamp();
	unsigned __int64 timestamp_precision();
	unsigned int current_thread_id();

	class mutex
	{
		char _buffer[1024];
		void *_critical_section;

		mutex(const mutex &);
		mutex &operator =(const mutex &);

	public:
		mutex();
		~mutex();

		void enter();
		void leave();
	};

	class scoped_lock
	{
		mutex &_mutex;

	public:
		scoped_lock(mutex &mtx);
		~scoped_lock();
	};


	inline scoped_lock::scoped_lock(mutex &mtx)
		: _mutex(mtx)
	{	_mutex.enter();	}

	inline scoped_lock::~scoped_lock()
	{	_mutex.leave();	}
}
