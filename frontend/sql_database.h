#pragma once

#include "sql_select.h"

namespace micro_profiler
{
	namespace sql
	{
		class connection
		{
		public:
			connection(const std::string &path);

			template <typename T>
			reader<T> select(const char *table_name);

			template <typename T, typename W>
			reader<T> select(const char *table_name, const W &where);

		private:
			connection_ptr _database;
		};



		inline connection::connection(const std::string &path)
			: _database(create_conneciton(path.c_str()))
		{	}

		template <typename T>
		inline reader<T> connection::select(const char *table_name)
		{	return select_builder<T>(table_name).create_reader(*_database);	}

		template <typename T, typename W>
		inline reader<T> connection::select(const char *table_name, const W &where)
		{	return select_builder<T>(table_name).create_reader(*_database, where);	}
	}
}
