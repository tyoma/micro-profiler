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

#include <common/file_stream.h>

#include <common/string.h>
#include <stdio.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
#ifdef _WIN32
		FILE *fopen(const string &path, const string &mode)
		{	return ::_wfopen(unicode(path).c_str(), unicode(mode).c_str());	}
#else
		FILE *fopen(const string &path, const string &mode)
		{	return ::fopen(path.c_str(), mode.c_str());	}
#endif
	}


	file_not_found_exception::file_not_found_exception(const string &path)
		: runtime_error("File '" + path + "' not found!")
	{	}


	read_file_stream::read_file_stream(const string &path)
		: _stream(fopen(path, "rb"))
	{
		if (!_stream)
			throw file_not_found_exception(path);
	}

	read_file_stream::~read_file_stream()
	{	fclose(static_cast<FILE *>(_stream));	}

	size_t read_file_stream::read_l(void *buffer, size_t buffer_length)
	{	return fread(buffer, 1, buffer_length, static_cast<FILE *>(_stream));	}

	void read_file_stream::read(void *buffer, size_t buffer_length)
	{
		if (read_l(buffer, buffer_length) < buffer_length)
			throw runtime_error("reading past end of file");
	}

	void read_file_stream::skip(size_t n)
	{	fseek(static_cast<FILE *>(_stream), static_cast<int>(n), SEEK_CUR);	}


	write_file_stream::write_file_stream(const string &path)
		: _stream(fopen(path, "wb"))
	{
	}

	write_file_stream::~write_file_stream()
	{	fclose(static_cast<FILE *>(_stream));	}

	void write_file_stream::write(const void *buffer, size_t buffer_length)
	{	fwrite(buffer, 1, buffer_length, static_cast<FILE *>(_stream));	}
}
