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
#include "remove.h"
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
			enum type {	deferred, immediate, exclusive,	};

		public:
			transaction(connection_ptr connection, type type_ = deferred, int timeout_ms = 500);
			~transaction();

			template <typename T>
			void create_table();

			template <typename T>
			void create_table(const char *name);

			template <typename T>
			reader<T> select();

			template <typename T>
			reader<T> select(const char *table_name);

			template <typename T, typename W>
			reader<T> select(const W &where);

			template <typename T, typename W>
			reader<T> select(const W &where, const char *table_name);

			template <typename T>
			inserter<T> insert();

			template <typename T>
			inserter<T> insert(const char *table_name);

			template <typename T, typename W>
			remover remove(const W &where);

			template <typename T, typename W>
			remover remove(const W &where, const char *table_name);

			void commit();

		private:
			void execute(const char *sql_statemet);

		private:
			connection_ptr _connection;
			bool _comitted;
		};

		struct table_name_visitor
		{
			template <typename U>
			void operator ()(U &table_name_)
			{	table_name = table_name_;	}

			template <typename U, typename V>
			void operator ()(U, V) const
			{	}

			std::string table_name;
		};



		template <typename T>
		inline std::string default_table_name()
		{
			table_name_visitor v;

			describe<T>(v);
			return std::move(v.table_name);
		}

		inline transaction::transaction(connection_ptr connection, type type_, int timeout_ms)
			: _connection(connection), _comitted(false)
		{
			const char *begin_sql[] = {	"BEGIN DEFERRED", "BEGIN IMMEDIATE", "BEGIN EXCLUSIVE",	};

			sqlite3_busy_timeout(_connection.get(), timeout_ms);
			execute(begin_sql[type_]);
		}

		inline transaction::~transaction()
		{
			try
			{
				if (!_comitted)
					execute("ROLLBACK");
			}
			catch (...)
			{  }
		}

		template <typename T>
		inline void transaction::create_table()
		{	create_table<T>(default_table_name<T>().c_str());	}

		template <typename T>
		inline void transaction::create_table(const char *name)
		{
			std::string create_table_ddl;

			format_create_table<T>(create_table_ddl, name);
			execute(create_table_ddl.c_str());
		}

		template <typename T>
		inline reader<T> transaction::select()
		{	return select<T>(default_table_name<T>().c_str());	}

		template <typename T>
		inline reader<T> transaction::select(const char *table_name)
		{	return select_builder<T>(table_name).create_reader(*_connection);	}

		template <typename T, typename W>
		inline reader<T> transaction::select(const W &where)
		{	return select<T>(where, default_table_name<T>().c_str());	}

		template <typename T, typename W>
		inline reader<T> transaction::select(const W &where, const char *table_name)
		{	return select_builder<T>(table_name).create_reader(*_connection, where);	}

		template <typename T>
		inline inserter<T> transaction::insert()
		{	return insert<T>(default_table_name<T>().c_str());	}

		template <typename T>
		inline inserter<T> transaction::insert(const char *table_name)
		{	return insert_builder<T>(table_name).create_inserter(*_connection);	}

		template <typename T, typename W>
		inline remover transaction::remove(const W &where)
		{	return remove<T>(where, default_table_name<T>().c_str());	}

		template <typename T, typename W>
		inline remover transaction::remove(const W &where, const char *table_name)
		{	return remove_builder(table_name).create_statement(*_connection, where);	}

		inline void transaction::commit()
		{	execute("COMMIT");	}

		inline void transaction::execute(const char *sql_statemet)
		try
		{
			statement stmt(create_statement(*_connection, sql_statemet));

			stmt.execute();
		}
		catch (execution_error &e)
		{
			sql_error::check_step(_connection, e.code);
		}


		inline sql_error::sql_error(const std::string &text)
			: std::runtime_error(text)
		{	}

		inline void sql_error::check_step(const connection_ptr &connection, int /*step_result*/)
		{
			std::string text = "SQLite error: ";
			
			text += sqlite3_errmsg(connection.get());
			text += "!";
			throw sql_error(text);
		}
	}
}
