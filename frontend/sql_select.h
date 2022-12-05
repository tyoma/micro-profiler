#pragma once

#include "sql_format.h"
#include "sql_misc.h"

namespace micro_profiler
{
	namespace sql
	{
		struct parameter_binder
		{
			void bind_next(int value)
			{	sqlite3_bind_int(&statement, index++, value);	}

			void bind_next(const std::string &value)
			{	sqlite3_bind_text(&statement, index++, value.c_str(), -1, SQLITE_TRANSIENT);	}

			template <typename T>
			void operator ()(const parameter<T> &e)
			{	bind_next(e.object);	}

			template <typename T, typename F>
			void operator ()(const column<T, F> &/*e*/)
			{	}

			template <typename L, typename R>
			void operator ()(const operator_<L, R> &e)
			{	(*this)(e.lhs), (*this)(e.rhs);	}

			sqlite3_stmt &statement;
			int index;
		};

		template <typename T>
		struct record_reader
		{
			void operator ()(int T::*field, const char * /*name*/)
			{	record.*field = sqlite3_column_int(&statement, index++);	}

			void operator ()(std::string T::*field, const char * /*name*/)
			{	record.*field = (const char *)sqlite3_column_text(&statement, index++);	}

			T &record;
			sqlite3_stmt &statement;
			int index;
		};

		template <typename T>
		class reader
		{
		public:
			reader(statement_ptr &&statement);

			bool operator ()(T& value);

		private:
			statement_ptr _statement;
		};

		template <typename T>
		class select_builder
		{
		public:
			select_builder(const char *table_name);

			template <typename F>
			void operator ()(F field, const char *name);

			reader<T> create_reader(sqlite3 &database) const;

			template <typename W>
			reader<T> create_reader(sqlite3 &database, const W &where) const;

		private:
			int _index;
			std::string _expression_text;
		};



		template <typename T>
		inline reader<T>::reader(statement_ptr &&statement)
			: _statement(std::move(statement))
		{	}

		template <typename T>
		inline bool reader<T>::operator ()(T& record)
		{
			switch (sqlite3_step(_statement.get()))
			{
			case SQLITE_DONE:
				return false;

			case SQLITE_ROW:
				record_reader<T> rr = {	record, *_statement, 0	};

				describe<T>(rr);
				return true;
			}
			throw 0;
		}


		template <typename T>
		inline select_builder<T>::select_builder(const char *table_name)
			: _index(0), _expression_text("SELECT")
		{
			describe<T>(*this);
			_expression_text += " FROM ";
			_expression_text += table_name;
		}

		template <typename T>
		template <typename F>
		inline void select_builder<T>::operator ()(F /*field*/, const char *name)
		{
			_expression_text += _index++ ? ',' : ' ';
			_expression_text += name;
		}

		template <typename T>
		inline reader<T> select_builder<T>::create_reader(sqlite3 &database) const
		{	return reader<T>(create_statement(database, _expression_text.c_str()));	}

		template <typename T>
		template <typename W>
		inline reader<T> select_builder<T>::create_reader(sqlite3 &database, const W &where) const
		{
			auto expression_text = _expression_text;

			expression_text += " WHERE ";
			where.visit(format_visitor(expression_text));

			auto s = create_statement(database, expression_text.c_str());
			parameter_binder b = {	*s, 1	};

			where.visit(b);
			return reader<T>(std::move(s));
		}

	}
}
