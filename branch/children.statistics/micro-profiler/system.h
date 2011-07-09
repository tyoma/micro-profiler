//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

namespace micro_profiler
{
	__declspec(dllexport) unsigned __int64 timestamp_precision();
	unsigned int current_thread_id();
	void yield();

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
