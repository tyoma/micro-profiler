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


		inline connection_ptr create_conneciton(const char *path)
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
