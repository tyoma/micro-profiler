#pragma once

namespace micro_profiler
{
	unsigned __int64 timestamp();
	unsigned __int64 timestamp_precision();
	unsigned int current_thread_id();

	class tls
	{
		unsigned int _tls_index;

		tls(const tls &other);
		const tls &operator =(const tls &rhs);

	public:
		tls();
		~tls();

		void *get() const;
		void set(void *value);
	};

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

		scoped_lock(const scoped_lock &other);
		const scoped_lock &operator =(const scoped_lock &rhs);

	public:
		scoped_lock(mutex &mtx) throw();
		~scoped_lock() throw();
	};


	inline scoped_lock::scoped_lock(mutex &mtx) throw()
		: _mutex(mtx)
	{	_mutex.enter();	}

	inline scoped_lock::~scoped_lock() throw()
	{	_mutex.leave();	}
}
