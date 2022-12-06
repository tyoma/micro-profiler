#pragma once

#include "sql_expression.h"
#include "sql_misc.h"

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

			void operator ()(std::string T::*field, const char *)
			{	sqlite3_bind_text(&statement, index++, (item.*field).c_str(), -1, SQLITE_STATIC);	}

			sqlite3_stmt &statement;
			const T &item;
			int index;
		};

		template <typename T>
		class inserter
		{
		public:
			inserter(statement_ptr &&statement);

			void operator ()(const T &item);

		private:
			statement_ptr _statement;
		};

		template <typename T>
		class insert_builder
		{
		public:
			insert_builder(const char *table_name);

			inserter<T> create_inserter(sqlite3 &database);

			template <typename F>
			void operator ()(F field, const char *name);

		private:
			std::string _expression_text;
			int _index;
		};



		template <typename T>
		inline inserter<T>::inserter(statement_ptr &&statement)
			: _statement(std::move(statement))
		{	}

		template <typename T>
		inline void inserter<T>::operator ()(const T &item)
		{
			field_binder<T> b = {	*_statement, item, 1	};

			describe<T>(b);
			sqlite3_step(_statement.get());
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
		inline inserter<T> insert_builder<T>::create_inserter(sqlite3 &database)
		{	return inserter<T>(create_statement(database, _expression_text.c_str()));	}

		template <typename T>
		template <typename F>
		inline void insert_builder<T>::operator ()(F /*field*/, const char *name)
		{
			_expression_text += _index++ ? ',' : ' ';
			_expression_text += name;
		}
	}
}
