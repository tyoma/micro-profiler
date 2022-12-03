#pragma once

#include <common/noncopyable.h>
#include <functional>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		class reader;

		class connection : noncopyable
		{
		public:
			connection(const std::string &path);
			~connection();

			template <typename T>
			reader<T> select(const char *table_name);

		private:
			sqlite3 *_database;
		};

		template <typename T>
		class reader
		{
		public:
			typedef std::function<void (T &record, sqlite3_stmt *statement)> reader_type;

		public:
			reader(reader &&other);
			~reader();

			bool operator ()(T& value);

		private:
			reader(const std::pair<sqlite3_stmt *, reader_type> &parameters);

			void operator =(reader &&rhs);

		private:
			sqlite3_stmt *_statement;
			reader_type _core;

		private:
			friend class connection;
		};

		template <typename T>
		class select_builder
		{
		public:
			void operator ()(int T::*field, const char *name);
			void operator ()(std::string T::*field, const char *name);

			std::string format_field_names() const;

		public:
			std::vector<std::string> names;
			typename reader<T>::reader_type reader_core;

		private:
			template <typename FieldT, typename ConvertorT>
			void register_field(FieldT T::*field, const char *name, const ConvertorT &convertor);
		};



		inline connection::connection(const std::string &path)
		{
			sqlite3_open_v2(path.c_str(), &_database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
		}

		inline connection::~connection()
		{	sqlite3_close(_database);	}

		template <typename T>
		inline reader<T> connection::select(const char *table_name)
		{
			select_builder<T> builder;
			sqlite3_stmt *statement = nullptr;

			describe(builder, static_cast<T*>(nullptr));

			auto text = "SELECT " + builder.format_field_names() + " FROM " + table_name;

			sqlite3_prepare_v2(_database, text.c_str(), -1, &statement, nullptr);
			return std::make_pair(statement, builder.reader_core);
		}


		template <typename T>
		inline reader<T>::reader(const std::pair<sqlite3_stmt *, reader_type> &parameters)
			: _statement(parameters.first), _core(parameters.second)
		{	}

		template <typename T>
		inline reader<T>::reader(reader &&other)
			: _statement(std::move(other._statement))
		{	}

		template <typename T>
		inline reader<T>::~reader()
		{	sqlite3_finalize(_statement);	}

		template <typename T>
		inline bool reader<T>::operator ()(T& record)
		{
			switch (sqlite3_step(_statement))
			{
			case SQLITE_DONE:
				return false;

			case SQLITE_ROW:
				_core(record, _statement);
				return true;
			}
			throw 0;
		}


		template <typename T>
		inline void select_builder<T>::operator ()(int T::*field, const char *name)
		{
			register_field<int>(field, name, [] (int &value, sqlite3_stmt *statement, int index) {
				value = sqlite3_column_int(statement, index);
			});
		}

		template <typename T>
		inline void select_builder<T>::operator ()(std::string T::*field, const char *name)
		{
			register_field<std::string>(field, name, [] (std::string &value, sqlite3_stmt *statement, int index) {
				value = (const char *)sqlite3_column_text(statement, index);
			});
		}

		template <typename T>
		inline std::string select_builder<T>::format_field_names() const
		{
			std::string names_list;

			for (auto i = names.begin(); i != names.end(); ++i)
			{
				if (names.begin() != i)
					names_list += ',';
				names_list += *i;
			}
			return names_list;
		}

		template <typename T>
		template <typename FieldT, typename ConvertorT>
		inline void select_builder<T>::register_field(FieldT T::*field, const char *name, const ConvertorT &convertor)
		{
			auto index = static_cast<int>(names.size());
			auto previous_core = reader_core;

			names.push_back(name);
			reader_core = [field, index, previous_core, convertor] (T &record, sqlite3_stmt *statement) {
				convertor(record.*field, statement, index);
				if (previous_core)
					previous_core(record, statement);
			};
		}

	}
}
