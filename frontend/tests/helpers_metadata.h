#pragma once

#include <common/unordered_map.h>
#include <common/protocol.h>
#include <frontend/database.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename SymbolsT>
		inline module_info_metadata create_metadata(const SymbolsT &symbols)
		{
			module_info_metadata m;

			m.symbols.assign(std::begin(symbols), std::end(symbols));
			return m;
		}

		template <typename SymbolsT, typename FilesT>
		inline module_info_metadata create_metadata(const SymbolsT &symbols, const FilesT &files)
		{
			auto m = create_metadata(symbols);

			m.source_files = containers::unordered_map<unsigned int /*file_id*/, std::string>(begin(files), end(files));
			return m;
		}

		template <typename SymbolsT>
		inline module_info_metadata &add_metadata(sdb::table<tables::module> &modules, unsigned persistent_id, const SymbolsT &symbols)
		{
			auto rec = modules.create();
			auto &m = *rec;

			(*rec).id = persistent_id;
			static_cast<module_info_metadata &>(*rec) = create_metadata(symbols);
			rec.commit();
			return m;
		}

		template <typename SymbolsT, typename FilesT>
		inline void add_metadata(sdb::table<tables::module> &modules, unsigned persistent_id, const SymbolsT &symbols, const FilesT &files)
		{
			auto rec = modules.create();

			(*rec).id = persistent_id;
			static_cast<module_info_metadata &>(*rec) = create_metadata(symbols, files);
			rec.commit();
		}
	}
}
