#pragma once

#include <sqlite++/database.h>
#include <vector>

namespace micro_profiler
{
	namespace sql
	{
		namespace tests
		{
			template <typename T>
			std::vector<T> read_all(reader<T> &&reader)
			{
				T item;
				std::vector<T> result;

				while (reader(item))
					result.push_back(item);
				return result;
			}

			template <typename T>
			std::vector<T> read_all(transaction &tx)
			{	return read_all(tx.select<T>());	}

			template <typename T>
			std::vector<T> read_all(std::string path)
			{
				transaction tx(create_connection(path.c_str()));
				return read_all<T>(tx);
			}

			template <typename T>
			void write_all(transaction &tx, std::vector<T> &records)
			{
				auto w = tx.insert<T>();

				for (auto i = records.begin(); i != records.end(); ++i)
					w(*i);
			}

			template <typename T>
			void write_all(transaction &tx, const std::vector<T> &records, const char *table_name)
			{
				auto w = tx.insert<T>(table_name);

				for (auto i = records.begin(); i != records.end(); ++i)
					w(*i);
			}

			template <typename T>
			void write_all(std::string path, const std::vector<T> &records, const char *table_name)
			{
				transaction tx(create_connection(path.c_str()));
				write_all(tx, records, table_name);
			}
		}
	}
}
