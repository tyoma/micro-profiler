#pragma once

#include "sql_expression.h"
#include "sql_misc.h"
#include "sql_types.h"

#include <cstdint>
#include <string>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		struct field_binder
		{
			void operator ()(int T::*field, const char *)
			{	sqlite3_bind_int(&statement, index++, item.*field);	}

			void operator ()(std::int64_t T::*field, const char *)
			{	sqlite3_bind_int64(&statement, index++, item.*field);	}

			void operator ()(std::string T::*field, const char *)
			{	sqlite3_bind_text(&statement, index++, (item.*field).c_str(), -1, SQLITE_STATIC);	}

			void operator ()(double T::*field, const char *)
			{	sqlite3_bind_double(&statement, index++, item.*field);	}

			template <typename U>
			void operator ()(U, const char *)
			{	}

			sqlite3_stmt &statement;
			const T &item;
			int index;
		};

		template <typename T>
		struct primary_key_binder
		{
			template <typename F>
			void operator ()(const primary_key<T, F> &field, const char *)
			{	item.*field.field = static_cast<F>(sqlite3_last_insert_rowid(&connection));	}

			template <typename U>
			void operator ()(U, const char *)
			{	}

			sqlite3 &connection;
			T &item;
		};

		template <typename T>
		class inserter
		{
		public:
			inserter(sqlite3 &connection, statement_ptr &&statement);

			template <typename T2>
			void operator ()(T2 &item);

		private:
			sqlite3 &_connection;
			statement_ptr _statement;
		};

		template <typename T>
		class insert_builder
		{
		public:
			insert_builder(const char *table_name);

			inserter<T> create_inserter(sqlite3 &connection);

			template <typename F>
			void operator ()(F field, const char *name);

			template <typename F>
			void operator ()(const primary_key<T, F> &field, const char *name);

		private:
			std::string _expression_text;
			int _index;
		};



		template <typename T>
		inline inserter<T>::inserter(sqlite3 &connection, statement_ptr &&statement)
			: _connection(connection), _statement(std::move(statement))
		{	}

		template <typename T>
		template <typename T2>
		inline void inserter<T>::operator ()(T2 &item)
		{
			field_binder<T> b = {	*_statement, item, 1	};
			primary_key_binder<T2> pkb = {	_connection, item	};

			describe<T>(b);
			sqlite3_step(_statement.get());
			describe<T>(pkb);
			sqlite3_reset(_statement.get());
			sqlite3_clear_bindings(_statement.get());
		}


		template <typename T>
		inline insert_builder<T>::insert_builder(const char *table_name)
			: _expression_text("INSERT INTO "), _index(0)
		{
			_expression_text += table_name;
			_expression_text += " (";
			describe<T>(*this);
			_expression_text += ") VALUES (";
			while (_index--)
				_expression_text += _index ? "?," : "?";
			_expression_text += ")";
		}

		template <typename T>
		inline inserter<T> insert_builder<T>::create_inserter(sqlite3 &connection)
		{	return inserter<T>(connection, create_statement(connection, _expression_text.c_str()));	}

		template <typename T>
		template <typename F>
		inline void insert_builder<T>::operator ()(F /*field*/, const char *name)
		{
			if (_index++)
				_expression_text += ',';
			_expression_text += name;
		}

		template <typename T>
		template <typename F>
		inline void insert_builder<T>::operator ()(const primary_key<T, F> &, const char *)
		{	}
	}
}
