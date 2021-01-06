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

#include <common/configuration_file.h>

using namespace std;

namespace micro_profiler
{
		shared_ptr<hive> file_hive::open_ini(const char *path)
		{	return shared_ptr<hive>(new file_hive(path));	}

		file_hive::file_hive(const char * /*path*/)
		{	}

		shared_ptr<hive> file_hive::create(const char * /*name*/)
		{	return shared_from_this();	}

		shared_ptr<const hive> file_hive::open(const char * /*name*/) const
		{	return shared_from_this();	}

		void file_hive::store(const char * /*name*/, int /*value*/)
		{	}

		void file_hive::store(const char * /*name*/, const char * /*value*/)
		{	}

		bool file_hive::load(const char * /*name*/, int &/*value*/) const
		{	return false;	}

		bool file_hive::load(const char * /*name*/, string &/*value*/) const
		{	return false;	}
}
