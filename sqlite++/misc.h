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

#include <memory>
#include <sqlite3.h>

namespace micro_profiler
{
	namespace sql
	{
		struct sqlite3_deleter;
		typedef std::shared_ptr<sqlite3> connection_ptr;
		typedef std::unique_ptr<sqlite3_stmt, sqlite3_deleter> statement_ptr;


		struct sqlite3_deleter
		{
			void operator ()(sqlite3_stmt *ptr) const
			{	sqlite3_finalize(ptr);	}
		};


		inline connection_ptr create_connection(const char *path)
		{
			sqlite3 *db = nullptr;

			sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
			return connection_ptr(db, [] (sqlite3 *ptr) {	sqlite3_close(ptr);	});
		}

		inline statement_ptr create_statement(sqlite3 &database,
			const char *expression_text)
		{
			sqlite3_stmt *p = nullptr;

			sqlite3_prepare_v2(&database, expression_text, -1, &p, nullptr);
			return statement_ptr(p);
		}
	}
}
