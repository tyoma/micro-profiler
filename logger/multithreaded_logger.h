//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "log.h"

#include <common/time.h>
#include <functional>
#include <list>
#include <mt/mutex.h>
#include <mt/tls.h>

namespace micro_profiler
{
	namespace log
	{
		class multithreaded_logger : public logger, noncopyable
		{
		public:
			typedef std::function<void (const char *text)> writer_t;
			typedef std::function<datetime ()> time_provider_t;

		public:
			multithreaded_logger(const writer_t &writer, const time_provider_t &time_provider);
			~multithreaded_logger();

			virtual void begin(const char *message, level level_) throw();
			virtual void add_attribute(const attribute &a) throw();
			virtual void commit() throw();

		private:
			buffer_t &get_buffer();

		private:
			const writer_t _writer;
			const time_provider_t _time_provider;
			std::list<buffer_t> _buffer_container;
			mt::tls<buffer_t> _buffers;
			mt::mutex _construction_mutex;
		};
	}
}
