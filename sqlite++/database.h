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

#include "insert.h"
#include "select.h"

namespace micro_profiler
{
	namespace sql
	{
		struct sql_error : std::runtime_error
		{
			sql_error(const std::string &text);

			static void check_step(const connection_ptr &connection, int step_result);
		};

		class transaction
		{
		public:
			enum type {	deferred, immediate,	};

		public:
			transaction(connection_ptr connection, type type_ = deferred, int timeout_ms = 500);
			~transaction();

			template <typename T>
			reader<T> select(const char *table_name);

			template <typename T, typename W>
			reader<T> select(const char *table_name, const W &where);

			template <typename T>
			inserter<T> insert(const char *table_name);

			void commit();

		private:
			connection_ptr _connection;
			bool _comitted;
		};



		inline transaction::transaction(connection_ptr connection, type type_, int timeout_ms)
			: _connection(connection), _comitted(false)
		{
			const char *begin_sql[] = {	"BEGIN DEFERRED", "BEGIN IMMEDIATE",	};
			const auto begin = create_statement(*_connection, begin_sql[type_]);

			sqlite3_busy_timeout(_connection.get(), timeout_ms);
			sql_error::check_step(_connection, sqlite3_step(begin.get()));
		}

		inline transaction::~transaction()
		{
			if (const auto commit = !_comitted ? create_statement(*_connection, "ROLLBACK") : nullptr)
				sqlite3_step(commit.get());
		}

		template <typename T>
		inline reader<T> transaction::select(const char *table_name)
		{	return select_builder<T>(table_name).create_reader(*_connection);	}

		template <typename T, typename W>
		inline reader<T> transaction::select(const char *table_name, const W &where)
		{	return select_builder<T>(table_name).create_reader(*_connection, where);	}

		template <typename T>
		inline inserter<T> transaction::insert(const char *table_name)
		{	return insert_builder<T>(table_name).create_inserter(*_connection);	}

		inline void transaction::commit()
		{
			const auto commit = create_statement(*_connection, "COMMIT");

			sql_error::check_step(_connection, sqlite3_step(commit.get()));
			_comitted = true;
		}


		inline sql_error::sql_error(const std::string &text)
			: std::runtime_error(text)
		{	}

		inline void sql_error::check_step(const connection_ptr &connection, int step_result)
		{
			if (SQLITE_DONE == step_result)
				return;
			std::string text = "SQLite error: ";
			
			text += sqlite3_errmsg(connection.get());
			text += "!";
			throw sql_error(text);
		}
	}
}
