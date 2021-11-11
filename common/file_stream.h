//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "noncopyable.h"

#include <stdexcept>
#include <string>

namespace micro_profiler
{
	struct file_not_found_exception : std::runtime_error
	{
		file_not_found_exception(const std::string &path);
	};

	class read_file_stream : noncopyable
	{
	public:
		read_file_stream(const std::string &path);
		~read_file_stream();

		std::size_t read_l(void *buffer, std::size_t buffer_length);
		void read(void *buffer, std::size_t buffer_length);
		void skip(std::size_t n);

	private:
		void *_stream;
	};

	class write_file_stream : noncopyable
	{
	public:
		write_file_stream(const std::string &path);
		~write_file_stream();

		void write(const void *buffer, std::size_t buffer_length);

	private:
		void *_stream;
	};
}
