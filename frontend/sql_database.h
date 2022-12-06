#pragma once

#include "sql_insert.h"
#include "sql_select.h"

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
		inline inserter<T> transaction::insert(const char * table_name)
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
