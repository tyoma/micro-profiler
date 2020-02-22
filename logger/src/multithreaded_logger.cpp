//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <logger/multithreaded_logger.h>

#include <stdio.h>
#include <string.h>

using namespace std;

namespace micro_profiler
{
	namespace log
	{
		namespace
		{
			void format_datetime(buffer_t &buffer, datetime dt)
			{
				const size_t l = buffer.size();

				buffer.resize(l + 21 /*compact ISO8601 with milliseconds + null-terminator*/);
				sprintf(&buffer[l], "%04u%02u%02uT%02u%02u%02u.%03uZ",
					dt.year + 1900, dt.month, dt.day,
					dt.hour, dt.minute, dt.second, dt.millisecond % 1000);
				buffer.pop_back();
			}

			void append_string(buffer_t &buffer, const char *text)
			{
				if (const size_t l = strlen(text))
					buffer.insert(buffer.end(), text, text + l);
			}
		}


		multithreaded_logger::multithreaded_logger(const writer_t &writer, const time_provider_t &time_provider)
			: _writer(writer), _time_provider(time_provider)
		{	e(*this, "---------- Application logging started.", info);	}

		multithreaded_logger::~multithreaded_logger()
		{	e(*this, "---------- Application logging complete. Bye!", info);	}

		void multithreaded_logger::begin(const char *message, level level_) throw()
		try
		{
			buffer_t &b = get_buffer();

			format_datetime(b, _time_provider());
			b.resize(b.size() + 3, ' ');
			b[b.size() - 2] = "SI"[level_];
			append_string(b, message);
			b.push_back('\n');
		}
		catch (const bad_alloc &)
		{
			_writer("Memory allocation failure in logger (begin)!");
		}
		catch (...)
		{
			_writer("Unexpected exception in logger (begin)!");
		}

		void multithreaded_logger::add_attribute(const attribute &a) throw()
		try
		{
			buffer_t &b = get_buffer();

			if (b.empty())
				return;
			b.push_back('\t');
			append_string(b, a.name);
			b.push_back(':');
			b.push_back(' ');
			a.format_value(b);
			b.push_back('\n');
		}
		catch (const bad_alloc &)
		{
			_writer("Memory allocation failure in logger (add_attribute)!");
		}
		catch (...)
		{
			_writer("Unexpected exception in logger (add_attribute)!");
		}

		void multithreaded_logger::commit() throw()
		try
		{
			buffer_t &b = get_buffer();

			b.push_back(0);
			_writer(&b[0]);
			b.clear();
		}
		catch (const bad_alloc &)
		{
			_writer("Memory allocation failure in logger (commit)!");
		}
		catch (...)
		{
			_writer("Unexpected exception in logger (commit)!");
		}

		buffer_t &multithreaded_logger::get_buffer()
		{
			buffer_t *p = _buffers.get();

			if (!p)
			{
				mt::lock_guard<mt::mutex> lock(_construction_mutex);

				p = &*_buffer_container.insert(_buffer_container.end(), buffer_t());
				_buffers.set(p);
			}
			return *p;
		}
	}
}
